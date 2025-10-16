#include"include/middle/SymbolTable.h"

std::shared_ptr<SymbolTable> SymbolTable::push_scope() {
    auto new_table = std::make_shared<SymbolTable>();
    new_table->_father = shared_from_this();
    return new_table;
}

std::shared_ptr<SymbolTable> SymbolTable::pop_scope() {
    return _father; 
}