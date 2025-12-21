#ifndef SELECTSTMT_H
#define SELECTSTMT_H
#include "Statement.h"
#include "../Expression/Expression.h"

/* 
 * this is select statement part
 */

class IfStmt : public Stmt {
private:
    ExpPtr condition;
    StmtPtr trueStmt;
    StmtPtr falseStmt;
public:
    IfStmt(size_t sl);

    Exp* getCondition() const;
    ExpPtr releaseCondition();
    void setCondition(ExpPtr cond);

    Stmt* getTrueStmt() const;
    StmtPtr releaseTrueStmt();
    void setTrueStmt(StmtPtr trueStatement);

    Stmt* getFalseStmt() const;
    StmtPtr releaseFalseStmt();
    void setFalseStmt(StmtPtr falseStatement);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using IfStmtPtr = std::unique_ptr<IfStmt>;

#endif