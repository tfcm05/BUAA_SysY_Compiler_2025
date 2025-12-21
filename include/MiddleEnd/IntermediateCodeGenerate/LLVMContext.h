#ifndef LLVMCONTEXT_H
#define LLVMCONTEXT_H

#include "Type.h"

class LLVMContext {
private:
    std::unique_ptr<LLVM::LLVMContextImpl> pImpl;
public:
    LLVMContext();
    ~LLVMContext();

    LLVM::VoidType* getVoidTyImpl();
    LLVM::IntegerType* getIntegerTyImpl(unsigned int bitWidth);
    LLVM::PointerType* getPointerTyImpl(LLVM::Type* elementType);
    LLVM::ArrayType* getArrayTyImpl(LLVM::Type* elementType, size_t size);
    LLVM::FunctionType* getFunctionTyImpl(LLVM::Type* returnType, const std::vector<LLVM::Type*>& params, bool isVarArg);
};

#endif