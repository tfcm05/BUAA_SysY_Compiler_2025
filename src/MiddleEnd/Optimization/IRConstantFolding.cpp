#include "MiddleEnd/Optimization/IRConstantFolding.h"
#include "MiddleEnd/Optimization/IRReplaceUses.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/Type.h"

#include <cstdint>

IRConstantFolding::IRConstantFolding(Module& module, LLVMContext& context)
    : module(module), context(context) {}

IRConstantFolding::~IRConstantFolding() = default;

std::vector<std::unique_ptr<ConstantInt>>& IRConstantFolding::constantPool() {
    static std::vector<std::unique_ptr<ConstantInt>> pool;
    return pool;
}

ConstantInt* IRConstantFolding::createConstantInt(LLVM::Type* type, int64_t value) {
    auto* ci = new ConstantInt(type, value, "const.fold");
    constantPool().push_back(std::unique_ptr<ConstantInt>(ci));
    return ci;
}

ConstantInt* IRConstantFolding::tryFold(Instruction* inst) {
    int64_t lhs = 0;
    int64_t rhs = 0;

    if (auto* addInst = dynamic_cast<AddInstr*>(inst)) {
        if (!addInst->getLHS()->isConstantInt() || !addInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(addInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(addInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), static_cast<int64_t>(static_cast<int32_t>(lhs + rhs)));
    }
    if (auto* subInst = dynamic_cast<SubInstr*>(inst)) {
        if (!subInst->getLHS()->isConstantInt() || !subInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(subInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(subInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), static_cast<int64_t>(static_cast<int32_t>(lhs - rhs)));
    }
    if (auto* mulInst = dynamic_cast<MulInstr*>(inst)) {
        if (!mulInst->getLHS()->isConstantInt() || !mulInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(mulInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(mulInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), static_cast<int64_t>(static_cast<int32_t>(lhs * rhs)));
    }
    if (auto* sdivInst = dynamic_cast<SdivInstr*>(inst)) {
        if (!sdivInst->getLHS()->isConstantInt() || !sdivInst->getRHS()->isConstantInt()) return nullptr;
        rhs = static_cast<ConstantInt*>(sdivInst->getRHS())->getValue();
        if (rhs == 0) return nullptr;
        lhs = static_cast<ConstantInt*>(sdivInst->getLHS())->getValue();
        if (lhs == INT32_MIN && rhs == -1) return nullptr;
        return createConstantInt(inst->getType(), static_cast<int64_t>(static_cast<int32_t>(lhs / rhs)));
    }
    if (auto* andInst = dynamic_cast<AndInstr*>(inst)) {
        if (!andInst->getLHS()->isConstantInt() || !andInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(andInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(andInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), lhs & rhs);
    }
    if (auto* orInst = dynamic_cast<OrInstr*>(inst)) {
        if (!orInst->getLHS()->isConstantInt() || !orInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(orInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(orInst->getRHS())->getValue();
        return createConstantInt(inst->getType(), lhs | rhs);
    }
    if (auto* icmpInst = dynamic_cast<IcmpInstr*>(inst)) {
        if (!icmpInst->getLHS()->isConstantInt() || !icmpInst->getRHS()->isConstantInt()) return nullptr;
        lhs = static_cast<ConstantInt*>(icmpInst->getLHS())->getValue();
        rhs = static_cast<ConstantInt*>(icmpInst->getRHS())->getValue();
        // Signed compare as i32 (SysY int)
        const int32_t a = static_cast<int32_t>(lhs);
        const int32_t b = static_cast<int32_t>(rhs);
        bool result = false;
        switch (icmpInst->getPredicate()) {
            case IcmpInstr::IcmpOp::EQ:  result = (a == b); break;
            case IcmpInstr::IcmpOp::NE:  result = (a != b); break;
            case IcmpInstr::IcmpOp::SLT: result = (a < b);  break;
            case IcmpInstr::IcmpOp::SLE: result = (a <= b); break;
            case IcmpInstr::IcmpOp::SGT: result = (a > b);  break;
            case IcmpInstr::IcmpOp::SGE: result = (a >= b); break;
        }
        return createConstantInt(inst->getType(), result ? 1 : 0);
    }
    if (auto* zextInst = dynamic_cast<ZextInstr*>(inst)) {
        Value* operand = zextInst->getOperand();
        if (!operand->isConstantInt()) return nullptr;
        int64_t v = static_cast<ConstantInt*>(operand)->getValue();
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

int IRConstantFolding::foldFunction(Function* function) {
    int folded = 0;
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
                    replaceUsesInFunction(function, inst, foldedConst);
                    ++folded;
                    bbChanged = true;
                } else {
                    newInstrs.push_back(inst);
                }
            }
            if (bbChanged) {
                bb->setInstructions(std::move(newInstrs));
                if (!newInstrs.empty()) {
                    bb->setTerminator(newInstrs.back()->isTerminator());
                }
                changed = true;
            }
        }
    }
    return folded;
}

int IRConstantFolding::runOnModule(Module& mod, LLVMContext& ctx) {
    (void)mod;
    (void)ctx;
    return run();
}

int IRConstantFolding::run() {
    int total = 0;
    for (Function* function : collectModuleFunctions(module)) {
        if (!function || function->getBasicBlocks().empty()) {
            continue;
        }
        total += foldFunction(function);
    }
    return total;
}
