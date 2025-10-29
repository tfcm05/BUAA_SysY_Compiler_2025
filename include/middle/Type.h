#ifndef TYPE_H
#define TYPE_H

#include <vector>
#include <memory>
#include "../front/AstNode.h"

class Type;
using TypePtr = std::shared_ptr<Type>;

class Type {
public:
    enum TypeID {
        VoidTyID,
        IntegerTyID,
        FunctionTyID,
        ArrayTyID,
    };
private:
    TypeID typeIdentifier;
    TypeQualifier typeQualifier;
public:
    Type(TypeID id, TypeQualifier tq = TypeQualifier::NONE) : typeIdentifier(id), typeQualifier(tq) {}
    virtual ~Type() = default;
    TypeID getTypeID() const { return typeIdentifier; }
    TypeQualifier getTypeQualifier() const { return typeQualifier; }

    void setTypeQualifier(TypeQualifier tq) { typeQualifier = tq; }

    bool isVoidTy() const { return typeIdentifier == VoidTyID; }
    bool isIntegerTy() const { return typeIdentifier == IntegerTyID; }
    bool isFunctionTy() const { return typeIdentifier == FunctionTyID; }
    bool isArrayTy() const { return typeIdentifier == ArrayTyID; }

    bool isConst() const { return typeQualifier == TypeQualifier::CONST; }

    static TypePtr getVoidTy();
    static TypePtr getIntegerTy();

    bool isCompatibleWith(TypePtr other) {
        if (this->isIntegerTy() && other->isIntegerTy()) return true;
        return false;
    }
};

class IntegerType : public Type {
public:
    IntegerType() : Type(Type::IntegerTyID) {}
};

class VoidType : public Type {
public:
    VoidType() : Type(Type::VoidTyID) {}
};

class ArrayType : public Type {
    TypePtr array;
    int size; // -1 for unknown size like func param `int a[]`
public:
    ArrayType(TypePtr ty, int n) : Type(Type::ArrayTyID), array(ty), size(n) {}

    TypePtr getElementType() const { return array; }
    int getNumElements() const { return size; }
};

class FunctionType : public Type {
    TypePtr returnType;
    std::vector<TypePtr> paramTypes;
    bool variadicFlag;
public:
    FunctionType(TypePtr retTy, const std::vector<TypePtr>& params, bool v = false)
        : Type(Type::FunctionTyID), returnType(retTy), paramTypes(params), variadicFlag(v) {}

    TypePtr getReturnType() const { return returnType; }

    const size_t getParamsCount() const { return paramTypes.size(); }
    TypePtr getParamType(size_t index) const { return paramTypes[index]; }
    const std::vector<TypePtr>& getParamTypes() const { return paramTypes; }

    bool isVariadic() const { return variadicFlag; }
    void setVariadic(bool v) { variadicFlag = v; }
};

TypePtr convertToType(BasicType basicType);

#endif