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

public:
    SemanticVisitor();
    ~SemanticVisitor();

    void visitCompUnit(AstNode* node);
private:
    // Helper to generate type names as per requirements
    std::string getTypeName(SymbolPtr symbol);

    void visitMainFunctionDecl(FunctionDecl* node);
    
    void visitStmt(Stmt* node);

    void visitVariableDeclarator(VariableDeclarator* node, TypePtr ty, StorageClass sc, int line);
    void visitVariableDecl(VariableDecl* node);

    TypePtr visitFormatParam(FormatParam* node);
    void visitFunctionDecl(FunctionDecl* node);

    void visitBlockStmt(BlockStmt* node);
    void visitExpStmt(ExpStmt* node);
    void visitForStmt(ForStmt* node);
    void visitIfStmt(IfStmt* node);
    void visitReturnStmt(ReturnStmt* node);
    void visitBreakStmt(BreakStmt* node);
    void visitContinueStmt(ContinueStmt* node);

    TypePtr visitExp(Exp* exp);

    TypePtr visitIdentifierExp(IdentifierExp* node);
    TypePtr visitLiteralExp(LiteralExp* node);
    TypePtr visitFunctionCallExp(FunctionCallExp* node);
    TypePtr visitSubscriptExp(SubscriptExp* node);
    TypePtr visitUnaryExp(UnaryExp* node);
    TypePtr visitBinaryExp(BinaryExp* node);
    TypePtr visitAssignExp(AssignExp* node);
    
};