#ifndef POSTFIXEXP_H
#define POSTFIXEXP_H
#include "Expression.h"

class FunctionCallExp : public Exp {
private:
    ExpPtr function;
    std::vector<ExpPtr> arguments;
public:
    FunctionCallExp(ExpPtr func, std::vector<ExpPtr> args, size_t sl);

    Exp* getFunction() const;
    ExpPtr releaseFunction();
    void setFunction(ExpPtr func);

    size_t getArgumentsCount() const;
    Exp* getArgument(size_t index) const;
    void addArgument(ExpPtr arg);
    const std::vector<ExpPtr>& getArguments() const;

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

class SubscriptExp : public Exp {
private:
    ExpPtr array;
    std::vector<ExpPtr> subscriptList;
public:
    SubscriptExp(ExpPtr arr, std::vector<ExpPtr> subs, size_t sl);

    Exp* getArray() const;
    ExpPtr releaseArray();
    void setArray(ExpPtr arr);
    
    size_t getSubscriptsCount() const;
    Exp* getSubscript(size_t index) const;
    void addSubScript(ExpPtr sub);
    const std::vector<ExpPtr>& getSubscripts() const;
    
    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using FunctionCallExpPtr = std::unique_ptr<FunctionCallExp>;
using SubscriptExpPtr = std::unique_ptr<SubscriptExp>;

#endif