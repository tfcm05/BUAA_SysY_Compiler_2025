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
Clang::ConstValue* SemanticInfo::getConstValue() const { return constValue; }
void SemanticInfo::setConstValue(Clang::ConstValue* cv) { constValue = cv; }

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
    // (顺序保持先变量后函数: 函数体可引用全局变量; 全局初始化器若引用未声明函数, 由 visitIdentifierExp 报 c 错误)
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

    // 全局变量: visitCompUnit 传入的 context 为 EVALUATE_CONST
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    bool isGlobal = semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST;

    auto varContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    varContext->setType(varType);

    for (size_t i = 0; i < node->getDeclaratorsCount(); ++i) {
        // 是否有初始化器 (`int x;` 等裸声明符没有, 对应 C 的零初始化)
        bool hasInit = dynamic_cast<InitialDeclarator*>(node->getDeclarator(i)) != nullptr;

        std::shared_ptr<SemanticInfo> varInfo;
        if (isGlobal || sc == StorageClass::STATIC || node->getDeclSpec().isConst()) {
            // 先做常量求值: 全局/static 变量的初始化器必须是常量表达式 (C 语义);
            // const 变量也先尝试常量求值, 以保留 SysY 中 const 变量可用作数组尺寸等编译期常量的能力
            auto constContext = std::dynamic_pointer_cast<SemanticContext>(varContext->clone());
            constContext->setVisitPurpose(SemanticContext::VisitPurpose::EVALUATE_CONST);
            varInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator(i)->accept(*this, constContext));

            if (!varInfo) {
                // 声明失败(如数组尺寸非常量), 错误已记录, 跳过该声明符
                continue;
            }

            if (hasInit && !varInfo->getConstValue()) {
                if (isGlobal || sc == StorageClass::STATIC) {
                    // C 语义: 全局/static 对象必须用常量表达式初始化
                    std::cout << "Error: Initializer of a global/static variable must be a constant expression, line: " << node->getStartLine() << std::endl;
                    continue;
                }
                // 局部 const 变量可用非常量表达式初始化 (C 语义: const 是只读限定符, 不要求编译期常量)
                varInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator(i)->accept(*this, varContext));
                if (!varInfo) {
                    continue;
                }
            }
        } else {
            // 局部普通变量: 普通求值
            varInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator(i)->accept(*this, varContext));
            if (!varInfo) {
                continue;
            }
        }

        if (curScope->existInCurScope(varInfo->getName())) {
            errorLog(ErrorCode::b, node->getStartLine(), varInfo->getName());
        } else {
            auto symbol = createSymbol(varInfo->getName(), sc, varInfo->getType(), Symbol::VARIABLE, varInfo->getConstValue());
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
    if (!funcInfo) {
        return nullptr;
    }

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
   
    if (curScope->existInCurScope(funcInfo->getName())) {
        errorLog(ErrorCode::b, node->getStartLine(), funcInfo->getName());
    } else {
        auto symbol = createSymbol(funcInfo->getName(), sc, funcInfo->getType(), Symbol::FUNCTION, nullptr);
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
        if (node->getBody()) {
            node->getBody()->accept(*this, bodyContext);
        }
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
    if (!paramInfo) {
        return nullptr;
    }

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::GENERATE_SYMBOL) {
        if (curScope->existInCurScope(paramInfo->getName())) {
            errorLog(ErrorCode::b, node->getStartLine(), paramInfo->getName());
        } else {
            auto symbol = createSymbol(paramInfo->getName(), sc, paramInfo->getType(), Symbol::PARAMETER, nullptr);
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
                if (!constInfo) {
                    return nullptr;
                }
                auto constValue = constInfo->getConstValue();
                if (!constValue || !constValue->isConstInteger()) {
                    std::cout << "Error: Array size is not a constant integer, line: " << node->getStartLine() << std::endl;
                    return nullptr;
                }
                auto intValue = static_cast<Clang::ConstInteger*>(constValue)->getIntValue();
                if (intValue <= 0) {
                    std::cout << "Error: Array size must be positive, line: " << node->getStartLine() << std::endl;
                    return nullptr;
                }
                currentType = clangContext.getArrayTyImpl(currentType, intValue);
            }
        } else {
            // 多维数组
            if (node->getArraySize(i - 1) == nullptr) {
                // 多维数组最外层尺寸缺省(如 int a[][3]) -> 退化为指针 T (*)[..]
                currentType = clangContext.getPointerTyImpl(currentType);
                continue;
            }
            auto constInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getArraySize(i - 1)->accept(*this, constContext));
            if (!constInfo) {
                return nullptr;
            }
            auto constValue = constInfo->getConstValue();
            if (!constValue || !constValue->isConstInteger()) {
                std::cout << "Error: Array size is not a constant integer, line: " << node->getStartLine() << std::endl;
                return nullptr;
            }
            auto intValue = static_cast<Clang::ConstInteger*>(constValue)->getIntValue();
            if (intValue <= 0) {
                std::cout << "Error: Array size must be positive, line: " << node->getStartLine() << std::endl;
                return nullptr;
            }
            currentType = clangContext.getArrayTyImpl(currentType, intValue);
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
        if (!paramInfo) {
            return nullptr;
        }
        paramTypes.push_back(paramInfo->getType());
    }
    currentType = clangContext.getFunctionTyImpl(currentType, paramTypes, false);
    
    auto funcContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    funcContext->setType(currentType);
    funcContext->setVisitPurpose(SemanticContext::VisitPurpose::DEFAULT);

    return node->getFunction()->accept(*this, funcContext);
}

InfoPtr SemanticVisitor::visitInitialDeclarator(InitialDeclarator* node, ContextPtr context) {
    auto info = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator()->accept(*this, context));
    if (!info) {
        return nullptr;
    }

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);
        auto initInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getInitializer()->accept(*this, context));
        if (!initInfo) {
            return nullptr;
        }
        // 非常量初始化器: info 不带 constValue 返回, 由 visitVariableDecl 决定报错(全局/static)或回退普通求值(局部 const)
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
    auto expInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getExp()->accept(*this, context));
    if (!expInfo) {
        return nullptr;
    }

    auto info = std::make_shared<SemanticInfo>();

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);
        
        auto constValue = expInfo->getConstValue();
        if (!constValue || !constValue->isConstInteger()) {
            // 非常量初始化表达式: 不报错也不静默归零, 交由上层决定 (全局/static 报错, 局部回退普通求值)
            return info;
        }
        auto intValue = static_cast<Clang::ConstInteger*>(constValue)->getIntValue();
        node->setConstValue(intValue);
        info->setConstValue(clangContext.getConstIntegerImpl(intValue));
    }

    return info;
}

InfoPtr SemanticVisitor::visitListInitializer(ListInitializer* node, ContextPtr context) {
    auto info = std::make_shared<SemanticInfo>();

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);

        auto constArray = std::vector<Clang::ConstValue*>{};
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            auto childInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getInitializer(i)->accept(*this, context));
            if (!childInfo) {
                return nullptr;
            }
            auto constValue = childInfo->getConstValue();
            if (!constValue) {
                // 数组初始化器中含非常量元素: 整个列表不是常量, 交由上层决定
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

InfoPtr SemanticVisitor::visitLiteralExp(LiteralExp* node, ContextPtr context) {

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);

    auto info = std::make_shared<SemanticInfo>();
    
    if (node->getLiteralType() == LiteralExp::LiteralType::NUMBER) {
        info->setType(clangContext.getIntegerTyImpl());
        auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
        if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
            info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);
            info->setConstValue(clangContext.getConstIntegerImpl(node->getNumberValue()));
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
        errorLog(ErrorCode::c, node->getStartLine(), node->getIdentifier());
        return nullptr;
    }
    // 将Symbol信息存储到AstNode中
    node->setSymbol(symbol);

    auto info = std::make_shared<SemanticInfo>();
    info->setType(symbol->getType());
    info->setLValueFlag(true);

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);
        info->setConstValue(symbol->getConstValue());
    }

    return info;
}

InfoPtr SemanticVisitor::visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) {
    if (!node->getFunction()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto funcInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getFunction()->accept(*this, context));
    
    if (!funcInfo || !funcInfo->getType() || !funcInfo->getType()->isFunctionTy()) {
        std::cout << "Error: Calling a non-function type, line: " << node->getStartLine() << std::endl;
        return nullptr;
    }
    auto funcType = static_cast<Clang::FunctionType*>(funcInfo->getType());

    auto info = std::make_shared<SemanticInfo>();
    info->setType(funcType->getReturnType());
    info->setLValueFlag(false);

    if (funcType->isVariadic()) {
        // 编译器不再内置 printf 等变参函数; SysY 不支持变参函数
        std::cout << "Error: Variadic functions are not supported (no builtin printf).\n";
        return nullptr;
    } 

    if (node->getArgumentsCount() != funcType->getParamTypesCount()) {
        std::string funcName;
        if (auto idExp = dynamic_cast<IdentifierExp*>(node->getFunction())) {
            funcName = idExp->getIdentifier();
        }
        errorLog(ErrorCode::d, node->getStartLine(), funcName);
        return nullptr;
    } else {
        for (size_t i = 0; i < node->getArgumentsCount(); ++i) {
            auto argInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getArgument(i)->accept(*this, context));
            if (!argInfo) {
                return nullptr;
            }
            auto argType = argInfo->getType();
               
            auto paramType = funcType->getParamType(i);
            if (!canAssign(paramType, argType)) {
                std::string funcName;
                if (auto idExp = dynamic_cast<IdentifierExp*>(node->getFunction())) {
                    funcName = idExp->getIdentifier();
                }
                errorLog(ErrorCode::e, node->getStartLine(), funcName);
                return nullptr;
            }
        }
    }
    
    return info;
}

InfoPtr SemanticVisitor::visitSubscriptExp(SubscriptExp* node, ContextPtr context) {
    if (!node->getArray()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto arrayInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getArray()->accept(*this, context));
    if (!arrayInfo || !arrayInfo->getType() || (!arrayInfo->getType()->isArrayTy() && !arrayInfo->getType()->isPointerTy())) {
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

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);
        
        auto constValue = arrayInfo->getConstValue();
        for (size_t i = 0; i < node->getSubscriptsCount(); ++i) {
            if (!constValue) {
                info->setConstValue(clangContext.getConstIntegerImpl(0));
                return info;
            }
            if (!constValue->isConstArray()) {
                std::cout << "Error: Subscripted array is not a constant array, line: " << node->getStartLine() << std::endl;
                return nullptr;
            }
            auto constArray = static_cast<Clang::ConstArray*>(constValue);

            auto subscriptInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getSubscript(i)->accept(*this, context));
            auto subscriptConstValue = subscriptInfo->getConstValue();
            if (!subscriptConstValue || !subscriptConstValue->isConstInteger()) {
                std::cout << "Error: Subscript is not a constant integer, line: " << node->getStartLine() << std::endl;
                return nullptr;
            }
            auto subscriptIntValue = static_cast<Clang::ConstInteger*>(subscriptConstValue)->getIntValue();
            if (subscriptIntValue < 0 || static_cast<size_t>(subscriptIntValue) >= constArray->getElementsSize()) {
                info->setConstValue(clangContext.getConstIntegerImpl(0));
                return info;
            }

            constValue = constArray->getElement(subscriptIntValue);
        }
        if (!constValue) {
            info->setConstValue(nullptr);
        }
        info->setConstValue(constValue);
    }

    return info;
}

InfoPtr SemanticVisitor::visitUnaryExp(UnaryExp* node, ContextPtr context) {
    if (!node->getExp()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto expInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getExp()->accept(*this, context));
    if (!expInfo) {
        return nullptr;
    }

    auto info = std::make_shared<SemanticInfo>();
    info->setType(expInfo->getType());
    info->setLValueFlag(false);

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);

        auto expConstValue = expInfo->getConstValue();
        if (!expConstValue || !expConstValue->isConstInteger()) {
            // 非常量操作数: 不报错, 仅表示该表达式不是常量 (由上层决定报错或回退普通求值)
            return info;
        }
        auto expIntValue = static_cast<Clang::ConstInteger*>(expConstValue)->getIntValue();
        int64_t constValue = 0;
        switch (node->getUnaryOp()) {
            case UnaryExp::UnaryOp::PLUS:
                constValue = expIntValue;
                break;
            case UnaryExp::UnaryOp::MINU:
                constValue = -expIntValue;
                break;
            case UnaryExp::UnaryOp::NOT:
            default:
                constValue = expIntValue == 0 ? 1 : 0;
                break;
        }

        info->setConstValue(clangContext.getConstIntegerImpl(constValue));
    }

    return info;
}

InfoPtr SemanticVisitor::visitBinaryExp(BinaryExp* node, ContextPtr context) {
    if (!node->getLhs() || !node->getRhs()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto lhsInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getLhs()->accept(*this, context));
    auto rhsInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getRhs()->accept(*this, context));

    auto info = std::make_shared<SemanticInfo>();
    info->setType(clangContext.getIntegerTyImpl());
    info->setLValueFlag(false);

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        if (!lhsInfo || !rhsInfo) {
            return nullptr;
        }
        info->setInfoKind(SemanticInfo::InfoKind::CONST_EXP_INFO);

        auto lhsConstValue = lhsInfo->getConstValue();
        if (!lhsConstValue || !lhsConstValue->isConstInteger()) {
            // 非常量操作数: 不报错, 仅表示该表达式不是常量 (由上层决定报错或回退普通求值)
            return info;
        }
        auto lhsIntValue = static_cast<Clang::ConstInteger*>(lhsConstValue)->getIntValue();
        auto rhsConstValue = rhsInfo->getConstValue();
        if (!rhsConstValue || !rhsConstValue->isConstInteger()) {
            return info;
        }
        auto rhsIntValue = static_cast<Clang::ConstInteger*>(rhsConstValue)->getIntValue();
        int64_t constValue;
        switch (node->getBinaryOp()) {
            case BinaryExp::BinaryOp::PLUS:
                constValue = lhsIntValue + rhsIntValue;
                break;
            case BinaryExp::BinaryOp::MINU:
                constValue = lhsIntValue - rhsIntValue;
                break;
            case BinaryExp::BinaryOp::MULT:
                constValue = lhsIntValue * rhsIntValue;
                break;
            case BinaryExp::BinaryOp::DIV:
                constValue = lhsIntValue / rhsIntValue;
                break;
            case BinaryExp::BinaryOp::MOD:
                constValue = lhsIntValue % rhsIntValue;
                break;

            case BinaryExp::BinaryOp::AND:
                constValue = (lhsIntValue && rhsIntValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::OR:
                constValue = (lhsIntValue || rhsIntValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::EQL:
                constValue = (lhsIntValue == rhsIntValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::LEQ:
                constValue = (lhsIntValue <= rhsIntValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::GEQ:
                constValue = (lhsIntValue >= rhsIntValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::LSS:
                constValue = (lhsIntValue < rhsIntValue) ? 1 : 0;
                break;
            case BinaryExp::BinaryOp::GRE:
                constValue = (lhsIntValue > rhsIntValue) ? 1 : 0;
                break;
            default:
                std::cout << "Error: Unknown binary operator in constant expression evaluation, line: " << node->getStartLine() << std::endl;
                return nullptr;
        }
        info->setConstValue(clangContext.getConstIntegerImpl(constValue));
    }

    return info;
}

InfoPtr SemanticVisitor::visitAssignExp(AssignExp* node, ContextPtr context) {
    if (!node->getLhs() || !node->getRhs()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto lhsInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getLhs()->accept(*this, context));
    if (!lhsInfo) {
        return nullptr;
    }
    auto lhsType = lhsInfo->getType();
    if (!lhsInfo->isLValue()) {
        std::cout << "Error: Left-hand side of assignment is not an l-value, line: " << node->getStartLine() << std::endl;
        return nullptr;
    }
    if (lhsType->isConst()) {
        std::string name;
        if (auto idExp = dynamic_cast<IdentifierExp*>(node->getLhs())) {
            name = idExp->getIdentifier();
        }
        errorLog(ErrorCode::h, node->getStartLine(), name);
        return nullptr;
    }

    node->getRhs()->accept(*this, context);

    return nullptr;
}

InfoPtr SemanticVisitor::visitCommaExp(CommaExp* node, ContextPtr context) {
    for (size_t i = 0; i < node->getExpsCount(); ++i) {
        if (!node->getExp(i)) {
            continue;
        }
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
    // for 头声明的变量作用域覆盖整个 for 语句 (C/SysY 语义)
    ScopeGuard guard(curScope);
    if (node->hasInitialDecl()) {
        node->getInitialDecl()->accept(*this, context);
    } else if (node->getInitial()) {
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

InfoPtr SemanticVisitor::visitWhileStmt(WhileStmt* node, ContextPtr context) {
    if (node->getCondition()) {
        node->getCondition()->accept(*this, context);
    }

    auto cloneContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    cloneContext->setLoopFlag(true);
    node->getBody()->accept(*this, cloneContext);

    return nullptr;
}

InfoPtr SemanticVisitor::visitDoWhileStmt(DoWhileStmt* node, ContextPtr context) {
    if (node->getCondition()) {
        node->getCondition()->accept(*this, context);
    }

    auto cloneContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    cloneContext->setLoopFlag(true);
    node->getBody()->accept(*this, cloneContext);

    return nullptr;
}

InfoPtr SemanticVisitor::visitIfStmt(IfStmt* node, ContextPtr context) {
    if (node->getCondition()) {
        node->getCondition()->accept(*this, context);
    }
    if (node->getTrueStmt()) {
        node->getTrueStmt()->accept(*this, context);
    }
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

Symbol* SemanticVisitor::createSymbol(const std::string& name, StorageClass sc, Clang::Type* type, Symbol::SymbolKind kind, Clang::ConstValue* constValue) {
    auto symbol = std::make_unique<Symbol>(name, sc, type, kind, constValue);
    auto result = symbol.get();

    symbolRecords[curScope->getScopeId()].push_back(std::move(symbol));
    curScope->addSymbol(name, result);

    return result;
}