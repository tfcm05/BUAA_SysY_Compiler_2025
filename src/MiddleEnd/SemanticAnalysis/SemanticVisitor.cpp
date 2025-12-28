#include "../../../include/MiddleEnd/SemanticAnalysis/SemanticVisitor.h"

// SemanticInfo implementation
SemanticInfo::SemanticInfo()
    : kind(InfoKind::DEFAULT), type(nullptr), lValueFlag(false), constValue(0) {}

SemanticInfo::InfoKind SemanticInfo::getInfoKind() const { return kind; }
void SemanticInfo::setInfoKind(InfoKind k) { kind = k; }

Clang::Type* SemanticInfo::getType() const { return type; }
void SemanticInfo::setType(Clang::Type* ty) { type = ty; }

bool SemanticInfo::isLValue() const { return lValueFlag; }
void SemanticInfo::setLValueFlag(bool lvf) { lValueFlag = lvf; }
int64_t SemanticInfo::getConstValue() const { return constValue; }
void SemanticInfo::setConstValue(int64_t cv) { constValue = cv; }

std::string SemanticInfo::getName() const { return name; }
void SemanticInfo::setName(const std::string& n) { name = n; }

// SemanticContext implementation
SemanticContext::SemanticContext()
    : purpose(VisitPurpose::DEFAULT), type(nullptr), loopFlag(false) {}

SemanticContext::VisitPurpose SemanticContext::getVisitPurpose() const { return purpose; }
void SemanticContext::setVisitPurpose(VisitPurpose vp) { purpose = vp; }

Clang::Type* SemanticContext::getType() const { return type; }
void SemanticContext::setType(Clang::Type* ty) { type = ty; }

bool SemanticContext::inLoop() const { return loopFlag; }
void SemanticContext::setLoopFlag(bool lf) { loopFlag = lf; }

ContextPtr SemanticContext::clone() {
    auto cloned = std::make_shared<SemanticContext>();
    cloned->setVisitPurpose(purpose);
    cloned->setType(type);
    cloned->setLoopFlag(loopFlag);
    return cloned;
}

// SemanticVisitor implementation
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

SemanticVisitor::SemanticVisitor(ClangContext& cc) : clangContext(cc), curScope(std::make_shared<SymbolTable>()) {

    // build-in functions
    {
        auto printfType = clangContext.getFunctionTyImpl(
            clangContext.getIntegerTyImpl(),
            // const char*
            std::vector<Clang::Type*>{ clangContext.getQualifiedTyImpl(clangContext.getPointerTyImpl(clangContext.getCharTyImpl()), 1) },
            true
        );
        auto printfSymbol = std::make_unique<Symbol>("printf", StorageClass::NONE, printfType, Symbol::FUNCTION);
        curScope->addSymbol("printf", printfSymbol.get());
        buildInSymbols.push_back(std::move(printfSymbol));
    }

    {
        auto getintType = clangContext.getFunctionTyImpl(
            clangContext.getIntegerTyImpl(),
            std::vector<Clang::Type*>{},
            false
        );
        auto getintSymbol = std::make_unique<Symbol>("getint", StorageClass::NONE, getintType, Symbol::FUNCTION);
        curScope->addSymbol("getint", getintSymbol.get());
        buildInSymbols.push_back(std::move(getintSymbol));
    }

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

InfoPtr SemanticVisitor::visitCompUnit(CompUnit* node, ContextPtr context) {
    // 全局变量需要计算初始化的常量表达式值
    auto globalVarContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    globalVarContext->setVisitPurpose(SemanticContext::VisitPurpose::EVALUATE_CONST);
    for (size_t i = 0; i < node->getVarDeclsCount(); ++i) {
        node->getVarDecl(i)->accept(*this, globalVarContext);
    }
    
    for (size_t i = 0; i < node->getFuncDeclsCount(); ++i) {
        node->getFuncDecl(i)->accept(*this, context);
    }

    node->getMainFuncDecl()->accept(*this, context);

    return nullptr;
}

InfoPtr SemanticVisitor::visitVariableDecl(VariableDecl* node, ContextPtr context) {
    auto sc = node->getDeclSpec().getStorageClass();

    auto varType = convertToType(node->getDeclSpec().getTypeSpecifier(), node->getDeclSpec().getTypeQualifier());

    auto varContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    varContext->setType(varType);

    for (size_t i = 0; i < node->getDeclaratorsCount(); ++i) {
        std::shared_ptr<SemanticInfo> varInfo;
        if (sc == StorageClass::STATIC) {
            auto staticContext = std::dynamic_pointer_cast<SemanticContext>(varContext->clone());
            staticContext->setVisitPurpose(SemanticContext::VisitPurpose::EVALUATE_CONST);
            varInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator(i)->accept(*this, staticContext));
        } else {
            varInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator(i)->accept(*this, varContext));
        }

        if (curScope->existInCurScope(varInfo->getName())) {
            errorLog(ErrorCode::b, node->getStartLine());
        } else {
            auto symbol = createSymbol(varInfo->getName(), sc, varInfo->getType(), Symbol::VARIABLE);
            // 将Symbol信息存储到Declarator中
            node->getDeclarator(i)->setSymbol(symbol);
        }
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitFunctionDecl(FunctionDecl* node, ContextPtr context) {
    auto sc = node->getDeclSpec().getStorageClass();

    auto retType = convertToType(node->getDeclSpec().getTypeSpecifier(), node->getDeclSpec().getTypeQualifier());
    
    // 处理函数名和类型, 参数不需要创建符号
    auto funcContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    funcContext->setType(retType);
    auto funcInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator()->accept(*this, funcContext));

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
   
    if (curScope->existInCurScope(funcInfo->getName())) {
        errorLog(ErrorCode::b, node->getStartLine());
    } else {
        auto symbol = createSymbol(funcInfo->getName(), sc, funcInfo->getType(), Symbol::FUNCTION);
        // 将Symbol信息存储到Declarator中
        node->getDeclarator()->setSymbol(symbol);
    }

    // 新作用域
    {
        ScopeGuard guard(curScope);

        // 参数, 需要创建符号
        auto paramContext = std::dynamic_pointer_cast<SemanticContext>(funcContext->clone());
        paramContext->setVisitPurpose(SemanticContext::VisitPurpose::GENERATE_SYMBOL);
        node->getDeclarator()->accept(*this, paramContext);

        // 函数体, 不需要再创建新作用域
        auto bodyContext = std::dynamic_pointer_cast<SemanticContext>(funcContext->clone());
        bodyContext->setVisitPurpose(SemanticContext::VisitPurpose::RESOLVE_FUNCTION_BODY_ONLY);
        node->getBody()->accept(*this, bodyContext);
    }
    return nullptr;
}

InfoPtr SemanticVisitor::visitParameterDecl(ParameterDecl* node, ContextPtr context) {
    auto sc = node->getDeclSpec().getStorageClass();

    auto paramType = convertToType(node->getDeclSpec().getTypeSpecifier(), node->getDeclSpec().getTypeQualifier());

    auto paramContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    paramContext->setType(paramType);
    paramContext->setVisitPurpose(SemanticContext::VisitPurpose::DEFAULT);
    auto paramInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator()->accept(*this, paramContext));

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::GENERATE_SYMBOL) {
        if (curScope->existInCurScope(paramInfo->getName())) {
            errorLog(ErrorCode::b, node->getStartLine());
        } else {
            auto symbol = createSymbol(paramInfo->getName(), sc, paramInfo->getType(), Symbol::PARAMETER);
            // 将Symbol信息存储到Declarator中
            node->getDeclarator()->setSymbol(symbol);
        }
    }

    return paramInfo;
}

InfoPtr SemanticVisitor::visitIdentDeclarator(IdentDeclarator* node, ContextPtr context) {
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);

    auto info = std::make_shared<SemanticInfo>();

    info->setName(node->getIdentifier());
    info->setType(semanticContext->getType());

    return info;
}

InfoPtr SemanticVisitor::visitArrayDeclarator(ArrayDeclarator* node, ContextPtr context) {
    auto constContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    constContext->setVisitPurpose(SemanticContext::VisitPurpose::EVALUATE_CONST);

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    Clang::Type* currentType = semanticContext->getType();
    for (size_t i = node->getArraySizesCount(); i > 0; --i) {
        if (i == node->getArraySizesCount()) {
            // 最内层数组
            if (node->getArraySize(i - 1) == nullptr) {
                // T arr[];
                // 实际上是T*
                currentType = clangContext.getPointerTyImpl(currentType);
            } else {
                // T arr[size];
                auto constInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getArraySize(i - 1)->accept(*this, constContext));
                currentType = clangContext.getArrayTyImpl(currentType, constInfo->getConstValue());
            }
        } else {
            // 多维数组
            auto constInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getArraySize(i - 1)->accept(*this, constContext));
            currentType = clangContext.getArrayTyImpl(currentType, constInfo->getConstValue());
        }
    }
    auto arrayContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    arrayContext->setType(currentType);

    return node->getArray()->accept(*this, arrayContext);
}

InfoPtr SemanticVisitor::visitFuncDeclarator(FuncDeclarator* node, ContextPtr context) {
    auto info = std::make_shared<SemanticInfo>();

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    Clang::Type* currentType = semanticContext->getType();
    std::vector<Clang::Type*> paramTypes;
    for (size_t i = 0; i < node->getParametersCount(); ++i) {
        auto paramInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getParameter(i)->accept(*this, context));
        paramTypes.push_back(paramInfo->getType());
    }
    currentType = clangContext.getFunctionTyImpl(currentType, paramTypes, false);
    
    auto funcContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    funcContext->setType(currentType);
    funcContext->setVisitPurpose(SemanticContext::VisitPurpose::DEFAULT);

    return node->getFunction()->accept(*this, funcContext);
}

InfoPtr SemanticVisitor::visitInitialDeclarator(InitialDeclarator* node, ContextPtr context) {
    auto info = node->getDeclarator()->accept(*this, context);

    node->getInitializer()->accept(*this, context);

    return info;
}

InfoPtr SemanticVisitor::visitExpInitializer(ExpInitializer* node, ContextPtr context) {
    auto expInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getExp()->accept(*this, context));

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        node->setConstValue(expInfo->getConstValue());
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitListInitializer(ListInitializer* node, ContextPtr context) {
    for (size_t i = 0; i < node->getInitializersCount(); ++i) {
        node->getInitializer(i)->accept(*this, context);
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitLiteralExp(LiteralExp* node, ContextPtr context) {

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);

    auto info = std::make_shared<SemanticInfo>();
    
    if (node->getLiteralType() == LiteralExp::LiteralType::NUMBER) {
        info->setType(clangContext.getIntegerTyImpl());
        auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
        if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
            info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);
            info->setConstValue(node->getNumberValue());
        }
    } else if (node->getLiteralType() == LiteralExp::LiteralType::STRING) {
        // const char*
        info->setType(clangContext.getQualifiedTyImpl(clangContext.getPointerTyImpl(clangContext.getCharTyImpl()), 1));
    }

    return info;
}

InfoPtr SemanticVisitor::visitIdentifierExp(IdentifierExp* node, ContextPtr context) {
    auto symbol = curScope->lookup(node->getIdentifier());

    if (!symbol) {
        errorLog(ErrorCode::c, node->getStartLine());
        return nullptr;
    }
    // 将Symbol信息存储到AstNode中
    node->setSymbol(symbol);

    auto info = std::make_shared<SemanticInfo>();
    info->setType(symbol->getType());
    info->setLValueFlag(true);

    return info;
}

InfoPtr SemanticVisitor::visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) {
    auto funcInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getFunction()->accept(*this, context));
    
    if (!funcInfo->getType() || !funcInfo->getType()->isFunctionTy()) {
        std::cout << "Error: Calling a non-function type, line: " << node->getStartLine() << std::endl;
        return nullptr;
    }
    auto funcType = static_cast<Clang::FunctionType*>(funcInfo->getType());

    auto info = std::make_shared<SemanticInfo>();
    info->setType(funcType->getReturnType());
    info->setLValueFlag(false);

    if (funcType->isVariadic()) {
        // printf
        auto formatStrExp = dynamic_cast<LiteralExp*>(node->getArgument(0));
        if (!formatStrExp || formatStrExp->getLiteralType() != LiteralExp::LiteralType::STRING) {
            return nullptr;
        }

        std::string formatStr = formatStrExp->getStringValue();
         int formatSpecifierCount = 0;
        for (size_t i = 0; i < formatStr.length(); ++i) {
            if (formatStr[i] == '%' && i + 1 < formatStr.length() && formatStr[i+1] == 'd') {
                formatSpecifierCount++;
                i++; // 跳过 'd'
            }
        }

        int realArgCount = node->getArgumentsCount() - 1;
        if (formatSpecifierCount != realArgCount) {
            errorLog(ErrorCode::l, node->getStartLine());
        }

        for (size_t i = 1; i < node->getArgumentsCount(); ++i) {
            node->getArgument(i)->accept(*this, context);
        }

        return info;
    } 

    if (node->getArgumentsCount() != funcType->getParamTypesCount()) {
        errorLog(ErrorCode::d, node->getStartLine());
        return nullptr;
    } else {
        for (size_t i = 0; i < node->getArgumentsCount(); ++i) {
            auto argInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getArgument(i)->accept(*this, context));
            auto argType = argInfo->getType();
               
            auto paramType = funcType->getParamType(i);
            if (!canAssign(paramType, argType)) {
                errorLog(ErrorCode::e, node->getStartLine());
                return nullptr;
            }
        }
    }
    
    return info;
}

InfoPtr SemanticVisitor::visitSubscriptExp(SubscriptExp* node, ContextPtr context) {
    auto arrayInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getArray()->accept(*this, context));
    if (!arrayInfo->getType() || (!arrayInfo->getType()->isArrayTy() && !arrayInfo->getType()->isPointerTy())) {
        std::cout << "Error: Subscript operator applied to non-array and non-pointer type, line: " << node->getStartLine() << std::endl;
        return nullptr;
    }
    
    auto info = std::make_shared<SemanticInfo>();
    auto resultType = arrayInfo->getType();
    for (size_t i = node->getSubscriptsCount(); i > 0; --i) {
        node->getSubscript(i - 1)->accept(*this, context);
        if (resultType->isArrayTy()) {
            auto arrayType = static_cast<Clang::ArrayType*>(resultType);
            resultType = arrayType->getElementType();
        } else if (resultType->isPointerTy()) {
            auto pointerType = static_cast<Clang::PointerType*>(resultType);
            resultType = pointerType->getPointedType();
        }
    }
    info->setType(resultType);
    info->setLValueFlag(true);

    return info;
}

InfoPtr SemanticVisitor::visitUnaryExp(UnaryExp* node, ContextPtr context) {
    auto expInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getExp()->accept(*this, context));

    auto info = std::make_shared<SemanticInfo>();
    info->setType(expInfo->getType());
    info->setLValueFlag(false);

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);

        int64_t constValue = 0;
        switch (node->getUnaryOp()) {
            case UnaryExp::UnaryOp::PLUS:
                constValue = expInfo->getConstValue();
                break;
            case UnaryExp::UnaryOp::MINU:
                constValue = -expInfo->getConstValue();
                break;
            case UnaryExp::UnaryOp::NOT:
            default:
                constValue = expInfo->getConstValue() == 0 ? 1 : 0;
                break;
        }

        info->setConstValue(constValue);
    }

    return info;
}

InfoPtr SemanticVisitor::visitBinaryExp(BinaryExp* node, ContextPtr context) {
    auto lhsInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getLhs()->accept(*this, context));
    auto rhsInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getRhs()->accept(*this, context));

    auto info = std::make_shared<SemanticInfo>();
    info->setType(clangContext.getIntegerTyImpl());
    info->setLValueFlag(false);

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);

        int64_t constValue;
        int64_t lhsConstValue = lhsInfo->getConstValue();
        int64_t rhsConstValue = rhsInfo->getConstValue();
        switch (node->getBinaryOp()) {
            case BinaryExp::BinaryOp::PLUS:
                constValue = lhsConstValue + rhsConstValue;
                break;
            case BinaryExp::BinaryOp::MINU:
                constValue = lhsConstValue - rhsConstValue;
                break;
            case BinaryExp::BinaryOp::MULT:
                constValue = lhsConstValue * rhsConstValue;
                break;
            case BinaryExp::BinaryOp::DIV:
                constValue = lhsConstValue / rhsConstValue;
                break;
            case BinaryExp::BinaryOp::MOD:
                constValue = lhsConstValue % rhsConstValue;
                break;

            case BinaryExp::BinaryOp::AND:
                constValue = (lhsConstValue && rhsConstValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::OR:
                constValue = (lhsConstValue || rhsConstValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::EQL:
                constValue = (lhsConstValue == rhsConstValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::LEQ:
                constValue = (lhsConstValue <= rhsConstValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::GEQ:
                constValue = (lhsConstValue >= rhsConstValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::LSS:
                constValue = (lhsConstValue < rhsConstValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::GRE:
                constValue = (lhsConstValue > rhsConstValue) ? 1 : 0;
                break;
            default:
                std::cout << "Error: Unknown binary operator in constant expression evaluation, line: " << node->getStartLine() << std::endl;
                return nullptr;
        }
        info->setConstValue(constValue);
    }

    return info;
}

InfoPtr SemanticVisitor::visitAssignExp(AssignExp* node, ContextPtr context) {
    auto lhsInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getLhs()->accept(*this, context));
    auto lhsType = lhsInfo->getType();
    if (!lhsInfo->isLValue()) {
        std::cout << "Error: Left-hand side of assignment is not an l-value, line: " << node->getStartLine() << std::endl;
        return nullptr;
    }
    if (lhsType->isConst()) {
        errorLog(ErrorCode::h, node->getStartLine());
        return nullptr;
    }

    node->getRhs()->accept(*this, context);

    return nullptr;
}

InfoPtr SemanticVisitor::visitCommaExp(CommaExp* node, ContextPtr context) {
    for (size_t i = 0; i < node->getExpsCount(); ++i) {
        node->getExp(i)->accept(*this, context);
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitExpStmt(ExpStmt* node, ContextPtr context) {
    if (node->getExp()) {
        node->getExp()->accept(*this, context);
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitForStmt(ForStmt* node, ContextPtr context) {
    if (node->getInitial()) {
        node->getInitial()->accept(*this, context);
    }
    if (node->getCondition()) {
        node->getCondition()->accept(*this, context);
    }
    if (node->getIteration()) {
        node->getIteration()->accept(*this, context);
    }

    auto cloneContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    cloneContext->setLoopFlag(true);
    node->getBody()->accept(*this, cloneContext);

    return nullptr;
}

InfoPtr SemanticVisitor::visitIfStmt(IfStmt* node, ContextPtr context) {
    node->getCondition()->accept(*this, context);
    node->getTrueStmt()->accept(*this, context);
    if (node->getFalseStmt()) {
        node->getFalseStmt()->accept(*this, context);
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitBreakStmt(BreakStmt* node, ContextPtr context) {
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (!semanticContext->inLoop()) {
        errorLog(ErrorCode::m, node->getStartLine());
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitContinueStmt(ContinueStmt* node, ContextPtr context) {
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (!semanticContext->inLoop()) {
        errorLog(ErrorCode::m, node->getStartLine());
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitReturnStmt(ReturnStmt* node, ContextPtr context) {
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    auto returnType = semanticContext->getType();

    if (node->getReturnExp()) {
        node->getReturnExp()->accept(*this, context);
    } else if (!returnType->isVoidTy()) {
        errorLog(ErrorCode::f, node->getStartLine());
    } 

    return nullptr;
}

InfoPtr SemanticVisitor::visitBlockStmt(BlockStmt* node, ContextPtr context) {
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::RESOLVE_FUNCTION_BODY_ONLY) {
        auto cloneContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
        cloneContext->setVisitPurpose(SemanticContext::VisitPurpose::DEFAULT);

        for (size_t i = 0; i < node->getBlockItemsCount(); ++i) {
            node->getBlockItem(i)->accept(*this, cloneContext);
        }
        
        auto returnType = semanticContext->getType();
        if (!returnType->isVoidTy()) {
            if (node->getBlockItemsCount() == 0) {
                errorLog(ErrorCode::g, node->getEndLine());
            } else {
                auto lastBlockItem = node->getBlockItem(node->getBlockItemsCount() - 1);
                if (dynamic_cast<ReturnStmt*>(lastBlockItem) == nullptr) {
                    errorLog(ErrorCode::g, node->getEndLine());
                }
            }
        }
    } else {
        ScopeGuard guard(curScope);
        for (size_t i = 0; i < node->getBlockItemsCount(); ++i) {
            node->getBlockItem(i)->accept(*this, context);
        }
    }

    return nullptr;
}

Clang::Type* SemanticVisitor::convertToType(TypeSpecifier ts, unsigned int tq) {
    if (ts == TypeSpecifier::INT) {
        if (tq & 1U) {
            return clangContext.getQualifiedTyImpl(clangContext.getIntegerTyImpl(), tq);
        }
        return clangContext.getIntegerTyImpl();
    }
    return clangContext.getVoidTyImpl();
}

bool SemanticVisitor::canAssign(Clang::Type* destType, Clang::Type* srcType) {
    // 0. 快速路径：如果类型完全相同，则一定兼容。
    if (destType == srcType) {
        return true;
    }

    // 防御性编程
    if (!destType || !srcType) {
        return false;
    }

    // 1. 规则：整数类型之间的兼容
    if (destType->isIntegerTy() && srcType->isIntegerTy()) {
        return true;
    }

    // 2. 规则：数组到指针的退化
    if (destType->isPointerTy() && srcType->isArrayTy()) {
        auto destPtr = static_cast<Clang::PointerType*>(destType);
        auto srcArray = static_cast<Clang::ArrayType*>(srcType);
        
        // 递归检查它们的元素类型是否兼容
        return canAssign(destPtr->getPointedType(), srcArray->getElementType());
    }

    return false;
}

Symbol* SemanticVisitor::createSymbol(const std::string& name, StorageClass sc, Clang::Type* type, Symbol::SymbolKind kind) {
    auto symbol = std::make_unique<Symbol>(name, sc, type, kind);
    auto result = symbol.get();

    symbolRecords[curScope->getScopeId()].push_back(std::move(symbol));
    curScope->addSymbol(name, result);

    return result;
}