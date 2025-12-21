#ifndef ITERASTMT_H
#define ITERASTMT_H
#include "Statement.h"
#include "../Expression/Expression.h"

/* 
 * this is iteration statement part
 */

class ForStmt : public Stmt {
private:
    ExpPtr initial;
    ExpPtr condition;
    ExpPtr iteration;
    StmtPtr stmt;
public:
    ForStmt(size_t sl);

    Exp* getInitial() const;
    ExpPtr releaseInitial();
    void setInitial(ExpPtr s);

    Exp* getCondition() const;
    ExpPtr releaseCondition();
    void setCondition(ExpPtr cond);

    Exp* getIteration() const;
    ExpPtr releaseIteration();
    void setIteration(ExpPtr iter);

    Stmt* getBody() const;
    StmtPtr releaseBody();
    void setBody(StmtPtr s);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using ForStmtPtr = std::unique_ptr<ForStmt>;

#endif