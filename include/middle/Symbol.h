#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <memory>
#include "Type.h"
#include "Value.h"

class Symbol {
public:
    enum SymbolKind {
        VARIABLE,
        FUNCTION,
    };

private:
    std::string name;
    StorageClass storageClass;
    TypePtr type;
    SymbolKind kind;
    ValuePtr value; // For IR generation

    bool isGlobal;
public:
    Symbol(std::string name, StorageClass storageClass, TypePtr type, SymbolKind kind, bool isGlobal)
        : name(name), storageClass(storageClass), type(type), kind(kind), value(nullptr), isGlobal(isGlobal) {}

    const std::string& getName() const { return name; }
    TypePtr getType() const { return type; }
    StorageClass getStorageClass() const { return storageClass; }
    SymbolKind getKind() const { return kind; }
    ValuePtr getValue() const { return value; }
    void setValue(ValuePtr v) { value = v; }
    bool getIsGlobal() const { return isGlobal; }
    void setIsGlobal(bool g) { isGlobal = g; }
};

using SymbolPtr = std::shared_ptr<Symbol>;

#endif