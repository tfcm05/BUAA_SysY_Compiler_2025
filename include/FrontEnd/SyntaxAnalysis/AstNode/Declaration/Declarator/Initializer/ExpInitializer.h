#ifndef EXPINITIALIZER_H
#define EXPINITIALIZER_H
#include <cstdint>
#include "Initializer.h"
#include "../../../Expression/Expression.h"

class ExpInitializer : public Initializer {
private:
    ExpPtr expression;

    int64_t constValue;
public:
    ExpInitializer(ExpPtr exp, size_t sl);

    Exp* getExp() const;
    std::unique_ptr<Exp> releaseExp();
    void setExp(ExpPtr exp);

    int64_t getConstValue() const;
    void setConstValue(int64_t cv);

    InfoPtr accept(class AstVisitor& visitor, ContextPtr context) override;
};

using ExpInitializerPtr = std::unique_ptr<ExpInitializer>;

#endif