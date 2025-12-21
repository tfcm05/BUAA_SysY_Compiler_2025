#ifndef VARIABLEDECLA_H
#define VARIABLEDECLA_H
#include "Declaration.h"
#include "../Expression/Expression.h"

/* 
 * this is declaration part
 */

class VariableDecl : public Decl {
private:
    DeclSpec declSpec;
    std::vector<DeclaratorPtr> declaratorList;
public:
    VariableDecl(size_t sl);

    DeclSpec getDeclSpec() const;
    void setDeclSpec(const DeclSpec& ds);
    
    size_t getDeclaratorsCount() const;
    Declarator* getDeclarator(size_t index) const;
    void addDeclarator(DeclaratorPtr decl);
    const std::vector<DeclaratorPtr>& getDeclarators() const;

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using VariableDeclPtr = std::unique_ptr<VariableDecl>;

#endif