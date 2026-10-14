#ifndef ASTVISITOR_H
#define ASTVISITOR_H
#include <memory>

class LiteralExp;
class IdentifierExp;
class FunctionCallExp;
class SubscriptExp;
class UnaryExp;
class BinaryExp;
class AssignExp;
class CommaExp;

class ExpStmt;
class ForStmt;
class WhileStmt;
class DoWhileStmt;
class IfStmt;
class BreakStmt;
class ContinueStmt;
class ReturnStmt;
class BlockStmt;

class ExpInitializer;
class ListInitializer;

class IdentDeclarator;
class PointerDeclarator;
class ArrayDeclarator;
class FuncDeclarator;
class InitialDeclarator;

class VariableDecl;
class FunctionDecl;
class ParameterDecl;
class CompUnit;

class Info;
using InfoPtr = std::shared_ptr<Info>;

class Info {
public:
    virtual ~Info() = default;
};

class Context;
using ContextPtr = std::shared_ptr<Context>;

class Context {
public:
    virtual ~Context() = default;
    virtual ContextPtr clone() = 0;
};

// Default empty visit bodies: new node kinds need not force every visitor to stub.
// SemanticVisitor still overrides all; IRVisitor skips unused declarator walks.
class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    virtual InfoPtr visitLiteralExp(LiteralExp* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitIdentifierExp(IdentifierExp* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitSubscriptExp(SubscriptExp* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitUnaryExp(UnaryExp* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitBinaryExp(BinaryExp* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitAssignExp(AssignExp* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitCommaExp(CommaExp* node, ContextPtr context) { (void)node; (void)context; return nullptr; }

    virtual InfoPtr visitExpStmt(ExpStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitForStmt(ForStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitWhileStmt(WhileStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitDoWhileStmt(DoWhileStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitIfStmt(IfStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitBreakStmt(BreakStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitContinueStmt(ContinueStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitReturnStmt(ReturnStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitBlockStmt(BlockStmt* node, ContextPtr context) { (void)node; (void)context; return nullptr; }

    virtual InfoPtr visitExpInitializer(ExpInitializer* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitListInitializer(ListInitializer* node, ContextPtr context) { (void)node; (void)context; return nullptr; }

    virtual InfoPtr visitIdentDeclarator(IdentDeclarator* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitPointerDeclarator(PointerDeclarator* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitArrayDeclarator(ArrayDeclarator* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitFuncDeclarator(FuncDeclarator* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitInitialDeclarator(InitialDeclarator* node, ContextPtr context) { (void)node; (void)context; return nullptr; }

    virtual InfoPtr visitVariableDecl(VariableDecl* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitFunctionDecl(FunctionDecl* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
    virtual InfoPtr visitParameterDecl(ParameterDecl* node, ContextPtr context) { (void)node; (void)context; return nullptr; }

    virtual InfoPtr visitCompUnit(CompUnit* node, ContextPtr context) { (void)node; (void)context; return nullptr; }
};
#endif
