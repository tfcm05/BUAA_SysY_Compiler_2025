#include "MiddleEnd/Optimization/Mem2Reg.h"
#include "MiddleEnd/Optimization/IRReplaceUses.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/FunctionInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/MemoryAccessInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/ConstInt.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/BasicBlock.h"
#include "MiddleEnd/IntermediateCodeGenerate/Type.h"

#include <cstdint>
#include <string>

Mem2Reg::Mem2Reg(Module& module, LLVMContext& context)
    : module(module), context(context) {}

std::vector<std::unique_ptr<Value>>& Mem2Reg::ownedPool() {
    static std::vector<std::unique_ptr<Value>> pool;
    return pool;
}

Value* Mem2Reg::getUndef(LLVM::Type* ty) {
    auto* ci = new ConstantInt(ty, 0, "mem2reg.undef");
    ownedPool().push_back(std::unique_ptr<Value>(ci));
    return ci;
}

LLVM::Type* Mem2Reg::allocatedType(AllocaInst* alloca) const {
    if (!alloca || !alloca->getType() || !alloca->getType()->isPointer()) {
        return nullptr;
    }
    return static_cast<LLVM::PointerType*>(alloca->getType())->getPointedType();
}

bool Mem2Reg::isPromotable(AllocaInst* alloca, Function* func,
                           std::vector<Instruction*>& loads,
                           std::vector<Instruction*>& stores,
                           BasicBlock*& soleBlock) const {
    loads.clear();
    stores.clear();
    soleBlock = nullptr;

    LLVM::Type* ty = allocatedType(alloca);
    if (!ty || ty->isArray() || ty->isFunction() || ty->isVoid()) {
        return false;
    }
    if (!ty->isInteger() && !ty->isPointer()) {
        return false;
    }

    BasicBlock* useBlock = nullptr;
    for (BasicBlock* bb : func->getBasicBlocks()) {
        for (Instruction* inst : bb->getInstructions()) {
            if (inst == alloca) {
                if (!useBlock) {
                    useBlock = bb;
                } else if (useBlock != bb) {
                    return false; // alloca itself counts as a use-site block
                }
                continue;
            }
            if (auto* load = dynamic_cast<LoadInst*>(inst)) {
                if (load->getPointer() == alloca) {
                    if (load->getType() != ty) {
                        return false;
                    }
                    loads.push_back(inst);
                    if (!useBlock) {
                        useBlock = bb;
                    } else if (useBlock != bb) {
                        return false;
                    }
                }
                continue;
            }
            if (auto* store = dynamic_cast<StoreInst*>(inst)) {
                if (store->getPointer() == alloca) {
                    if (!store->getValue() || store->getValue()->getType() != ty) {
                        return false; // refuse ill-typed stores (e.g. i1 into i32*)
                    }
                    stores.push_back(inst);
                    if (!useBlock) {
                        useBlock = bb;
                    } else if (useBlock != bb) {
                        return false;
                    }
                    continue;
                }
                if (store->getValue() == alloca) {
                    return false;
                }
                continue;
            }
            if (auto* gep = dynamic_cast<GetElementPtrInst*>(inst)) {
                if (gep->getPointer() == alloca) {
                    return false;
                }
                for (size_t i = 0; i < gep->getIndicesCount(); ++i) {
                    if (gep->getIndex(i) == alloca) {
                        return false;
                    }
                }
                continue;
            }
            if (auto* call = dynamic_cast<CallInstr*>(inst)) {
                for (size_t i = 0; i < call->getArgumentsCount(); ++i) {
                    if (call->getArgument(i) == alloca) {
                        return false;
                    }
                }
                continue;
            }
            auto mentions = [&](Value* v) { return v == alloca; };
            if (auto* bin = dynamic_cast<AddInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<SubInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<MulInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<SdivInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<AndInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<OrInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<IcmpInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* z = dynamic_cast<ZextInstr*>(inst)) {
                if (mentions(z->getOperand())) return false;
            } else if (auto* br = dynamic_cast<BrInstr*>(inst)) {
                if (mentions(br->getCondition())) return false;
            } else if (auto* ret = dynamic_cast<RetInstr*>(inst)) {
                if (mentions(ret->getReturnValue())) return false;
            } else if (auto* phi = dynamic_cast<PhiInstr*>(inst)) {
                for (size_t i = 0; i < phi->getIncomingCount(); ++i) {
                    if (mentions(phi->getIncoming(i).first)) return false;
                }
            }
        }
    }

    soleBlock = useBlock;
    // Conservative: require a sole block (alloca + all load/store there).
    return soleBlock != nullptr;
}

void Mem2Reg::removeInstructions(
    BasicBlock* bb, const std::unordered_set<Instruction*>& doomed) const {
    if (!bb || doomed.empty()) {
        return;
    }
    std::vector<Instruction*> kept;
    kept.reserve(bb->getInstructionsCount());
    for (Instruction* inst : bb->getInstructions()) {
        if (doomed.count(inst) == 0) {
            kept.push_back(inst);
        }
    }
    bb->setInstructions(kept);
    bb->setTerminator(!kept.empty() && kept.back()->isTerminator());
}

int Mem2Reg::promoteSingleBlock(AllocaInst* alloca, Function* func,
                                BasicBlock* bb,
                                const std::vector<Instruction*>& loads,
                                const std::vector<Instruction*>& stores) {
    LLVM::Type* ty = allocatedType(alloca);
    Value* cur = getUndef(ty);

    std::unordered_set<Instruction*> doomed;
    doomed.insert(alloca);
    for (Instruction* s : stores) {
        doomed.insert(s);
    }
    for (Instruction* l : loads) {
        doomed.insert(l);
    }

    // Walk in program order: store updates current; load → replace uses.
    for (Instruction* inst : bb->getInstructions()) {
        if (auto* store = dynamic_cast<StoreInst*>(inst)) {
            if (store->getPointer() == alloca) {
                cur = store->getValue();
            }
        } else if (auto* load = dynamic_cast<LoadInst*>(inst)) {
            if (load->getPointer() == alloca) {
                replaceUsesInFunction(func, load, cur);
            }
        }
    }

    removeInstructions(bb, doomed);
    return 1;
}

int Mem2Reg::promoteFunction(Function* func) {
    if (!func || func->getBasicBlocks().empty()) {
        return 0;
    }

    std::vector<AllocaInst*> allocas;
    for (BasicBlock* bb : func->getBasicBlocks()) {
        for (Instruction* inst : bb->getInstructions()) {
            if (auto* alloca = dynamic_cast<AllocaInst*>(inst)) {
                allocas.push_back(alloca);
            }
        }
    }

    int promoted = 0;
    for (AllocaInst* alloca : allocas) {
        std::vector<Instruction*> loads;
        std::vector<Instruction*> stores;
        BasicBlock* sole = nullptr;
        if (!isPromotable(alloca, func, loads, stores, sole)) {
            continue;
        }
        promoted += promoteSingleBlock(alloca, func, sole, loads, stores);
    }
    return promoted;
}

int Mem2Reg::runOnModule(Module& mod, LLVMContext& ctx) {
    (void)mod;
    (void)ctx;
    return run();
}

int Mem2Reg::run() {
    int total = 0;
    for (Function* func : collectModuleFunctions(module)) {
        total += promoteFunction(func);
    }
    return total;
}
