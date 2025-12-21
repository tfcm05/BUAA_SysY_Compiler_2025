#ifndef COMPSTMT_H
#define COMPSTMT_H
#include "Statement.h"
#include "../Declaration/Declaration.h"

/* 
 * this is compound statement part
 */

class BlockStmt : public Stmt {
private:    
    size_t endline;
    std::vector<AstNodePtr> blockItemList;
public:
    BlockStmt(size_t sl);

    size_t getEndLine() const;
    void setEndLine(size_t el);

    size_t getBlockItemsCount() const;
    AstNode* getBlockItem(size_t index) const;
    void addBlockItem(AstNodePtr item);
    const std::vector<AstNodePtr>& getBlockItems() const;

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using BlockStmtPtr = std::unique_ptr<BlockStmt>;

#endif