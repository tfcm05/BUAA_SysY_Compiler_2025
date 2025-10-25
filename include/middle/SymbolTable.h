#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include<unordered_map>
#include<memory>
#include"Symbol.h"

class SymbolTable : public std::enable_shared_from_this<SymbolTable> {
    private:
        std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
        std::shared_ptr<SymbolTable> father;

        bool existInScope(const std::string &name);

        // 作用域序号
        int id;
        static int nextId;
    public:
        SymbolTable(); // 自增 id 的构造函数

        std::shared_ptr<SymbolTable> pushScope();
        std::shared_ptr<SymbolTable> popScope();

        // 返回是否成功添加（若当前作用域已存在相同名字则返回 false）
        bool addSymbol(const std::string& name, std::shared_ptr<Symbol> symbol);
        // 在当前作用域及父作用域中查找符号（自下而上）
        std::shared_ptr<Symbol> lookup(const std::string &name);

        int getId() const { return id; }
};

extern std::shared_ptr<SymbolTable> curScope;

#endif