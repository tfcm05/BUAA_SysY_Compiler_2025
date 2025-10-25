#ifndef ASTNODE_H
#define ASTNODE_H

#include<fstream>
#include<memory>
#include<vector>
#include<string>
#include<variant>

class AstNode {
public:
    virtual ~AstNode() = default;
};

/*
 * this is expression part
 */

class Exp : public AstNode { public: virtual ~Exp() = default; };

class LiteralExp : public Exp {
private:
    std::variant<int, std::string> value;
public:
    LiteralExp(int val);
    LiteralExp(const std::string& val);

    bool isInt() const;
    bool isString() const;

    int getIntValue() const;
    const std::string& getStringValue() const;

    void setIntValue(int val);
    void setStringValue(const std::string& val);
};

class IdentifierExp : public Exp {
private:
    std::string identifier;
public:
    IdentifierExp(const std::string& id);
    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);
};

class FunctionCallExp : public Exp {
private:
    std::unique_ptr<Exp> function;
    std::vector<std::unique_ptr<Exp>> RealParams;
public:
    FunctionCallExp(std::unique_ptr<Exp> func, std::vector<std::unique_ptr<Exp>> params);

    Exp* getFunction() const;
    std::unique_ptr<Exp> releaseFunction();
    void setFunction(std::unique_ptr<Exp> func);

    size_t getRealParamsCount() const;
    Exp* getRealParam(size_t index) const;
    void addRealParam(std::unique_ptr<Exp> param);
    const std::vector<std::unique_ptr<Exp>>& getRealParams() const;
};

class SubscriptExp : public Exp {
private:
    std::unique_ptr<Exp> array;
    std::unique_ptr<Exp> subscript;
public:
    SubscriptExp(std::unique_ptr<Exp> arr, std::unique_ptr<Exp> sub);

    Exp* getArray() const;
    Exp* getSubscript() const;

    std::unique_ptr<Exp> releaseArray();
    std::unique_ptr<Exp> releaseSubscript();

    void setArray(std::unique_ptr<Exp> arr);
    void setSubscript(std::unique_ptr<Exp> sub);
};

class UnaryExp : public Exp {
public:
    enum class Op { PLUS, MINU, NOT };
private:
    Op op;
    std::unique_ptr<Exp> exp;
public:
    UnaryExp(Op operation, std::unique_ptr<Exp> expression);

    Op getOp() const;
    void setOp(Op operation);

    Exp* getExp() const;
    std::unique_ptr<Exp> releaseExp();
    void setExp(std::unique_ptr<Exp> expression);
};

class BinaryExp : public Exp {
public:
    enum class BinaryOp { PLUS, MINU, MULT, DIV, MOD, ASSIGN, OR, AND, EQL, NEQ, GRE, GEQ, LSS, LEQ, COMMA, UNKNOWN };
private:
    std::unique_ptr<Exp> lhs;
    BinaryOp op;
    std::unique_ptr<Exp> rhs;
public:
    BinaryExp(std::unique_ptr<Exp> left, BinaryOp operation, std::unique_ptr<Exp> right);

    Exp* getLhs() const;
    Exp* getRhs() const;
    BinaryOp getOp() const;

    std::unique_ptr<Exp> releaseLhs();
    std::unique_ptr<Exp> releaseRhs();

    void setLhs(std::unique_ptr<Exp> left);
    void setRhs(std::unique_ptr<Exp> right);
    void setOp(BinaryOp operation);
};

/* 
 * this is statement part
 */
class Stmt : public AstNode {
private:
    int line;
public:
    Stmt(int line = 0);
    int getLine() const;
    void setLine(int l);
};

/*
 * this is type part
 */
enum class BasicType { INT, VOID };

class VariableDef : public AstNode {
private:
    std::string identifier;
    bool arrayFlag;
    std::unique_ptr<Exp> arraySize;
    std::vector<std::unique_ptr<Exp>> initial;
public:
    VariableDef(const std::string& id, bool arrayFlag = false,
             std::unique_ptr<Exp> arrSize = nullptr,
             std::vector<std::unique_ptr<Exp>> init = {});

    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);

    bool isArray() const;
    void setIsArray(bool arrayFlag);

    Exp* getArraySize() const;
    std::unique_ptr<Exp> releaseArraySize();
    void setArraySize(std::unique_ptr<Exp> arrSize);

    size_t getInitialCount() const;
    Exp* getInitial(size_t index) const;
    void addInitial(std::unique_ptr<Exp> init);
    const std::vector<std::unique_ptr<Exp>>& getInitials() const;
};

class VariableDefStmt : public Stmt {
private:
    bool staticFlag;
    bool constFlag;
    BasicType varType;
    std::vector<std::unique_ptr<VariableDef>> defs;
public:
    VariableDefStmt(int line, std::vector<std::unique_ptr<VariableDef>> defs = {});

    bool isStatic() const;
    bool isConst() const;
    void setStaticFlag(bool flag);
    void setConstFlag(bool flag);

    BasicType getVarType() const;
    void setVarType(BasicType type);

    size_t getDefsCount() const;
    VariableDef* getDef(size_t index) const;
    void addDef(std::unique_ptr<VariableDef> decl);
    const std::vector<std::unique_ptr<VariableDef>>& getDefs() const;
};

class Param : public AstNode {
    std::string identifier;
    BasicType paramType;
    bool arrayFlag;
public:
    Param(const std::string& id, BasicType type, bool arrayFlag = false);

    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);

    BasicType getParamType() const;
    void setParamType(BasicType type);

    bool isArray() const;
    void setIsArray(bool arrayFlag);
};

class FunctionDefStmt : public Stmt {
    std::string identifier;
    BasicType funcType;
    std::vector<std::unique_ptr<Param>> formatParams;
    std::unique_ptr<Stmt> stmt;
public:
     FunctionDefStmt(int line, BasicType funcType,
                 std::vector<std::unique_ptr<Param>> params = {},
                 std::unique_ptr<Stmt> statement = nullptr);

    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);

    BasicType getFuncType() const;
    void setFuncType(BasicType type);
 
    size_t getFormatParamsCount() const;
    Param* getFormatParam(size_t index) const;
    void addFormatParam(std::unique_ptr<Param> param);
    const std::vector<std::unique_ptr<Param>>& getFormatParams() const;
    void setFormatParams(std::vector<std::unique_ptr<Param>> params);
 
    bool hasStmt() const;
    Stmt* getStmt() const;
    std::unique_ptr<Stmt> releaseStmt();
    void setStmt(std::unique_ptr<Stmt> statement);

};

class ExpStmt : public Stmt {
private:
    std::unique_ptr<Exp> exp;
public:
    ExpStmt(int line, std::unique_ptr<Exp> exp);

    Exp* getExp() const;
    std::unique_ptr<Exp> releaseExp();
    void setExp(std::unique_ptr<Exp> exp);
};

class ForStmt : public Stmt {
private:
    std::unique_ptr<Stmt> initial;
    std::unique_ptr<Exp> condition;
    std::unique_ptr<Exp> iteration;
    std::unique_ptr<Stmt> stmt;
public:
    ForStmt(int line, std::unique_ptr<Stmt> init = nullptr, std::unique_ptr<Exp> cond = nullptr,
            std::unique_ptr<Exp> iter = nullptr, std::unique_ptr<Stmt> statement = nullptr);

    Stmt* getInitial() const;
    std::unique_ptr<Stmt> releaseInitial();
    void setInitial(std::unique_ptr<Stmt> init);

    Exp* getCondition() const;
    std::unique_ptr<Exp> releaseCondition();
    void setCondition(std::unique_ptr<Exp> cond);

    Exp* getIteration() const;
    std::unique_ptr<Exp> releaseIteration();
    void setIteration(std::unique_ptr<Exp> iter);

    Stmt* getStmt() const;
    std::unique_ptr<Stmt> releaseStmt();
    void setStmt(std::unique_ptr<Stmt> statement);
};

class IfStmt : public Stmt {
private:
    std::unique_ptr<Exp> condition;
    std::unique_ptr<Stmt> trueStmt;
    std::unique_ptr<Stmt> falseStmt;
public:
    IfStmt(int line, std::unique_ptr<Exp> cond = nullptr,
           std::unique_ptr<Stmt> trueStatement = nullptr,
           std::unique_ptr<Stmt> falseStatement = nullptr);

    Exp* getCondition() const;
    std::unique_ptr<Exp> releaseCondition();
    void setCondition(std::unique_ptr<Exp> cond);

    Stmt* getTrueStmt() const;
    std::unique_ptr<Stmt> releaseTrueStmt();
    void setTrueStmt(std::unique_ptr<Stmt> trueStatement);

    Stmt* getFalseStmt() const;
    std::unique_ptr<Stmt> releaseFalseStmt();
    void setFalseStmt(std::unique_ptr<Stmt> falseStatement);
};

class JumpStmt : public Stmt {
public:
    enum class JumpStmtType { RETURN, BREAK, CONTINUE };
private:
    JumpStmtType jumpStmtType;
    std::unique_ptr<Exp> returnExp;
public:
    JumpStmt(int line, JumpStmtType jumpStmtType, std::unique_ptr<Exp> retExp = nullptr);
    JumpStmtType getJumpStmtType() const;
    void setJumpStmtType(JumpStmtType jumpStmtType);

    Exp* getReturnExp() const;
    std::unique_ptr<Exp> releaseReturnExp();
    void setReturnExp(std::unique_ptr<Exp> retExp);
};

class BlockStmt : public Stmt {
private:    
    int endline;
    std::vector<std::unique_ptr<Stmt>> stmts;
public:
    BlockStmt(int line, std::vector<std::unique_ptr<Stmt>> statements = {});

    int getEndLine() const;
    void setEndLine(int l);

    size_t getStmtsCount() const;
    Stmt* getStmt(size_t index) const;
    void addStmt(std::unique_ptr<Stmt> stmt);
    const std::vector<std::unique_ptr<Stmt>>& getStmts() const;
};

class CompUnit : public AstNode {
private:
    std::vector<std::unique_ptr<VariableDefStmt>> globalVarDefStmts;
    std::vector<std::unique_ptr<FunctionDefStmt>> funcDefStmts;
    std::unique_ptr<FunctionDefStmt> mainFuncDef;
public:

    CompUnit();

    void addGlobalVarDefStmt(std::unique_ptr<VariableDefStmt> varDefStmt);
    void addFuncDefStmt(std::unique_ptr<FunctionDefStmt> funcDefStmt);
    void setMainFuncDef(std::unique_ptr<FunctionDefStmt> mainFunc);

    size_t getGlobalVarDefStmtsCount() const;
    VariableDefStmt* getGlobalVarDefStmt(size_t index) const;
    const std::vector<std::unique_ptr<VariableDefStmt>>& getGlobalVarDefStmts() const;

    size_t getFuncDefStmtsCount() const;
    FunctionDefStmt* getFuncDefStmt(size_t index) const;
    const std::vector<std::unique_ptr<FunctionDefStmt>>& getFuncDefStmts() const;

    FunctionDefStmt* getMainFuncDef() const;
};

#endif