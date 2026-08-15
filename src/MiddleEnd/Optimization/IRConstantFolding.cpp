#include "../../../include/MiddleEnd/Optimization/IRConstantFolding.h"

#include "../../../include/MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/FunctionInstr.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/MemoryAccessInstr.h"

#include <cstdint>

IRConstantFolding::IRConstantFolding(Module& module, LLVMContext& context)
    : module(module), context(context) {}

IRConstantFolding::~IRConstantFolding() = default;

std::vector<std::unique_ptr<ConstantInt>>& IRConstantFolding::constantPool() {
    static std::vector<std::unique_ptr<ConstantInt>> pool;
    return pool;
}

ConstantInt* IRConstantFolding::createConstantInt(LLVM::Type* type, int64_t value) {
    auto ci = new ConstantInt(type, value, "const.fold");
    constantPool().push_back(std::unique_ptr<ConstantInt>(ci));
    return ci;
}

ConstantInt* IRConstantFolding::tryFold(Instruction* inst) {
    int64_t lhs = 0;
    int64_t rhs = 0;

    // 二元算术/逻辑运算: 两个操作数均为 ConstantInt 才折叠
    if (auto addInst = dynamic_cast<AddInstr*>(inst)) {
        if (!addInst->getLHS()->isConstantInt() || !addInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(addInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(addInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), static_cast<int64_t>(static_cast<int32_t>(lhs + rhs)));
    }
    if (auto subInst = dynamic_cast<SubInstr*>(inst)) {
        if (!subInst->getLHS()->isConstantInt() || !subInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(subInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(subInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), static_cast<int64_t>(static_cast<int32_t>(lhs - rhs)));
    }
    if (auto mulInst = dynamic_cast<MulInstr*>(inst)) {
        if (!mulInst->getLHS()->isConstantInt() || !mulInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(mulInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(mulInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), static_cast<int64_t>(static_cast<int32_t>(lhs * rhs)));
    }
    if (auto sdivInst = dynamic_cast<SdivInstr*>(inst)) {
        if (!sdivInst->getLHS()->isConstantInt() || !sdivInst->getRHS()->isConstantInt()) return nullptr;
        rhs = static_cast<ConstantInt*>(sdivInst->getRHS())->getValue();
        // 除数为 0: 保留原指令, 不折叠
        if (rhs == 0) return nullptr;
        lhs = static_cast<ConstantInt*>(sdivInst->getLHS())->getValue();
        // INT_MIN / -1 在 C++ 中是未定义行为, 保留原指令
        if (lhs == INT32_MIN && rhs == -1) return nullptr;
        return createConstantInt(inst->getType(), static_cast<int64_t>(static_cast<int32_t>(lhs / rhs)));
    }
    if (auto andInst = dynamic_cast<AndInstr*>(inst)) {
        if (!andInst->getLHS()->isConstantInt() || !andInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(andInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(andInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), lhs & rhs);
    }
    if (auto orInst = dynamic_cast<OrInstr*>(inst)) {
        if (!orInst->getLHS()->isConstantInt() || !orInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(orInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(orInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), lhs | rhs);
    }
    if (auto icmpInst = dynamic_cast<IcmpInstr*>(inst)) {
        if (!icmpInst->getLHS()->isConstantInt() || !icmpInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(icmpInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(icmpInst->getRHS())->getValue();
        bool result = false;
        switch (icmpInst->getPredicate()) {
            case IcmpInstr::IcmpOp::EQ:  result = (lhs == rhs); break;
            case IcmpInstr::IcmpOp::NE:  result = (lhs != rhs); break;
            case IcmpInstr::IcmpOp::SLT: result = (lhs < rhs);  break;
            case IcmpInstr::IcmpOp::SLE: result = (lhs <= rhs); break;
            case IcmpInstr::IcmpOp::SGT: result = (lhs > rhs);  break;
            case IcmpInstr::IcmpOp::SGE: result = (lhs >= rhs); break;
        }
        return createConstantInt(inst->getType(), result ? 1 : 0);
    }
    if (auto zextInst = dynamic_cast<ZextInstr*>(inst)) {
        Value* operand = zextInst->getOperand();
        if (!operand->isConstantInt()) return nullptr;
        int64_t v = static_cast<ConstantInt*>(operand)->getValue();
        // 按目标位宽做无符号扩展截断 (SysY 中实际为 i1 -> i32, 结果为 0/1)
        unsigned int bitWidth = 0;
        if (inst->getType() && inst->getType()->isInteger()) {
            bitWidth = static_cast<LLVM::IntegerType*>(inst->getType())->getBitWidth();
        }
        if (bitWidth > 0 && bitWidth < 64) {
            v &= (static_cast<int64_t>(1) << bitWidth) - 1;
        }
        return createConstantInt(inst->getType(), v);
    }

    return nullptr;
}

void IRConstantFolding::replaceUses(Function* function, Value* oldValue, Value* newValue) {
    if (oldValue == newValue) return;
    for (size_t bi = 0; bi < function->getBasicBlocksCount(); ++bi) {
        BasicBlock* bb = function->getBasicBlock(bi);
        for (Instruction* inst : bb->getInstructions()) {
            // 算术与逻辑指令
            if (auto addInst = dynamic_cast<AddInstr*>(inst)) {
                if (addInst->getLHS() == oldValue) addInst->setLHS(newValue);
                if (addInst->getRHS() == oldValue) addInst->setRHS(newValue);
                continue;
            }
            if (auto subInst = dynamic_cast<SubInstr*>(inst)) {
                if (subInst->getLHS() == oldValue) subInst->setLHS(newValue);
                if (subInst->getRHS() == oldValue) subInst->setRHS(newValue);
                continue;
            }
            if (auto mulInst = dynamic_cast<MulInstr*>(inst)) {
                if (mulInst->getLHS() == oldValue) mulInst->setLHS(newValue);
                if (mulInst->getRHS() == oldValue) mulInst->setRHS(newValue);
                continue;
            }
            if (auto sdivInst = dynamic_cast<SdivInstr*>(inst)) {
                if (sdivInst->getLHS() == oldValue) sdivInst->setLHS(newValue);
                if (sdivInst->getRHS() == oldValue) sdivInst->setRHS(newValue);
                continue;
            }
            if (auto andInst = dynamic_cast<AndInstr*>(inst)) {
                if (andInst->getLHS() == oldValue) andInst->setLHS(newValue);
                if (andInst->getRHS() == oldValue) andInst->setRHS(newValue);
                continue;
            }
            if (auto orInst = dynamic_cast<OrInstr*>(inst)) {
                if (orInst->getLHS() == oldValue) orInst->setLHS(newValue);
                if (orInst->getRHS() == oldValue) orInst->setRHS(newValue);
                continue;
            }
            if (auto icmpInst = dynamic_cast<IcmpInstr*>(inst)) {
                if (icmpInst->getLHS() == oldValue) icmpInst->setLHS(newValue);
                if (icmpInst->getRHS() == oldValue) icmpInst->setRHS(newValue);
                continue;
            }
            if (auto zextInst = dynamic_cast<ZextInstr*>(inst)) {
                if (zextInst->getOperand() == oldValue) zextInst->setOperand(newValue);
                continue;
            }
            // 控制流指令
            if (auto brInst = dynamic_cast<BrInstr*>(inst)) {
                if (brInst->getCondition() == oldValue) brInst->setCondition(newValue);
                continue;
            }
            if (auto retInst = dynamic_cast<RetInstr*>(inst)) {
                if (retInst->getReturnValue() == oldValue) retInst->setReturnValue(newValue);
                continue;
            }
            if (auto phiInst = dynamic_cast<PhiInstr*>(inst)) {
                bool changed = false;
                std::vector<std::pair<Value*, BasicBlock*>> incomings = phiInst->getIncomingValues();
                for (auto& incoming : incomings) {
                    if (incoming.first == oldValue) {
                        incoming.first = newValue;
                        changed = true;
                    }
                }
                if (changed) phiInst->setIncoming(std::move(incomings));
                continue;
            }
            // 函数调用指令
            if (auto callInst = dynamic_cast<CallInstr*>(inst)) {
                bool changed = false;
                std::vector<Value*> args = callInst->getArguments();
                for (auto& arg : args) {
                    if (arg == oldValue) {
                        arg = newValue;
                        changed = true;
                    }
                }
                if (changed) callInst->setArguments(std::move(args));
                continue;
            }
            // 内存访问指令
            if (auto loadInst = dynamic_cast<LoadInst*>(inst)) {
                if (loadInst->getPointer() == oldValue) loadInst->setPointer(newValue);
                continue;
            }
            if (auto storeInst = dynamic_cast<StoreInst*>(inst)) {
                if (storeInst->getValue() == oldValue) storeInst->setValue(newValue);
                if (storeInst->getPointer() == oldValue) storeInst->setPointer(newValue);
                continue;
            }
            if (auto gepInst = dynamic_cast<GetElementPtrInst*>(inst)) {
                bool changed = false;
                std::vector<Value*> indices = gepInst->getIndices();
                for (auto& index : indices) {
                    if (index == oldValue) {
                        index = newValue;
                        changed = true;
                    }
                }
                if (gepInst->getPointer() == oldValue) {
                    gepInst->setPointer(newValue);
                    changed = true;
                }
                if (changed) gepInst->setIndices(std::move(indices));
                continue;
            }
        }
    }
}

int IRConstantFolding::foldFunction(Function* function) {
    int folded = 0;
    // 反复折叠直至本函数无变化: 处理折叠链 (如 %1 = add 1,2; %2 = add %1,3)
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t bi = 0; bi < function->getBasicBlocksCount(); ++bi) {
            BasicBlock* bb = function->getBasicBlock(bi);
            const std::vector<Instruction*>& instrs = bb->getInstructions();
            std::vector<Instruction*> newInstrs;
            newInstrs.reserve(instrs.size());
            bool bbChanged = false;
            for (Instruction* inst : instrs) {
                ConstantInt* foldedConst = tryFold(inst);
                if (foldedConst) {
                    // 用折叠常量替换本函数内对该指令结果的所有引用
                    replaceUses(function, inst, foldedConst);
                    ++folded;
                    bbChanged = true;
                    // 该指令不再保留在指令列表中
                } else {
                    newInstrs.push_back(inst);
                }
            }
            if (bbChanged) {
                bb->setInstructions(std::move(newInstrs));
                changed = true;
            }
        }
    }
    return folded;
}

int IRConstantFolding::run() {
    int total = 0;
    for (Function* function : module.getFunctions()) {
        total += foldFunction(function);
    }
    // main 函数在 IRVisitor 中只 setMainFunction, 不在 functions 列表中, 需单独处理
    Function* mainFunction = module.getMainFunction();
    if (mainFunction) {
        bool mainInList = false;
        for (Function* function : module.getFunctions()) {
            if (function == mainFunction) {
                mainInList = true;
                break;
            }
        }
        if (!mainInList) {
            total += foldFunction(mainFunction);
        }
    }
    return total;
}
