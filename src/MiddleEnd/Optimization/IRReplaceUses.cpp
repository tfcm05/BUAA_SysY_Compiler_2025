#include "MiddleEnd/Optimization/IRReplaceUses.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/GlobalValue/Function.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/FunctionInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/MemoryAccessInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/BasicBlock.h"

bool replaceUsesInFunction(Function* function, Value* oldValue, Value* newValue) {
    if (!function || !oldValue || oldValue == newValue) {
        return false;
    }
    bool any = false;
    for (BasicBlock* bb : function->getBasicBlocks()) {
        for (Instruction* inst : bb->getInstructions()) {
            if (inst == oldValue) {
                continue;
            }

            if (auto* addI = dynamic_cast<AddInstr*>(inst)) {
                if (addI->getLHS() == oldValue) { addI->setLHS(newValue); any = true; }
                if (addI->getRHS() == oldValue) { addI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto* subI = dynamic_cast<SubInstr*>(inst)) {
                if (subI->getLHS() == oldValue) { subI->setLHS(newValue); any = true; }
                if (subI->getRHS() == oldValue) { subI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto* mulI = dynamic_cast<MulInstr*>(inst)) {
                if (mulI->getLHS() == oldValue) { mulI->setLHS(newValue); any = true; }
                if (mulI->getRHS() == oldValue) { mulI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto* divI = dynamic_cast<SdivInstr*>(inst)) {
                if (divI->getLHS() == oldValue) { divI->setLHS(newValue); any = true; }
                if (divI->getRHS() == oldValue) { divI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto* andI = dynamic_cast<AndInstr*>(inst)) {
                if (andI->getLHS() == oldValue) { andI->setLHS(newValue); any = true; }
                if (andI->getRHS() == oldValue) { andI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto* orI = dynamic_cast<OrInstr*>(inst)) {
                if (orI->getLHS() == oldValue) { orI->setLHS(newValue); any = true; }
                if (orI->getRHS() == oldValue) { orI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto* icmpI = dynamic_cast<IcmpInstr*>(inst)) {
                if (icmpI->getLHS() == oldValue) { icmpI->setLHS(newValue); any = true; }
                if (icmpI->getRHS() == oldValue) { icmpI->setRHS(newValue); any = true; }
                continue;
            }
            if (auto* zextI = dynamic_cast<ZextInstr*>(inst)) {
                if (zextI->getOperand() == oldValue) { zextI->setOperand(newValue); any = true; }
                continue;
            }
            if (auto* loadI = dynamic_cast<LoadInst*>(inst)) {
                if (loadI->getPointer() == oldValue) { loadI->setPointer(newValue); any = true; }
                continue;
            }
            if (auto* storeI = dynamic_cast<StoreInst*>(inst)) {
                if (storeI->getValue() == oldValue) { storeI->setValue(newValue); any = true; }
                if (storeI->getPointer() == oldValue) { storeI->setPointer(newValue); any = true; }
                continue;
            }
            if (auto* gepI = dynamic_cast<GetElementPtrInst*>(inst)) {
                if (gepI->getPointer() == oldValue) { gepI->setPointer(newValue); any = true; }
                bool changed = false;
                auto indices = gepI->getIndices();
                for (auto& idx : indices) {
                    if (idx == oldValue) {
                        idx = newValue;
                        changed = true;
                    }
                }
                if (changed) {
                    gepI->setIndices(indices);
                    any = true;
                }
                continue;
            }
            if (auto* brI = dynamic_cast<BrInstr*>(inst)) {
                if (brI->getCondition() == oldValue) { brI->setCondition(newValue); any = true; }
                continue;
            }
            if (auto* retI = dynamic_cast<RetInstr*>(inst)) {
                if (retI->getReturnValue() == oldValue) { retI->setReturnValue(newValue); any = true; }
                continue;
            }
            if (auto* phiI = dynamic_cast<PhiInstr*>(inst)) {
                bool changed = false;
                auto incomings = phiI->getIncomingValues();
                for (auto& pair : incomings) {
                    if (pair.first == oldValue) {
                        pair.first = newValue;
                        changed = true;
                    }
                }
                if (changed) {
                    phiI->setIncoming(incomings);
                    any = true;
                }
                continue;
            }
            if (auto* callI = dynamic_cast<CallInstr*>(inst)) {
                bool changed = false;
                auto args = callI->getArguments();
                for (auto& arg : args) {
                    if (arg == oldValue) {
                        arg = newValue;
                        changed = true;
                    }
                }
                if (changed) {
                    callI->setArguments(args);
                    any = true;
                }
                continue;
            }
        }
    }
    return any;
}
