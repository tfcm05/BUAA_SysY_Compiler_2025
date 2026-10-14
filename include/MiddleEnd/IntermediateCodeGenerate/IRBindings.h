#ifndef IR_BINDINGS_H
#define IR_BINDINGS_H

#include <unordered_map>

class Symbol;
class Value;

// Session map: Symbol* -> LLVM IR Value* for the current IR generation.
// Keeps IR bindings off Symbol so semantic analysis stays free of IR state.
class IRBindings {
private:
    std::unordered_map<Symbol*, Value*> bindings;
public:
    void set(Symbol* symbol, Value* value) {
        if (!symbol) {
            return;
        }
        bindings[symbol] = value;
    }

    Value* get(Symbol* symbol) const {
        if (!symbol) {
            return nullptr;
        }
        auto it = bindings.find(symbol);
        return it == bindings.end() ? nullptr : it->second;
    }

    bool contains(Symbol* symbol) const {
        return symbol && bindings.find(symbol) != bindings.end();
    }

    void clear() { bindings.clear(); }
};

#endif
