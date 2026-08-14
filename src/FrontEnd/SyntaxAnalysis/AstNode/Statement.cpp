#include "../../../../include/FrontEnd/SyntaxAnalysis/AstNode.h"

/*
 * Stmt and derived classes
 */

 // ----- Stmt -----
Stmt::Stmt(size_t sl) : AstNode(sl) {}

// ----- Expression Statements -----
ExpStmt::ExpStmt(size_t sl)
    : Stmt(sl) {}

Exp* ExpStmt::getExp() const {
    return exp.get();
}
ExpPtr ExpStmt::releaseExp() {
    return std::move(exp);
}
void ExpStmt::setExp(ExpPtr e) {
    exp = std::move(e);
}

InfoPtr ExpStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitExpStmt(this, context);
}

// ----- Iterative Statements -----
// ForStmt
ForStmt::ForStmt(size_t sl)
    : Stmt(sl) {}

Exp* ForStmt::getInitial() const {
    return initial.get();
}
ExpPtr ForStmt::releaseInitial() {
    return std::move(initial);
}
void ForStmt::setInitial(ExpPtr s) {
    initial = std::move(s);
}

bool ForStmt::hasInitialDecl() const {
    return initialDecl != nullptr;
}
VariableDecl* ForStmt::getInitialDecl() const {
    return initialDecl.get();
}
VariableDeclPtr ForStmt::releaseInitialDecl() {
    return std::move(initialDecl);
}
void ForStmt::setInitialDecl(VariableDeclPtr d) {
    initialDecl = std::move(d);
}

Exp* ForStmt::getCondition() const {
    return condition.get();
}
ExpPtr ForStmt::releaseCondition() {
    return std::move(condition);
}
void ForStmt::setCondition(ExpPtr cond) {
    condition = std::move(cond);
}

Exp* ForStmt::getIteration() const {
    return iteration.get();
}
ExpPtr ForStmt::releaseIteration() {
    return std::move(iteration);
}
void ForStmt::setIteration(ExpPtr iter) {
    iteration = std::move(iter);
}

Stmt* ForStmt::getBody() const {
    return stmt.get();
}
StmtPtr ForStmt::releaseBody() {
    return std::move(stmt);
}
void ForStmt::setBody(StmtPtr s) {
    stmt = std::move(s);
}

InfoPtr ForStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitForStmt(this, context);
}

// WhileStmt
WhileStmt::WhileStmt(size_t sl)
    : Stmt(sl) {}

Exp* WhileStmt::getCondition() const {
    return condition.get();
}
ExpPtr WhileStmt::releaseCondition() {
    return std::move(condition);
}
void WhileStmt::setCondition(ExpPtr cond) {
    condition = std::move(cond);
}

Stmt* WhileStmt::getBody() const {
    return stmt.get();
}
StmtPtr WhileStmt::releaseBody() {
    return std::move(stmt);
}
void WhileStmt::setBody(StmtPtr s) {
    stmt = std::move(s);
}

InfoPtr WhileStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitWhileStmt(this, context);
}

// DoWhileStmt
DoWhileStmt::DoWhileStmt(size_t sl)
    : Stmt(sl) {}

Exp* DoWhileStmt::getCondition() const {
    return condition.get();
}
ExpPtr DoWhileStmt::releaseCondition() {
    return std::move(condition);
}
void DoWhileStmt::setCondition(ExpPtr cond) {
    condition = std::move(cond);
}

Stmt* DoWhileStmt::getBody() const {
    return stmt.get();
}
StmtPtr DoWhileStmt::releaseBody() {
    return std::move(stmt);
}
void DoWhileStmt::setBody(StmtPtr s) {
    stmt = std::move(s);
}

InfoPtr DoWhileStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitDoWhileStmt(this, context);
}

// ----- Selection Statements -----
// IfStmt
IfStmt::IfStmt(size_t sl)
    : Stmt(sl) {}

Exp* IfStmt::getCondition() const {
    return condition.get();
}
ExpPtr IfStmt::releaseCondition() {
    return std::move(condition);
}
void IfStmt::setCondition(ExpPtr cond) {
    condition = std::move(cond);
}

Stmt* IfStmt::getTrueStmt() const {
    return trueStmt.get();
}
StmtPtr IfStmt::releaseTrueStmt() {
    return std::move(trueStmt);
}
void IfStmt::setTrueStmt(StmtPtr trueStatement) {
    trueStmt = std::move(trueStatement);
}

Stmt* IfStmt::getFalseStmt() const {
    return falseStmt.get();
}
StmtPtr IfStmt::releaseFalseStmt() {
    return std::move(falseStmt);
}
void IfStmt::setFalseStmt(StmtPtr falseStatement) {
    falseStmt = std::move(falseStatement);
}

InfoPtr IfStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitIfStmt(this, context);
}

// ----- Jump Statements -----
// BreakStmt
BreakStmt::BreakStmt(size_t sl)
    : Stmt(sl) {}

InfoPtr BreakStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitBreakStmt(this, context);
}

// ContinueStmt
ContinueStmt::ContinueStmt(size_t sl)
    : Stmt(sl) {}

InfoPtr ContinueStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitContinueStmt(this, context);
}

// ReturnStmt
ReturnStmt::ReturnStmt(size_t sl)
: Stmt(sl) {}

Exp* ReturnStmt::getReturnExp() const {
    return returnExp.get();
}
ExpPtr ReturnStmt::releaseReturnExp() {
    return std::move(returnExp);
}
void ReturnStmt::setReturnExp(ExpPtr reType) {
    returnExp = std::move(reType);
}

InfoPtr ReturnStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitReturnStmt(this, context);
}

// ----- Compound Statements -----
// BlockStmt
BlockStmt::BlockStmt(size_t sl)
    : Stmt(sl), endline(sl) {}

size_t BlockStmt::getEndLine() const {
    return endline;
}
void BlockStmt::setEndLine(size_t el) {
    endline = el;
}

size_t BlockStmt::getBlockItemsCount() const {
    return blockItemList.size();
}
AstNode* BlockStmt::getBlockItem(size_t index) const {
    if (index < blockItemList.size()) {
        return blockItemList[index].get();
    }
    return nullptr;
}
void BlockStmt::addBlockItem(AstNodePtr item) {
    blockItemList.push_back(std::move(item));
}
const std::vector<AstNodePtr>& BlockStmt::getBlockItems() const {
    return blockItemList;
}

InfoPtr BlockStmt::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitBlockStmt(this, context);
}