#ifndef PRIMARYEXP_H
#define PRIMARYEXP_H
#include <cstdint>
#include "Expression.h"

/*
 * this is primary expression part
 */

class LiteralExp : public Exp {
public:
    enum class LiteralType { NUMBER, STRING };
private:
    LiteralType literalType;

    int64_t valueNumber;
    std::string valueString;
public:
    LiteralExp(int64_t val, size_t sl);
    LiteralExp(const std::string& val, size_t sl);

    LiteralType getLiteralType() const;

    int64_t getNumberValue() const;
    const std::string& getStringValue() const;

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

class IdentifierExp : public Exp {
private:
    std::string identifier;
public:
    IdentifierExp(const std::string& id, size_t sl);
    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using LiteralExpPtr = std::unique_ptr<LiteralExp>;
using IdentifierExpPtr = std::unique_ptr<IdentifierExp>;

#endif