#include "MiddleEnd/Optimization/DeadInstructionElimination.h"
#include "MiddleEnd/Optimization/IRReplaceUses.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/FunctionInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/MemoryAccessInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/BasicBlock.h"

#include <vector>

DeadInstructionElimination::DeadInstructionElimination(Module& module) : module(module) {}

int DeadInstructionElimination::runOnModule(Module& mod, LLVMContext& ctx) {
    (void)mod;
    (void)ctx;
    return run();
}

bool DeadInstructionElimination::isRemovable(Instruction* instr) {
    // Whitelist: pure compute / load / unused alloca. No Store/Call/terminators/Phi/GEP.
    return dynamic_cast<AddInstr*>(instr) || dynamic_cast<SubInstr*>(instr) ||
           dynamic_cast<MulInstr*>(instr) || dynamic_cast<SdivInstr*>(instr) ||
           dynamic_cast<AndInstr*>(instr) || dynamic_cast<OrInstr*>(instr) ||
           dynamic_cast<ZextInstr*>(instr) || dynamic_cast<IcmpInstr*>(instr) ||
           dynamic_cast<LoadInst*>(instr) || dynamic_cast<AllocaInst*>(instr);
}

void DeadInstructionElimination::collectOperands(Instruction* instr, std::set<Value*>& referenced) {
    auto add = [&referenced](Value* v) {
        if (v) {
            referenced.insert(v);
        }
    };

    if (auto* i = dynamic_cast<AddInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<SubInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<MulInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<SdivInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<AndInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<OrInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<IcmpInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<ZextInstr*>(instr)) {
        add(i->getOperand());
    } else if (auto* i = dynamic_cast<LoadInst*>(instr)) {
        add(i->getPointer());
    } else if (auto* i = dynamic_cast<StoreInst*>(instr)) {
        add(i->getValue());
        add(i->getPointer());
    } else if (auto* i = dynamic_cast<GetElementPtrInst*>(instr)) {
        add(i->getPointer());
        for (size_t k = 0; k < i->getIndicesCount(); ++k) {
            add(i->getIndex(k));
        }
    } else if (auto* i = dynamic_cast<BrInstr*>(instr)) {
        add(i->getCondition());
    } else if (auto* i = dynamic_cast<RetInstr*>(instr)) {
        add(i->getReturnValue());
    } else if (auto* i = dynamic_cast<PhiInstr*>(instr)) {
        for (size_t k = 0; k < i->getIncomingCount(); ++k) {
            add(i->getIncoming(k).first);
        }
    } else if (auto* i = dynamic_cast<CallInstr*>(instr)) {
        for (size_t k = 0; k < i->getArgumentsCount(); ++k) {
            add(i->getArgument(k));
        }
    }
}

int DeadInstructionElimination::eliminateInFunction(Function* func) {
    int removed = 0;
    bool changed = true;
    while (changed) {
        changed = false;

        std::set<Value*> referenced;
        const auto& bbs = func->getBasicBlocks();
        for (auto* bb : bbs) {
            for (auto* inst : bb->getInstructions()) {
                collectOperands(inst, referenced);
            }
        }

        for (auto* bb : bbs) {
            const auto& insts = bb->getInstructions();
            if (insts.empty()) {
                continue;
            }
            std::vector<Instruction*> kept;
            kept.reserve(insts.size());
            for (auto* inst : insts) {
                if (isRemovable(inst) && referenced.find(inst) == referenced.end()) {
                    ++removed;
                    changed = true;
                } else {
                    kept.push_back(inst);
                }
            }
            if (kept.size() != insts.size()) {
                bb->setInstructions(kept);
                bb->setTerminator(!kept.empty() && kept.back()->isTerminator());
            }
        }
    }
    return removed;
}

int DeadInstructionElimination::run() {
    int total = 0;
    for (auto* func : collectModuleFunctions(module)) {
        if (!func || func->getBasicBlocks().empty()) {
            continue;
        }
        total += eliminateInFunction(func);
    }
    return total;
}
