#include "../../../include/MiddleEnd/SemanticAnalysis/Symbol.h"

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

Value* Symbol::getValue() const { return value; }
void Symbol::setValue(Value* v) { value = v; }

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
            // 递归调用以处理指向的类型，然后加上 '*'
            return prefix + typeToStringRecursive(pointerType->getPointedType()) + "*";
        }

        case Clang::Type::ArrayTyID: {
            auto arrayType = static_cast<const Clang::ArrayType*>(type);
            // 递归调用以处理元素类型，然后加上 '[size]'
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
            // 递归调用以处理返回类型，然后加上参数列表
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