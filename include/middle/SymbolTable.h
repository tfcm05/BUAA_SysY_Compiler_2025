#include<unordered_map>
#include<memory>
#include"Symbol.h"

static std::shared_ptr<SymbolTable> curScope;

class SymbolTable : public std::enable_shared_from_this<SymbolTable> {
    private:
        std::unordered_map<std::string, std::shared_ptr<Symbol>> _symbols;
        std::shared_ptr<SymbolTable> _father;
    public:
        std::shared_ptr<SymbolTable> push_scope();
        std::shared_ptr<SymbolTable> pop_scope();
};