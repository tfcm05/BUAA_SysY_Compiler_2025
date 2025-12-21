#ifndef STATEMENT_H
#define STATEMENT_H
#include "../AstNode.h"

/* 
 * this is statement part
 */

class Stmt : public AstNode {
public:
    Stmt(size_t sl);
    virtual ~Stmt() = default;
};

using StmtPtr = std::unique_ptr<Stmt>;

#endif