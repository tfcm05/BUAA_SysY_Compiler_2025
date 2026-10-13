#ifndef SEMANTIC_VISITOR_H
#define SEMANTIC_VISITOR_H
#include "../../FrontEnd/SyntaxAnalysis/AstVisitor.h"
#include "../../FrontEnd/SyntaxAnalysis/AstNode.h"
#include "../../Error.h"
#include "SymbolTable.h"
#include "ClangContext.h"
#include "ConstValue.h"
#include <fstream>
#include <iostream>
#include <map>

class SemanticInfo : public Info {
private:
    // 声明的declType和表达式的expType
    Clang::Type* type;;

    // 表达式
    bool lValueFlag;
        // 常量
        Clang::ConstValue* constValue;

    // 声明
    std::string name;
public:
    SemanticInfo();

    Clang::Type* getType() const;
    void setType(Clang::Type* ty);

    bool isLValue() const;
    void setLValueFlag(bool lvf);
    Clang::ConstValue* getConstValue() const;
    void setConstValue(Clang::ConstValue* cv);

    std::string getName() const;
    void setName(const std::string& n);
};

class SemanticContext : public Context {
public:
    enum class VisitPurpose {
        DEFAULT,                   // 默认
        GENERATE_SYMBOL,          // 生成符号 (参数)
        EVALUATE_CONST,           // 常量求值
        RESOLVE_FUNCTION_BODY_ONLY // 只解析函数体，不创建新作用域
    };
private:
    VisitPurpose purpose;

    // 函数的returnType和变量的varType
    Clang::Type* type;

    // 是否在循环体内
    bool loopFlag;

    // File-scope / static-storage declaration region (not const-eval).
    // Set when visiting global VariableDecls from CompUnit.
    bool staticStorageRegion;
public:
    SemanticContext();
    ContextPtr clone() override;

    VisitPurpose getVisitPurpose() const;
    void setVisitPurpose(VisitPurpose vp);

    Clang::Type* getType() const;
    void setType(Clang::Type* ty);

    bool inLoop() const;
    void setLoopFlag(bool lf);

    bool isStaticStorageRegion() const;
    void setStaticStorageRegion(bool ssr);
};

class SemanticVisitor : public AstVisitor {
private:
    // 当前作用域
    SymbolTablePtr curScope;

    // 核心上下文
    ClangContext& clangContext;
    
    // 符号表输出文件
    std::ofstream outputFile;
    
    std::map<int, std::vector<std::unique_ptr<Symbol>>> symbolRecords;

    Clang::Type* convertToType(TypeSpecifier ts, unsigned int tq);
    bool canAssign(Clang::Type* destType, Clang::Type* srcType);
    Symbol* createSymbol(const std::string& name, StorageClass sc, Clang::Type* type, Symbol::SymbolKind kind, Clang::ConstValue* constValue);
public:
    SemanticVisitor(ClangContext& cc);
    ~SemanticVisitor() override;

    // Expressions
    InfoPtr visitLiteralExp(LiteralExp* node, ContextPtr context) override;
    InfoPtr visitIdentifierExp(IdentifierExp* node, ContextPtr context) override;
    InfoPtr visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) override;
    InfoPtr visitSubscriptExp(SubscriptExp* node, ContextPtr context) override;
    InfoPtr visitUnaryExp(UnaryExp* node, ContextPtr context) override;
    InfoPtr visitBinaryExp(BinaryExp* node, ContextPtr context) override;
    InfoPtr visitAssignExp(AssignExp* node, ContextPtr context) override;
    InfoPtr visitCommaExp(CommaExp* node, ContextPtr context) override;

    // Statements
    InfoPtr visitExpStmt(ExpStmt* node, ContextPtr context) override;
    InfoPtr visitForStmt(ForStmt* node, ContextPtr context) override;
    InfoPtr visitWhileStmt(WhileStmt* node, ContextPtr context) override;
    InfoPtr visitDoWhileStmt(DoWhileStmt* node, ContextPtr context) override;
    InfoPtr visitIfStmt(IfStmt* node, ContextPtr context) override;
    InfoPtr visitBreakStmt(BreakStmt* node, ContextPtr context) override;
    InfoPtr visitContinueStmt(ContinueStmt* node, ContextPtr context) override;
    InfoPtr visitReturnStmt(ReturnStmt* node, ContextPtr context) override;
    InfoPtr visitBlockStmt(BlockStmt* node, ContextPtr context) override;

    // Initializers
    InfoPtr visitExpInitializer(ExpInitializer* node, ContextPtr context) override;
    InfoPtr visitListInitializer(ListInitializer* node, ContextPtr context) override;

    // Declarators
    InfoPtr visitIdentDeclarator(IdentDeclarator* node, ContextPtr context) override;
    InfoPtr visitPointerDeclarator(PointerDeclarator* node, ContextPtr context) override;
    InfoPtr visitArrayDeclarator(ArrayDeclarator* node, ContextPtr context) override;
    InfoPtr visitFuncDeclarator(FuncDeclarator* node, ContextPtr context) override;
    InfoPtr visitInitialDeclarator(InitialDeclarator* node, ContextPtr context) override;

    // Declarations
    InfoPtr visitVariableDecl(VariableDecl* node, ContextPtr context) override;
    InfoPtr visitFunctionDecl(FunctionDecl* node, ContextPtr context) override;
    InfoPtr visitParameterDecl(ParameterDecl* node, ContextPtr context) override;

    // CompUnit
    InfoPtr visitCompUnit(CompUnit* node, ContextPtr context) override;
};


#endif