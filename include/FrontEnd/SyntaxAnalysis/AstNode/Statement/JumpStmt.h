#ifndef JUMPSTMT_H
#define JUMPSTMT_H
#include "Statement.h"
#include "../Expression/Expression.h"

/* 
 * this is jump statement part
 */

class BreakStmt : public Stmt {
public:
    BreakStmt(size_t sl);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt(size_t sl);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

class ReturnStmt : public Stmt {
private:
    ExpPtr returnExp;
public:
    ReturnStmt(size_t sl);

    Exp* getReturnExp() const;
    ExpPtr releaseReturnExp();
    void setReturnExp(ExpPtr reType);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using BreakStmtPtr = std::unique_ptr<BreakStmt>;
using ContinueStmtPtr = std::unique_ptr<ContinueStmt>;
using ReturnStmtPtr = std::unique_ptr<ReturnStmt>;

#endif