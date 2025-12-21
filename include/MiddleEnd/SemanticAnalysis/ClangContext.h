#ifndef CLANGCONTEXT_H
#define CLANGCONTEXT_H

#include "Type.h"

class ClangContext {
private:
    std::unique_ptr<Clang::ClangContextImpl> pImpl;
public:
    ClangContext();
    ~ClangContext();

    Clang::VoidType* getVoidTyImpl();
    Clang::IntegerType* getIntegerTyImpl();
    Clang::CharType* getCharTyImpl();

    Clang::PointerType* getPointerTyImpl(Clang::Type* pointedType);
    Clang::ArrayType* getArrayTyImpl(Clang::Type* elementType, size_t size);
    Clang::FunctionType* getFunctionTyImpl(Clang::Type* returnType, const std::vector<Clang::Type*>& params, bool isVarArg);

    Clang::Type* getQualifiedTyImpl(Clang::Type* baseType, unsigned int qualifier);

    void scan(unsigned int mode);
};

#endif