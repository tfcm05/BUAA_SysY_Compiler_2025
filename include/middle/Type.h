#ifndef TYPE_H
#define TYPE_H

#include <vector>
#include <memory>
#include "../front/AstNode.h"

class Type;
class IntegerType;
class CharType;
class VoidType;
class ArrayType;
class FunctionType;
using TypePtr = std::shared_ptr<Type>;

class Type : public std::enable_shared_from_this<Type> {
public:
    enum TypeID {
        VoidTyID,
        IntegerTyID,
        CharTyID,
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

    bool isVoidTy() const { return typeIdentifier == VoidTyID; }
    bool isIntegerTy() const { return typeIdentifier == IntegerTyID; }
    bool isCharTy() const { return typeIdentifier == CharTyID; }
    bool isFunctionTy() const { return typeIdentifier == FunctionTyID; }
    bool isArrayTy() const { return typeIdentifier == ArrayTyID; }

    TypeQualifier getTypeQualifier() const { return typeQualifier; }

    bool isConst() const { return typeQualifier == TypeQualifier::CONST; }

    void setTypeQualifier(TypeQualifier tq) { typeQualifier = tq; }

    bool isCompatibleWith(TypePtr other);

    static TypePtr getVoidTy();
    static TypePtr getIntegerTy();
    static TypePtr getCharTy();

    static TypePtr getConstIntegerTy();
    static TypePtr getConstCharTy();
};

class IntegerType : public Type {
public:
    IntegerType() : Type(Type::IntegerTyID) {}
};

class VoidType : public Type {
public:
    VoidType() : Type(Type::VoidTyID) {}
};

class CharType : public Type {
public:
    CharType() : Type(Type::CharTyID) {}
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

inline bool Type::isCompatibleWith(TypePtr other) {
    if (this->isIntegerTy() && other->isIntegerTy()) return true;
    else if (this->isArrayTy() && other->isArrayTy()) {
        auto thisArr = std::dynamic_pointer_cast<ArrayType>(shared_from_this());
        auto otherArr = std::dynamic_pointer_cast<ArrayType>(other);
        return (thisArr->getElementType())->isCompatibleWith((otherArr->getElementType()));
    }
    return false;
}

TypePtr convertToType(BasicType basicType);
TypePtr convertToType(BasicType basicType, TypeQualifier tq);

#endif