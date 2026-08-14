#include "../../../include/MiddleEnd/SemanticAnalysis/SemanticVisitor.h"

// SemanticInfo implementation
SemanticInfo::SemanticInfo()
    : type(nullptr), lValueFlag(false), constValue(0) {}

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
    bool isConstVar = node->getDeclSpec().isConst();

    // C 存储期判定: 全局变量 (visitCompUnit 传入 EVALUATE_CONST) 和 static 局部变量具有静态存储期
    // 静态存储期对象的初始化器必须是常量表达式 (C 标准)
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    bool isStaticStorage = (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST)
                           || (sc == StorageClass::STATIC);

    auto varContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    varContext->setType(varType);

    for (size_t i = 0; i < node->getDeclaratorsCount(); ++i) {
        bool hasInit = dynamic_cast<InitialDeclarator*>(node->getDeclarator(i)) != nullptr;
        std::shared_ptr<SemanticInfo> varInfo;

        // 静态存储期 (全局/static) 或 const 变量: 先尝试常量求值
        //   全局/static: 非常量初始化器 → 报错 (C 语义)
        //   局部 const: 非常量初始化器 → 回退普通求值 (C: const 是只读限定符, 非编译期常量)
        if (isStaticStorage || isConstVar) {
            auto constContext = std::dynamic_pointer_cast<SemanticContext>(varContext->clone());
            constContext->setVisitPurpose(SemanticContext::VisitPurpose::EVALUATE_CONST);
            int errCountBefore = getErrorCount();
            varInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator(i)->accept(*this, constContext));

            if (!varInfo) {
                continue;  // 声明失败 (如数组尺寸非常量), 错误已记录
            }

            if (hasInit && !varInfo->getConstValue()) {
                if (isStaticStorage) {
                    // 仅当初始化器本身无错误时才报 (避免与未定义名字等重复报错)
                    if (getErrorCount() == errCountBefore) {
                        errorLog(ErrorCode::GlobalStaticNonConstInit, node->getStartLine());
                    }
                    continue;
                }
                // 局部 const: 回退普通求值
                varInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator(i)->accept(*this, varContext));
                if (!varInfo) {
                    continue;
                }
            }
        } else {
            // 局部自动变量: 普通求值
            varInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getDeclarator(i)->accept(*this, varContext));
            if (!varInfo) {
                continue;
            }
        }

        if (curScope->existInCurScope(varInfo->getName())) {
            errorLog(ErrorCode::RedefinedName, node->getStartLine(), varInfo->getName());
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
        errorLog(ErrorCode::RedefinedName, node->getStartLine(), funcInfo->getName());
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
            errorLog(ErrorCode::RedefinedName, node->getStartLine(), paramInfo->getName());
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

InfoPtr SemanticVisitor::visitPointerDeclarator(PointerDeclarator* node, ContextPtr context) {
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    Clang::Type* currentType = semanticContext->getType();

    // 将当前类型包装为指针类型 (如 char* = pointer to char)
    auto ptrType = clangContext.getPointerTyImpl(currentType);

    auto ptrContext = std::dynamic_pointer_cast<SemanticContext>(context->clone());
    ptrContext->setType(ptrType);

    return node->getPointer()->accept(*this, ptrContext);
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
                    errorLog(ErrorCode::ArraySizeNotConst, node->getStartLine());
                    return nullptr;
                }
                auto intValue = static_cast<Clang::ConstInteger*>(constValue)->getIntValue();
                if (intValue <= 0) {
                    errorLog(ErrorCode::ArraySizeNotPositive, node->getStartLine());
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
                errorLog(ErrorCode::ArraySizeNotConst, node->getStartLine());
                return nullptr;
            }
            auto intValue = static_cast<Clang::ConstInteger*>(constValue)->getIntValue();
            if (intValue <= 0) {
                errorLog(ErrorCode::ArraySizeNotPositive, node->getStartLine());
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
    currentType = clangContext.getFunctionTyImpl(currentType, paramTypes, node->isVariadic());
    
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
            info->setConstValue(clangContext.getConstIntegerImpl(node->getNumberValue()));
        }
    } else if (node->getLiteralType() == LiteralExp::LiteralType::STRING) {
        // const char* = pointer to const char (C: 字符串字面量类型)
        info->setType(clangContext.getPointerTyImpl(
            clangContext.getQualifiedTyImpl(clangContext.getCharTyImpl(), 1)));
    }

    return info;
}

InfoPtr SemanticVisitor::visitIdentifierExp(IdentifierExp* node, ContextPtr context) {
    auto symbol = curScope->lookup(node->getIdentifier());

    if (!symbol) {
        errorLog(ErrorCode::UndefinedName, node->getStartLine(), node->getIdentifier());
        return nullptr;
    }
    // 将Symbol信息存储到AstNode中
    node->setSymbol(symbol);

    auto info = std::make_shared<SemanticInfo>();
    info->setType(symbol->getType());
    info->setLValueFlag(true);

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {
        info->setConstValue(symbol->getConstValue());
    }

    return info;
}

InfoPtr SemanticVisitor::visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) {
    if (!node->getFunction()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    int errCountBefore = getErrorCount();
    auto funcInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getFunction()->accept(*this, context));
    
    if (!funcInfo || !funcInfo->getType() || !funcInfo->getType()->isFunctionTy()) {
        // 仅当函数表达式本身无错误时才报 (避免与未定义名字重复报错)
        if (getErrorCount() == errCountBefore) {
            errorLog(ErrorCode::CallNonFunction, node->getStartLine());
        }
        return nullptr;
    }
    auto funcType = static_cast<Clang::FunctionType*>(funcInfo->getType());

    auto info = std::make_shared<SemanticInfo>();
    info->setType(funcType->getReturnType());
    info->setLValueFlag(false);

    if (funcType->isVariadic()) {
        // 变参函数 (如 printf): 实参个数 >= 形参个数, 多余实参做默认类型提升
        if (node->getArgumentsCount() < funcType->getParamTypesCount()) {
            std::string funcName;
            if (auto idExp = dynamic_cast<IdentifierExp*>(node->getFunction())) {
                funcName = idExp->getIdentifier();
            }
            errorLog(ErrorCode::ParamCountMismatch, node->getStartLine(), funcName);
            return nullptr;
        }
        // 检查固定参数的类型匹配
        for (size_t i = 0; i < funcType->getParamTypesCount(); ++i) {
            auto argInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getArgument(i)->accept(*this, context));
            if (!argInfo) {
                return nullptr;
            }
            if (!canAssign(funcType->getParamType(i), argInfo->getType())) {
                std::string funcName;
                if (auto idExp = dynamic_cast<IdentifierExp*>(node->getFunction())) {
                    funcName = idExp->getIdentifier();
                }
                errorLog(ErrorCode::ParamTypeMismatch, node->getStartLine(), funcName);
                return nullptr;
            }
        }
        // 检查剩余变参实参的合法性 (expr 必须成功求值)
        for (size_t i = funcType->getParamTypesCount(); i < node->getArgumentsCount(); ++i) {
            if (!node->getArgument(i)->accept(*this, context)) {
                return nullptr;
            }
        }
        return info;
    }

    if (node->getArgumentsCount() != funcType->getParamTypesCount()) {
        std::string funcName;
        if (auto idExp = dynamic_cast<IdentifierExp*>(node->getFunction())) {
            funcName = idExp->getIdentifier();
        }
        errorLog(ErrorCode::ParamCountMismatch, node->getStartLine(), funcName);
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
                errorLog(ErrorCode::ParamTypeMismatch, node->getStartLine(), funcName);
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
        errorLog(ErrorCode::SubscriptNonArray, node->getStartLine());
        return nullptr;
    }
    
    auto info = std::make_shared<SemanticInfo>();
    auto resultType = arrayInfo->getType();
    for (size_t i = node->getSubscriptsCount(); i > 0; --i) {
        if (!node->getSubscript(i - 1)) {
            // 语法错误恢复后可能产生空子节点
            continue;
        }
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

        auto constValue = arrayInfo->getConstValue();
        for (size_t i = 0; i < node->getSubscriptsCount(); ++i) {
            if (!node->getSubscript(i)) {
                return nullptr;
            }
            if (!constValue) {
                // 非 const 数组(参数/局部/无常量值): 该下标表达式不是常量, 软失败交由上层决定
                return info;
            }
            if (!constValue->isConstArray()) {
                return info;
            }
            auto constArray = static_cast<Clang::ConstArray*>(constValue);

            auto subscriptInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getSubscript(i)->accept(*this, context));
            if (!subscriptInfo) {
                // 下标表达式求值失败, 错误已由子节点记录
                return nullptr;
            }
            auto subscriptConstValue = subscriptInfo->getConstValue();
            if (!subscriptConstValue || !subscriptConstValue->isConstInteger()) {
                // 非常量下标: 软失败
                return info;
            }
            auto subscriptIntValue = static_cast<Clang::ConstInteger*>(subscriptConstValue)->getIntValue();
            if (subscriptIntValue < 0 || static_cast<size_t>(subscriptIntValue) >= constArray->getElementsSize()) {
                // 越界下标: 软失败, 交由上层决定 (全局/static 报"必须常量", 局部回退普通求值)
                return info;
            }

            constValue = constArray->getElement(subscriptIntValue);
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
    if (!lhsInfo) {
        return nullptr;
    }

    auto info = std::make_shared<SemanticInfo>();
    info->setType(clangContext.getIntegerTyImpl());
    info->setLValueFlag(false);

    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (semanticContext->getVisitPurpose() == SemanticContext::VisitPurpose::EVALUATE_CONST) {

        auto lhsConstValue = lhsInfo->getConstValue();
        if (!lhsConstValue || !lhsConstValue->isConstInteger()) {
            // 非常量操作数: 不报错, 仅表示该表达式不是常量 (由上层决定报错或回退普通求值)
            return info;
        }
        auto lhsIntValue = static_cast<Clang::ConstInteger*>(lhsConstValue)->getIntValue();

        // 短路求值: && 左侧为 0 / || 左侧非 0 时无需求右侧 (C 语义, 也避免 0 && (1/0) 除零)
        if (node->getBinaryOp() == BinaryExp::BinaryOp::AND && lhsIntValue == 0) {
            info->setConstValue(clangContext.getConstIntegerImpl(0));
            return info;
        }
        if (node->getBinaryOp() == BinaryExp::BinaryOp::OR && lhsIntValue != 0) {
            info->setConstValue(clangContext.getConstIntegerImpl(1));
            return info;
        }

        auto rhsInfo = std::dynamic_pointer_cast<SemanticInfo>(node->getRhs()->accept(*this, context));
        if (!rhsInfo) {
            return nullptr;
        }
        auto rhsConstValue = rhsInfo->getConstValue();
        if (!rhsConstValue || !rhsConstValue->isConstInteger()) {
            // 非常量操作数: 软失败
            return info;
        }
        auto rhsIntValue = static_cast<Clang::ConstInteger*>(rhsConstValue)->getIntValue();

        // 编译期可检出的除零/模零 (C 语义: 除零为 UB, 编译器报错而非崩溃)
        if ((node->getBinaryOp() == BinaryExp::BinaryOp::DIV || node->getBinaryOp() == BinaryExp::BinaryOp::MOD) && rhsIntValue == 0) {
            errorLog(ErrorCode::ConstDivByZero, node->getStartLine());
            return nullptr;
        }

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
            case BinaryExp::BinaryOp::NEQ:
                constValue = (lhsIntValue != rhsIntValue) ? 1 : 0;
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
                return info;
        }
        info->setConstValue(clangContext.getConstIntegerImpl(constValue));
    } else {
        // 非常量上下文: 右侧仍需语义检查
        node->getRhs()->accept(*this, context);
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
        errorLog(ErrorCode::AssignmentToNonLValue, node->getStartLine());
        return nullptr;
    }
    if (lhsType->isConst()) {
        std::string name;
        if (auto idExp = dynamic_cast<IdentifierExp*>(node->getLhs())) {
            name = idExp->getIdentifier();
        }
        errorLog(ErrorCode::ConstAssignment, node->getStartLine(), name);
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
    if (node->getBody()) {
        node->getBody()->accept(*this, cloneContext);
    }

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
        errorLog(ErrorCode::BreakContinueOutsideLoop, node->getStartLine());
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitContinueStmt(ContinueStmt* node, ContextPtr context) {
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    if (!semanticContext->inLoop()) {
        errorLog(ErrorCode::BreakContinueOutsideLoop, node->getStartLine());
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitReturnStmt(ReturnStmt* node, ContextPtr context) {
    auto semanticContext = std::dynamic_pointer_cast<SemanticContext>(context);
    auto returnType = semanticContext->getType();

    if (node->getReturnExp()) {
        node->getReturnExp()->accept(*this, context);
        // C 语义: void 函数不得 return 表达式
        if (returnType->isVoidTy()) {
            errorLog(ErrorCode::ReturnTypeMismatch, node->getStartLine());
        }
    } else if (!returnType->isVoidTy()) {
        errorLog(ErrorCode::ReturnTypeMismatch, node->getStartLine());
    } 

    return nullptr;
}

// 判断语句是否保证以 return 结束 (缺 return 分析, 对应 C 的 always-return 判定):
//   return 语句 → true; if 带 else 且两分支都保证 → true; 块语句最后一条保证 → true; 其余 false
static bool alwaysReturns(AstNode* node) {
    if (auto retStmt = dynamic_cast<ReturnStmt*>(node)) {
        (void)retStmt;
        return true;
    }
    if (auto ifStmt = dynamic_cast<IfStmt*>(node)) {
        if (!ifStmt->getFalseStmt() || !ifStmt->getTrueStmt()) {
            return false;   // 无 else: 条件为假时可能不执行
        }
        return alwaysReturns(ifStmt->getTrueStmt()) && alwaysReturns(ifStmt->getFalseStmt());
    }
    if (auto blockStmt = dynamic_cast<BlockStmt*>(node)) {
        // 遍历块中所有语句: 任何一条保证返回即可 (return 可在块中间, 后续为 unreachable)
        for (size_t i = 0; i < blockStmt->getBlockItemsCount(); ++i) {
            if (alwaysReturns(blockStmt->getBlockItem(i))) {
                return true;
            }
        }
        return false;
    }
    return false;
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
            // 函数体必须保证以 return 结束: 遍历所有语句, 任何一条保证返回即可
            if (!alwaysReturns(node)) {
                errorLog(ErrorCode::MissingReturn, node->getEndLine());
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

    // 1. 整数与字符类型兼容 (C: char 是小整数, int ← char 隐式转换)
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

    // 3. 指针兼容: 去除 const 限定后比较指向类型 (C: const char* ← char*, char* ← void* 等)
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