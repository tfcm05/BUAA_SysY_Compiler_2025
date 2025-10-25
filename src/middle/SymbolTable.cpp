#include"../../include/middle/Symbol.h"
#include"../../include/middle/SymbolTable.h"
#include <iostream>

std::shared_ptr<SymbolTable> curScope;

// 静态成员初始化
int SymbolTable::nextId = 0;

SymbolTable::SymbolTable() : id(nextId++) { }

std::shared_ptr<SymbolTable> SymbolTable::pushScope() {
    auto newTable = std::make_shared<SymbolTable>();
    newTable->father = shared_from_this();
    return newTable;
}

std::shared_ptr<SymbolTable> SymbolTable::popScope() {
    return father; 
}

bool SymbolTable::existInScope(const std::string &name) {
    return symbols.find(name) != symbols.end();
}

bool SymbolTable::addSymbol(const std::string& name, std::shared_ptr<Symbol> symbolPtr) {
    if (!existInScope(name)) {
        symbols[name] = symbolPtr;
        return true;
    } else {
        // 简单错误输出，后续可替换为工程统一的 errorLog 接口
        std::cerr << "Error: symbol '" << name << "' is already defined in current scope.\n";
        return false;
    }
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