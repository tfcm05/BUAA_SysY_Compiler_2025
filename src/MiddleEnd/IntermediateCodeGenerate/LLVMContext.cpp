#include "../../../include/MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"
#include <unordered_map>

namespace LLVM {

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

class LLVMContextImpl {
public:
    // 构造函数，需要传入外部的 LLVMContext 引用
    LLVMContextImpl(LLVMContext& C) :
        voidTy(C),
        int1Ty(C, 1),
        int8Ty(C, 8),
        int32Ty(C, 32) {}

    // 基础类型直接作为成员
    VoidType voidTy;
    IntegerType int1Ty;
    IntegerType int8Ty;
    IntegerType int32Ty;

    // 派生类型的缓存
    std::unordered_map<unsigned int, IntegerType*> integerTypes;
    std::unordered_map<Type*, PointerType*> pointerTypes;
    std::unordered_map<std::pair<Type*, size_t>, ArrayType*, pair_hash> arrayTypes;
    
    // 使用 unique_ptr 来自动管理所有动态分配的类型的内存
    std::vector<std::unique_ptr<Type>> ownedTypes;

    IntegerType* createIntegerType(LLVMContext& C, unsigned int bitWidth) {
        auto newType = std::unique_ptr<IntegerType>(new IntegerType(C, bitWidth));
        IntegerType* result = newType.get();
        ownedTypes.push_back(std::move(newType));
        integerTypes[bitWidth] = result;
        return result;
    }

    PointerType* createPointerType(Type* elementType) {
        auto newType = std::unique_ptr<PointerType>(new PointerType(elementType));
        PointerType* result = newType.get();
        ownedTypes.push_back(std::move(newType));
        pointerTypes[elementType] = result;
        return result;
    }

    ArrayType* createArrayType(Type* elementType, size_t size) {
        auto newType = std::unique_ptr<ArrayType>(new ArrayType(elementType, size));
        ArrayType* result = newType.get();
        ownedTypes.push_back(std::move(newType));
        arrayTypes[std::make_pair(elementType, size)] = result;
        return result;
    }

    FunctionType* createFunctionType(Type* returnType, const std::vector<Type*>& params, bool isVarArg) {
        auto newType = std::unique_ptr<FunctionType>(new FunctionType(returnType, params, isVarArg));
        FunctionType* result = newType.get();
        ownedTypes.push_back(std::move(newType));
        return result;
    }
};

}

LLVMContext::LLVMContext() : pImpl(new LLVM::LLVMContextImpl(*this)) {}
LLVMContext::~LLVMContext() = default;

LLVM::VoidType* LLVMContext::getVoidTyImpl() {
    return &pImpl->voidTy;
}

LLVM::IntegerType* LLVMContext::getIntegerTyImpl(unsigned int bitWidth) {
    // 优先返回预先创建的常用类型
    if (bitWidth == 1) return &pImpl->int1Ty;
    if (bitWidth == 8) return &pImpl->int8Ty;
    if (bitWidth == 32) return &pImpl->int32Ty;

    // 在缓存中查找
    auto it = pImpl->integerTypes.find(bitWidth);
    if (it != pImpl->integerTypes.end()) {
        return it->second;
    }

    return pImpl->createIntegerType(*this, bitWidth);
}

LLVM::PointerType* LLVMContext::getPointerTyImpl(LLVM::Type* elementType) {
    auto it = pImpl->pointerTypes.find(elementType);
    if (it != pImpl->pointerTypes.end()) {
        return it->second;
    }

    return pImpl->createPointerType(elementType);
}

LLVM::ArrayType* LLVMContext::getArrayTyImpl(LLVM::Type* elementType, size_t size) {
    auto key = std::make_pair(elementType, size);
    auto it = pImpl->arrayTypes.find(key);
    if (it != pImpl->arrayTypes.end()) {
        return it->second;
    }

    return pImpl->createArrayType(elementType, size);
}

LLVM::FunctionType* LLVMContext::getFunctionTyImpl(LLVM::Type* returnType, const std::vector<LLVM::Type*>& params, bool isVarArg) {
    return pImpl->createFunctionType(returnType, params, isVarArg);
}