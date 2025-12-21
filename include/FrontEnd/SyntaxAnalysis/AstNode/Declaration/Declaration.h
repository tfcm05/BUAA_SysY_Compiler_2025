#ifndef DECLARATION_H
#define DECLARATION_H
#include "DeclSpec.h"
#include "Declarator/Declarator.h"
#include "Declarator/Initializer/Initializer.h"
#include "../AstNode.h"
#include "../../AstVisitor.h"

/*
 * this is declaration part
 */

class Decl : public AstNode {
public:
    Decl(size_t sl);
    virtual ~Decl() = default;
};

using DeclPtr =  std::unique_ptr<Decl>;

#endif