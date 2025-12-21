#ifndef CLANG_TYPE_H
#define CLANG_TYPE_H

#include "../../FrontEnd/SyntaxAnalysis/AstNode/Declaration/DeclSpec.h"
#include <vector>
#include <memory>

class ClangContext;

namespace Clang {

class ClangContextImpl;

class Type {
public:
    enum TypeID {
        VoidTyID,
        CharTyID, // i8
        IntegerTyID, // i32

        ArrayTyID,
        PointerTyID,
        FunctionTyID,
    };
private:
    TypeID typeIdentifier;
    unsigned int qualifier;
    ClangContext& context;
protected:
    // 明确通过构造函数创建实例
    explicit Type(ClangContext& context, TypeID id);
public:
    // 禁用拷贝构造和赋值操作
    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;

    TypeID getTypeID() const;
    ClangContext& getContext() const;

    virtual ~Type() = default;

    // 基本类型
    bool isVoidTy() const;
    bool isCharTy() const;
    bool isIntegerTy() const;

    // 派生类型
    bool isArrayTy() const;
    bool isPointerTy() const;
    bool isFunctionTy() const;

    // 限定类型
    unsigned int getQualifier() const;
    void setQualifier(unsigned int tq);
    void addQualifier(unsigned int tq);
    bool isConst() const;
};

// 空类型
class VoidType : public Type {
private:
    VoidType(ClangContext& context);
    friend class ClangContextImpl;
public:
    static VoidType* get(ClangContext& context);
};

// 字符类型 (i8)
class CharType : public Type {
private:
    CharType(ClangContext& context);
    friend class ClangContextImpl;
public:
    static CharType* get(ClangContext& context);
};

// 整数类型 (i32)
class IntegerType : public Type {
private:
    IntegerType(ClangContext& context);
    friend class ClangContextImpl;
public:
    static IntegerType* get(ClangContext& context);
};

// 数组类型
class ArrayType : public Type {
private:
    Type* elementType;
    size_t size;

    ArrayType(Type* eleTy, size_t s);
    friend class ClangContextImpl;
public:
    static ArrayType* get(Type* eleTy, size_t s);

    Type* getElementType() const;
    size_t getSize() const;
};

// 指针类型
class PointerType : public Type {
private:
    Type* pointedType;
    PointerType(Type* pointedType);
    friend class ClangContextImpl;
public:
    static PointerType* get(Type* pointedType);

    Type* getPointedType() const;
};

// 函数类型
class FunctionType : public Type {
private:
    Type* returnType;
    std::vector<Type*> paramTypes;
    bool variadicFlag;

    FunctionType(Type* retTy, const std::vector<Type*>& params, bool vf = false);
    friend class ClangContextImpl;
public:
    static FunctionType* get(Type* retTy, const std::vector<Type*>& params, bool vf = false);

    Type* getReturnType() const;
    size_t getParamTypesCount() const;
    Type* getParamType(size_t index) const;
    const std::vector<Type*>& getParamTypes() const;
    bool isVariadic() const;
    void setVariadicFlag(bool vf);
};

}

#endif