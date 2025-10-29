// filepath: d:\compiler2\src\middle\Visitor.cpp
#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <map>
#include <regex>
#include "../../include/Error.h"
#include "../../include/middle/Visitor.h"
#include "../../include/middle/Type.h"

SemanticVisitor::SemanticVisitor() {
    curScope = std::make_shared<SymbolTable>(); // Global scope

    // build-in functions
    {
        auto printfType = std::make_shared<FunctionType>(Type::getIntegerTy(), std::vector<TypePtr>{});
        printfType->setVariadic(true);
        auto printfSymbol = std::make_shared<Symbol>("printf", StorageClass::NONE, printfType, Symbol::FUNCTION, true);
        curScope->addSymbol("printf", printfSymbol);
    }

    {
        auto getintType = std::make_shared<FunctionType>(Type::getIntegerTy(), std::vector<TypePtr>{});
        auto getintSymbol = std::make_shared<Symbol>("getint", StorageClass::NONE, getintType, Symbol::FUNCTION, true);
        curScope->addSymbol("getint", getintSymbol);
    }

    outputFile.open("symbol.txt");
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open symbol.txt for writing." << std::endl;
    }
}

SemanticVisitor::~SemanticVisitor() {
    if (outputFile.is_open()) {
        // 遍历 map
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

void SemanticVisitor::visitCompUnit(AstNode* node) {
    auto p = dynamic_cast<CompUnit*>(node);

    for (size_t i = 0; i < p->getGlobalVarDeclCount(); ++i) {
        visitStmt(p->getGlobalVarDecl(i));
    }
    for (size_t i = 0; i < p->getFuncDeclCount(); ++i) {
        visitStmt(p->getFuncDecl(i));
    }
    visitMainFunctionDecl(p->getMainFuncDecl());
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
    else if (dynamic_cast<LiteralExp*>(node)) {
        // No action needed for these nodes in this phase
    } else {
        throw std::runtime_error("Unknown AST node type in Visitor");
    }
}

void SemanticVisitor::visitVariableDecl(VariableDecl* node) {
    auto sc = node->getStorageClass();

    auto tq = node->getTypeQualifier();
    auto declType = node->getDeclType();
    auto ty = convertToType(declType);
    ty->setTypeQualifier(tq);

    auto line = node->getLine();
    for (size_t i = 0; i < node->getDeclaratorsCount(); ++i) {
        visitVariableDeclarator(node->getDeclarator(i), ty, sc, line);
    }
}

void SemanticVisitor::visitVariableDeclarator(VariableDeclarator* node, TypePtr ty, StorageClass sc, int line) {
    if (curScope->existInCurScope(node->getIdentifier())) {
        errorLog(ErrorCode::b, line);
        return;
    }

    TypePtr type;
    if (node->getDimensionSizesCount() > 0) {
        type = std::make_shared<ArrayType>(ty, -1); // Simplified array type
    } else {
        type = ty;
    }
    
    auto symbol = std::make_shared<Symbol>(node->getIdentifier(), sc, type, Symbol::VARIABLE, curScope->getScopeId() == 1);
    curScope->addSymbol(node->getIdentifier(), symbol);

    symbolRecords[curScope->getScopeId()].push_back({symbol->getName(), getTypeName(symbol)});

    for (size_t i = 0; i < node->getInitialsCount(); ++i) {
        visitExp(node->getInitial(i));
    }
}

void SemanticVisitor::visitFunctionDecl(FunctionDecl* node) {
    if (curScope->existInCurScope(node->getIdentifier())) {
        errorLog(ErrorCode::b, node->getLine());
        return;
    }

    TypePtr retType = convertToType(node->getFuncType());

    // create new scope
    curScope = curScope->pushScope();

    std::vector<TypePtr> paramTypes;
    for (size_t i = 0; i < node->getFormatParamsCount(); ++i) {
        paramTypes.push_back(visitFormatParam(node->getFormatParam(i)));
    }

    // avoid using visitBlockStmt create new scope
    BlockStmt* body = node->getFunctionBody();
    if (body) {
        for (size_t i = 0; i < body->getStmtsCount(); ++i) {
            if (i == body->getStmtsCount() - 1) {
                if (dynamic_cast<ReturnStmt*>(body->getStmt(i)) && retType->isVoidTy()) {
                    errorLog(ErrorCode::f, body->getEndLine());
                } else if (!dynamic_cast<ReturnStmt*>(body->getStmt(i)) && !retType->isVoidTy()) {
                    errorLog(ErrorCode::g, body->getEndLine());
                }
            }
            visitStmt(body->getStmt(i));
        }
    } else {
        // error
        std::cerr << "Error: Function body is not a block statement." << std::endl;
    }

    // pop function scope
    curScope = curScope->popScope();

    // funcType
    auto funcType = std::make_shared<FunctionType>(retType, paramTypes);

    // funcSymbol
    auto symbol = std::make_shared<Symbol>(node->getIdentifier(), StorageClass::NONE, funcType, Symbol::FUNCTION, true);
    curScope->addSymbol(node->getIdentifier(), symbol);
        
    symbolRecords[curScope->getScopeId()].push_back({symbol->getName(), getTypeName(symbol)});

}

void SemanticVisitor::visitMainFunctionDecl(FunctionDecl* node) {
    // create new scope
    curScope = curScope->pushScope();

    BlockStmt* body = node->getFunctionBody();
    if (body) {
        for (size_t i = 0; i < body->getStmtsCount(); ++i) {
            visitStmt(body->getStmt(i));
        }
    } else {
        // error
        std::cerr << "Error: Main function body is not a block statement." << std::endl;
    }

    // pop function scope
    curScope = curScope->popScope();
}

TypePtr SemanticVisitor::visitFormatParam(FormatParam* node) {
    TypePtr type;
    if (node->getDimensionSizesCount() > 0) {
        type = std::make_shared<ArrayType>(convertToType(node->getParamType()), -1);
    } else {
        type = convertToType(node->getParamType());
    }

    auto symbol = std::make_shared<Symbol>(node->getIdentifier(), StorageClass::NONE, type, Symbol::VARIABLE, false);
    curScope->addSymbol(node->getIdentifier(), symbol);

    symbolRecords[curScope->getScopeId()].push_back({symbol->getName(), getTypeName(symbol)});

    return type;
}

void SemanticVisitor::visitBlockStmt(BlockStmt* node) {
    curScope = curScope->pushScope();

    for (size_t i = 0; i < node->getStmtsCount(); ++i) {
        visitStmt(node->getStmt(i));
    }

    curScope = curScope->popScope();
}

TypePtr SemanticVisitor::visitExp(Exp* exp) {
    if (!exp) return nullptr;
    if (auto p = dynamic_cast<IdentifierExp*>(exp)) return visitIdentifierExp(p);
    if (auto p = dynamic_cast<LiteralExp*>(exp)) return visitLiteralExp(p);
    if (auto p = dynamic_cast<FunctionCallExp*>(exp)) return visitFunctionCallExp(p);
    if (auto p = dynamic_cast<SubscriptExp*>(exp)) return visitSubscriptExp(p);
    if (auto p = dynamic_cast<UnaryExp*>(exp)) return visitUnaryExp(p);
    if (auto p = dynamic_cast<BinaryExp*>(exp)) return visitBinaryExp(p);
    if (auto p = dynamic_cast<AssignExp*>(exp)) return visitAssignExp(p);
    throw std::runtime_error("Unknown Expression type in getExpType");
}

TypePtr SemanticVisitor::visitIdentifierExp(IdentifierExp* node) {
    auto symbol = curScope->lookup(node->getIdentifier());
    if (!symbol) {
        
        return Type::getIntegerTy();
    }
    return symbol->getType();
}

TypePtr SemanticVisitor::visitLiteralExp(LiteralExp* node) {
    return Type::getIntegerTy();
}

TypePtr SemanticVisitor::visitFunctionCallExp(FunctionCallExp* node) {
    // 目前只会出现int | void func(int param, int params[])
    auto type = visitExp(node->getFunction());

    // check if is function type
    if (!type->isFunctionTy()) {
        // error
        std::cout << "Error: Calling a non-function variable." << std::endl;
        return Type::getIntegerTy();
    }

    
    auto funcType = std::dynamic_pointer_cast<FunctionType>(type);
    

    // check param count
    if (!funcType->isVariadic() && funcType->getParamsCount() != node->getRealParamsCount()) {
        // errorLog(ErrorCode::d, line);
    }

    // check param types
    if (!funcType->isVariadic()) {
        for (size_t i = 0; i < node->getRealParamsCount(); ++i) {
            TypePtr formalType = funcType->getParamType(i);
            TypePtr realType = visitExp(node->getRealParam(i));
            if (!formalType->isCompatibleWith(realType)) {
                // errorLog(ErrorCode::e, line);
            }
        }
    }

    return funcType->getReturnType();
}
TypePtr SemanticVisitor::visitSubscriptExp(SubscriptExp* node) {
    auto type = visitExp(node->getArray());

    // check if is array type
    if (!type->isArrayTy()) {
        std::cout << "Error: Subscript operator applied to non-array type." << std::endl;
        return Type::getIntegerTy();
    }

    auto arrayType = std::dynamic_pointer_cast<ArrayType>(type);

    // 暂且无视下标类型检查
    visitExp(node->getSubscript());

    return arrayType->getElementType();
}
TypePtr SemanticVisitor::visitUnaryExp(UnaryExp* node) {
    return visitExp(node->getExp());
}
TypePtr SemanticVisitor::visitBinaryExp(BinaryExp* node) {
    // 暂且无视对类型是否可以计算的检查
    visitExp(node->getLhs());
    visitExp(node->getRhs());

    return Type::getIntegerTy();
}
TypePtr SemanticVisitor::visitAssignExp(AssignExp* node) {
    // check for assignment to const
    auto lhsType = visitExp(node->getLhs());

    // 暂且无视对类型是否可以计算的检查
    visitExp(node->getRhs());

    return lhsType;
}
void SemanticVisitor::visitExpStmt(ExpStmt* node) {
    if(node->getExp()) {
        visitExp(node->getExp());
    }
}
void SemanticVisitor::visitForStmt(ForStmt* node) {
    loopDepth++;
    visitExp(node->getInitial());
    visitExp(node->getCondition());
    visitExp(node->getIteration());
    visitStmt(node->getBody());
    loopDepth--;
}
void SemanticVisitor::visitIfStmt(IfStmt* node) {
    visitExp(node->getCondition());
    visitStmt(node->getTrueStmt());
    visitStmt(node->getFalseStmt());
}
void SemanticVisitor::visitReturnStmt(ReturnStmt* node) {
    visitExp(node->getReturnExp());
}

void SemanticVisitor::visitBreakStmt(BreakStmt* node) {
    // m: break outside of loop
    if (loopDepth == 0) {
        errorLog(ErrorCode::m, node->getLine());
    }
}

void SemanticVisitor::visitContinueStmt(ContinueStmt* node) {
    // m: continue outside of loop
    if (loopDepth == 0) {
        errorLog(ErrorCode::m, node->getLine());
    }
}

// Helper function to get the required type name string
std::string SemanticVisitor::getTypeName(SymbolPtr symbol) {
    TypePtr type = symbol->getType();
    if (type->isFunctionTy()) {
        auto funcType = std::dynamic_pointer_cast<FunctionType>(type);
        if (funcType->getReturnType()->isVoidTy()) {
            return "VoidFunc";
        } else {
            return "IntFunc";
        }
    } else if (type->isArrayTy()) {
        auto arrType = std::dynamic_pointer_cast<ArrayType>(type);
        if (arrType->getElementType()->isConst()) {
            return "ConstIntArray";
        } else if (symbol->getStorageClass() == StorageClass::STATIC) {
            return "StaticIntArray";
        } else {
            return "IntArray";
        }
    } else if (type->isIntegerTy()) {
        auto intType = std::dynamic_pointer_cast<IntegerType>(type);
        if (intType->isConst()) {
            return "ConstInt";
        } else if (symbol->getStorageClass() == StorageClass::STATIC) {
            return "StaticInt";
        } else {
            return "Int";
        }
    }
    return "Unknown";
}