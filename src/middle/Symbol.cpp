#include "../../include/middle/Symbol.h"

Symbol::Symbol(std::string name, StorageClass storageClass, TypePtr type, SymbolKind kind)
    : name(std::move(name)), storageClass(storageClass), type(std::move(type)), kind(kind), value(nullptr) {}

const std::string& Symbol::getName() const { return name; }
TypePtr Symbol::getType() const { return type; }
StorageClass Symbol::getStorageClass() const { return storageClass; }
Symbol::SymbolKind Symbol::getKind() const { return kind; }
ValuePtr Symbol::getValue() const { return value; }
void Symbol::setValue(ValuePtr v) { value = v; }