#include "../../../include/MiddleEnd/SemanticAnalysis/ClangContext.h"
#include "../../../include/MiddleEnd/SemanticAnalysis/Type.h"

#include <unordered_map>
#include <stdio.h>

namespace Clang {

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

// ClangContext 的私有实现，存储所有唯一的类型实例
class ClangContextImpl {
public:
    // 构造函数，需要传入外部的 ClangContext 引用
    ClangContextImpl(ClangContext& C) :
        voidTy(C),
        integerTy(C),
        charTy(C) {}

    // 基础类型直接作为成员变量
    VoidType voidTy;
    IntegerType integerTy;
    CharType charTy;

    // 派生类型的缓存
    std::unordered_map<Type*, PointerType*> pointerTypes;
    std::unordered_map<std::pair<Type*, size_t>, ArrayType*, pair_hash> arrayTypes;

    // 限定类型的缓存
    std::unordered_map<std::pair<Type*, unsigned int>, Type*, pair_hash> qualifiedTypes;
    
    // 使用 unique_ptr 的 vector 来自动管理所有动态分配的类型的内存
    std::vector<std::unique_ptr<Type>> ownedTypes;

    // 限定类型的辅助创建函数
    Type* createQualifiedType(Type* baseType, unsigned int qualifier) {
        switch (baseType->getTypeID()) {
            case Type::IntegerTyID: {
                auto newType = std::unique_ptr<IntegerType>(new IntegerType(baseType->getContext()));
                newType->setQualifier(qualifier);
                Type* result = newType.get();
                ownedTypes.push_back(std::move(newType));
                qualifiedTypes[std::make_pair(baseType, qualifier)] = result;
                return result;
            }
            default:
                return nullptr;
        }
    }

    // 指针类型的辅助创建函数
    PointerType* createPointerType(Type* elementType) {
        auto newType = std::unique_ptr<PointerType>(new PointerType(elementType));
        PointerType* result = newType.get();
        ownedTypes.push_back(std::move(newType));
        pointerTypes[elementType] = result;
        return result;
    }

    // 数组类型的辅助创建函数
    ArrayType* createArrayType(Type* elementType, size_t size) {
        auto newType = std::unique_ptr<ArrayType>(new ArrayType(elementType, size));
        ArrayType* result = newType.get();
        ownedTypes.push_back(std::move(newType));
        arrayTypes[std::make_pair(elementType, size)] = result;
        return result;
    }

    // 函数类型的辅助创建函数
    FunctionType* createFunctionType(Type* returnType, const std::vector<Type*>& params, bool isVarArg) {
        auto newType = std::unique_ptr<FunctionType>(new FunctionType(returnType, params, isVarArg));
        FunctionType* result = newType.get();
        ownedTypes.push_back(std::move(newType));
        return result;
    }

    void scanTypes(unsigned int mode) {
        switch (mode)
        {
        case 0:
            for (const auto& type : ownedTypes) {
                Type::TypeID id = type->getTypeID();
                printf("TypeID: %d\n", static_cast<int>(id));
            }
            break;
        case 1:
            for (const auto& pair : pointerTypes) {
                Type* pointedType = pair.first;
                printf("Pointer to TypeID: %d\n", static_cast<int>(pointedType->getTypeID()));
            }
            break;
        case 2:
            for (const auto& pair : arrayTypes) {
                Type* elementType = pair.first.first;
                size_t size = pair.first.second;
                printf("Array of TypeID: %d, Size: %zu\n", static_cast<int>(elementType->getTypeID()), size);
            }
            break;
        case 3:
            for (const auto& pair : qualifiedTypes) {
                Type* baseType = pair.first.first;
                unsigned int qualifier = pair.first.second;
                printf("Qualified TypeID: %d, Qualifier: %u\n", static_cast<int>(baseType->getTypeID()), qualifier);
            }
            break;
        default:
            break;
        }
    }
};

}

// --- ClangContext 的实现 ---

ClangContext::ClangContext() : pImpl(new Clang::ClangContextImpl(*this)) {}
ClangContext::~ClangContext() = default;

Clang::VoidType* ClangContext::getVoidTyImpl() {
    return &pImpl->voidTy;
}

Clang::IntegerType* ClangContext::getIntegerTyImpl() {
    return &pImpl->integerTy;
}

Clang::CharType* ClangContext::getCharTyImpl() {
    return &pImpl->charTy;
}

Clang::PointerType* ClangContext::getPointerTyImpl(Clang::Type* elementType) {
    // 1. 在缓存中查找
    auto it = pImpl->pointerTypes.find(elementType);
    if (it != pImpl->pointerTypes.end()) {
        return it->second; // 找到了，直接返回
    }

    // 2. 如果没找到，转发创建任务给impl
    return pImpl->createPointerType(elementType);
}

Clang::ArrayType* ClangContext::getArrayTyImpl(Clang::Type* elementType, size_t size) {
    auto key = std::make_pair(elementType, size);
    auto it = pImpl->arrayTypes.find(key);
    if (it != pImpl->arrayTypes.end()) {
        return it->second;
    }

    return pImpl->createArrayType(elementType, size);
}

Clang::FunctionType* ClangContext::getFunctionTyImpl(Clang::Type* returnType, const std::vector<Clang::Type*>& params, bool isVarArg) {
    return pImpl->createFunctionType(returnType, params, isVarArg);
}

Clang::Type* ClangContext::getQualifiedTyImpl(Clang::Type* baseType, unsigned int qualifier) {
    if (qualifier == 0) {
        return baseType;
    }

    auto key = std::make_pair(baseType, qualifier);
    auto it = pImpl->qualifiedTypes.find(key);
    if (it != pImpl->qualifiedTypes.end()) {
        return it->second;
    }

   return pImpl->createQualifiedType(baseType, qualifier);
}

void ClangContext::scan(unsigned int mode) {
    pImpl->scanTypes(mode);
}