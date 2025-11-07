#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include "../front/AstNode.h"
#include "SymbolTable.h"

struct SymbolInfo {
    std::string symbolName;
    std::string typeName;
};

class SemanticVisitor {
private:
    SymbolTablePtr curScope;
    std::ofstream outputFile;
    std::map<int, std::vector<SymbolInfo>> symbolRecords;
    
    int loopDepth = 0;
    TypePtr currentFuncReturnType = nullptr;

public:
    SemanticVisitor();
    ~SemanticVisitor();

    void visitCompUnit(AstNode* node);
private:
    std::string getTypeName(SymbolPtr symbol);
    void addSymbol(const std::string& name, StorageClass sc, TypePtr type, Symbol::SymbolKind kind, int line);
    void visitVariableDeclarator(VariableDeclarator* node, TypeSpecifier ts, TypeQualifier tq, StorageClass sc, int line);
    void visitFormatParam(FormatParam* node, TypePtr type, int line);
    void visitFunctionBody(BlockStmt* body, bool needsReturn);
    void checkPrintfArgs(FunctionCallExp* node, int line);

    void visitMainFunctionDecl(FunctionDecl* node);

    void visitStmt(Stmt* node);
    void visitVariableDecl(VariableDecl* node);
    void visitFunctionDecl(FunctionDecl* node);
    void visitBlockStmt(BlockStmt* node);
    void visitExpStmt(ExpStmt* node);
    void visitForStmt(ForStmt* node);
    void visitIfStmt(IfStmt* node);
    void visitReturnStmt(ReturnStmt* node);
    void visitBreakStmt(BreakStmt* node);
    void visitContinueStmt(ContinueStmt* node);

    TypePtr visitExp(Exp* exp, int line);
    TypePtr visitIdentifierExp(IdentifierExp* node, int line);
    TypePtr visitLiteralExp(LiteralExp* node, int line);
    TypePtr visitFunctionCallExp(FunctionCallExp* node, int line);
    TypePtr visitSubscriptExp(SubscriptExp* node, int line);
    TypePtr visitUnaryExp(UnaryExp* node, int line);
    TypePtr visitBinaryExp(BinaryExp* node, int line);
    TypePtr visitAssignExp(AssignExp* node, int line);
};