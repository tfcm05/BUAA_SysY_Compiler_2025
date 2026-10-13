#ifndef IR_CONSTANT_FOLDING_H
#define IR_CONSTANT_FOLDING_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/ConstInt.h"
#include "MiddleEnd/Optimization/Pass.h"

#include <memory>
#include <vector>

// Fold pure arithmetic/logic/icmp/zext when operands are ConstantInt.
// Replaces uses via replaceUsesInFunction (scan-based; users[] not maintained).
// Folded constants live in a process-static pool. Removed instructions stay
// owned by IRBuilder (list removal only).
class IRConstantFolding : public Pass {
private:
    Module& module;
    LLVMContext& context;

    static std::vector<std::unique_ptr<ConstantInt>>& constantPool();
    ConstantInt* tryFold(Instruction* inst);
    ConstantInt* createConstantInt(LLVM::Type* type, int64_t value);
    int foldFunction(Function* function);

public:
    IRConstantFolding(Module& module, LLVMContext& context);
    ~IRConstantFolding() override;

    const char* getName() const override { return "IRConstantFolding"; }
    int runOnModule(Module& module, LLVMContext& context) override;
    int run();
};

#endif
