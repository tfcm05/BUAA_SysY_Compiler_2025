#include "MiddleEnd/SemanticAnalysis/SymbolTable.h"

std::shared_ptr<SymbolTable> curScope;

// static member initialization
size_t SymbolTable::nextId = 1;

SymbolTable::SymbolTable() : id(nextId++) { }

std::shared_ptr<SymbolTable> SymbolTable::pushScope() {
    auto newTable = std::make_shared<SymbolTable>();
    newTable->father = shared_from_this();
    return newTable;
}

std::shared_ptr<SymbolTable> SymbolTable::popScope() {
    return father; 
}

void SymbolTable::addSymbol(const std::string& name, Symbol* symbol) {
    if (!existInCurScope(name)) {
        symbols[name] = symbol;
    }
}

bool SymbolTable::existInCurScope(const std::string& name) {
    return symbols.find(name) != symbols.end();
}

bool SymbolTable::existInCurAndPreScope(const std::string& name) {
    if (existInCurScope(name)) return true;
    if (father) return father->existInCurAndPreScope(name);
    return false;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    std::shared_ptr<SymbolTable> t = shared_from_this();
    while (t) {
        auto it = t->symbols.find(name);
        if (it != t->symbols.end()) return it->second;
        t = t->father;
    }
    return nullptr;
}

size_t SymbolTable::getScopeId() const {
    return id;
}
