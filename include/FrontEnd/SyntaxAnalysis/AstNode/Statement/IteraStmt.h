#ifndef ITERASTMT_H
#define ITERASTMT_H
#include "Statement.h"
#include "../Expression/Expression.h"
#include "../Declaration/VariableDecl.h"

/* 
 * this is iteration statement part
 */

class ForStmt : public Stmt {
private:
    ExpPtr initial;
    VariableDeclPtr initialDecl; // for(int i=0; ...) 形式的初始化声明
    ExpPtr condition;
    ExpPtr iteration;
    StmtPtr stmt;
public:
    ForStmt(size_t sl);

    Exp* getInitial() const;
    ExpPtr releaseInitial();
    void setInitial(ExpPtr s);

    bool hasInitialDecl() const;
    VariableDecl* getInitialDecl() const;
    VariableDeclPtr releaseInitialDecl();
    void setInitialDecl(VariableDeclPtr d);

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

class WhileStmt : public Stmt {
private:
    ExpPtr condition;
    StmtPtr stmt;
public:
    WhileStmt(size_t sl);

    Exp* getCondition() const;
    ExpPtr releaseCondition();
    void setCondition(ExpPtr cond);

    Stmt* getBody() const;
    StmtPtr releaseBody();
    void setBody(StmtPtr s);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using WhileStmtPtr = std::unique_ptr<WhileStmt>;

class DoWhileStmt : public Stmt {
private:
    ExpPtr condition;
    StmtPtr stmt;
public:
    DoWhileStmt(size_t sl);

    Exp* getCondition() const;
    ExpPtr releaseCondition();
    void setCondition(ExpPtr cond);

    Stmt* getBody() const;
    StmtPtr releaseBody();
    void setBody(StmtPtr s);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using DoWhileStmtPtr = std::unique_ptr<DoWhileStmt>;

#endif