#include "MiddleEnd/Optimization/ConstantBranchFolding.h"

ConstantBranchFolding::ConstantBranchFolding(Module& module) : module(module) {}

int ConstantBranchFolding::run() {
    int simplified = 0;
    for (auto* func : module.getFunctions()) {
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
                    // 恒真: 无条件跳转到 trueBB
                    br->setCondition(nullptr);
                    br->setFalseBlock(nullptr);
                } else {
                    // 恒假: 无条件跳转到 falseBB
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
