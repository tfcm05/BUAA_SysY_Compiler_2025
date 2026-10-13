#include "MiddleEnd/SemanticAnalysis/SemanticVisitor.h"
#include "MiddleEnd/SemanticAnalysis/ScopeGuard.h"
#include "FrontEnd/SyntaxAnalysis/AstNodes.h"
#include "Error.h"
#include <iostream>

// SemanticInfo implementation
SemanticInfo::SemanticInfo()
    : type(nullptr), lValueFlag(false), constValue(nullptr) {}

Clang::Type* SemanticInfo::getType() const { return type; }
void SemanticInfo::setType(Clang::Type* ty) { type = ty; }

bool SemanticInfo::isLValue() const { return lValueFlag; }
void SemanticInfo::setLValueFlag(bool lvf) { lValueFlag = lvf; }
Clang::ConstValue* SemanticInfo::getConstValue() const { return constValue; }
void SemanticInfo::setConstValue(Clang::ConstValue* cv) { constValue = cv; }

std::string SemanticInfo::getName() const { return name; }
void SemanticInfo::setName(const std::string& n) { name = n; }

// SemanticContext implementation — explicit mode flags
SemanticContext::SemanticContext()
    : evaluateConstFlag(false), generateParamSymbolsFlag(false),
      functionBodyOnlyFlag(false), type(nullptr), loopFlag(false),
      staticStorageRegion(false) {}

bool SemanticContext::wantConst() const { return evaluateConstFlag; }
void SemanticContext::setWantConst(bool v) { evaluateConstFlag = v; }
bool SemanticContext::generateParamSymbols() const { return generateParamSymbolsFlag; }
void SemanticContext::setGenerateParamSymbols(bool v) { generateParamSymbolsFlag = v; }
bool SemanticContext::functionBodyOnly() const { return functionBodyOnlyFlag; }
void SemanticContext::setFunctionBodyOnly(bool v) { functionBodyOnlyFlag = v; }

void SemanticContext::clearModes() {
    evaluateConstFlag = false;
    generateParamSymbolsFlag = false;
    functionBodyOnlyFlag = false;
}

Clang::Type* SemanticContext::getType() const { return type; }
void SemanticContext::setType(Clang::Type* ty) { type = ty; }

bool SemanticContext::inLoop() const { return loopFlag; }
void SemanticContext::setLoopFlag(bool lf) { loopFlag = lf; }

bool SemanticContext::isStaticStorageRegion() const { return staticStorageRegion; }
void SemanticContext::setStaticStorageRegion(bool ssr) { staticStorageRegion = ssr; }

ContextPtr SemanticContext::clone() {
    auto cloned = std::make_shared<SemanticContext>();
    cloned->setWantConst(evaluateConstFlag);
    cloned->setGenerateParamSymbols(generateParamSymbolsFlag);
    cloned->setFunctionBodyOnly(functionBodyOnlyFlag);
    cloned->setType(type);
    cloned->setLoopFlag(loopFlag);
    cloned->setStaticStorageRegion(staticStorageRegion);
    return cloned;
}

SemanticVisitor::SemanticVisitor(ClangContext& cc) : clangContext(cc), curScope(std::make_shared<SymbolTable>()) {

    outputFile.open("symbol.txt");
    if (!outputFile.is_open()) {
        std::cout << "Error: Could not open symbol.txt for writing." << std::endl;
    }
}

SemanticVisitor::~SemanticVisitor() {
    if (outputFile.is_open()) {
        for (const auto& pair : symbolRecords) {
            int scopeId = pair.first;
            const auto& symbolsInScope = pair.second;
            for (const auto& symbol : symbolsInScope) {
                outputFile << scopeId << " "
                           << symbol->getName() << " "
                           << symbol->typeToString() << std::endl;
            }
        }
        outputFile.close();
    }
    /*
    printf("ClangContext Type Scan Results:\n");
    clangContext.scan(0);
    printf("pointer Types:\n");
    clangContext.scan(1);
    printf("array Types:\n");
    clangContext.scan(2);
    printf("qualified Types:\n");
    clangContext.scan(3);
    */
}

Clang::Type* SemanticVisitor::convertToType(TypeSpecifier ts, unsigned int tq) {
    Clang::Type* baseType = nullptr;
    if (ts == TypeSpecifier::INT) {
        baseType = clangContext.getIntegerTyImpl();
    } else if (ts == TypeSpecifier::CHAR) {
        baseType = clangContext.getCharTyImpl();
    } else {
        return clangContext.getVoidTyImpl();
    }
    if (tq) {
        return clangContext.getQualifiedTyImpl(baseType, tq);
    }
    return baseType;
}

bool SemanticVisitor::canAssign(Clang::Type* destType, Clang::Type* srcType) {
    // 0. 快速路径：如果类型完全相同，则一定兼容
    if (destType == srcType) {
        return true;
    }
    if (!destType || !srcType) {
        return false;
    }

    // 1. 整数与字符类型兼容
    if ((destType->isIntegerTy() || destType->isCharTy()) &&
        (srcType->isIntegerTy() || srcType->isCharTy())) {
        return true;
    }

    // 2. 数组到指针的退化
    if (destType->isPointerTy() && srcType->isArrayTy()) {
        auto destPtr = static_cast<Clang::PointerType*>(destType);
        auto srcArray = static_cast<Clang::ArrayType*>(srcType);
        return canAssign(destPtr->getPointedType(), srcArray->getElementType());
    }

    // 3. 指针兼容: 去除 const 限定后比较指向类型
    if (destType->isPointerTy() && srcType->isPointerTy()) {
        auto destPtr = static_cast<Clang::PointerType*>(destType);
        auto srcPtr = static_cast<Clang::PointerType*>(srcType);
        return canAssign(destPtr->getPointedType(), srcPtr->getPointedType());
    }

    return false;
}

Symbol* SemanticVisitor::createSymbol(const std::string& name, StorageClass sc, Clang::Type* type, Symbol::SymbolKind kind, Clang::ConstValue* constValue) {
    auto symbol = std::make_unique<Symbol>(name, sc, type, kind, constValue);
    auto result = symbol.get();

    symbolRecords[curScope->getScopeId()].push_back(std::move(symbol));
    curScope->addSymbol(name, result);

    return result;
}

void SemanticVisitor::bindSymbol(AstNode* node, Symbol* symbol) {
    analysisResult.bindSymbol(node, symbol);
    // Dual-write AST for IR paths that still call getSymbol() on Declarator.
    // IdentifierExp dual-write keeps IR resolveSymbol fallback working.
    if (auto* id = dynamic_cast<IdentifierExp*>(node)) {
        id->setSymbol(symbol);
    } else if (auto* decl = dynamic_cast<Declarator*>(node)) {
        decl->setSymbol(symbol);
    }
}

