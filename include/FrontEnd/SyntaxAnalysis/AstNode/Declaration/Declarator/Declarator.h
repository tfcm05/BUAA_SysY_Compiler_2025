#ifndef DECLARATOR_H
#define DECLARATOR_H
#include "../../AstNode.h"

class Declarator : public AstNode {
public:
    Declarator(size_t sl);

    virtual ~Declarator() = default;
};

using DeclaratorPtr = std::unique_ptr<Declarator>;

#endif
