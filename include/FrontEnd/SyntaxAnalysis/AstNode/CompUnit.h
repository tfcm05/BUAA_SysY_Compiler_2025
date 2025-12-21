#ifndef COMPUIT_H
#define COMPUIT_H
#include "AstNode.h"
#include "Declaration/VariableDecl.h"
#include "Declaration/FunctionDecl.h"

class CompUnit : public AstNode {
private:
    std::vector<VariableDeclPtr> varDeclList;
    std::vector<FunctionDeclPtr> funcDeclList;
    FunctionDeclPtr mainFuncDecl;
public:
    CompUnit(size_t sl);

    size_t getVarDeclsCount() const;
    VariableDecl* getVarDecl(size_t index) const;
    void addVarDecl(VariableDeclPtr decl);
    const std::vector<VariableDeclPtr>& getVarDecls() const;

    size_t getFuncDeclsCount() const;
    FunctionDecl* getFuncDecl(size_t index) const;
    void addFuncDecl(FunctionDeclPtr decl);
    const std::vector<FunctionDeclPtr>& getFuncDecls() const;

    FunctionDecl* getMainFuncDecl() const;
    FunctionDeclPtr releaseMainFuncDecl();
    void setMainFuncDecl(FunctionDeclPtr decl);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using CompUnitPtr = std::unique_ptr<CompUnit>;

#endif