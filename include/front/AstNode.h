#ifndef ASTNODE_H
#define ASTNODE_H

#include<fstream>
#include<memory>
#include<vector>
#include<string>
#include<variant>

/*
 * this is type part
 */
enum class TypeSpecifier { INT, VOID };

enum class StorageClass { NONE, STATIC, };

enum class TypeQualifier { NONE, CONST, };

class AstNode {
public:
    virtual ~AstNode() = default;
};

/*
 * this is expression part
 */

class Exp : public AstNode {
public:
    virtual ~Exp() = default;
};

class LiteralExp : public Exp {
public:
    enum class LiteralType { NUMBER, STRING };
private:
    LiteralType literalType;
    std::variant<int, std::string> value;
public:
    LiteralExp(int val);
    LiteralExp(const std::string& val);

    LiteralType getLiteralType() const;

    int getIntValue() const;
    const std::string& getStringValue() const;
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
    FunctionCallExp(std::unique_ptr<Exp> func, std::vector<std::unique_ptr<Exp>> rParams);

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
    enum class UnaryOp { PLUS, MINU, NOT };
private:
    UnaryOp unaryOp;
    std::unique_ptr<Exp> exp;
public:
    UnaryExp(UnaryOp uop, std::unique_ptr<Exp> e);

    UnaryOp getUnaryOp() const;
    void setUnaryOp(UnaryOp uop);

    Exp* getExp() const;
    std::unique_ptr<Exp> releaseExp();
    void setExp(std::unique_ptr<Exp> e);
};

class BinaryExp : public Exp {
public:
    enum class BinaryOp { PLUS, MINU, MULT, DIV, MOD, OR, AND, EQL, NEQ, GRE, GEQ, LSS, LEQ, COMMA, UNKNOWN };
private:
    std::unique_ptr<Exp> lhs;
    BinaryOp binaryOp;
    std::unique_ptr<Exp> rhs;
public:
    BinaryExp(std::unique_ptr<Exp> l, BinaryOp bop, std::unique_ptr<Exp> r);

    Exp* getLhs() const;
    Exp* getRhs() const;
    BinaryOp getBinaryOp() const;

    std::unique_ptr<Exp> releaseLhs();
    std::unique_ptr<Exp> releaseRhs();

    void setLhs(std::unique_ptr<Exp> l);
    void setRhs(std::unique_ptr<Exp> r);
    void setBinaryOp(BinaryOp bop);
};

class AssignExp : public Exp {
public:
    enum class AssignType { EQ, ADD_EQ, SUB_EQ, MUL_EQ, DIV_EQ, MOD_EQ };
private:
    std::unique_ptr<Exp> lhs;
    AssignType assignType;
    std::unique_ptr<Exp> rhs;
public:
    AssignExp(std::unique_ptr<Exp> l, AssignType op, std::unique_ptr<Exp> r);

    Exp* getLhs() const;
    Exp* getRhs() const;
    AssignType getAssignType() const;

    std::unique_ptr<Exp> releaseLhs();
    std::unique_ptr<Exp> releaseRhs();

    void setLhs(std::unique_ptr<Exp> l);
    void setRhs(std::unique_ptr<Exp> r);
};

/* 
 * this is statement part
 */
class Stmt : public AstNode {
private:
    int startLine;
public:
    Stmt(int sl);
    virtual ~Stmt() = default;
    int getLine() const;
    void setLine(int sl);
};

class ExpStmt : public Stmt {
private:
    std::unique_ptr<Exp> exp;
public:
    ExpStmt(int sl);

    Exp* getExp() const;
    std::unique_ptr<Exp> releaseExp();
    void setExp(std::unique_ptr<Exp> e);
};

class ForStmt : public Stmt {
private:
    std::unique_ptr<Exp> initial;
    std::unique_ptr<Exp> condition;
    std::unique_ptr<Exp> iteration;
    std::unique_ptr<Stmt> stmt;
public:
    ForStmt(int sl);

    Exp* getInitial() const;
    std::unique_ptr<Exp> releaseInitial();
    void setInitial(std::unique_ptr<Exp> s);

    Exp* getCondition() const;
    std::unique_ptr<Exp> releaseCondition();
    void setCondition(std::unique_ptr<Exp> cond);

    Exp* getIteration() const;
    std::unique_ptr<Exp> releaseIteration();
    void setIteration(std::unique_ptr<Exp> iter);

    Stmt* getBody() const;
    std::unique_ptr<Stmt> releaseBody();
    void setBody(std::unique_ptr<Stmt> s);
};

class IfStmt : public Stmt {
private:
    std::unique_ptr<Exp> condition;
    std::unique_ptr<Stmt> trueStmt;
    std::unique_ptr<Stmt> falseStmt;
public:
    IfStmt(int sl);

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

class BreakStmt : public Stmt {
public:
    BreakStmt(int sl);
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt(int sl);
};

class ReturnStmt : public Stmt {
private:
    std::unique_ptr<Exp> returnExp;
public:
    ReturnStmt(int sl);

    Exp* getReturnExp() const;
    std::unique_ptr<Exp> releaseReturnExp();
    void setReturnExp(std::unique_ptr<Exp> reType);
};

class BlockStmt : public Stmt {
private:    
    int endline;
    std::vector<std::unique_ptr<Stmt>> stmts;
public:
    BlockStmt(int sl);

    int getEndLine() const;
    void setEndLine(int el);

    size_t getStmtsCount() const;
    Stmt* getStmt(size_t index) const;
    void addStmt(std::unique_ptr<Stmt> s);
    const std::vector<std::unique_ptr<Stmt>>& getStmts() const;
};

class VariableDeclarator : public AstNode {
private:
    std::string identifier;
    std::vector<std::unique_ptr<Exp>> dimensionSizes;
    std::vector<std::unique_ptr<Exp>> initials;
public:
    VariableDeclarator();

    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);

    size_t getDimensionSizesCount() const;
    Exp* getDimensionSize(size_t index) const;
    void addDimensionSize(std::unique_ptr<Exp> dimSize);
    const std::vector<std::unique_ptr<Exp>>& getDimensionSizes() const;

    size_t getInitialsCount() const;
    Exp* getInitial(size_t index) const;
    void addInitial(std::unique_ptr<Exp> init);
    const std::vector<std::unique_ptr<Exp>>& getInitials() const;
};

class VariableDecl : public Stmt {
private:
    StorageClass storageClass;
    TypeQualifier typeQualifier;
    TypeSpecifier typeSpecifier;
    std::vector<std::unique_ptr<VariableDeclarator>> declarators;
public:
    VariableDecl(int sl);

    StorageClass getStorageClass() const;
    void setStorageClass(StorageClass storageClass);

    TypeQualifier getTypeQualifier() const;
    void setTypeQualifier(TypeQualifier typeQualifier);

    TypeSpecifier getVarType() const;
    void setVarType(TypeSpecifier type);

    size_t getDeclaratorsCount() const;
    VariableDeclarator* getDeclarator(size_t index) const;

    const std::vector<std::unique_ptr<VariableDeclarator>>& getDeclarators() const;

    void addDeclarator(std::unique_ptr<VariableDeclarator> decl);
};

class FormatParam : public AstNode {
    std::string identifier;
    TypeSpecifier paramType;
    std::vector<std::unique_ptr<Exp>> dimensionSizes;
public:
    FormatParam();

    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);

    TypeSpecifier getParamType() const;
    void setParamType(TypeSpecifier type);

    size_t getDimensionSizesCount() const;
    Exp* getDimensionSize(size_t index) const;
    void addDimensionSize(std::unique_ptr<Exp> dimSize);
    const std::vector<std::unique_ptr<Exp>>& getDimensionSizes() const;
};

class FunctionDecl : public Stmt {
    std::string identifier;
    TypeSpecifier returnType;
    std::vector<std::unique_ptr<FormatParam>> formatParams;
    std::unique_ptr<BlockStmt> functionBody;
public:
    FunctionDecl(int sl);

    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);

    TypeSpecifier getReturnType() const;
    void setReturnType(TypeSpecifier rType);

    size_t getFormatParamsCount() const;
    FormatParam* getFormatParam(size_t index) const;
    void addFormatParam(std::unique_ptr<FormatParam> param);
    const std::vector<std::unique_ptr<FormatParam>>& getFormatParams() const;

    BlockStmt* getFunctionBody() const;
    std::unique_ptr<BlockStmt> releaseFunctionBody();
    void setFunctionBody(std::unique_ptr<BlockStmt> b);
};

class CompUnit : public AstNode {
private:
    std::vector<std::unique_ptr<VariableDecl>> globalVarDecl;
    std::vector<std::unique_ptr<FunctionDecl>> funcDecl;
    std::unique_ptr<FunctionDecl> mainFuncDecl;
public:
    CompUnit();

    size_t getGlobalVarDeclCount() const;
    VariableDecl* getGlobalVarDecl(size_t index) const;
    void addGlobalVarDecl(std::unique_ptr<VariableDecl> decl);
    const std::vector<std::unique_ptr<VariableDecl>>& getGlobalVarDecls() const;

    size_t getFuncDeclCount() const;
    FunctionDecl* getFuncDecl(size_t index) const;
    void addFuncDecl(std::unique_ptr<FunctionDecl> decl);
    const std::vector<std::unique_ptr<FunctionDecl>>& getFuncDecls() const;

    FunctionDecl* getMainFuncDecl() const;
    std::unique_ptr<FunctionDecl> releaseMainFuncDecl();
    void setMainFuncDecl(std::unique_ptr<FunctionDecl> decl);
};

#endif