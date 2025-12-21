#ifndef LLVM_TYPE_H
#define LLVM_TYPE_H

#include <vector>
#include <memory>

class LLVMContext;

namespace LLVM {

class LLVMContextImpl;

class Type {
public:
    enum TypeID {
        VoidTyID,
        IntegerTyID,

        PointerTyID,
        ArrayTyID,
        FunctionTyID
    };
private:
    TypeID typeIdentifier;
    LLVMContext& context;
protected:
    // 明确通过构造函数创建实例
    explicit Type(LLVMContext& context, TypeID id);
public:
    // 禁用拷贝构造和赋值操作
    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;

    TypeID getTypeID() const;
    LLVMContext& getContext() const;

    // 便捷类型检查
    bool isVoid() const;
    bool isInteger() const;
    bool isPointer() const;
    bool isArray() const;
    bool isFunction() const;

    // 静态获取基础类型实例
    static Type* getVoidTy(LLVMContext& context);
    static Type* getInt1Ty(LLVMContext& context);
    static Type* getInt8Ty(LLVMContext& context);
    static Type* getInt32Ty(LLVMContext& context);
};

class VoidType : public Type {
private:
    // 私有构造函数，防止外部直接创建实例，由get()调用
    VoidType(LLVMContext& context);

    friend class LLVMContextImpl;
public:
    static VoidType* get(LLVMContext& context);
};

class IntegerType : public Type {
private:
    unsigned int bitWidth;

    IntegerType(LLVMContext& context, unsigned int bw);

    friend class LLVMContextImpl;
public:
    // 获取特定位宽的整数类型实例
    static IntegerType* get(LLVMContext& context, unsigned int bw);
    unsigned int getBitWidth() const;
};

class PointerType : public Type {
private:
    Type* pointedType;

    PointerType(Type* elementType);

    friend class LLVMContextImpl;
public:
    // 获取指向指定元素类型的指针类型实例
    static PointerType* get(Type* elementType);
    Type* getPointedType() const;
};

class ArrayType : public Type {
private:
    Type* elementType;
    size_t size;

    ArrayType(Type* elemType, size_t sz);

    friend class LLVMContextImpl;
public:
    // 获取指定元素类型和大小的数组类型实例
    static ArrayType* get(Type* elemType, size_t sz);
    Type* getElementType() const;
    size_t getSize() const;
};

class FunctionType : public Type {
private:
    Type* returnType;
    std::vector<Type*> paramTypes;
    bool variadicFlag;

    FunctionType(Type* retType, const std::vector<Type*>& params, bool vf);

    friend class LLVMContextImpl;
public:
    // 获取指定返回类型和参数类型的函数类型实例
    static FunctionType* get(Type* retType, const std::vector<Type*>& params, bool vf = false);
    Type* getReturnType() const;
    size_t getParamsCount() const;
    Type* getParamType(size_t index) const;
    const std::vector<Type*>& getParamTypes() const;
    bool isVariadicFunc() const;
    bool setVariadicFlag(bool vf);
};

}

#endif