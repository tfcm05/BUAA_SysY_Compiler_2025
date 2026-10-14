#ifndef UNARYEXPRESSION_H
#define UNARYEXPRESSION_H
#include "Expression.h"

/*
 * this is unary expression part
 */

class UnaryExp : public Exp {
public:
    enum class UnaryOp { PLUS, MINU, NOT, ADDR, DEREF };
private:
    UnaryOp unaryOp;
    std::unique_ptr<Exp> exp;
public:
    UnaryExp(UnaryOp uop, std::unique_ptr<Exp> e, size_t sl);

    UnaryOp getUnaryOp() const;
    void setUnaryOp(UnaryOp uop);

    Exp* getExp() const;
    std::unique_ptr<Exp> releaseExp();
    void setExp(std::unique_ptr<Exp> e);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using UnaryExpPtr = std::unique_ptr<UnaryExp>;

#endif