#ifndef SEMANTIC_SCOPE_GUARD_H
#define SEMANTIC_SCOPE_GUARD_H

#include "SymbolTable.h"
#include <memory>

// RAII push/pop of SymbolTable scope.
class ScopeGuard {
public:
    explicit ScopeGuard(std::shared_ptr<SymbolTable>& scope) : currentScope(scope) {
        currentScope = currentScope->pushScope();
    }
    ~ScopeGuard() {
        currentScope = currentScope->popScope();
    }
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
private:
    std::shared_ptr<SymbolTable>& currentScope;
};

#endif
