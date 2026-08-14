#include "MiddleEnd/Optimization/AlgebraicSimplification.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/ConstInt.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/FunctionInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/MemoryAccessInstr.h"

AlgebraicSimplification::AlgebraicSimplification(Module& mod, LLVMContext& ctx)
    : module(mod), llvmContext(ctx) {}

bool AlgebraicSimplification::isConstInt(Value* v, int64_t val) const {
    if (!v || !v->isConstantInt()) {
        return false;
    }
    return static_cast<ConstantInt*>(v)->getValue() == val;
}

Value* AlgebraicSimplification::getConstInt(LLVM::Type* type, int64_t val) {
    auto& cache = (val == 0) ? zeroCache : oneCache;
    auto it = cache.find(type);
    if (it != cache.end()) {
        return it->second.get();
    }
    auto ci = new ConstantInt(type, val, "");
    cache[type] = std::unique_ptr<Value>(ci);
    return ci;
}

bool AlgebraicSimplification::replaceUsesInFunction(Function* func, Value* oldValue, Value* newValue) {
    bool any = false;
    for (BasicBlock* bb : func->getBasicBlocks()) {
        for (Instruction* inst : bb->getInstructions()) {
            // 不替换指令自身 (防止把自身操作数改为新值造成误改)
            if (inst == oldValue) {
                continue;
            }

            // 二元算术/逻辑指令
            if (auto addI = dynamic_cast<AddInstr*>(inst)) {
                if (addI->getLHS() == oldValue) { addI->setLHS(newValue); any = true; }
                if (addI->getRHS() == oldValue) { addI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto subI = dynamic_cast<SubInstr*>(inst)) {
                if (subI->getLHS() == oldValue) { subI->setLHS(newValue); any = true; }
                if (subI->getRHS() == oldValue) { subI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto mulI = dynamic_cast<MulInstr*>(inst)) {
                if (mulI->getLHS() == oldValue) { mulI->setLHS(newValue); any = true; }
                if (mulI->getRHS() == oldValue) { mulI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto divI = dynamic_cast<SdivInstr*>(inst)) {
                if (divI->getLHS() == oldValue) { divI->setLHS(newValue); any = true; }
                if (divI->getRHS() == oldValue) { divI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto andI = dynamic_cast<AndInstr*>(inst)) {
                if (andI->getLHS() == oldValue) { andI->setLHS(newValue); any = true; }
                if (andI->getRHS() == oldValue) { andI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto orI = dynamic_cast<OrInstr*>(inst)) {
                if (orI->getLHS() == oldValue) { orI->setLHS(newValue); any = true; }
                if (orI->getRHS() == oldValue) { orI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto icmpI = dynamic_cast<IcmpInstr*>(inst)) {
                if (icmpI->getLHS() == oldValue) { icmpI->setLHS(newValue); any = true; }
                if (icmpI->getRHS() == oldValue) { icmpI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto zextI = dynamic_cast<ZextInstr*>(inst)) {
                if (zextI->getOperand() == oldValue) { zextI->setOperand(newValue); any = true; }
                continue;
            }

            // 内存指令
            if (auto loadI = dynamic_cast<LoadInst*>(inst)) {
                if (loadI->getPointer() == oldValue) { loadI->setPointer(newValue); any = true; }
                continue;
            }
            if (auto storeI = dynamic_cast<StoreInst*>(inst)) {
                if (storeI->getValue() == oldValue) { storeI->setValue(newValue); any = true; }
                if (storeI->getPointer() == oldValue) { storeI->setPointer(newValue); any = true; }
                continue;
            }
            if (auto gepI = dynamic_cast<GetElementPtrInst*>(inst)) {
                if (gepI->getPointer() == oldValue) { gepI->setPointer(newValue); any = true; }
                bool changed = false;
                auto indices = gepI->getIndices();
                for (auto& idx : indices) {
                    if (idx == oldValue) {
                        idx = newValue;
                        changed = true;
                    }
                }
                if (changed) { gepI->setIndices(indices); any = true; }
                continue;
            }

            // 控制流指令
            if (auto brI = dynamic_cast<BrInstr*>(inst)) {
                if (brI->getCondition() == oldValue) { brI->setCondition(newValue); any = true; }
                continue;
            }
            if (auto retI = dynamic_cast<RetInstr*>(inst)) {
                if (retI->getReturnValue() == oldValue) { retI->setReturnValue(newValue); any = true; }
                continue;
            }
            if (auto phiI = dynamic_cast<PhiInstr*>(inst)) {
                bool changed = false;
                auto incomings = phiI->getIncomingValues();
                for (auto& [val, block] : incomings) {
                    if (val == oldValue) {
                        val = newValue;
                        changed = true;
                    }
                }
                if (changed) { phiI->setIncoming(incomings); any = true; }
                continue;
            }

            // 调用指令
            if (auto callI = dynamic_cast<CallInstr*>(inst)) {
                bool changed = false;
                auto args = callI->getArguments();
                for (auto& arg : args) {
                    if (arg == oldValue) {
                        arg = newValue;
                        changed = true;
                    }
                }
                if (changed) { callI->setArguments(args); any = true; }
                continue;
            }
        }
    }
    return any;
}

int AlgebraicSimplification::run() {
    int total = 0;
    const std::vector<Function*>& funcs = module.getFunctions();

    // 固定点迭代: 前一轮的化简可能暴露新的可化简指令 (如 or 0, a -> a 后 a 的结果被复用)
    const int MAX_ITER = 8;
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        int changed = 0;

        for (Function* func : funcs) {
            if (!func) continue;
            for (BasicBlock* bb : func->getBasicBlocks()) {
                if (!bb) continue;
                for (Instruction* inst : bb->getInstructions()) {
                    if (!inst) continue;

                    Value* replacement = nullptr;

                    if (auto addI = dynamic_cast<AddInstr*>(inst)) {
                        if (isConstInt(addI->getRHS(), 0)) {
                            replacement = addI->getLHS();
                        } else if (isConstInt(addI->getLHS(), 0)) {
                            replacement = addI->getRHS();
                        }
                    } else if (auto subI = dynamic_cast<SubInstr*>(inst)) {
                        if (isConstInt(subI->getRHS(), 0)) {
                            replacement = subI->getLHS();
                        }
                    } else if (auto mulI = dynamic_cast<MulInstr*>(inst)) {
                        if (isConstInt(mulI->getLHS(), 0) || isConstInt(mulI->getRHS(), 0)) {
                            replacement = getConstInt(mulI->getType(), 0);
                        } else if (isConstInt(mulI->getRHS(), 1)) {
                            replacement = mulI->getLHS();
                        } else if (isConstInt(mulI->getLHS(), 1)) {
                            replacement = mulI->getRHS();
                        }
                    } else if (auto divI = dynamic_cast<SdivInstr*>(inst)) {
                        if (isConstInt(divI->getRHS(), 1)) {
                            replacement = divI->getLHS();
                        }
                    } else if (auto andI = dynamic_cast<AndInstr*>(inst)) {
                        if (isConstInt(andI->getLHS(), 0) || isConstInt(andI->getRHS(), 0)) {
                            replacement = getConstInt(andI->getType(), 0);
                        }
                    } else if (auto orI = dynamic_cast<OrInstr*>(inst)) {
                        if (isConstInt(orI->getLHS(), 0)) {
                            replacement = orI->getRHS();
                        } else if (isConstInt(orI->getRHS(), 0)) {
                            replacement = orI->getLHS();
                        }
                    } else if (auto icmpI = dynamic_cast<IcmpInstr*>(inst)) {
                        if (icmpI->getLHS() == icmpI->getRHS()) {
                            switch (icmpI->getPredicate()) {
                                case IcmpInstr::IcmpOp::EQ:
                                case IcmpInstr::IcmpOp::SLE:
                                case IcmpInstr::IcmpOp::SGE:
                                    replacement = getConstInt(icmpI->getType(), 1);
                                    break;
                                case IcmpInstr::IcmpOp::NE:
                                case IcmpInstr::IcmpOp::SLT:
                                case IcmpInstr::IcmpOp::SGT:
                                    replacement = getConstInt(icmpI->getType(), 0);
                                    break;
                            }
                        }
                    }

                    if (replacement && replacement != inst) {
                        if (replaceUsesInFunction(func, inst, replacement)) {
                            ++changed;
                        }
                    }
                }
            }
        }

        total += changed;
        if (changed == 0) {
            break;
        }
    }

    return total;
}
