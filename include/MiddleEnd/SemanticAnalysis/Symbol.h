#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <memory>
#include "DeclEnums.h"
#include "Type.h"
#include "ConstValue.h"

class Symbol {
public:
    enum SymbolKind {
        VARIABLE,
        FUNCTION,
        PARAMETER,
    };
private:
    std::string name;
    StorageClass storageClass;
    Clang::Type* type;
    SymbolKind kind;

    Clang::ConstValue* constValue = nullptr;
public:
    Symbol(std::string name, StorageClass storageClass, Clang::Type* type, SymbolKind kind, Clang::ConstValue* constValue);

    const std::string& getName() const;
    void setName(const std::string& newName);

    Clang::Type* getType() const;
    void setType(Clang::Type* t);

    StorageClass getStorageClass() const;
    void setStorageClass(StorageClass sc);
    bool isStatic() const;

    SymbolKind getKind() const;
    void setKind(SymbolKind k);

    Clang::ConstValue* getConstValue() const;
    void setConstValue(Clang::ConstValue* cv);

    std::string typeToString();
};

using SymbolPtr = std::unique_ptr<Symbol>;

#endif
