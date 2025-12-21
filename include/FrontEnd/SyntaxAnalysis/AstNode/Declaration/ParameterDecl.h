#ifndef PARAMETERDECLARATION_H
#define PARAMETERDECLARATION_H
#include "Declaration.h"

class ParameterDecl : public Decl {
private:
    DeclSpec declSpec;
    DeclaratorPtr declarator;
public:
    ParameterDecl(size_t sl);

    DeclSpec getDeclSpec() const;
    void setDeclSpec(const DeclSpec& ds);

    Declarator* getDeclarator() const;
    DeclaratorPtr releaseDeclarator();
    void setDeclarator(DeclaratorPtr decl);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using ParameterDeclPtr = std::unique_ptr<ParameterDecl>;

#endif