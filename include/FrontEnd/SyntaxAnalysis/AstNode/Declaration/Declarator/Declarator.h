#ifndef DECLARATOR_H
#define DECLARATOR_H
#include "../../AstNode.h"

class Declarator : public AstNode {
private:
    Symbol* symbol = nullptr;
public:
    Declarator(size_t sl);

    Symbol* getSymbol() const;
    void setSymbol(Symbol* s);

    virtual ~Declarator() = default;
};

using DeclaratorPtr = std::unique_ptr<Declarator>;

#endif
