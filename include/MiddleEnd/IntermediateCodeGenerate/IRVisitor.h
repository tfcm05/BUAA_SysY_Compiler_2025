#ifndef IR_VISITOR_H
#define IR_VISITOR_H

#include "../../FrontEnd/SyntaxAnalysis/AstVisitor.h"
#include "../../FrontEnd/SyntaxAnalysis/AstNode.h"
#include "../SemanticAnalysis/SymbolTable.h"
#include "IRBuilder.h"
#include "IRPrinter.h"

#include <string>
#include <unordered_map>
#include <iostream>
#include <vector>

class IRInfo : public Info {
private:
    Value* value;
public:
    IRInfo(Value* val);

    Value* getValue() const;
    void setValue(Value* val);
};

class IRContext : public Context {
public:
    enum visitPurpose {
        DEFAULT,               // 默认
        CREATE_GLOBAL_VARIABLE, // 全局变量
        USE_LOCATION_VALUE,   // 左值
    };
private:
    visitPurpose purpose;

    Value* values[2];

    // 初始化相关
    LLVM::Type *type;
    std::vector<size_t> indices;
public:
    IRContext();

    visitPurpose getVisitPurpose() const;
    void setVisitPurpose(visitPurpose vp);

    Value* getValue() const;
    void setValue(Value* val);

    Value* getValue(size_t index) const;
    void setValue(size_t index, Value* val);

    LLVM::Type* getType() const;
    void setType(LLVM::Type* ty);

    size_t getIndicesCount() const;
    size_t getIndex(size_t index) const;
    void addIndex(size_t index);
    const std::vector<size_t>& getIndices() const;

    ContextPtr clone() override;
};

class IRVisitor : public AstVisitor {
private:
    Module& module;
    IRBuilder& builder;
    LLVMContext& llvmContext;

    std::unordered_map<Clang::Type*, LLVM::Type*> typeCache;

    LLVM::Type* convertType(Clang::Type* clangType);
public:
    IRVisitor(Module& mod, IRBuilder& bld, LLVMContext& ctx);
    ~IRVisitor();

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