#include "MiddleEnd/Optimization/AlgebraicSimplification.h"
#include "MiddleEnd/Optimization/IRReplaceUses.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/ConstInt.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/FunctionInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/MemoryAccessInstr.h"

#include <memory>
#include <unordered_map>
#include <vector>

AlgebraicSimplification::AlgebraicSimplification(Module& mod, LLVMContext& ctx)
    : module(mod), llvmContext(ctx) {}

bool AlgebraicSimplification::isConstInt(Value* v, int64_t val) const {
    if (!v || !v->isConstantInt()) {
        return false;
    }
    return static_cast<ConstantInt*>(v)->getValue() == val;
}

Value* AlgebraicSimplification::getConstInt(LLVM::Type* type, int64_t val) {
    // 进程级静态缓存: 常量生命周期覆盖整个程序, IRPrinter 打印时仍有效
    static std::unordered_map<LLVM::Type*, std::unordered_map<int64_t, std::unique_ptr<Value>>> cache;
    auto& byVal = cache[type];
    auto it = byVal.find(val);
    if (it != byVal.end()) {
        return it->second.get();
    }
    auto ci = new ConstantInt(type, val, "");
    byVal[val] = std::unique_ptr<Value>(ci);
    return ci;
}


int AlgebraicSimplification::runOnModule(Module& mod, LLVMContext& ctx) {
    // Bound at construction; ignore alternate Module/Context to keep lifetime simple.
    (void)mod;
    (void)ctx;
    return run();
}

int AlgebraicSimplification::run() {
    int total = 0;

    std::vector<Function*> funcs = collectModuleFunctions(module);

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
