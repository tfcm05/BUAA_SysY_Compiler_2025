#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <unordered_map>
#include <memory>
#include "Symbol.h"

using SymbolTablePtr = std::shared_ptr<class SymbolTable>;

class SymbolTable : public std::enable_shared_from_this<SymbolTable> {
private:
    std::unordered_map<std::string, Symbol*> symbols;
    std::shared_ptr<SymbolTable> father;

    // 作用域序号
    size_t id;
    static size_t nextId;

public:
    SymbolTable();

    std::shared_ptr<SymbolTable> pushScope();
    std::shared_ptr<SymbolTable> popScope();

    // 在当前作用域中添加符号
    void addSymbol(const std::string& name, Symbol* symbol);
    // 仅在当前作用域中查找符号
    bool existInCurScope(const std::string& name);
    // 在当前作用域及之前作用域中查找符号
    bool existInCurAndPreScope(const std::string& name);
    // 在当前作用域及之前作用域中查找符号，并返回符号
    Symbol* lookup(const std::string& name);

    size_t getScopeId() const;
};

extern std::shared_ptr<SymbolTable> curScope;

#endif