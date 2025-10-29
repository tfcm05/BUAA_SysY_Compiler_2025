#include"../../include/middle/Symbol.h"
#include"../../include/middle/SymbolTable.h"
#include <iostream>

std::shared_ptr<SymbolTable> curScope;

// 静态成员初始化
int SymbolTable::nextId = 1;

SymbolTable::SymbolTable() : id(nextId++) { }

std::shared_ptr<SymbolTable> SymbolTable::pushScope() {
    auto newTable = std::make_shared<SymbolTable>();
    newTable->father = shared_from_this();
    return newTable;
}

std::shared_ptr<SymbolTable> SymbolTable::popScope() {
    return father; 
}

void SymbolTable::addSymbol(const std::string& name, std::shared_ptr<Symbol> symbolPtr) {
    symbols[name] = symbolPtr;
}

bool SymbolTable::existInCurScope(const std::string &name) {
    return symbols.find(name) != symbols.end();
}

bool SymbolTable::existInCurAndPreScope(const std::string &name) {
    if (existInCurScope(name)) return true;
    if (father) return father->existInCurAndPreScope(name);
    return false;
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string &name) {
    // 从当前表向父作用域逐级查找
    std::shared_ptr<SymbolTable> t = shared_from_this();
    while (t) {
        auto it = t->symbols.find(name);
        if (it != t->symbols.end()) return it->second;
        t = t->father;
    }
    return nullptr;
}

int SymbolTable::getScopeId() const {
    return id;
}