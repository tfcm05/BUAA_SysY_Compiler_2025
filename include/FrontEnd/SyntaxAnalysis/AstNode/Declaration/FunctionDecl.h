#ifndef FUNCTIONDECL_H
#define FUNCTIONDECL_H
#include "Declaration.h"
#include "../Statement/CompStmt.h"

/* 
 * this is function declaration part
 */

class FunctionDecl : public Decl {
private:
    DeclSpec declSpec;
    DeclaratorPtr declarator;
    BlockStmtPtr body;
public:
    FunctionDecl(size_t sl);

    DeclSpec getDeclSpec() const;
    void setDeclSpec(const DeclSpec& ds);

    Declarator* getDeclarator() const;
    DeclaratorPtr releaseDeclarator();
    void setDeclarator(DeclaratorPtr decl);

    BlockStmt* getBody() const;
    BlockStmtPtr releaseBody();
    void setBody(BlockStmtPtr b);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using FunctionDeclPtr = std::unique_ptr<FunctionDecl>;

#endif