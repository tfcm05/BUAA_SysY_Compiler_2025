#ifndef ANALYSIS_RESULT_H
#define ANALYSIS_RESULT_H

#include <cstdint>
#include <unordered_map>

class AstNode;
class Exp;
class Symbol;
namespace Clang {
class Type;
class ConstValue;
}

// Session annotations produced by semantic analysis.
// Keeps Symbol*/type/const off the AST blackboard over time; callers should
// prefer these maps. AST setSymbol remains as a transitional dual-write.
class AnalysisResult {
private:
    std::unordered_map<AstNode*, Symbol*> symbols;
    std::unordered_map<Exp*, Clang::Type*> expTypes;
    std::unordered_map<Exp*, Clang::ConstValue*> expConsts;
    std::unordered_map<AstNode*, int64_t> initConsts;
public:
    void bindSymbol(AstNode* node, Symbol* symbol) {
        if (!node) {
            return;
        }
        symbols[node] = symbol;
    }

    Symbol* getSymbol(AstNode* node) const {
        if (!node) {
            return nullptr;
        }
        auto it = symbols.find(node);
        return it == symbols.end() ? nullptr : it->second;
    }

    bool hasSymbol(AstNode* node) const {
        return node && symbols.find(node) != symbols.end();
    }

    void setExpType(Exp* exp, Clang::Type* type) {
        if (!exp) {
            return;
        }
        expTypes[exp] = type;
    }

    Clang::Type* getExpType(Exp* exp) const {
        if (!exp) {
            return nullptr;
        }
        auto it = expTypes.find(exp);
        return it == expTypes.end() ? nullptr : it->second;
    }

    void setExpConst(Exp* exp, Clang::ConstValue* value) {
        if (!exp) {
            return;
        }
        expConsts[exp] = value;
    }

    Clang::ConstValue* getExpConst(Exp* exp) const {
        if (!exp) {
            return nullptr;
        }
        auto it = expConsts.find(exp);
        return it == expConsts.end() ? nullptr : it->second;
    }

    void setInitConst(AstNode* init, int64_t value) {
        if (!init) {
            return;
        }
        initConsts[init] = value;
    }

    bool hasInitConst(AstNode* init) const {
        return init && initConsts.find(init) != initConsts.end();
    }

    int64_t getInitConst(AstNode* init) const {
        auto it = initConsts.find(init);
        return it == initConsts.end() ? 0 : it->second;
    }

    void clear() {
        symbols.clear();
        expTypes.clear();
        expConsts.clear();
        initConsts.clear();
    }

    size_t symbolCount() const { return symbols.size(); }
};

#endif
