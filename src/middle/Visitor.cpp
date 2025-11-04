#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <map>
#include <regex>
#include "../../include/Error.h"
#include "../../include/middle/Visitor.h"
#include "../../include/middle/Type.h"

class ScopeGuard {
public:
    ScopeGuard(std::shared_ptr<SymbolTable>& scope) : currentScope(scope) {
        currentScope = currentScope->pushScope();
    }
    ~ScopeGuard() {
        currentScope = currentScope->popScope();
    }
private:
    std::shared_ptr<SymbolTable>& currentScope;
};

SemanticVisitor::SemanticVisitor() {
    curScope = std::make_shared<SymbolTable>();

    // build-in functions
    {
        auto printfType = std::make_shared<FunctionType>(Type::getIntegerTy(), std::vector<TypePtr>{});
        printfType->setVariadic(true);
        curScope->addSymbol("printf", std::make_shared<Symbol>("printf", StorageClass::NONE, printfType, Symbol::FUNCTION));
    }

    {
        auto getintType = std::make_shared<FunctionType>(Type::getIntegerTy(), std::vector<TypePtr>{});
        curScope->addSymbol("getint", std::make_shared<Symbol>("getint", StorageClass::NONE, getintType, Symbol::FUNCTION));
    }

    outputFile.open("symbol.txt");
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open symbol.txt for writing." << std::endl;
    }
}

SemanticVisitor::~SemanticVisitor() {
    if (outputFile.is_open()) {
        for (const auto& pair : symbolRecords) {
            int scopeId = pair.first;
            const auto& symbolsInScope = pair.second;
            for (const auto& symbolInfo : symbolsInScope) {
                outputFile << scopeId << " "
                           << symbolInfo.symbolName << " "
                           << symbolInfo.typeName << std::endl;
            }
        }
        outputFile.close();
    }
}

void SemanticVisitor::addSymbol(const std::string& name, StorageClass sc, TypePtr type, Symbol::SymbolKind kind, int line) {
    if (curScope->existInCurScope(name)) {
        errorLog(ErrorCode::b, line);
        return;
    }
    auto symbol = std::make_shared<Symbol>(name, sc, type, kind);
    curScope->addSymbol(name, symbol);
    symbolRecords[curScope->getScopeId()].push_back({symbol->getName(), getTypeName(symbol)});
}

void SemanticVisitor::visitVariableDeclarator(VariableDeclarator* node, BasicType dt, TypeQualifier tq, StorageClass sc, int line) {
    // base type
    TypePtr baseTy = convertToType(dt, tq);

    // full type
    TypePtr type;
    if (node->getDimensionSizesCount() > 0) {
        visitExp(node->getDimensionSize(0), line);
        type = std::make_shared<ArrayType>(baseTy, -1);
        type->setTypeQualifier(tq);
    } else {
        type = baseTy;
    }

    for (size_t i = 0; i < node->getInitialsCount(); ++i) {
        visitExp(node->getInitial(i), line);
    }

    addSymbol(node->getIdentifier(), sc, type, Symbol::VARIABLE, line);
}

void SemanticVisitor::visitFormatParam(FormatParam* node, TypePtr type, int line) {
    addSymbol(node->getIdentifier(), StorageClass::NONE, type, Symbol::VARIABLE, line);
}

void SemanticVisitor::visitFunctionBody(BlockStmt* body, bool needsReturn) {
    if (!body) {
        std::cout << "Error: Function body is null." << std::endl;
        return;
    }

    for (size_t i = 0; i < body->getStmtsCount(); ++i) {
        visitStmt(body->getStmt(i));
    }

    if (needsReturn) {
        size_t stmtsCount = body->getStmtsCount();
        if (stmtsCount == 0 || !dynamic_cast<ReturnStmt*>(body->getStmt(stmtsCount - 1))) {
            errorLog(ErrorCode::g, body->getEndLine());
        }
    }
}

void SemanticVisitor::checkPrintfArgs(FunctionCallExp* node, int line) {
    if (node->getRealParamsCount() == 0) return;

    auto formatStrExp = dynamic_cast<LiteralExp*>(node->getRealParam(0));
    if (!formatStrExp || formatStrExp->getLiteralType() != LiteralExp::LiteralType::STRING) {
        return;
    }

    std::string formatStr = formatStrExp->getStringValue();
    int formatSpecifierCount = 0;
    for (size_t i = 0; i < formatStr.length(); ++i) {
        if (formatStr[i] == '%' && i + 1 < formatStr.length() && formatStr[i+1] == 'd') {
            formatSpecifierCount++;
            i++; // 跳过 'd'
        }
    }

    int realArgCount = node->getRealParamsCount() - 1;
    if (formatSpecifierCount != realArgCount) {
        errorLog(ErrorCode::l, line);
    }

    for (size_t i = 1; i < node->getRealParamsCount(); ++i) {
        visitExp(node->getRealParam(i), line);
    }
}

void SemanticVisitor::visitCompUnit(AstNode* node) {
    auto p = dynamic_cast<CompUnit*>(node);

    for (size_t i = 0; i < p->getGlobalVarDeclCount(); ++i) {
        visitStmt(p->getGlobalVarDecl(i));
    }
    for (size_t i = 0; i < p->getFuncDeclCount(); ++i) {
        visitStmt(p->getFuncDecl(i));
    }

    visitMainFunctionDecl(p->getMainFuncDecl());

    dumpErrorsToFile();
}

void SemanticVisitor::visitMainFunctionDecl(FunctionDecl* node) {
    ScopeGuard scopeGuard(curScope);

    visitFunctionBody(node->getFunctionBody(), true);
}


void SemanticVisitor::visitStmt(Stmt* node) {
    if (!node) return;

    if (auto p = dynamic_cast<VariableDecl*>(node)) visitVariableDecl(p);
    else if (auto p = dynamic_cast<FunctionDecl*>(node)) visitFunctionDecl(p);
    else if (auto p = dynamic_cast<BlockStmt*>(node)) visitBlockStmt(p);
    else if (auto p = dynamic_cast<ExpStmt*>(node)) visitExpStmt(p);
    else if (auto p = dynamic_cast<IfStmt*>(node)) visitIfStmt(p);
    else if (auto p = dynamic_cast<ForStmt*>(node)) visitForStmt(p);
    else if (auto p = dynamic_cast<ReturnStmt*>(node)) visitReturnStmt(p);
    else if (auto p = dynamic_cast<BreakStmt*>(node)) visitBreakStmt(p);
    else if (auto p = dynamic_cast<ContinueStmt*>(node)) visitContinueStmt(p);
    else {
        throw std::runtime_error("Unknown AST node type in Visitor");
    }
}

void SemanticVisitor::visitVariableDecl(VariableDecl* node) {
    auto sc = node->getStorageClass();
    auto tq = node->getTypeQualifier();
    auto dt = node->getDeclType();
    auto line = node->getLine();
    for (size_t i = 0; i < node->getDeclaratorsCount(); ++i) {
        visitVariableDeclarator(node->getDeclarator(i), dt, tq, sc, line);
    }
}

void SemanticVisitor::visitFunctionDecl(FunctionDecl* node) {
    // return type
    TypePtr retType = convertToType(node->getReturnType());

    // parameter types
    std::vector<TypePtr> paramTypes;
    for (size_t i = 0; i < node->getFormatParamsCount(); ++i) {
        FormatParam* paramNode = node->getFormatParam(i);
        TypePtr paramType;
        if (paramNode->getDimensionSizesCount() > 0) {
            paramType = std::make_shared<ArrayType>(convertToType(paramNode->getParamType()), -1);
        } else {
            paramType = convertToType(paramNode->getParamType());
        }
        paramTypes.push_back(paramType);
    }

    auto funcType = std::make_shared<FunctionType>(retType, paramTypes);
    addSymbol(node->getIdentifier(), StorageClass::NONE, funcType, Symbol::FUNCTION, node->getLine());

    ScopeGuard scopeGuard(curScope);
    currentFuncReturnType = retType;

    // parameter symbols
    for (size_t i = 0; i < node->getFormatParamsCount(); ++i) {
        visitFormatParam(node->getFormatParam(i), paramTypes[i], node->getLine());
    }

    // function body
    visitFunctionBody(node->getFunctionBody(), !retType->isVoidTy());

    currentFuncReturnType = nullptr;
}

void SemanticVisitor::visitBlockStmt(BlockStmt* node) {
    ScopeGuard guard(curScope);

    for (size_t i = 0; i < node->getStmtsCount(); ++i) {
        visitStmt(node->getStmt(i));
    }
}

void SemanticVisitor::visitExpStmt(ExpStmt* node) {
    visitExp(node->getExp(), node->getLine());
}

void SemanticVisitor::visitForStmt(ForStmt* node) {
    loopDepth++;
    visitExp(node->getInitial(), node->getLine());
    visitExp(node->getCondition(), node->getLine());
    visitExp(node->getIteration(), node->getLine());
    visitStmt(node->getBody());
    loopDepth--;
}

void SemanticVisitor::visitIfStmt(IfStmt* node) {
    visitExp(node->getCondition(), node->getLine());
    visitStmt(node->getTrueStmt());
    visitStmt(node->getFalseStmt());
}

void SemanticVisitor::visitReturnStmt(ReturnStmt* node) {
    visitExp(node->getReturnExp(), node->getLine());

    if (currentFuncReturnType && currentFuncReturnType->isVoidTy() && node->getReturnExp() != nullptr) {
        errorLog(ErrorCode::f, node->getLine());
    }
}

void SemanticVisitor::visitBreakStmt(BreakStmt* node) {
    // break outside of loop
    if (loopDepth == 0) {
        errorLog(ErrorCode::m, node->getLine());
    }
}

void SemanticVisitor::visitContinueStmt(ContinueStmt* node) {
    // continue outside of loop
    if (loopDepth == 0) {
        errorLog(ErrorCode::m, node->getLine());
    }
}

TypePtr SemanticVisitor::visitExp(Exp* exp, int line) {
    if (!exp) return nullptr;
    if (auto p = dynamic_cast<IdentifierExp*>(exp)) return visitIdentifierExp(p, line);
    if (auto p = dynamic_cast<LiteralExp*>(exp)) return visitLiteralExp(p, line);
    if (auto p = dynamic_cast<FunctionCallExp*>(exp)) return visitFunctionCallExp(p, line);
    if (auto p = dynamic_cast<SubscriptExp*>(exp)) return visitSubscriptExp(p, line);
    if (auto p = dynamic_cast<UnaryExp*>(exp)) return visitUnaryExp(p, line);
    if (auto p = dynamic_cast<BinaryExp*>(exp)) return visitBinaryExp(p, line);
    if (auto p = dynamic_cast<AssignExp*>(exp)) return visitAssignExp(p, line);
    if (auto p = dynamic_cast<LiteralExp*>(exp)) return visitLiteralExp(p, line);
    throw std::runtime_error("Unknown Expression type in getExpType");
}

TypePtr SemanticVisitor::visitIdentifierExp(IdentifierExp* node, int line) {
    auto symbol = curScope->lookup(node->getIdentifier());

    if (!symbol) {
        errorLog(ErrorCode::c, line);
        return nullptr;
    }

    return symbol->getType();
}

TypePtr SemanticVisitor::visitLiteralExp(LiteralExp* node, int line) {
    if (node->getLiteralType() == LiteralExp::LiteralType::STRING) {
        auto stringConst = std::make_shared<ArrayType>(Type::getConstCharTy(), node->getStringValue().length() + 1);
        stringConst->setTypeQualifier(TypeQualifier::CONST);
        return stringConst;
    } else if (node->getLiteralType() == LiteralExp::LiteralType::NUMBER) {
        return Type::getConstIntegerTy();
    }
    return nullptr;
}

TypePtr SemanticVisitor::visitFunctionCallExp(FunctionCallExp* node, int line) {
    auto type = visitExp(node->getFunction(), line);

    if (!type) {
        return nullptr;
    }

    // check function type
    if (type && !type->isFunctionTy()) {
        std::cout << "Error: Calling a non-function variable." << std::endl;
        return nullptr;
    }
    
    auto funcType = std::dynamic_pointer_cast<FunctionType>(type);
    
    if (!funcType->isVariadic()) {
        // check param count
        if (funcType->getParamsCount() != node->getRealParamsCount()) {
            errorLog(ErrorCode::d, line);
        } else {
            // check param types
            for (size_t i = 0; i < node->getRealParamsCount(); ++i) {
                TypePtr formalType = funcType->getParamType(i);
                TypePtr realType = visitExp(node->getRealParam(i), line);
                if (!formalType->isCompatibleWith(realType)) {
                    errorLog(ErrorCode::e, line);
                }
            }
        }
    } else {
        // build-in functions
        if (auto idExp = dynamic_cast<IdentifierExp*>(node->getFunction())) {
            if (idExp->getIdentifier() == "printf") {
                checkPrintfArgs(node, line);
            }
        }
    }

    return funcType->getReturnType();
}

TypePtr SemanticVisitor::visitSubscriptExp(SubscriptExp* node, int line) {
    auto type = visitExp(node->getArray(), line);

    if (!type) {
        return nullptr;
    }

    // check array type
    if (type && !type->isArrayTy()) {
        std::cout << "Error: Subscript operator applied to non-array type." << std::endl;
        return nullptr;
    }

    auto arrayType = std::dynamic_pointer_cast<ArrayType>(type);

    visitExp(node->getSubscript(), line);

    return arrayType->getElementType();
}

TypePtr SemanticVisitor::visitUnaryExp(UnaryExp* node, int line) {
    return visitExp(node->getExp(), line);
}

TypePtr SemanticVisitor::visitBinaryExp(BinaryExp* node, int line) {
    visitExp(node->getLhs(), line);
    visitExp(node->getRhs(), line);
    return Type::getIntegerTy();
}

TypePtr SemanticVisitor::visitAssignExp(AssignExp* node, int line) {
    auto lhsType = visitExp(node->getLhs(), line);

    if (!lhsType) {
        return nullptr;
    }

    // check lval is const
    if (lhsType && lhsType->isConst()) {
        errorLog(ErrorCode::h, line);
    }

    visitExp(node->getRhs(), line);

    return lhsType;
}

// Helper function to get the required type name string
std::string SemanticVisitor::getTypeName(SymbolPtr symbol) {
    TypePtr type = symbol->getType();
    std::string prefix;
    if (type->isConst()) {
        prefix = "Const";
    } else if (symbol->getStorageClass() == StorageClass::STATIC) {
        prefix = "Static";
    }

    if (type->isFunctionTy()) {
        auto funcType = std::dynamic_pointer_cast<FunctionType>(type);
        return funcType->getReturnType()->isVoidTy() ? "VoidFunc" : "IntFunc";
    } else if (type->isArrayTy()) {
        return prefix + "IntArray";
    } else if (type->isIntegerTy()) {
        return prefix.empty() ? "Int" : prefix + "Int";
    }
    return "Unknown";
}