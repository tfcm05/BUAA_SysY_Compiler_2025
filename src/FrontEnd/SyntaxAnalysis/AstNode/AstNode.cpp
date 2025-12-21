#include "../../../../include/FrontEnd/SyntaxAnalysis/AstNode.h"


/*
 * AstNode
 */

AstNode::AstNode(size_t sl) : startLine(sl) {}

size_t AstNode::getStartLine() const {
    return startLine;
}
void AstNode::setStartLine(size_t sl) {
    startLine = sl;
}


/*
 * CompUnit
 */

CompUnit::CompUnit(size_t sl) : AstNode(sl) {}

// CompUnit
size_t CompUnit::getVarDeclsCount() const {
    return varDeclList.size();
}
VariableDecl* CompUnit::getVarDecl(size_t index) const {
    if (index < varDeclList.size()) {
        return varDeclList[index].get();
    }
    return nullptr;
}
void CompUnit::addVarDecl(VariableDeclPtr decl) {
    varDeclList.push_back(std::move(decl));
}
const std::vector<VariableDeclPtr>& CompUnit::getVarDecls() const {
    return varDeclList;
}

size_t CompUnit::getFuncDeclsCount() const {
    return funcDeclList.size();
}
FunctionDecl* CompUnit::getFuncDecl(size_t index) const {
    if (index < funcDeclList.size()) {
        return funcDeclList[index].get();
    }
    return nullptr;
}
void CompUnit::addFuncDecl(FunctionDeclPtr decl) {
    funcDeclList.push_back(std::move(decl));
}
const std::vector<FunctionDeclPtr>& CompUnit::getFuncDecls() const {
    return funcDeclList;
}

FunctionDecl* CompUnit::getMainFuncDecl() const {
    return mainFuncDecl.get();
}
FunctionDeclPtr CompUnit::releaseMainFuncDecl() {
    return std::move(mainFuncDecl);
}
void CompUnit::setMainFuncDecl(FunctionDeclPtr decl) {
    mainFuncDecl = std::move(decl);
}

InfoPtr CompUnit::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitCompUnit(this, context);
}