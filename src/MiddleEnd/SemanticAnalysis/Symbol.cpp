#include "MiddleEnd/SemanticAnalysis/Symbol.h"

Symbol::Symbol(std::string name, StorageClass storageClass, Clang::Type* type, SymbolKind kind, Clang::ConstValue* constValue)
    : name(std::move(name)), storageClass(storageClass), type(type), kind(kind), constValue(constValue) {}

const std::string& Symbol::getName() const { return name; }
void Symbol::setName(const std::string& newName) { name = newName; }

Clang::Type* Symbol::getType() const { return type; }
void Symbol::setType(Clang::Type* t) { type = t; }

StorageClass Symbol::getStorageClass() const { return storageClass; }
void Symbol::setStorageClass(StorageClass sc) { storageClass = sc; }
bool Symbol::isStatic() const { return storageClass == StorageClass::STATIC; }

Symbol::SymbolKind Symbol::getKind() const { return kind; }
void Symbol::setKind(SymbolKind k) { kind = k; }

Clang::ConstValue* Symbol::getConstValue() const { return constValue; }
void Symbol::setConstValue(Clang::ConstValue* cv) { constValue = cv; }

static std::string typeToStringRecursive(const Clang::Type* type) {
    if (!type) {
        return "UnknownType";
    }

    std::string prefix;
    if (type->isConst()) {
        prefix = "const ";
    }

    switch (type->getTypeID()) {
        case Clang::Type::VoidTyID:
            return prefix + "void";
        case Clang::Type::CharTyID:
            return prefix + "char";
        case Clang::Type::IntegerTyID:
            return prefix + "int";
        case Clang::Type::PointerTyID: {
            auto pointerType = static_cast<const Clang::PointerType*>(type);
            return prefix + typeToStringRecursive(pointerType->getPointedType()) + "*";
        }

        case Clang::Type::ArrayTyID: {
            auto arrayType = static_cast<const Clang::ArrayType*>(type);
            size_t size = arrayType->getSize();
            return prefix + typeToStringRecursive(arrayType->getElementType()) + "[" + (size != 0 ? std::to_string(size) : "") + "]";
        }

        case Clang::Type::FunctionTyID: {
            auto funcType = static_cast<const Clang::FunctionType*>(type);
            std::string paramsStr;
            const auto& params = funcType->getParamTypes();
            for (size_t i = 0; i < params.size(); ++i) {
                paramsStr += typeToStringRecursive(params[i]);
                if (i < params.size() - 1) {
                    paramsStr += ", ";
                }
            }
            return prefix + typeToStringRecursive(funcType->getReturnType()) + "(" + paramsStr + ")";
        }

        default:
            return "UnhandledType";
    }
}

std::string Symbol::typeToString() {
    std::string prefix;
    if (storageClass == StorageClass::STATIC) {
        prefix = "static ";
    }
    
    return prefix + typeToStringRecursive(this->type);
}
