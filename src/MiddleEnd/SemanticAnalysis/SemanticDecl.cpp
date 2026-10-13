#include "MiddleEnd/SemanticAnalysis/SemanticVisitor.h"
#include "MiddleEnd/SemanticAnalysis/ScopeGuard.h"
#include "MiddleEnd/SemanticAnalysis/DeclaratorTypeBuilder.h"
#include "FrontEnd/SyntaxAnalysis/AstNodes.h"
#include "Error.h"

InfoPtr SemanticVisitor::visitCompUnit(CompUnit* node, ContextPtr context) {
    // Single pass in source order; Guard toggles file-scope vs function modes.
    SemanticContext& sem = asSem(context);
    for (size_t i = 0; i < node->getDeclCount(); ++i) {
        if (auto* varDecl = dynamic_cast<VariableDecl*>(node->getDecl(i))) {
            SemanticContextGuard guard(sem);
            sem.setStaticStorageRegion(true);
            sem.clearModes();
            varDecl->accept(*this, context);
        } else if (auto* funcDecl = dynamic_cast<FunctionDecl*>(node->getDecl(i))) {
            SemanticContextGuard guard(sem);
            sem.setStaticStorageRegion(false);
            sem.clearModes();
            funcDecl->accept(*this, context);
        }
    }
    return nullptr;
}

InfoPtr SemanticVisitor::visitVariableDecl(VariableDecl* node, ContextPtr context) {
    auto sc = node->getDeclSpec().getStorageClass();
    SemanticContext& semanticContext = asSem(context);

    auto varType = convertToType(node->getDeclSpec().getTypeSpecifier(),
                                 node->getDeclSpec().getTypeQualifier());
    bool isConstVar = node->getDeclSpec().isConst();
    bool isStaticStorage = semanticContext.isStaticStorageRegion()
                           || (sc == StorageClass::STATIC);

    SemanticContextGuard typeGuard(semanticContext);
    semanticContext.setType(varType);

    for (size_t i = 0; i < node->getDeclaratorsCount(); ++i) {
        bool hasInit = dynamic_cast<InitialDeclarator*>(node->getDeclarator(i)) != nullptr;
        std::shared_ptr<SemanticInfo> varInfo;
        bool needFallback = false;

        if (isStaticStorage || isConstVar) {
            int errCountBefore = getErrorCount();
            {
                SemanticContextGuard constGuard(semanticContext);
                semanticContext.setWantConst(true);
                varInfo = asSemInfo(node->getDeclarator(i)->accept(*this, context));
            }
            if (!varInfo) {
                continue;
            }
            if (hasInit && !varInfo->getConstValue()) {
                if (isStaticStorage) {
                    if (getErrorCount() == errCountBefore) {
                        errorLog(ErrorCode::GlobalStaticNonConstInit, node->getStartLine());
                    }
                    continue;
                }
                needFallback = true;  // local const: re-eval without wantConst
            }
        } else {
            needFallback = true;
        }

        if (needFallback) {
            varInfo = asSemInfo(node->getDeclarator(i)->accept(*this, context));
            if (!varInfo) {
                continue;
            }
        }

        if (curScope->existInCurScope(varInfo->getName())) {
            errorLog(ErrorCode::RedefinedName, node->getStartLine(), varInfo->getName());
        } else {
            auto symbol = createSymbol(varInfo->getName(), sc, varInfo->getType(),
                                       Symbol::VARIABLE, varInfo->getConstValue());
            bindSymbol(node->getDeclarator(i), symbol);
        }
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitFunctionDecl(FunctionDecl* node, ContextPtr context) {
    auto sc = node->getDeclSpec().getStorageClass();
    auto retType = convertToType(node->getDeclSpec().getTypeSpecifier(),
                                 node->getDeclSpec().getTypeQualifier());

    SemanticContext& sem = asSem(context);
    std::shared_ptr<SemanticInfo> funcInfo;
    {
        SemanticContextGuard guard(sem);
        sem.setType(retType);
        sem.clearModes();
        funcInfo = asSemInfo(node->getDeclarator()->accept(*this, context));
    }
    if (!funcInfo) {
        return nullptr;
    }

    auto existingSymbol = curScope->lookup(funcInfo->getName());
    if (existingSymbol && existingSymbol->getKind() == Symbol::FUNCTION) {
        bindSymbol(node->getDeclarator(), existingSymbol);
    } else if (existingSymbol) {
        errorLog(ErrorCode::RedefinedName, node->getStartLine(), funcInfo->getName());
        return nullptr;
    } else {
        auto symbol = createSymbol(funcInfo->getName(), sc, funcInfo->getType(),
                                   Symbol::FUNCTION, nullptr);
        bindSymbol(node->getDeclarator(), symbol);
    }

    if (node->getBody()) {
        ScopeGuard scopeGuard(curScope);

        {
            SemanticContextGuard guard(sem);
            sem.setType(retType);
            sem.clearModes();
            sem.setGenerateParamSymbols(true);
            node->getDeclarator()->accept(*this, context);
        }
        {
            SemanticContextGuard guard(sem);
            sem.setType(retType);
            sem.clearModes();
            sem.setFunctionBodyOnly(true);
            node->getBody()->accept(*this, context);
        }
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitParameterDecl(ParameterDecl* node, ContextPtr context) {
    auto sc = node->getDeclSpec().getStorageClass();
    auto paramType = convertToType(node->getDeclSpec().getTypeSpecifier(),
                                   node->getDeclSpec().getTypeQualifier());

    SemanticContext& semanticContext = asSem(context);
    std::shared_ptr<SemanticInfo> paramInfo;
    {
        SemanticContextGuard guard(semanticContext);
        semanticContext.setType(paramType);
        semanticContext.clearModes();
        paramInfo = asSemInfo(node->getDeclarator()->accept(*this, context));
    }
    if (!paramInfo) {
        return nullptr;
    }

    if (semanticContext.generateParamSymbols()) {
        if (curScope->existInCurScope(paramInfo->getName())) {
            errorLog(ErrorCode::RedefinedName, node->getStartLine(), paramInfo->getName());
        } else {
            auto symbol = createSymbol(paramInfo->getName(), sc, paramInfo->getType(),
                                       Symbol::PARAMETER, nullptr);
            bindSymbol(node->getDeclarator(), symbol);
        }
    }

    return paramInfo;
}

InfoPtr SemanticVisitor::visitIdentDeclarator(IdentDeclarator* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    auto info = std::make_shared<SemanticInfo>();
    info->setName(node->getIdentifier());
    info->setType(semanticContext.getType());
    return info;
}

InfoPtr SemanticVisitor::visitPointerDeclarator(PointerDeclarator* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    auto ptrType = DeclaratorTypeBuilder::wrapPointer(clangContext, semanticContext.getType());
    SemanticContextGuard guard(semanticContext);
    semanticContext.setType(ptrType);
    return node->getPointer()->accept(*this, context);
}

InfoPtr SemanticVisitor::visitArrayDeclarator(ArrayDeclarator* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    Clang::Type* currentType = semanticContext.getType();

    for (size_t i = node->getArraySizesCount(); i > 0; --i) {
        if (node->getArraySize(i - 1) == nullptr) {
            currentType = DeclaratorTypeBuilder::wrapPointer(clangContext, currentType);
            continue;
        }
        std::shared_ptr<SemanticInfo> constInfo;
        {
            SemanticContextGuard guard(semanticContext);
            semanticContext.setWantConst(true);
            constInfo = asSemInfo(node->getArraySize(i - 1)->accept(*this, context));
        }
        if (!constInfo) {
            return nullptr;
        }
        auto constValue = constInfo->getConstValue();
        if (!constValue || !constValue->isConstInteger()) {
            errorLog(ErrorCode::ArraySizeNotConst, node->getStartLine());
            return nullptr;
        }
        auto intValue = static_cast<Clang::ConstInteger*>(constValue)->getIntValue();
        if (intValue <= 0) {
            errorLog(ErrorCode::ArraySizeNotPositive, node->getStartLine());
            return nullptr;
        }
        currentType = DeclaratorTypeBuilder::wrapArray(clangContext, currentType, intValue);
    }

    SemanticContextGuard guard(semanticContext);
    semanticContext.setType(currentType);
    return node->getArray()->accept(*this, context);
}

InfoPtr SemanticVisitor::visitFuncDeclarator(FuncDeclarator* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    Clang::Type* currentType = semanticContext.getType();
    std::vector<Clang::Type*> paramTypes;
    paramTypes.reserve(node->getParametersCount());
    for (size_t i = 0; i < node->getParametersCount(); ++i) {
        auto paramInfo = asSemInfo(node->getParameter(i)->accept(*this, context));
        if (!paramInfo) {
            return nullptr;
        }
        paramTypes.push_back(paramInfo->getType());
    }
    currentType = DeclaratorTypeBuilder::wrapFunction(
        clangContext, currentType, paramTypes, node->isVariadic());

    SemanticContextGuard guard(semanticContext);
    semanticContext.setType(currentType);
    semanticContext.clearModes();
    return node->getFunction()->accept(*this, context);
}

InfoPtr SemanticVisitor::visitInitialDeclarator(InitialDeclarator* node, ContextPtr context) {
    auto info = asSemInfo(node->getDeclarator()->accept(*this, context));
    if (!info) {
        return nullptr;
    }

    SemanticContext& semanticContext = asSem(context);
    if (semanticContext.wantConst()) {
        auto initInfo = asSemInfo(node->getInitializer()->accept(*this, context));
        if (!initInfo) {
            return nullptr;
        }
        auto constValue = initInfo->getConstValue();
        if (constValue) {
            info->setConstValue(constValue);
        }
    } else {
        node->getInitializer()->accept(*this, context);
    }

    return info;
}

InfoPtr SemanticVisitor::visitExpInitializer(ExpInitializer* node, ContextPtr context) {
    auto expInfo = asSemInfo(node->getExp()->accept(*this, context));
    if (!expInfo) {
        return nullptr;
    }

    auto info = std::make_shared<SemanticInfo>();
    SemanticContext& semanticContext = asSem(context);
    if (semanticContext.wantConst()) {
        auto constValue = expInfo->getConstValue();
        if (!constValue || !constValue->isConstInteger()) {
            return info;
        }
        auto intValue = static_cast<Clang::ConstInteger*>(constValue)->getIntValue();
        node->setConstValue(intValue);
        analysisResult.setInitConst(node, intValue);
        info->setConstValue(clangContext.getConstIntegerImpl(intValue));
    }

    return info;
}

InfoPtr SemanticVisitor::visitListInitializer(ListInitializer* node, ContextPtr context) {
    auto info = std::make_shared<SemanticInfo>();
    SemanticContext& semanticContext = asSem(context);
    if (semanticContext.wantConst()) {
        auto constArray = std::vector<Clang::ConstValue*>{};
        constArray.reserve(node->getInitializersCount());
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            auto childInfo = asSemInfo(node->getInitializer(i)->accept(*this, context));
            if (!childInfo) {
                return nullptr;
            }
            auto constValue = childInfo->getConstValue();
            if (!constValue) {
                return info;
            }
            constArray.push_back(constValue);
        }
        info->setConstValue(clangContext.getConstArrayImpl(constArray));
    } else {
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            node->getInitializer(i)->accept(*this, context);
        }
    }

    return info;
}
