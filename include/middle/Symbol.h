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

public:
    Symbol(std::string name, StorageClass storageClass, TypePtr type, SymbolKind kind)
        : name(name), storageClass(storageClass), type(type), kind(kind), value(nullptr) {}

    const std::string& getName() const { return name; }
    TypePtr getType() const { return type; }
    StorageClass getStorageClass() const { return storageClass; }
    SymbolKind getKind() const { return kind; }
    ValuePtr getValue() const { return value; }
    void setValue(ValuePtr v) { value = v; }
};

using SymbolPtr = std::shared_ptr<Symbol>;

#endif