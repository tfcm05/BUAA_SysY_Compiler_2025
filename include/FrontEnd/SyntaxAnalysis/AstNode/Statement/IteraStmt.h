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
    // SysY 文法: ForInitStmt -> VarDecl | Exp ';'  (二选一)
    // 声明形式: for (int i = 0; ...)  表达式形式: for (i = 0; ...)
    // 两者语义等价: 声明的变量 i 与后续 condition/iteration/body 中的赋值引用为同一变量,
    // 其作用域覆盖整个 for 语句(含 condition/iteration/body), 循环结束后不可见 (C 语义)
    // 统一用 AstNodePtr 存储: Exp 和 VariableDecl 均为 AstNode 子类,
    // accept() 虚函数分派自动转发到正确的 visit 方法, 调用方无需区分类型
    AstNodePtr initial;
    ExpPtr condition;
    ExpPtr iteration;
    StmtPtr stmt;
public:
    ForStmt(size_t sl);

    AstNode* getInitial() const;
    AstNodePtr releaseInitial();
    void setInitial(AstNodePtr s);

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