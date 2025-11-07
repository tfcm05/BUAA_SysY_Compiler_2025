#include "../../include/middle/Type.h"

/* type */
Type::Type(TypeID id, TypeQualifier tq) : typeIdentifier(id), typeQualifier(tq) {}

Type::TypeID Type::getTypeID() const { return typeIdentifier; }

bool Type::isVoidTy() const { return typeIdentifier == VoidTyID; }
bool Type::isIntegerTy() const { return typeIdentifier == IntegerTyID; }
bool Type::isCharTy() const { return typeIdentifier == CharTyID; }
bool Type::isFunctionTy() const { return typeIdentifier == FunctionTyID; }
bool Type::isArrayTy() const { return typeIdentifier == ArrayTyID; }

TypeQualifier Type::getTypeQualifier() const { return typeQualifier; }

bool Type::isConst() const { return typeQualifier == TypeQualifier::CONST; }

void Type::setTypeQualifier(TypeQualifier tq) { typeQualifier = tq; }

/* Integer Type */
IntegerType::IntegerType() : Type(Type::IntegerTyID) {}
VoidType::VoidType() : Type(Type::VoidTyID) {}
CharType::CharType() : Type(Type::CharTyID) {}

/* Array Type */
ArrayType::ArrayType(TypePtr ty, int n) : Type(Type::ArrayTyID), array(ty), size(n) {}
TypePtr ArrayType::getElementType() const { return array; }
int ArrayType::getNumElements() const { return size; }

/* Function Type */
FunctionType::FunctionType(TypePtr retTy, const std::vector<TypePtr>& params, bool v)
    : Type(Type::FunctionTyID), returnType(retTy), paramTypes(params), variadicFlag(v) {}

TypePtr FunctionType::getReturnType() const { return returnType; }

const size_t FunctionType::getParamsCount() const { return paramTypes.size(); }
TypePtr FunctionType::getParamType(size_t index) const { return paramTypes[index]; }
const std::vector<TypePtr>& FunctionType::getParamTypes() const { return paramTypes; }

bool FunctionType::isVariadic() const { return variadicFlag; }
void FunctionType::setVariadic(bool v) { variadicFlag = v; }

/* Static Type Instances */
TypePtr Type::getVoidTy() {
    static TypePtr voidTy = std::make_shared<VoidType>();
    return voidTy;
}

TypePtr Type::getIntegerTy() {
    static TypePtr intTy = std::make_shared<IntegerType>();
    return intTy;
}

TypePtr Type::getCharTy() {
    static TypePtr charTy = std::make_shared<CharType>();
    return charTy;
}

TypePtr Type::getConstIntegerTy() {
    static TypePtr constIntTy = [] {
        auto ty = std::make_shared<IntegerType>();
        ty->setTypeQualifier(TypeQualifier::CONST);
        return ty;
    }();
    return constIntTy;
}

TypePtr Type::getConstCharTy() {
    static TypePtr constCharTy = [] {
        auto ty = std::make_shared<CharType>();
        ty->setTypeQualifier(TypeQualifier::CONST);
        return ty;
    }();
    return constCharTy;
}

bool Type::isCompatibleWith(TypePtr other) {
    if (this->isIntegerTy() && other->isIntegerTy()) return true;
    else if (this->isArrayTy() && other->isArrayTy()) {
        auto thisArr = std::dynamic_pointer_cast<ArrayType>(shared_from_this());
        auto otherArr = std::dynamic_pointer_cast<ArrayType>(other);
        return thisArr->getElementType()->isCompatibleWith(otherArr->getElementType());
    }
    return false;
}

TypePtr convertToType(TypeSpecifier ts) {
    if (ts == TypeSpecifier::INT) {
        return Type::getIntegerTy();
    }
    return Type::getVoidTy();
}

TypePtr convertToType(TypeSpecifier ts, TypeQualifier tq) {
    if (ts == TypeSpecifier::INT) {
        if (tq == TypeQualifier::CONST) {
            return Type::getConstIntegerTy();
        }
        return Type::getIntegerTy();
    }
    return Type::getVoidTy();
}