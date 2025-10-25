#include "../../include/front/AstNode.h"

#include <utility>
#include <variant>

// ---- CompUnit ----
CompUnit::CompUnit() = default;

void CompUnit::addGlobalVarDefStmt(std::unique_ptr<VariableDefStmt> varDefStmt) {
    if (varDefStmt) globalVarDefStmts.push_back(std::move(varDefStmt));
}

void CompUnit::addFuncDefStmt(std::unique_ptr<FunctionDefStmt> funcDefStmt) {
    if (funcDefStmt) funcDefStmts.push_back(std::move(funcDefStmt));
}

void CompUnit::setMainFuncDef(std::unique_ptr<FunctionDefStmt> mainFuncDefStmt) { mainFuncDef = std::move(mainFuncDefStmt); }

size_t CompUnit::getGlobalVarDefStmtsCount() const { return globalVarDefStmts.size(); }

VariableDefStmt* CompUnit::getGlobalVarDefStmt(size_t index) const {
    return index < globalVarDefStmts.size() ? globalVarDefStmts[index].get() : nullptr;
}

const std::vector<std::unique_ptr<VariableDefStmt>>& CompUnit::getGlobalVarDefStmts() const { return globalVarDefStmts; }

size_t CompUnit::getFuncDefStmtsCount() const { return funcDefStmts.size(); }

FunctionDefStmt* CompUnit::getFuncDefStmt(size_t index) const {
    return index < funcDefStmts.size() ? funcDefStmts[index].get() : nullptr;
}

const std::vector<std::unique_ptr<FunctionDefStmt>>& CompUnit::getFuncDefStmts() const { return funcDefStmts; }

FunctionDefStmt* CompUnit::getMainFuncDef() const { return mainFuncDef.get(); }

// ---- Exp subclasses ----

// LiteralExp
LiteralExp::LiteralExp(int val) : value(val) {}
LiteralExp::LiteralExp(const std::string& val) : value(val) {}

bool LiteralExp::isInt() const { return std::holds_alternative<int>(value); }
bool LiteralExp::isString() const { return std::holds_alternative<std::string>(value); }

int LiteralExp::getIntValue() const {
    if (auto p = std::get_if<int>(&value)) return *p;
    return 0; // 若类型不匹配，返回默认 0
}

const std::string& LiteralExp::getStringValue() const {
    static const std::string empty;
    if (auto p = std::get_if<std::string>(&value)) return *p;
    return empty;
}

void LiteralExp::setIntValue(int val) { value = val; }
void LiteralExp::setStringValue(const std::string& val) { value = val; }

// IdentifierExp
IdentifierExp::IdentifierExp(const std::string& id) : identifier(id) {}
const std::string& IdentifierExp::getIdentifier() const { return identifier; }
void IdentifierExp::setIdentifier(const std::string& id) { identifier = id; }

// FunctionCallExp
FunctionCallExp::FunctionCallExp(std::unique_ptr<Exp> func, std::vector<std::unique_ptr<Exp>> params)
    : function(std::move(func)), RealParams(std::move(params)) {}

Exp* FunctionCallExp::getFunction() const { return function.get(); }

std::unique_ptr<Exp> FunctionCallExp::releaseFunction() { return std::move(function); }

void FunctionCallExp::setFunction(std::unique_ptr<Exp> func) { function = std::move(func); }

size_t FunctionCallExp::getRealParamsCount() const { return RealParams.size(); }

Exp* FunctionCallExp::getRealParam(size_t index) const { return index < RealParams.size() ? RealParams[index].get() : nullptr; }

void FunctionCallExp::addRealParam(std::unique_ptr<Exp> param) {
    if (param) RealParams.push_back(std::move(param));
}

const std::vector<std::unique_ptr<Exp>>& FunctionCallExp::getRealParams() const { return RealParams; }

// SubscriptExp
SubscriptExp::SubscriptExp(std::unique_ptr<Exp> arr, std::unique_ptr<Exp> sub)
    : array(std::move(arr)), subscript(std::move(sub)) {}

Exp* SubscriptExp::getArray() const { return array.get(); }
Exp* SubscriptExp::getSubscript() const { return subscript.get(); }

std::unique_ptr<Exp> SubscriptExp::releaseArray() { return std::move(array); }
std::unique_ptr<Exp> SubscriptExp::releaseSubscript() { return std::move(subscript); }

void SubscriptExp::setArray(std::unique_ptr<Exp> arr) { array = std::move(arr); }
void SubscriptExp::setSubscript(std::unique_ptr<Exp> sub) { subscript = std::move(sub); }

// UnaryExp
UnaryExp::UnaryExp(Op operation, std::unique_ptr<Exp> expression)
    : op(operation), exp(std::move(expression)) {}

UnaryExp::Op UnaryExp::getOp() const { return op; }
void UnaryExp::setOp(Op operation) { op = operation; }

Exp* UnaryExp::getExp() const { return exp.get(); }
std::unique_ptr<Exp> UnaryExp::releaseExp() { return std::move(exp); }
void UnaryExp::setExp(std::unique_ptr<Exp> expression) { exp = std::move(expression); }

// BinaryExp
BinaryExp::BinaryExp(std::unique_ptr<Exp> left, BinaryOp operation, std::unique_ptr<Exp> right)
    : lhs(std::move(left)), op(operation), rhs(std::move(right)) {}

Exp* BinaryExp::getLhs() const { return lhs.get(); }
Exp* BinaryExp::getRhs() const { return rhs.get(); }
BinaryExp::BinaryOp BinaryExp::getOp() const { return op; }

std::unique_ptr<Exp> BinaryExp::releaseLhs() { return std::move(lhs); }
std::unique_ptr<Exp> BinaryExp::releaseRhs() { return std::move(rhs); }

void BinaryExp::setLhs(std::unique_ptr<Exp> left) { lhs = std::move(left); }
void BinaryExp::setRhs(std::unique_ptr<Exp> right) { rhs = std::move(right); }
void BinaryExp::setOp(BinaryOp operation) { op = operation; }

// ---- Stmt ----
Stmt::Stmt(int line) : line(line) {}
int Stmt::getLine() const { return line; }
void Stmt::setLine(int l) { line = l; }

// ---- VariableDef ----
VariableDef::VariableDef(const std::string& id, bool arrayFlag,
                         std::unique_ptr<Exp> arrSize, std::vector<std::unique_ptr<Exp>> init)
    : identifier(id), arrayFlag(arrayFlag), arraySize(std::move(arrSize)), initial(std::move(init)) {}

const std::string& VariableDef::getIdentifier() const { return identifier; }
void VariableDef::setIdentifier(const std::string& id) { identifier = id; }

bool VariableDef::isArray() const { return arrayFlag; }
void VariableDef::setIsArray(bool arrayFlag) { VariableDef::arrayFlag = arrayFlag; }

Exp* VariableDef::getArraySize() const { return arraySize.get(); }
std::unique_ptr<Exp> VariableDef::releaseArraySize() { return std::move(arraySize); }
void VariableDef::setArraySize(std::unique_ptr<Exp> arrSize) { arraySize = std::move(arrSize); }

size_t VariableDef::getInitialCount() const { return initial.size(); }
Exp* VariableDef::getInitial(size_t index) const { return index < initial.size() ? initial[index].get() : nullptr; }
void VariableDef::addInitial(std::unique_ptr<Exp> init) { if (init) initial.push_back(std::move(init)); }
const std::vector<std::unique_ptr<Exp>>& VariableDef::getInitials() const { return initial; }

// ---- VariableDefStmt ----
VariableDefStmt::VariableDefStmt(int line, std::vector<std::unique_ptr<VariableDef>> defs)
    : Stmt(line), staticFlag(false), constFlag(false), defs(std::move(defs)) {}

bool VariableDefStmt::isStatic() const { return staticFlag; }
bool VariableDefStmt::isConst() const { return constFlag; }
void VariableDefStmt::setStaticFlag(bool flag) { staticFlag = flag; }
void VariableDefStmt::setConstFlag(bool flag) { constFlag = flag; }

BasicType VariableDefStmt::getVarType() const { return varType; }
void VariableDefStmt::setVarType(BasicType type) { varType = type; }

size_t VariableDefStmt::getDefsCount() const { return defs.size(); }

VariableDef* VariableDefStmt::getDef(size_t index) const { return index < defs.size() ? defs[index].get() : nullptr; }
void VariableDefStmt::addDef(std::unique_ptr<VariableDef> decl) { if (decl) defs.push_back(std::move(decl)); }
const std::vector<std::unique_ptr<VariableDef>>& VariableDefStmt::getDefs() const { return defs; }

// ---- Param ----
Param::Param(const std::string& id, BasicType type, bool arrayFlag) : identifier(id), paramType(type), arrayFlag(arrayFlag) {}

const std::string& Param::getIdentifier() const { return identifier; }
void Param::setIdentifier(const std::string& id) { identifier = id; }

BasicType Param::getParamType() const { return paramType; }
void Param::setParamType(BasicType type) { paramType = type; }

bool Param::isArray() const { return arrayFlag; }
void Param::setIsArray(bool arrayFlag) { Param::arrayFlag = arrayFlag; }

// ---- FunctionDefStmt ----
FunctionDefStmt::FunctionDefStmt(int line, BasicType retType, std::vector<std::unique_ptr<Param>> params, std::unique_ptr<Stmt> statement)
    : Stmt(line), identifier(), funcType(retType), formatParams(std::move(params)), stmt(std::move(statement)) {}

const std::string& FunctionDefStmt::getIdentifier() const { return identifier; }
void FunctionDefStmt::setIdentifier(const std::string& id) { identifier = id; }

BasicType FunctionDefStmt::getFuncType() const { return funcType; }
void FunctionDefStmt::setFuncType(BasicType type) { funcType = type; }

size_t FunctionDefStmt::getFormatParamsCount() const { return formatParams.size(); }
Param* FunctionDefStmt::getFormatParam(size_t index) const { return index < formatParams.size() ? formatParams[index].get() : nullptr; }
void FunctionDefStmt::addFormatParam(std::unique_ptr<Param> param) { if (param) formatParams.push_back(std::move(param)); }
const std::vector<std::unique_ptr<Param>>& FunctionDefStmt::getFormatParams() const { return formatParams; }
void FunctionDefStmt::setFormatParams(std::vector<std::unique_ptr<Param>> params) { formatParams = std::move(params); }

bool FunctionDefStmt::hasStmt() const { return stmt != nullptr; }
Stmt* FunctionDefStmt::getStmt() const { return stmt.get(); }
std::unique_ptr<Stmt> FunctionDefStmt::releaseStmt() { return std::move(stmt); }
void FunctionDefStmt::setStmt(std::unique_ptr<Stmt> statement) { stmt = std::move(statement); }

// ---- ExpStmt ----
ExpStmt::ExpStmt(int line, std::unique_ptr<Exp> exp) : Stmt(line), exp(std::move(exp)) {}

Exp* ExpStmt::getExp() const { return exp.get(); }
std::unique_ptr<Exp> ExpStmt::releaseExp() { return std::move(exp); }
void ExpStmt::setExp(std::unique_ptr<Exp> exp) { ExpStmt::exp = std::move(exp); }

// ---- ForStmt ----
ForStmt::ForStmt(int line, std::unique_ptr<Stmt> init, std::unique_ptr<Exp> cond,
                 std::unique_ptr<Exp> iter, std::unique_ptr<Stmt> statement)
    : Stmt(line), initial(std::move(init)), condition(std::move(cond)), iteration(std::move(iter)), stmt(std::move(statement)) {}

Stmt* ForStmt::getInitial() const { return initial.get(); }
std::unique_ptr<Stmt> ForStmt::releaseInitial() { return std::move(initial); }
void ForStmt::setInitial(std::unique_ptr<Stmt> init) { initial = std::move(init); }

Exp* ForStmt::getCondition() const { return condition.get(); }
std::unique_ptr<Exp> ForStmt::releaseCondition() { return std::move(condition); }
void ForStmt::setCondition(std::unique_ptr<Exp> cond) { condition = std::move(cond); }

Exp* ForStmt::getIteration() const { return iteration.get(); }
std::unique_ptr<Exp> ForStmt::releaseIteration() { return std::move(iteration); }
void ForStmt::setIteration(std::unique_ptr<Exp> iter) { iteration = std::move(iter); }

Stmt* ForStmt::getStmt() const { return stmt.get(); }
std::unique_ptr<Stmt> ForStmt::releaseStmt() { return std::move(stmt); }
void ForStmt::setStmt(std::unique_ptr<Stmt> statement) { stmt = std::move(statement); }

// ---- IfStmt ----
IfStmt::IfStmt(int line, std::unique_ptr<Exp> cond, std::unique_ptr<Stmt> trueStatement, std::unique_ptr<Stmt> falseStatement)
    : Stmt(line), condition(std::move(cond)), trueStmt(std::move(trueStatement)), falseStmt(std::move(falseStatement)) {}

Exp* IfStmt::getCondition() const { return condition.get(); }
std::unique_ptr<Exp> IfStmt::releaseCondition() { return std::move(condition); }
void IfStmt::setCondition(std::unique_ptr<Exp> cond) { condition = std::move(cond); }

Stmt* IfStmt::getTrueStmt() const { return trueStmt.get(); }
std::unique_ptr<Stmt> IfStmt::releaseTrueStmt() { return std::move(trueStmt); }
void IfStmt::setTrueStmt(std::unique_ptr<Stmt> trueStatement) { trueStmt = std::move(trueStatement); }

Stmt* IfStmt::getFalseStmt() const { return falseStmt.get(); }
std::unique_ptr<Stmt> IfStmt::releaseFalseStmt() { return std::move(falseStmt); }
void IfStmt::setFalseStmt(std::unique_ptr<Stmt> falseStatement) { falseStmt = std::move(falseStatement); }

// ---- JumpStmt ----
JumpStmt::JumpStmt(int line, JumpStmtType jumpType, std::unique_ptr<Exp> retExp)
    : Stmt(line), jumpStmtType(jumpType), returnExp(std::move(retExp)) {}

JumpStmt::JumpStmtType JumpStmt::getJumpStmtType() const { return jumpStmtType; }
void JumpStmt::setJumpStmtType(JumpStmtType jumpType) { jumpStmtType = jumpType; }

Exp* JumpStmt::getReturnExp() const { return returnExp.get(); }
std::unique_ptr<Exp> JumpStmt::releaseReturnExp() { return std::move(returnExp); }
void JumpStmt::setReturnExp(std::unique_ptr<Exp> retExp) { returnExp = std::move(retExp); }

// ---- BlockStmt ----
BlockStmt::BlockStmt(int line, std::vector<std::unique_ptr<Stmt>> statements)
    : Stmt(line), endline(0), stmts(std::move(statements)) {}

int BlockStmt::getEndLine() const { return endline; }
void BlockStmt::setEndLine(int l) { endline = l; }

size_t BlockStmt::getStmtsCount() const { return stmts.size(); }
Stmt* BlockStmt::getStmt(size_t index) const { return index < stmts.size() ? stmts[index].get() : nullptr; }
void BlockStmt::addStmt(std::unique_ptr<Stmt> stmt_) { if (stmt_) stmts.push_back(std::move(stmt_)); }
const std::vector<std::unique_ptr<Stmt>>& BlockStmt::getStmts() const { return stmts; }
