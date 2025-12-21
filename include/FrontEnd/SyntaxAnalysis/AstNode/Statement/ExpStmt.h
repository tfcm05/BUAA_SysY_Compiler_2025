#ifndef EXPSTMT_H
#define EXPSTMT_H 
#include "Statement.h"
#include "../Expression/Expression.h"

class ExpStmt : public Stmt {
private:
    ExpPtr exp;
public:
    ExpStmt(size_t sl);

    Exp* getExp() const;
    ExpPtr releaseExp();
    void setExp(ExpPtr e);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using ExpStmtPtr = std::unique_ptr<ExpStmt>;

#endif