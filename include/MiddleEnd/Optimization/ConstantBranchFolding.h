#ifndef MIDDLEEND_OPTIMIZATION_CONSTANT_BRANCH_FOLDING_H
#define MIDDLEEND_OPTIMIZATION_CONSTANT_BRANCH_FOLDING_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/Optimization/Pass.h"

// If BrInstr condition is ConstantInt: rewrite to unconditional br.
// Unreachable blocks are left for DeadBlockElimination.
class ConstantBranchFolding : public Pass {
private:
    Module& module;
public:
    explicit ConstantBranchFolding(Module& module);

    const char* getName() const override { return "ConstantBranchFolding"; }
    int runOnModule(Module& module, LLVMContext& context) override;
    int run();
};

#endif
