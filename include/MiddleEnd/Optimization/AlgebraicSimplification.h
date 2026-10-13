#ifndef MIDDLEEND_OPTIMIZATION_ALGEBRAIC_SIMPLIFICATION_H
#define MIDDLEEND_OPTIMIZATION_ALGEBRAIC_SIMPLIFICATION_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"
#include "MiddleEnd/Optimization/Pass.h"

#include <cstdint>

// Algebraic identity rewrite via replaceUsesInFunction; does not delete instrs.
class AlgebraicSimplification : public Pass {
private:
    Module& module;
    LLVMContext& llvmContext;

    bool isConstInt(Value* v, int64_t val) const;
    Value* getConstInt(LLVM::Type* type, int64_t val);

public:
    AlgebraicSimplification(Module& mod, LLVMContext& ctx);

    const char* getName() const override { return "AlgebraicSimplification"; }
    int runOnModule(Module& module, LLVMContext& context) override;
    int run();
};

#endif
