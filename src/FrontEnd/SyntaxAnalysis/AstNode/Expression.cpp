#include "FrontEnd/SyntaxAnalysis/AstNodes.h"

/*
 * Exp and derived classes
 */

Exp::Exp(size_t sl) : AstNode(sl) {}

// ----- PrimaryExp -----
// LiteralExp
LiteralExp::LiteralExp(int64_t val, size_t sl)
    : Exp(sl), literalType(LiteralType::NUMBER), valueNumber(val) {}
LiteralExp::LiteralExp(const std::string& val, size_t sl)
    : Exp(sl), literalType(LiteralType::STRING), valueString(val) {}

LiteralExp::LiteralType LiteralExp::getLiteralType() const {
    return literalType;
}
int64_t LiteralExp::getNumberValue() const {
    return valueNumber;
}
const std::string& LiteralExp::getStringValue() const {
    return valueString;
}

InfoPtr LiteralExp::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitLiteralExp(this, context);
}

// IdentifierExp
IdentifierExp::IdentifierExp(const std::string& id, size_t sl)
    : Exp(sl), identifier(id) {}

const std::string& IdentifierExp::getIdentifier() const {
    return identifier;
}
void IdentifierExp::setIdentifier(const std::string& id) {
    identifier = id;
}

InfoPtr IdentifierExp::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitIdentifierExp(this, context);
}

// ----- PostfixExp -----
// FunctionCallExp
FunctionCallExp::FunctionCallExp(ExpPtr func, std::vector<ExpPtr> args, size_t sl)
    : Exp(sl), function(std::move(func)), arguments(std::move(args)) {}

Exp* FunctionCallExp::getFunction() const {
    return function.get();
}
ExpPtr FunctionCallExp::releaseFunction() {
    return std::move(function);
}
void FunctionCallExp::setFunction(ExpPtr func) {
    function = std::move(func);
}

size_t FunctionCallExp::getArgumentsCount() const {
    return arguments.size();
}
Exp* FunctionCallExp::getArgument(size_t index) const {
    if (index < arguments.size()) {
        return arguments[index].get();
    }
    return nullptr;
}
void FunctionCallExp::addArgument(ExpPtr arg) {
    arguments.push_back(std::move(arg));
}
const std::vector<ExpPtr>& FunctionCallExp::getArguments() const {
    return arguments;
}

InfoPtr FunctionCallExp::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitFunctionCallExp(this, context);
}

// SubscriptExp
SubscriptExp::SubscriptExp(ExpPtr arr, std::vector<ExpPtr> subs, size_t sl)
    : Exp(sl), array(std::move(arr)), subscriptList(std::move(subs)) {}

Exp* SubscriptExp::getArray() const {
    return array.get();
}
ExpPtr SubscriptExp::releaseArray() {
    return std::move(array);
}
void SubscriptExp::setArray(ExpPtr arr) {
    array = std::move(arr);
}

size_t SubscriptExp::getSubscriptsCount() const {
    return subscriptList.size();
}
Exp* SubscriptExp::getSubscript(size_t index) const {
    if (index < subscriptList.size()) {
        return subscriptList[index].get();
    }
    return nullptr;
}
void SubscriptExp::addSubScript(ExpPtr sub) {
    subscriptList.push_back(std::move(sub));
}
const std::vector<ExpPtr>& SubscriptExp::getSubscripts() const {
    return subscriptList;
}

InfoPtr SubscriptExp::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitSubscriptExp(this, context);
}

// ----- UnaryExp -----
UnaryExp::UnaryExp(UnaryOp uop, ExpPtr e, size_t sl)
    : Exp(sl), unaryOp(uop), exp(std::move(e)) {}

UnaryExp::UnaryOp UnaryExp::getUnaryOp() const {
    return unaryOp;
}
void UnaryExp::setUnaryOp(UnaryOp uop) {
    unaryOp = uop;
}

Exp* UnaryExp::getExp() const {
    return exp.get();
}
ExpPtr UnaryExp::releaseExp() {
    return std::move(exp);
}
void UnaryExp::setExp(ExpPtr e) {
    exp = std::move(e);
}

InfoPtr UnaryExp::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitUnaryExp(this, context);
}

// ----- BinaryExp -----
// BinaryExp
BinaryExp::BinaryExp(ExpPtr l, BinaryOp bop, ExpPtr r, size_t sl)
    : Exp(sl), lhs(std::move(l)), binaryOp(bop), rhs(std::move(r)) {}

Exp* BinaryExp::getLhs() const {
    return lhs.get();
}
ExpPtr BinaryExp::releaseLhs() {
    return std::move(lhs);
}
void BinaryExp::setLhs(ExpPtr l) {
    lhs = std::move(l);
}

BinaryExp::BinaryOp BinaryExp::getBinaryOp() const {
    return binaryOp;
}
void BinaryExp::setBinaryOp(BinaryOp bop) {
    binaryOp = bop;
}

Exp* BinaryExp::getRhs() const {
    return rhs.get();
}
ExpPtr BinaryExp::releaseRhs() {
    return std::move(rhs);
}
void BinaryExp::setRhs(ExpPtr r) {
    rhs = std::move(r);
}

InfoPtr BinaryExp::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitBinaryExp(this, context);
}

// CommaExp
CommaExp::CommaExp(size_t sl) : Exp(sl) {}

size_t CommaExp::getExpsCount() const {
    return expList.size();
}
Exp* CommaExp::getExp(size_t index) const {
    if (index < expList.size()) {
        return expList[index].get();
    }
    return nullptr;
}
void CommaExp::addExp(ExpPtr e) {
    expList.push_back(std::move(e));
}
const std::vector<ExpPtr>& CommaExp::getExps() const {
    return expList;
}

InfoPtr CommaExp::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitCommaExp(this, context);
}

// AssignExp
AssignExp::AssignExp(ExpPtr l, AssignOp op, ExpPtr r, size_t sl)
    : Exp(sl), lhs(std::move(l)), assignOp(op), rhs(std::move(r)) {}

Exp* AssignExp::getLhs() const {
    return lhs.get();
}
ExpPtr AssignExp::releaseLhs() {
    return std::move(lhs);
}
void AssignExp::setLhs(ExpPtr l) {
    lhs = std::move(l);
}

AssignExp::AssignOp AssignExp::getAssignOp() const {
    return assignOp;
}
void AssignExp::setAssignOp(AssignOp op) {
    assignOp = op;
}

Exp* AssignExp::getRhs() const {
    return rhs.get();
}
ExpPtr AssignExp::releaseRhs() {
    return std::move(rhs);
}
void AssignExp::setRhs(ExpPtr r) {
    rhs = std::move(r);
}

InfoPtr AssignExp::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitAssignExp(this, context);
}

// ----- End of Exp and derived classes -----