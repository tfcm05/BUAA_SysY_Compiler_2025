#include "FrontEnd/SyntaxAnalysis/AstNodes.h"


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

size_t CompUnit::getDeclCount() const {
    return declarations.size();
}
AstNode* CompUnit::getDecl(size_t index) const {
    if (index < declarations.size()) {
        return declarations[index].get();
    }
    return nullptr;
}
void CompUnit::addDecl(AstNodePtr decl) {
    declarations.push_back(std::move(decl));
}

InfoPtr CompUnit::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitCompUnit(this, context);
}