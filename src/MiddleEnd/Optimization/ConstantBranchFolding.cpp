#include "MiddleEnd/Optimization/ConstantBranchFolding.h"
#include "MiddleEnd/Optimization/IRReplaceUses.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/ConstInt.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/BasicBlock.h"

ConstantBranchFolding::ConstantBranchFolding(Module& module) : module(module) {}

int ConstantBranchFolding::runOnModule(Module& mod, LLVMContext& ctx) {
    (void)mod;
    (void)ctx;
    return run();
}

int ConstantBranchFolding::run() {
    int simplified = 0;
    for (auto* func : collectModuleFunctions(module)) {
        if (!func) {
            continue;
        }
        for (auto* bb : func->getBasicBlocks()) {
            for (auto* inst : bb->getInstructions()) {
                auto* br = dynamic_cast<BrInstr*>(inst);
                if (!br) {
                    continue;
                }
                Value* cond = br->getCondition();
                if (!cond || !cond->isConstantInt()) {
                    continue;
                }
                auto* cint = static_cast<ConstantInt*>(cond);
                if (cint->getValue() != 0) {
                    br->setCondition(nullptr);
                    br->setFalseBlock(nullptr);
                } else {
                    br->setCondition(nullptr);
                    br->setTrueBlock(br->getFalseBlock());
                    br->setFalseBlock(nullptr);
                }
                ++simplified;
            }
        }
    }
    return simplified;
}
