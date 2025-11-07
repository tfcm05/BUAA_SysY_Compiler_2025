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
    Type(TypeID id, TypeQualifier tq = TypeQualifier::NONE);
    virtual ~Type() = default;
    TypeID getTypeID() const;

    bool isVoidTy() const;
    bool isIntegerTy() const;
    bool isCharTy() const;
    bool isFunctionTy() const;
    bool isArrayTy() const;

    TypeQualifier getTypeQualifier() const;

    bool isConst() const;

    void setTypeQualifier(TypeQualifier tq);

    bool isCompatibleWith(TypePtr other);

    static TypePtr getVoidTy();
    static TypePtr getIntegerTy();
    static TypePtr getCharTy();

    static TypePtr getConstIntegerTy();
    static TypePtr getConstCharTy();
};

class IntegerType : public Type {
public:
    IntegerType();
};

class VoidType : public Type {
public:
    VoidType();
};

class CharType : public Type {
public:
    CharType();
};

class ArrayType : public Type {
    TypePtr array;
    int size; // -1 for unknown size like func param `int a[]`
public:
    ArrayType(TypePtr ty, int n);

    TypePtr getElementType() const;
    int getNumElements() const;
};

class FunctionType : public Type {
    TypePtr returnType;
    std::vector<TypePtr> paramTypes;
    bool variadicFlag;
public:
    FunctionType(TypePtr retTy, const std::vector<TypePtr>& params, bool v = false);

    TypePtr getReturnType() const;

    const size_t getParamsCount() const;
    TypePtr getParamType(size_t index) const;
    const std::vector<TypePtr>& getParamTypes() const;

    bool isVariadic() const;
    void setVariadic(bool v);
};

TypePtr convertToType(TypeSpecifier ts);
TypePtr convertToType(TypeSpecifier ts, TypeQualifier tq);

#endif