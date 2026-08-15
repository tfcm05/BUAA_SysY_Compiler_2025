#ifndef COMPUIT_H
#define COMPUIT_H
#include "AstNode.h"

class CompUnit : public AstNode {
private:
    std::vector<AstNodePtr> declarations;
public:
    CompUnit(size_t sl);

    size_t getDeclCount() const;
    AstNode* getDecl(size_t index) const;
    void addDecl(AstNodePtr decl);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using CompUnitPtr = std::unique_ptr<CompUnit>;

#endif