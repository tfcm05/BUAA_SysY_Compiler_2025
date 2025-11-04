#include"../../include/front/AstNode.h"
#include<utility>
#include<stdexcept>

/*
 * Exp and its derived classes
 */

LiteralExp::LiteralExp(int val)
    : literalType(LiteralType::NUMBER), value(val) {}

LiteralExp::LiteralExp(const std::string& val)
    : literalType(LiteralType::STRING), value(val) {}

LiteralExp::LiteralType LiteralExp::getLiteralType() const {
    return literalType;
}

int LiteralExp::getIntValue() const {
    return std::get<int>(value);
}

const std::string& LiteralExp::getStringValue() const {
    return std::get<std::string>(value);
}

IdentifierExp::IdentifierExp(const std::string& id)
    : identifier(id) {}

const std::string& IdentifierExp::getIdentifier() const {
    return identifier;
}

void IdentifierExp::setIdentifier(const std::string& id) {
    identifier = id;
}

FunctionCallExp::FunctionCallExp(std::unique_ptr<Exp> func, std::vector<std::unique_ptr<Exp>> rParams)
    : function(std::move(func)), RealParams(std::move(rParams)) {}

Exp* FunctionCallExp::getFunction() const {
    return function.get();
}

std::unique_ptr<Exp> FunctionCallExp::releaseFunction() {
    return std::move(function);
}

void FunctionCallExp::setFunction(std::unique_ptr<Exp> func) {
    function = std::move(func);
}

size_t FunctionCallExp::getRealParamsCount() const {
    return RealParams.size();
}

Exp* FunctionCallExp::getRealParam(size_t index) const {
    if (index < RealParams.size()) return RealParams[index].get();
    return nullptr;
}

void FunctionCallExp::addRealParam(std::unique_ptr<Exp> param) {
    RealParams.push_back(std::move(param));
}

const std::vector<std::unique_ptr<Exp>>& FunctionCallExp::getRealParams() const {
    return RealParams;
}

SubscriptExp::SubscriptExp(std::unique_ptr<Exp> arr, std::unique_ptr<Exp> sub)
    : array(std::move(arr)), subscript(std::move(sub)) {}

Exp* SubscriptExp::getArray() const {
    return array.get();
}

Exp* SubscriptExp::getSubscript() const {
    return subscript.get();
}

std::unique_ptr<Exp> SubscriptExp::releaseArray() {
    return std::move(array);
}

std::unique_ptr<Exp> SubscriptExp::releaseSubscript() {
    return std::move(subscript);
}

void SubscriptExp::setArray(std::unique_ptr<Exp> arr) {
    array = std::move(arr);
}

void SubscriptExp::setSubscript(std::unique_ptr<Exp> sub) {
    subscript = std::move(sub);
}

UnaryExp::UnaryExp(UnaryOp uop, std::unique_ptr<Exp> e)
    : unaryOp(uop), exp(std::move(e)) {}

UnaryExp::UnaryOp UnaryExp::getUnaryOp() const {
    return unaryOp;
}

void UnaryExp::setUnaryOp(UnaryOp uop) {
    unaryOp = uop;
}

Exp* UnaryExp::getExp() const {
    return exp.get();
}

std::unique_ptr<Exp> UnaryExp::releaseExp() {
    return std::move(exp);
}

void UnaryExp::setExp(std::unique_ptr<Exp> e) {
    exp = std::move(e);
}

BinaryExp::BinaryExp(std::unique_ptr<Exp> l, BinaryOp bop, std::unique_ptr<Exp> r)
    : lhs(std::move(l)), binaryOp(bop), rhs(std::move(r)) {}

Exp* BinaryExp::getLhs() const {
    return lhs.get();
}

Exp* BinaryExp::getRhs() const {
    return rhs.get();
}

BinaryExp::BinaryOp BinaryExp::getBinaryOp() const {
    return binaryOp;
}

std::unique_ptr<Exp> BinaryExp::releaseLhs() {
    return std::move(lhs);
}

std::unique_ptr<Exp> BinaryExp::releaseRhs() {
    return std::move(rhs);
}

void BinaryExp::setLhs(std::unique_ptr<Exp> l) {
    lhs = std::move(l);
}

void BinaryExp::setRhs(std::unique_ptr<Exp> r) {
    rhs = std::move(r);
}

void BinaryExp::setBinaryOp(BinaryOp bop) {
    binaryOp = bop;
}

AssignExp::AssignExp(std::unique_ptr<Exp> l, AssignType op, std::unique_ptr<Exp> r)
    : lhs(std::move(l)), assignType(op), rhs(std::move(r)) {}

Exp* AssignExp::getLhs() const {
    return lhs.get();
}

Exp* AssignExp::getRhs() const {
    return rhs.get();
}

AssignExp::AssignType AssignExp::getAssignType() const {
    return assignType;  
}

std::unique_ptr<Exp> AssignExp::releaseLhs() {
    return std::move(lhs);
}

std::unique_ptr<Exp> AssignExp::releaseRhs() {
    return std::move(rhs);
}

void AssignExp::setLhs(std::unique_ptr<Exp> l) {
    lhs = std::move(l);
}

void AssignExp::setRhs(std::unique_ptr<Exp> r) {
    rhs = std::move(r);
}

/*
 * Stmt and derived classes
 */

Stmt::Stmt(int sl)
    : startLine(sl) {}

int Stmt::getLine() const { return startLine; }

void Stmt::setLine(int sl) { startLine = sl; }

VariableDeclarator::VariableDeclarator() {}

const std::string& VariableDeclarator::getIdentifier() const { return identifier; }

void VariableDeclarator::setIdentifier(const std::string& id) {
    identifier = id;
}

// VariableDeclarator
size_t VariableDeclarator::getDimensionSizesCount() const { return dimensionSizes.size(); }
Exp* VariableDeclarator::getDimensionSize(size_t index) const { return dimensionSizes.at(index).get(); }
void VariableDeclarator::addDimensionSize(std::unique_ptr<Exp> dimSize) { dimensionSizes.push_back(std::move(dimSize)); }
const std::vector<std::unique_ptr<Exp>>& VariableDeclarator::getDimensionSizes() const { return dimensionSizes; }

size_t VariableDeclarator::getInitialsCount() const { return initials.size(); }
Exp* VariableDeclarator::getInitial(size_t index) const { return initials.at(index).get(); }
void VariableDeclarator::addInitial(std::unique_ptr<Exp> init) { initials.push_back(std::move(init)); }
const std::vector<std::unique_ptr<Exp>>& VariableDeclarator::getInitials() const { return initials; }

// VariableDecl
VariableDecl::VariableDecl(int sl)
    : Stmt(sl), storageClass(StorageClass::NONE), typeQualifier(TypeQualifier::NONE) {}

StorageClass VariableDecl::getStorageClass() const { return storageClass; }

void VariableDecl::setStorageClass(StorageClass sc) {
    storageClass = sc;
}

TypeQualifier VariableDecl::getTypeQualifier() const {
    return typeQualifier;
}

void VariableDecl::setTypeQualifier(TypeQualifier tq) {
    typeQualifier = tq;
}

BasicType VariableDecl::getDeclType() const {
    return DeclType;
}

void VariableDecl::setDeclType(BasicType type) {
    DeclType = type;
}

// VariableDecl
size_t VariableDecl::getDeclaratorsCount() const { return declarators.size(); }
VariableDeclarator* VariableDecl::getDeclarator(size_t index) const { return declarators.at(index).get(); }
void VariableDecl::addDeclarator(std::unique_ptr<VariableDeclarator> decl) { declarators.push_back(std::move(decl)); }
const std::vector<std::unique_ptr<VariableDeclarator>>& VariableDecl::getDeclarators() const { return declarators; }

// FormatParam
FormatParam::FormatParam() {}

const std::string& FormatParam::getIdentifier() const { return identifier; }

void FormatParam::setIdentifier(const std::string& id) {
    identifier = id;
}

BasicType FormatParam::getParamType() const {
    return paramType;
}

void FormatParam::setParamType(BasicType type) {
    paramType = type;
}

// FormatParam
size_t FormatParam::getDimensionSizesCount() const { return dimensionSizes.size(); }
Exp* FormatParam::getDimensionSize(size_t index) const { return dimensionSizes.at(index).get(); }
void FormatParam::addDimensionSize(std::unique_ptr<Exp> dimSize) { dimensionSizes.push_back(std::move(dimSize)); }
const std::vector<std::unique_ptr<Exp>>& FormatParam::getDimensionSizes() const { return dimensionSizes; }

// FunctionDecl
FunctionDecl::FunctionDecl(int sl)
    : Stmt(sl) {}

const std::string& FunctionDecl::getIdentifier() const { return identifier; }

void FunctionDecl::setIdentifier(const std::string& id) {
    identifier = id;
}

BasicType FunctionDecl::getReturnType() const {
    return returnType;
}

void FunctionDecl::setReturnType(BasicType rType) {
    returnType = rType;
}

// FunctionDecl
size_t FunctionDecl::getFormatParamsCount() const { return formatParams.size(); }
FormatParam* FunctionDecl::getFormatParam(size_t index) const { return formatParams.at(index).get(); }
void FunctionDecl::addFormatParam(std::unique_ptr<FormatParam> param) { formatParams.push_back(std::move(param)); }
const std::vector<std::unique_ptr<FormatParam>>& FunctionDecl::getFormatParams() const { return formatParams; }

BlockStmt* FunctionDecl::getFunctionBody() const {
    return functionBody.get();
}

std::unique_ptr<BlockStmt> FunctionDecl::releaseFunctionBody() {
    return std::move(functionBody);
}

void FunctionDecl::setFunctionBody(std::unique_ptr<BlockStmt> b) {
    functionBody = std::move(b);
}

ExpStmt::ExpStmt(int sl)
    : Stmt(sl), exp(nullptr) {}

Exp* ExpStmt::getExp() const {
    return exp.get();
}

std::unique_ptr<Exp> ExpStmt::releaseExp() {
    return std::move(exp);
}

void ExpStmt::setExp(std::unique_ptr<Exp> e) {
    exp = std::move(e);
}

ForStmt::ForStmt(int sl)
    : Stmt(sl), initial(nullptr), condition(nullptr), iteration(nullptr), stmt(nullptr) {}

Exp* ForStmt::getInitial() const {
    return initial.get();
}

std::unique_ptr<Exp> ForStmt::releaseInitial() {
    return std::move(initial);
}

void ForStmt::setInitial(std::unique_ptr<Exp> s) {
    initial = std::move(s);
}

Exp* ForStmt::getCondition() const {
    return condition.get();
}

std::unique_ptr<Exp> ForStmt::releaseCondition() {
    return std::move(condition);
}

void ForStmt::setCondition(std::unique_ptr<Exp> cond) {
    condition = std::move(cond);
}

Exp* ForStmt::getIteration() const {
    return iteration.get();
}

std::unique_ptr<Exp> ForStmt::releaseIteration() {
    return std::move(iteration);
}

void ForStmt::setIteration(std::unique_ptr<Exp> iter) {
    iteration = std::move(iter);
}

Stmt* ForStmt::getBody() const {
    return stmt.get();
}

std::unique_ptr<Stmt> ForStmt::releaseBody() {
    return std::move(stmt);
}

void ForStmt::setBody(std::unique_ptr<Stmt> s) {
    stmt = std::move(s);
}

IfStmt::IfStmt(int sl)
    : Stmt(sl), condition(nullptr), trueStmt(nullptr), falseStmt(nullptr) {}

Exp* IfStmt::getCondition() const {
    return condition.get();
}

std::unique_ptr<Exp> IfStmt::releaseCondition() {
    return std::move(condition);
}

void IfStmt::setCondition(std::unique_ptr<Exp> cond) {
    condition = std::move(cond);
}

Stmt* IfStmt::getTrueStmt() const {
    return trueStmt.get();
}

std::unique_ptr<Stmt> IfStmt::releaseTrueStmt() {
    return std::move(trueStmt);
}

void IfStmt::setTrueStmt(std::unique_ptr<Stmt> trueStatement) {
    trueStmt = std::move(trueStatement);
}

Stmt* IfStmt::getFalseStmt() const {
    return falseStmt.get();
}

std::unique_ptr<Stmt> IfStmt::releaseFalseStmt() {
    return std::move(falseStmt);
}

void IfStmt::setFalseStmt(std::unique_ptr<Stmt> falseStatement) {
    falseStmt = std::move(falseStatement);
}

BreakStmt::BreakStmt(int sl)
    : Stmt(sl) {}

ContinueStmt::ContinueStmt(int sl)
    : Stmt(sl) {}

ReturnStmt::ReturnStmt(int sl)
    : Stmt(sl), returnExp(nullptr) {}

Exp* ReturnStmt::getReturnExp() const {
    return returnExp.get();
}

std::unique_ptr<Exp> ReturnStmt::releaseReturnExp() {
    return std::move(returnExp);
}

void ReturnStmt::setReturnExp(std::unique_ptr<Exp> reType) {
    returnExp = std::move(reType);
}

BlockStmt::BlockStmt(int sl)
    : Stmt(sl), endline(0) {}

int BlockStmt::getEndLine() const { return endline; }

void BlockStmt::setEndLine(int el) {
    endline = el;
}

// BlockStmt
size_t BlockStmt::getStmtsCount() const { return stmts.size(); }
Stmt* BlockStmt::getStmt(size_t index) const { return stmts.at(index).get(); }
void BlockStmt::addStmt(std::unique_ptr<Stmt> s) { stmts.push_back(std::move(s)); }
const std::vector<std::unique_ptr<Stmt>>& BlockStmt::getStmts() const { return stmts; }

/*
 * CompUnit
 */

CompUnit::CompUnit() {}

// CompUnit
size_t CompUnit::getGlobalVarDeclCount() const { return globalVarDecl.size(); }
VariableDecl* CompUnit::getGlobalVarDecl(size_t index) const { return globalVarDecl.at(index).get(); }
void CompUnit::addGlobalVarDecl(std::unique_ptr<VariableDecl> decl) { globalVarDecl.push_back(std::move(decl)); }
const std::vector<std::unique_ptr<VariableDecl>>& CompUnit::getGlobalVarDecls() const { return globalVarDecl; }

size_t CompUnit::getFuncDeclCount() const { return funcDecl.size(); }
FunctionDecl* CompUnit::getFuncDecl(size_t index) const { return funcDecl.at(index).get(); }
void CompUnit::addFuncDecl(std::unique_ptr<FunctionDecl> decl) { funcDecl.push_back(std::move(decl)); }
const std::vector<std::unique_ptr<FunctionDecl>>& CompUnit::getFuncDecls() const { return funcDecl; }

FunctionDecl* CompUnit::getMainFuncDecl() const {
    return mainFuncDecl.get();
}

std::unique_ptr<FunctionDecl> CompUnit::releaseMainFuncDecl() {
    return std::move(mainFuncDecl);
}

void CompUnit::setMainFuncDecl(std::unique_ptr<FunctionDecl> decl) {
    mainFuncDecl = std::move(decl);
}