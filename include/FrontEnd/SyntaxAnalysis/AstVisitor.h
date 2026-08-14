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

class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    // Expressions
    virtual InfoPtr visitLiteralExp(LiteralExp* node, ContextPtr context) = 0;
    virtual InfoPtr visitIdentifierExp(IdentifierExp* node, ContextPtr context) = 0;
    virtual InfoPtr visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) = 0;
    virtual InfoPtr visitSubscriptExp(SubscriptExp* node, ContextPtr context) = 0;
    virtual InfoPtr visitUnaryExp(UnaryExp* node, ContextPtr context) = 0;
    virtual InfoPtr visitBinaryExp(BinaryExp* node, ContextPtr context) = 0;
    virtual InfoPtr visitAssignExp(AssignExp* node, ContextPtr context) = 0;
    virtual InfoPtr visitCommaExp(CommaExp* node, ContextPtr context) = 0;

    // Statements
    virtual InfoPtr visitExpStmt(ExpStmt* node, ContextPtr context) = 0;
    virtual InfoPtr visitForStmt(ForStmt* node, ContextPtr context) = 0;
    virtual InfoPtr visitWhileStmt(WhileStmt* node, ContextPtr context) = 0;
    virtual InfoPtr visitDoWhileStmt(DoWhileStmt* node, ContextPtr context) = 0;
    virtual InfoPtr visitIfStmt(IfStmt* node, ContextPtr context) = 0;
    virtual InfoPtr visitBreakStmt(BreakStmt* node, ContextPtr context) = 0;
    virtual InfoPtr visitContinueStmt(ContinueStmt* node, ContextPtr context) = 0;
    virtual InfoPtr visitReturnStmt(ReturnStmt* node, ContextPtr context) = 0;
    virtual InfoPtr visitBlockStmt(BlockStmt* node, ContextPtr context) = 0;

    // Initializers
    virtual InfoPtr visitExpInitializer(ExpInitializer* node, ContextPtr context) = 0;
    virtual InfoPtr visitListInitializer(ListInitializer* node, ContextPtr context) = 0;

    // Declarators
    virtual InfoPtr visitIdentDeclarator(IdentDeclarator* node, ContextPtr context) = 0;
    virtual InfoPtr visitArrayDeclarator(ArrayDeclarator* node, ContextPtr context) = 0;
    virtual InfoPtr visitFuncDeclarator(FuncDeclarator* node, ContextPtr context) = 0;
    virtual InfoPtr visitInitialDeclarator(InitialDeclarator* node, ContextPtr context) = 0;

    // Declarations
    virtual InfoPtr visitVariableDecl(VariableDecl* node, ContextPtr context) = 0;
    virtual InfoPtr visitFunctionDecl(FunctionDecl* node, ContextPtr context) = 0;
    virtual InfoPtr visitParameterDecl(ParameterDecl* node, ContextPtr context) = 0;

    // CompUnit
    virtual InfoPtr visitCompUnit(CompUnit* node, ContextPtr context) = 0;
};
#endif