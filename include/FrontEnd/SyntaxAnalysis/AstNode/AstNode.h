#ifndef ASTNODE_H
#define ASTNODE_H
#include "../AstVisitor.h"
#include <string>
#include <vector>
#include <memory>

class Symbol;

class AstNode {
private:
    size_t startLine;
public:
    AstNode(size_t sl);
    virtual ~AstNode() = default;
    size_t getStartLine() const;
    void setStartLine(size_t sl);

    virtual InfoPtr accept(AstVisitor& visitor, ContextPtr context) = 0;
};

using AstNodePtr = std::unique_ptr<AstNode>;

#endif