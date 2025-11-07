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
    Symbol(std::string name, StorageClass storageClass, TypePtr type, SymbolKind kind);

    const std::string& getName() const;
    TypePtr getType() const;
    StorageClass getStorageClass() const;
    SymbolKind getKind() const;
    ValuePtr getValue() const;
    void setValue(ValuePtr v);
};

using SymbolPtr = std::shared_ptr<Symbol>;

#endif