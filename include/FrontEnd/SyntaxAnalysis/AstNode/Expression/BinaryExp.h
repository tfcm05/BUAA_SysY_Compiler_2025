#ifndef BINARYEXP_H
#define BINARYEXP_H
#include "Expression.h"

/*
 * this is binary expression part
 */

class BinaryExp : public Exp {
public:
    enum class BinaryOp { PLUS, MINU, MULT, DIV, MOD, OR, AND, EQL, NEQ, GRE, GEQ, LSS, LEQ };
private:
    ExpPtr lhs;
    BinaryOp binaryOp;
    ExpPtr rhs;
public:
    BinaryExp(ExpPtr l, BinaryOp bop, ExpPtr r, size_t sl);

    Exp* getLhs() const;
    Exp* getRhs() const;
    BinaryOp getBinaryOp() const;

    ExpPtr releaseLhs();
    ExpPtr releaseRhs();

    void setLhs(ExpPtr l);
    void setRhs(ExpPtr r);
    void setBinaryOp(BinaryOp bop);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

class CommaExp : public Exp {
private:
    std::vector<ExpPtr> expList;
public:
    CommaExp(size_t sl);

    size_t getExpsCount() const;
    Exp* getExp(size_t index) const;
    void addExp(ExpPtr e);
    const std::vector<ExpPtr>& getExps() const;

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

class AssignExp : public Exp {
public:
    enum class AssignOp { ASSIGN };
private:
    ExpPtr lhs;
    AssignOp assignOp;
    ExpPtr rhs;
public:
    AssignExp(ExpPtr l, AssignOp op, ExpPtr r, size_t sl);

    Exp* getLhs() const;
    Exp* getRhs() const;
    AssignOp getAssignOp() const;

    ExpPtr releaseLhs();
    ExpPtr releaseRhs();

    void setLhs(ExpPtr l);
    void setRhs(ExpPtr r);
    void setAssignOp(AssignOp op);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using BinaryExpPtr = std::unique_ptr<BinaryExp>;
using AssignExpPtr = std::unique_ptr<AssignExp>;
using CommaExpPtr = std::unique_ptr<CommaExp>;

#endif