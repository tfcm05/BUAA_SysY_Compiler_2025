#include "MiddleEnd/SemanticAnalysis/Type.h"
#include "MiddleEnd/SemanticAnalysis/ClangContext.h"

namespace Clang {

//===----------------------------------------------------------------------===//
// Type 基类实现
//===----------------------------------------------------------------------===//

Type::Type(ClangContext& context, TypeID id)
    : typeIdentifier(id), qualifier(0), context(context) {}

Type::TypeID Type::getTypeID() const {
    return typeIdentifier;
}

ClangContext& Type::getContext() const {
    return context;
}

bool Type::isVoidTy() const { return getTypeID() == VoidTyID; }
bool Type::isCharTy() const { return getTypeID() == CharTyID; }
bool Type::isIntegerTy() const { return getTypeID() == IntegerTyID; }
bool Type::isArrayTy() const { return getTypeID() == ArrayTyID; }
bool Type::isPointerTy() const { return getTypeID() == PointerTyID; }
bool Type::isFunctionTy() const { return getTypeID() == FunctionTyID; }

unsigned int Type::getQualifier() const {
    return qualifier;
}

void Type::setQualifier(unsigned int tq) {
    qualifier = tq;
}

void Type::addQualifier(unsigned int tq) {
    qualifier |= tq;
}

bool Type::isConst() const {
    return (qualifier & 1U) != 0;
}

//===----------------------------------------------------------------------===//
// VoidType 实现
//===----------------------------------------------------------------------===//

VoidType::VoidType(ClangContext& context) : Type(context, VoidTyID) {}

VoidType* VoidType::get(ClangContext& context) {
    return context.getVoidTyImpl();
}

//===----------------------------------------------------------------------===//
// CharType 实现
//===----------------------------------------------------------------------===//

CharType::CharType(ClangContext& context) : Type(context, CharTyID) {}

CharType* CharType::get(ClangContext& context) {
    return context.getCharTyImpl();
}

//===----------------------------------------------------------------------===//
// IntegerType 实现
//===----------------------------------------------------------------------===//

IntegerType::IntegerType(ClangContext& context) : Type(context, IntegerTyID) {}

IntegerType* IntegerType::get(ClangContext& context) {
    return context.getIntegerTyImpl();
}

//===----------------------------------------------------------------------===//
// ArrayType 实现
//===----------------------------------------------------------------------===//

ArrayType::ArrayType(Type* eleTy, size_t s)
    : Type(eleTy->getContext(), ArrayTyID), elementType(eleTy), size(s) {}

ArrayType* ArrayType::get(Type* eleTy, size_t s) {
    return eleTy->getContext().getArrayTyImpl(eleTy, s);
}

Type* ArrayType::getElementType() const {
    return elementType;
}

size_t ArrayType::getSize() const {
    return size;
}

//===----------------------------------------------------------------------===//
// PointerType 实现
//===----------------------------------------------------------------------===//

PointerType::PointerType(Type* pointedTo)
    : Type(pointedTo->getContext(), PointerTyID), pointedType(pointedTo) {}

PointerType* PointerType::get(Type* pointedTo) {
    return pointedTo->getContext().getPointerTyImpl(pointedTo);
}

Type* PointerType::getPointedType() const {
    return pointedType;
}

//===----------------------------------------------------------------------===//
// FunctionType 实现
//===----------------------------------------------------------------------===//

FunctionType::FunctionType(Type* retTy, const std::vector<Type*>& params, bool vf)
    : Type(retTy->getContext(), FunctionTyID), returnType(retTy), paramTypes(params), variadicFlag(vf) {}

FunctionType* FunctionType::get(Type* retTy, const std::vector<Type*>& params, bool vf) {
    return retTy->getContext().getFunctionTyImpl(retTy, params, vf);
}

Type* FunctionType::getReturnType() const {
    return returnType;
}

size_t FunctionType::getParamTypesCount() const {
    return paramTypes.size();
}

Type* FunctionType::getParamType(size_t index) const {
    return paramTypes[index];
}

const std::vector<Type*>& FunctionType::getParamTypes() const {
    return paramTypes;
}

bool FunctionType::isVariadic() const {
    return variadicFlag;
}

void FunctionType::setVariadicFlag(bool vf) {
    variadicFlag = vf;
}

}


