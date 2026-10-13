#include "../../../include/MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/EnsureI1.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/ArrayInitIR.h"

// IRInfo Implementation
IRInfo::IRInfo(Value* val)
    : value(val) {}

Value* IRInfo::getValue() const { return value; }
void IRInfo::setValue(Value* val) { value = val; }

// IRContext Implementation — explicit mode flags
IRContext::IRContext()
    : createGlobalFlag(false), useLocationFlag(false), addressValue(nullptr),
      continueBB(nullptr), breakBB(nullptr), type(nullptr) {}

bool IRContext::createGlobal() const { return createGlobalFlag; }
void IRContext::setCreateGlobal(bool v) { createGlobalFlag = v; }
bool IRContext::useLocation() const { return useLocationFlag; }
void IRContext::setUseLocation(bool v) { useLocationFlag = v; }
void IRContext::clearModes() {
    createGlobalFlag = false;
    useLocationFlag = false;
}

Value* IRContext::getAddressValue() const { return addressValue; }
void IRContext::setAddressValue(Value* val) { addressValue = val; }

BasicBlock* IRContext::getContinueBB() const { return continueBB; }
void IRContext::setContinueBB(BasicBlock* bb) { continueBB = bb; }
BasicBlock* IRContext::getBreakBB() const { return breakBB; }
void IRContext::setBreakBB(BasicBlock* bb) { breakBB = bb; }

Value* IRContext::getValue() const { return addressValue; }
void IRContext::setValue(Value* val) { addressValue = val; }

LLVM::Type* IRContext::getType() const { return type; }
void IRContext::setType(LLVM::Type* ty) { type = ty; }

size_t IRContext::getIndicesCount() const { return indices.size(); }
size_t IRContext::getIndex(size_t index) const { return indices.at(index); }
void IRContext::addIndex(size_t index) { indices.push_back(index); }
void IRContext::clearIndices() { indices.clear(); }
void IRContext::setIndices(const std::vector<size_t>& idxs) { indices = idxs; }
const std::vector<size_t>& IRContext::getIndices() const { return indices; }

ContextPtr IRContext::clone() {
    auto cloned = std::make_shared<IRContext>();
    cloned->setCreateGlobal(createGlobalFlag);
    cloned->setUseLocation(useLocationFlag);
    cloned->addressValue = addressValue;
    cloned->continueBB = continueBB;
    cloned->breakBB = breakBB;
    cloned->setType(type);
    cloned->indices = indices;
    return cloned;
}

// IRVisitor Implementation
IRVisitor::IRVisitor(Module& mod, IRBuilder& bld, LLVMContext& ctx, const AnalysisResult& ar)
    : module(mod), builder(bld), llvmContext(ctx), analysis(ar) {}

Symbol* IRVisitor::resolveSymbol(AstNode* node) const {
    if (auto* sym = analysis.getSymbol(node)) {
        return sym;
    }
    // Migration fallback: Declarator / IdentifierExp may still carry Symbol*.
    if (auto* id = dynamic_cast<IdentifierExp*>(node)) {
        return id->getSymbol();
    }
    if (auto* decl = dynamic_cast<Declarator*>(node)) {
        return decl->getSymbol();
    }
    return nullptr;
}

void IRVisitor::finalize() {
    // Ensure every function basic block has a terminator before passes / print.
    builder.ensureTerminatorInCurrentFunction();
}

LLVM::Type* IRVisitor::convertType(Clang::Type* clangType) {
    if (!clangType) return llvmContext.getVoidTyImpl();

    if (auto it = typeCache.find(clangType); it != typeCache.end()) {
        return it->second;
    }

    LLVM::Type* llvmType = nullptr;
    switch (clangType->getTypeID()) {
        case Clang::Type::VoidTyID:
            llvmType = llvmContext.getVoidTyImpl();
            break;
        case Clang::Type::CharTyID:
            llvmType = llvmContext.getIntegerTyImpl(8);
            break;
        case Clang::Type::IntegerTyID:
            llvmType = llvmContext.getIntegerTyImpl(32);
            break;
        case Clang::Type::PointerTyID: {
            auto ptrType = static_cast<Clang::PointerType*>(clangType);
            auto elementType = convertType(ptrType->getPointedType());
            llvmType = llvmContext.getPointerTyImpl(elementType);
            break;
        }
        case Clang::Type::ArrayTyID: {
            auto arrayType = static_cast<Clang::ArrayType*>(clangType);
            auto elementType = convertType(arrayType->getElementType());
            llvmType = llvmContext.getArrayTyImpl(elementType, arrayType->getSize());
            break;
        }
        case Clang::Type::FunctionTyID: {
            auto funcType = static_cast<Clang::FunctionType*>(clangType);
            std::vector<LLVM::Type*> params;
            params.reserve(funcType->getParamTypesCount());
            for (size_t i = 0; i < funcType->getParamTypesCount(); ++i) {
                params.push_back(convertType(funcType->getParamType(i)));
            }
            auto retType = convertType(funcType->getReturnType());
            llvmType = llvmContext.getFunctionTyImpl(retType, params, funcType->isVariadic());
            break;
        }
        default:
            llvmType = llvmContext.getVoidTyImpl();
            break;
    }

    typeCache[clangType] = llvmType;
    return llvmType;
}

InfoPtr IRVisitor::visitCompUnit(CompUnit* node, ContextPtr context) {
    // 单遍扫描: 按源码顺序处理
    auto varContext = std::static_pointer_cast<IRContext>(context->clone());
    varContext->setCreateGlobal(true);
    varContext->setUseLocation(false);
    auto funcContext = std::static_pointer_cast<IRContext>(context->clone());
    funcContext->clearModes();

    for (size_t i = 0; i < node->getDeclCount(); ++i) {
        if (auto varDecl = dynamic_cast<VariableDecl*>(node->getDecl(i))) {
            varDecl->accept(*this, varContext);
        } else if (auto funcDecl = dynamic_cast<FunctionDecl*>(node->getDecl(i))) {
            funcDecl->accept(*this, funcContext);
        }
    }

    return nullptr;
}

InfoPtr IRVisitor::visitVariableDecl(VariableDecl* node, ContextPtr context) {
    IRContext& irContext = asIR(context);

    for (size_t i = 0; i < node->getDeclaratorsCount(); ++i) {
        auto declarator = node->getDeclarator(i);
        auto symbol = resolveSymbol(declarator);
        if (!symbol) {
            continue;
        }
        if (symbol->getKind() != Symbol::VARIABLE) {
            continue;
        }
        auto llvmType = convertType(symbol->getType());

        // 全局和 static 都用全局变量实现 (C: static 具有静态存储期), 区别仅在初始化上下文
        bool isGlobalVar = (irContext.createGlobal()) || symbol->isStatic();
        if (isGlobalVar) {
            auto globalVar = builder.createGlobalVariable(llvmType);
            module.addGlobalVariable(globalVar);
            bindings.set(symbol, globalVar);

            auto initDeclarator = dynamic_cast<InitialDeclarator*>(declarator);
            if (!initDeclarator || !initDeclarator->getInitializer()) {
                continue;
            }

            auto initContext = std::static_pointer_cast<IRContext>(context->clone());
            initContext->setCreateGlobal(true);
            initContext->setUseLocation(false); // 全局/static 初始化器需常量求值
            initContext->setType(llvmType);

            auto initializerInfo = asIRInfo(initDeclarator->getInitializer()->accept(*this, initContext));
            if (!initializerInfo) {
                continue;
            }
            if (auto constantValue = dynamic_cast<Constant*>(initializerInfo->getValue())) {
                globalVar->setInitializer(constantValue);
            }
        } else {
            // 局部自动变量: alloca + store
            auto allocaInst = builder.createAlloca(llvmType);
            bindings.set(symbol, allocaInst);

            auto addressContext = std::static_pointer_cast<IRContext>(context->clone());
            addressContext->setValue(allocaInst);
            addressContext->setType(llvmType);

            auto initDeclarator = dynamic_cast<InitialDeclarator*>(declarator);
            if (!initDeclarator || !initDeclarator->getInitializer()) {
                continue;
            }

            initDeclarator->getInitializer()->accept(*this, addressContext);
        }
    }

    return nullptr;
}

InfoPtr IRVisitor::visitFunctionDecl(FunctionDecl* node, ContextPtr context) {
    auto funcSymbol = resolveSymbol(node->getDeclarator());
    if (!funcSymbol) {
        return nullptr;
    }
    if (funcSymbol->getKind() != Symbol::FUNCTION) {
        return nullptr;
    }

    auto llvmFuncType = convertType(funcSymbol->getType());
    auto existingFunction = dynamic_cast<Function*>(bindings.get(funcSymbol));

    if (node->getBody()) {
        // 有函数体: 创建函数定义
        builder.ensureTerminatorInCurrentFunction();

        if (existingFunction) {
            // 前面已有原型声明 (mode 1): 复用已有 Function 对象, 添加入口基本块
            auto entryBB = new BasicBlock(funcSymbol->getName() + ".entry");
            existingFunction->addBasicBlock(entryBB);
            builder.setInsertFunction(existingFunction);
            builder.setInsertPoint(entryBB);
        } else {
            auto function = builder.createFunction(llvmFuncType, funcSymbol->getName(), 0);
            if (funcSymbol->getName() == "main") {
                module.setMainFunction(function);
            } else {
                module.addFunction(function);
            }
            bindings.set(funcSymbol, function);
        }

        // 为形参创建 Argument
        auto funcDeclarator = dynamic_cast<FuncDeclarator*>(node->getDeclarator());
        for (size_t i = 0; i < funcDeclarator->getParametersCount(); ++i) {
            auto paramDecl = funcDeclarator->getParameter(i);
            auto paramSymbol = resolveSymbol(paramDecl->getDeclarator());
            if (!paramSymbol || paramSymbol->getKind() != Symbol::PARAMETER) {
                continue;
            }
            auto arg = builder.createArgument(convertType(paramSymbol->getType()));
            bindings.set(paramSymbol, arg);
        }

        node->getBody()->accept(*this, context);
    } else {
        // 无函数体: 声明函数 (如 int getint();)
        if (!existingFunction) {
            auto function = builder.createFunction(llvmFuncType, funcSymbol->getName(), 1);
            if (funcSymbol->getName() == "main") {
                module.setMainFunction(function);
            } else {
                module.addFunction(function);
            }
            bindings.set(funcSymbol, function);
        }
    }

    return nullptr;
}

InfoPtr IRVisitor::visitParameterDecl(ParameterDecl* node, ContextPtr context) {
    return nullptr;
}

InfoPtr IRVisitor::visitExpInitializer(ExpInitializer* node, ContextPtr context) {
    IRContext& irContext = asIR(context);
    if (irContext.createGlobal()) {
        int64_t initConst = analysis.hasInitConst(node) ? analysis.getInitConst(node) : node->getConstValue();
        auto constExpValue = builder.createConstInt(llvmContext.getIntegerTyImpl(32), initConst);
        return std::make_shared<IRInfo>(constExpValue);
    } else {
        auto expInfo = asIRInfo(node->getExp()->accept(*this, context));
        if (!expInfo) {
            return nullptr;
        }
        auto expValue = expInfo->getValue();

        auto address = irContext.getValue();
        auto indices = irContext.getIndices();

        if (indices.empty()) {
            builder.createStore(expValue, address);
            return nullptr;
        }

        std::vector<Value*> gepIndices;
        gepIndices.push_back(builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0));
        for (size_t index : indices) {
            gepIndices.push_back(builder.createConstInt(llvmContext.getIntegerTyImpl(32), index));
        }
        auto ptr = builder.createGEP(address, std::move(gepIndices));
        builder.createStore(expValue, ptr);
        return nullptr;
    }
}

InfoPtr IRVisitor::visitListInitializer(ListInitializer* node, ContextPtr context) {
    // {}
    IRContext& irContext = asIR(context);
    auto type = irContext.getType();
    LLVM::ArrayType* arrayType = nullptr;
    size_t arraySize = 0;
    if (type->isArray()) {
        arrayType = static_cast<LLVM::ArrayType*>(type);
        arraySize = arrayType->getSize();
    } else {
        return nullptr;
    }
    if (irContext.createGlobal()) {
        auto initializerContext = std::static_pointer_cast<IRContext>(context->clone());
        initializerContext->setType(arrayType->getElementType());
        // 将所有子项展平为标量序列: 统一处理平铺 {1,2,3,4} / 嵌套 {{1,2},{3,4}} / 缺行 {{1,2}}
        std::vector<Constant*> scalars;
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            auto initializerInfo = asIRInfo(node->getInitializer(i)->accept(*this, initializerContext));
            if (!initializerInfo) {
                return nullptr;
            }
            auto constValue = dynamic_cast<Constant*>(initializerInfo->getValue());
            if (!constValue) {
                return nullptr;
            }
            flattenConstArray(constValue, scalars);
        }
        // 按数组形状重建; 不足部分自动补零 (C 语义: 部分初始化)
        size_t offset = 0;
        auto arrayConst = buildArrayConstantFromScalars(arrayType, scalars, offset, builder, llvmContext);
        return std::make_shared<IRInfo>(arrayConst);
    } else {
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            auto initializerContext = std::static_pointer_cast<IRContext>(context->clone());
            initializerContext->setType(arrayType->getElementType());
            initializerContext->addIndex(i);

            node->getInitializer(i)->accept(*this, initializerContext);
        }
        // C 语义: 部分初始化时未列出的元素补零 (元素为数组时递归)
        for (size_t i = node->getInitializersCount(); i < arraySize; ++i) {
            storeZeroElement(builder, llvmContext, irContext.getValue(), arrayType->getElementType(), {i});
        }
        return nullptr;
    }
}

InfoPtr IRVisitor::visitIdentifierExp(IdentifierExp* node, ContextPtr context) {
    auto symbol = resolveSymbol(node);
    if (!symbol) {
        return nullptr;
    }

    auto value = bindings.get(symbol);
    if (!value) {
        // 不再内置任何函数(printf/getint/putint/putstr 均需用户在 SysY 中声明)
        return nullptr;
    }

    if (symbol->getKind() == Symbol::FUNCTION || symbol->getKind() == Symbol::PARAMETER) {
        return std::make_shared<IRInfo>(value);
    } 
    
    IRContext& irContext = asIR(context);
    if (irContext.useLocation()) {
        return std::make_shared<IRInfo>(value);
    } else {
        auto type = static_cast<LLVM::PointerType*>(value->getType())->getPointedType();
        if (type->isArray()) {
            auto gepInst = builder.createGEP(value, {builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0),
                                                  builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0)});
            return std::make_shared<IRInfo>(gepInst);
        }
        auto loadInst = builder.createLoad(value);
        return std::make_shared<IRInfo>(loadInst);
    }
}

InfoPtr IRVisitor::visitLiteralExp(LiteralExp* node, ContextPtr context) {
    switch (node->getLiteralType()) {
        case LiteralExp::LiteralType::NUMBER: {
            auto value = builder.createConstInt(llvmContext.getIntegerTyImpl(32), node->getNumberValue());
            return std::make_shared<IRInfo>(value);
        }
        case LiteralExp::LiteralType::STRING: {
            // 转义已在词法分析阶段处理, 追加字符串终止符
            auto string = node->getStringValue();
            // 最后添加字符串结束符
            string.push_back('\0');
            auto value = builder.createConstString(llvmContext.getIntegerTyImpl(8), string);
            auto gloalConstStr = builder.createGlobalVariable(llvmContext.getArrayTyImpl(llvmContext.getIntegerTyImpl(8), string.size()));
            module.addGlobalConstStr(gloalConstStr);
            gloalConstStr->setInitializer(dynamic_cast<Constant*>(value));

            IRContext& irContext = asIR(context);
            if (irContext.useLocation()) {
                return std::make_shared<IRInfo>(gloalConstStr);
            } else {
                auto gepInst = builder.createGEP(gloalConstStr, {
                    builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0),
                    builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0)
                });
                return std::make_shared<IRInfo>(gepInst);   
            }
        }
        default:
            return nullptr;
    }
}

InfoPtr IRVisitor::visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) {
    if (!node->getFunction()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto functionIRInfo = asIRInfo(node->getFunction()->accept(*this, context));
    if (!functionIRInfo) {
        return nullptr;
    }
    auto value = functionIRInfo->getValue();
    if (!value) {
        return nullptr;
    }
    auto function = dynamic_cast<Function*>(value);
    if (!function) {
        return nullptr;
    }

    auto funcType = static_cast<LLVM::FunctionType*>(function->getType());

    std::vector<Value*> args;
    args.reserve(node->getArgumentsCount());
    for (size_t i = 0; i < node->getArgumentsCount(); ++i) {
        auto argIRInfo = asIRInfo(node->getArgument(i)->accept(*this, context));
        if (!argIRInfo) {
            return nullptr;
        }
        auto arg = argIRInfo->getValue();
        args.push_back(arg);
    }

    auto callInst = builder.createCall(function, std::move(args));
    return std::make_shared<IRInfo>(callInst);
}

InfoPtr IRVisitor::visitSubscriptExp(SubscriptExp* node, ContextPtr context) {
    if (!node->getArray()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    std::vector<Value*> indices;
    indices.reserve(node->getSubscriptsCount() + 1);
    auto subscriptContext = std::static_pointer_cast<IRContext>(context->clone());
    subscriptContext->clearModes();
    for (size_t i = 0; i < node->getSubscriptsCount(); ++i) {
        auto indexInfo = asIRInfo(node->getSubscript(i)->accept(*this, subscriptContext));
        if (!indexInfo) {
            return nullptr;
        }
        auto index = indexInfo->getValue();
        indices.push_back(index);
    }

    IRContext& irContext = asIR(context);
    if (irContext.useLocation()) {
        auto arrayIRInfo = asIRInfo(node->getArray()->accept(*this, context));
        if (!arrayIRInfo) {
            return nullptr;
        }
        auto ptr = arrayIRInfo->getValue();
        if (!ptr) {
            return nullptr;
        }
        auto ptrType = static_cast<LLVM::PointerType*>(ptr->getType());
        // 仅当指针指向完整 N 维数组(数组变量, 维数>=下标数)时, GEP 需额外 0 跳过最外层;
        // 参数退化指针(如 int a[][3] -> [3 x i32]*) 的维数小于下标数, 无需额外 0
        size_t arrayDims = 0;
        LLVM::Type* dimType = ptrType->getPointedType();
        while (dimType && dimType->isArray()) {
            arrayDims++;
            dimType = static_cast<LLVM::ArrayType*>(dimType)->getElementType();
        }
        if (arrayDims >= 1 && arrayDims >= node->getSubscriptsCount()) {
            indices.insert(indices.begin(), builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0));
        }
        auto elementPtr = builder.createGEP(ptr, std::move(indices));
        return std::make_shared<IRInfo>(elementPtr);
    } else {
        auto arrayContext = std::static_pointer_cast<IRContext>(context->clone());
        arrayContext->setUseLocation(true);
        arrayContext->setCreateGlobal(false);
        auto arrayIRInfo = asIRInfo(node->getArray()->accept(*this, arrayContext));
        if (!arrayIRInfo) {
            return nullptr;
        }
        auto ptr = arrayIRInfo->getValue();
        if (!ptr) {
            return nullptr;
        }
        auto ptrType = static_cast<LLVM::PointerType*>(ptr->getType());
        // 仅当指针指向完整 N 维数组(数组变量, 维数>=下标数)时, GEP 需额外 0 跳过最外层;
        // 参数退化指针(如 int a[][3] -> [3 x i32]*) 的维数小于下标数, 无需额外 0
        size_t arrayDims = 0;
        LLVM::Type* dimType = ptrType->getPointedType();
        while (dimType && dimType->isArray()) {
            arrayDims++;
            dimType = static_cast<LLVM::ArrayType*>(dimType)->getElementType();
        }
        if (arrayDims >= 1 && arrayDims >= node->getSubscriptsCount()) {
            indices.insert(indices.begin(), builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0));
        }
        auto elementPtr = builder.createGEP(ptr, std::move(indices));
        auto loadInst = builder.createLoad(elementPtr);
        return std::make_shared<IRInfo>(loadInst);
    }
}

InfoPtr IRVisitor::visitUnaryExp(UnaryExp* node, ContextPtr context) {
    if (!node->getExp()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto expIRInfo = asIRInfo(node->getExp()->accept(*this, context));
    if (!expIRInfo) {
        return nullptr;
    }
    auto expValue = expIRInfo->getValue();

    switch (node->getUnaryOp()) {
        case UnaryExp::UnaryOp::PLUS:
            return std::make_shared<IRInfo>(expValue);
        case UnaryExp::UnaryOp::MINU: {
            auto zeroValue = builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0);
            auto negValue = builder.createSub(zeroValue, expValue);
            return std::make_shared<IRInfo>(negValue);
        }
        case UnaryExp::UnaryOp::NOT: {
            auto boolValue = builder.createIcmp(IcmpInstr::IcmpOp::EQ, expValue, builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0));
            return std::make_shared<IRInfo>(boolValue);
        }
        default:
            return nullptr;
    }
}

InfoPtr IRVisitor::visitBinaryExp(BinaryExp* node, ContextPtr context) {
    /* ensureI1 helper extracted */

    if (!node->getLhs() || !node->getRhs()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto lhsInfo = asIRInfo(node->getLhs()->accept(*this, context));
    if (!lhsInfo) {
        return nullptr;
    }
    auto lhsValue = lhsInfo->getValue();

    switch (node->getBinaryOp()) {
        case BinaryExp::BinaryOp::AND: {
            auto currentBlock = builder.getInsertBlock();
            auto rhsBlock = builder.createBasicBlock();
            auto mergeBlock = builder.createBasicBlock();

            builder.setInsertPoint(currentBlock);
            auto lhsBool = ensureI1(builder, llvmContext, lhsValue);
            builder.createBr(lhsBool, rhsBlock, mergeBlock);

            builder.setInsertPoint(rhsBlock);
            auto rhsInfo = asIRInfo(node->getRhs()->accept(*this, context));
            if (!rhsInfo) {
                // rhs 求值失败, 错误已由子节点记录
                return nullptr;
            }
            auto rhsBool = ensureI1(builder, llvmContext, rhsInfo->getValue());
            auto rhsEndBlock = builder.getInsertBlock();
            builder.createBr(mergeBlock);

            builder.setInsertPoint(mergeBlock);
            std::vector<std::pair<Value*, BasicBlock*>> incomings;
            incomings.emplace_back(builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0), currentBlock);
            incomings.emplace_back(rhsBool, rhsEndBlock);
            auto phi = builder.createPhi(llvmContext.getIntegerTyImpl(1), std::move(incomings));
            return std::make_shared<IRInfo>(phi);
        }
        case BinaryExp::BinaryOp::OR: {
            auto currentBlock = builder.getInsertBlock();
            auto rhsBlock = builder.createBasicBlock();
            auto mergeBlock = builder.createBasicBlock();

            builder.setInsertPoint(currentBlock);
            auto lhsBool = ensureI1(builder, llvmContext, lhsValue);
            builder.createBr(lhsBool, mergeBlock, rhsBlock);

            builder.setInsertPoint(rhsBlock);
            auto rhsInfo = asIRInfo(node->getRhs()->accept(*this, context));
            if (!rhsInfo) {
                // rhs 求值失败, 错误已由子节点记录
                return nullptr;
            }
            auto rhsBool = ensureI1(builder, llvmContext, rhsInfo->getValue());
            auto rhsEndBlock = builder.getInsertBlock();
            builder.createBr(mergeBlock);

            builder.setInsertPoint(mergeBlock);
            std::vector<std::pair<Value*, BasicBlock*>> incomings;
            incomings.emplace_back(builder.createConstInt(llvmContext.getIntegerTyImpl(1), 1), currentBlock);
            incomings.emplace_back(rhsBool, rhsEndBlock);
            auto phi = builder.createPhi(llvmContext.getIntegerTyImpl(1), std::move(incomings));
            return std::make_shared<IRInfo>(phi);
        }
        default:
            break;
    }

    auto rhsInfo = asIRInfo(node->getRhs()->accept(*this, context));
    if (!rhsInfo) {
        return nullptr;
    }
    auto rhsValue = rhsInfo->getValue();

    auto lhsType = lhsValue->getType();
    auto rhsType = rhsValue->getType();
    if (lhsType != rhsType) {
        if (lhsType->isInteger() && rhsType->isInteger()) {
            auto lhsIntType = static_cast<LLVM::IntegerType*>(lhsType);
            auto rhsIntType = static_cast<LLVM::IntegerType*>(rhsType);
            auto targetType = lhsIntType->getBitWidth() >= rhsIntType->getBitWidth() ? lhsType : rhsType;

            if (lhsType != targetType) {
                lhsValue = builder.createZext(targetType, lhsValue);
            }
            if (rhsType != targetType) {
                rhsValue = builder.createZext(targetType, rhsValue);
            }
        } else {
            rhsValue = builder.createZext(lhsType, rhsValue);
        }
    }

    switch (node->getBinaryOp()) {
        case BinaryExp::BinaryOp::PLUS: {
            auto addValue = builder.createAdd(lhsValue, rhsValue);
            return std::make_shared<IRInfo>(addValue);
        }
        case BinaryExp::BinaryOp::MOD: {
            auto divValue = builder.createSdiv(lhsValue, rhsValue);
            auto mulValue = builder.createMul(divValue, rhsValue);
            auto remValue = builder.createSub(lhsValue, mulValue);
            return std::make_shared<IRInfo>(remValue);
        }
        case BinaryExp::BinaryOp::MINU: {
            auto subValue = builder.createSub(lhsValue, rhsValue);
            return std::make_shared<IRInfo>(subValue);
        }
        case BinaryExp::BinaryOp::MULT: {
            auto mulValue = builder.createMul(lhsValue, rhsValue);
            return std::make_shared<IRInfo>(mulValue);
        }
        case BinaryExp::BinaryOp::DIV: {
            auto divValue = builder.createSdiv(lhsValue, rhsValue);
            return std::make_shared<IRInfo>(divValue);
        }
        case BinaryExp::BinaryOp::EQL: {
            auto eqValue = builder.createIcmp(IcmpInstr::IcmpOp::EQ, lhsValue, rhsValue);
            return std::make_shared<IRInfo>(eqValue);
        }
        case BinaryExp::BinaryOp::NEQ: {
            auto neqValue = builder.createIcmp(IcmpInstr::IcmpOp::NE, lhsValue, rhsValue);
            return std::make_shared<IRInfo>(neqValue);
        }
        case BinaryExp::BinaryOp::LSS: {
            auto lssValue = builder.createIcmp(IcmpInstr::IcmpOp::SLT, lhsValue, rhsValue);
            return std::make_shared<IRInfo>(lssValue);
        }
        case BinaryExp::BinaryOp::LEQ: {
            auto leqValue = builder.createIcmp(IcmpInstr::IcmpOp::SLE, lhsValue, rhsValue);
            return std::make_shared<IRInfo>(leqValue);
        }
        case BinaryExp::BinaryOp::GRE: {
            auto gtrValue = builder.createIcmp(IcmpInstr::IcmpOp::SGT, lhsValue, rhsValue);
            return std::make_shared<IRInfo>(gtrValue);
        }
        case BinaryExp::BinaryOp::GEQ: {
            auto geqValue = builder.createIcmp(IcmpInstr::IcmpOp::SGE, lhsValue, rhsValue);
            return std::make_shared<IRInfo>(geqValue);
        }
        default:
            return nullptr;
    }
}

InfoPtr IRVisitor::visitAssignExp(AssignExp* node, ContextPtr context) {
    if (!node->getLhs() || !node->getRhs()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto lhsContext = std::static_pointer_cast<IRContext>(context->clone());
    lhsContext->setUseLocation(true);
    lhsContext->setCreateGlobal(false);
    auto lhsIRInfo = asIRInfo(node->getLhs()->accept(*this, lhsContext));

    auto rhsIRInfo = asIRInfo(node->getRhs()->accept(*this, context));
    if (!lhsIRInfo || !rhsIRInfo) {
        return nullptr;
    }

    auto lhsValue = lhsIRInfo->getValue();
    auto rhsValue = rhsIRInfo->getValue();
    if (!lhsValue || !rhsValue) {
        return nullptr;
    }

    auto lhsType = lhsValue->getType();
    if (!lhsType->isPointer()) {
        return nullptr;
    }
    auto lhsPointeeType = static_cast<LLVM::PointerType*>(lhsType)->getPointedType();

    auto rhsType = rhsValue->getType();
    if (lhsPointeeType != rhsType) {
        rhsValue = builder.createZext(lhsPointeeType, rhsValue);
    }

    builder.createStore(rhsValue, lhsValue);
    return std::make_shared<IRInfo>(rhsValue);
}

InfoPtr IRVisitor::visitCommaExp(CommaExp* node, ContextPtr context) {
    InfoPtr lastInfo;
    for (size_t i = 0; i < node->getExpsCount(); ++i) {
        if (!node->getExp(i)) {
            continue;
        }
        lastInfo = node->getExp(i)->accept(*this, context);
    }
    // 逗号表达式的值为最后一个子表达式的值 (C 语义)
    return lastInfo;
}

InfoPtr IRVisitor::visitExpStmt(ExpStmt* node, ContextPtr context) {
    if (node->getExp()) {
        node->getExp()->accept(*this, context);
    }
    return nullptr;
}

InfoPtr IRVisitor::visitForStmt(ForStmt* node, ContextPtr context) {
    if (node->getInitial()) {
        node->getInitial()->accept(*this, context);
    }

    auto preheader = builder.getInsertBlock();

    auto condBlock = builder.createBasicBlock();
    auto bodyBlock = builder.createBasicBlock();
    auto iterBlock = builder.createBasicBlock();
    auto exitBlock = builder.createBasicBlock();

    builder.setInsertPoint(preheader);
    builder.createBr(condBlock);

    builder.setInsertPoint(condBlock);
    if (node->getCondition()) {
        auto condInfo = asIRInfo(node->getCondition()->accept(*this, context));
        if (!condInfo) {
            return nullptr;
        }
        auto condValue = ensureI1(builder, llvmContext, condInfo->getValue());
        builder.createBr(condValue, bodyBlock, exitBlock);
    } else {
        builder.createBr(bodyBlock);
    }

    builder.setInsertPoint(bodyBlock);
    if (node->getBody()) {
        IRContext& ir = asIR(context);
        IRContextGuard guard(ir);
        ir.setContinueBB(iterBlock);
        ir.setBreakBB(exitBlock);
        node->getBody()->accept(*this, context);
    }
    // body 可能以 return/break 结束 (已含终止指令), 此时不再追加跳转
    if (!builder.getInsertBlock()->isTerminator()) {
        builder.createBr(iterBlock);
    }

    builder.setInsertPoint(iterBlock);
    if (node->getIteration()) {
        node->getIteration()->accept(*this, context);
    }
    builder.createBr(condBlock);

    builder.setInsertPoint(exitBlock);
    return nullptr;
}

InfoPtr IRVisitor::visitWhileStmt(WhileStmt* node, ContextPtr context) {
    auto preheader = builder.getInsertBlock();

    auto condBlock = builder.createBasicBlock();
    auto bodyBlock = builder.createBasicBlock();
    auto exitBlock = builder.createBasicBlock();

    builder.setInsertPoint(preheader);
    builder.createBr(condBlock);

    builder.setInsertPoint(condBlock);
    if (node->getCondition()) {
        auto condInfo = asIRInfo(node->getCondition()->accept(*this, context));
        if (!condInfo) {
            return nullptr;
        }
        auto condValue = ensureI1(builder, llvmContext, condInfo->getValue());
        builder.createBr(condValue, bodyBlock, exitBlock);
    } else {
        builder.createBr(exitBlock);
    }

    builder.setInsertPoint(bodyBlock);
    if (node->getBody()) {
        IRContext& ir = asIR(context);
        IRContextGuard guard(ir);
        ir.setContinueBB(condBlock);
        ir.setBreakBB(exitBlock);
        node->getBody()->accept(*this, context);
    }
    if (!builder.getInsertBlock()->isTerminator()) {
        builder.createBr(condBlock);
    }

    builder.setInsertPoint(exitBlock);
    return nullptr;
}

InfoPtr IRVisitor::visitDoWhileStmt(DoWhileStmt* node, ContextPtr context) {
    auto preheader = builder.getInsertBlock();

    auto bodyBlock = builder.createBasicBlock();
    auto condBlock = builder.createBasicBlock();
    auto exitBlock = builder.createBasicBlock();

    builder.setInsertPoint(preheader);
    builder.createBr(bodyBlock);

    builder.setInsertPoint(bodyBlock);
    if (node->getBody()) {
        IRContext& ir = asIR(context);
        IRContextGuard guard(ir);
        ir.setContinueBB(condBlock);
        ir.setBreakBB(exitBlock);
        node->getBody()->accept(*this, context);
    }
    if (!builder.getInsertBlock()->isTerminator()) {
        builder.createBr(condBlock);
    }

    builder.setInsertPoint(condBlock);
    if (node->getCondition()) {
        auto condInfo = asIRInfo(node->getCondition()->accept(*this, context));
        if (!condInfo) {
            return nullptr;
        }
        auto condValue = ensureI1(builder, llvmContext, condInfo->getValue());
        builder.createBr(condValue, bodyBlock, exitBlock);
    } else {
        builder.createBr(exitBlock);
    }

    builder.setInsertPoint(exitBlock);
    return nullptr;
}

InfoPtr IRVisitor::visitIfStmt(IfStmt* node, ContextPtr context) {
    auto entryBlock = builder.getInsertBlock();
    auto trueBlock = builder.createBasicBlock();
    BasicBlock* falseBlock = nullptr;
    if (node->getFalseStmt()) {
        falseBlock = builder.createBasicBlock();
    }
    auto mergeBlock = builder.createBasicBlock();

    builder.setInsertPoint(entryBlock);
    if (node->getCondition()) {
        auto condInfo = asIRInfo(node->getCondition()->accept(*this, context));
        if (!condInfo) {
            return nullptr;
        }
        auto condValue = ensureI1(builder, llvmContext, condInfo->getValue());
        builder.createBr(condValue, trueBlock, falseBlock ? falseBlock : mergeBlock);
    } else {
        builder.createBr(falseBlock ? falseBlock : mergeBlock);
    }

    builder.setInsertPoint(trueBlock);
    if (node->getTrueStmt()) {
        node->getTrueStmt()->accept(*this, context);
    }
    {
        auto afterTrueBlock = builder.getInsertBlock();
        if (afterTrueBlock && !afterTrueBlock->isTerminator()) {
            builder.createBr(mergeBlock);
        }
    }

    if (falseBlock) {
        builder.setInsertPoint(falseBlock);
        node->getFalseStmt()->accept(*this, context);
        {
            auto afterFalseBlock = builder.getInsertBlock();
            if (afterFalseBlock && !afterFalseBlock->isTerminator()) {
                builder.createBr(mergeBlock);
            }
        }
    }

    builder.setInsertPoint(mergeBlock);
    return nullptr;
}

InfoPtr IRVisitor::visitBreakStmt(BreakStmt* node, ContextPtr context) {
    IRContext& breakContext = asIR(context);
    auto breakTarget = breakContext.getBreakBB();
    if (!breakTarget) {
        return nullptr;
    }
    builder.createBr(breakTarget);

    return nullptr;
}

InfoPtr IRVisitor::visitContinueStmt(ContinueStmt* node, ContextPtr context) {
    IRContext& continueContext = asIR(context);
    auto continueTarget = continueContext.getContinueBB();
    if (!continueTarget) {
        return nullptr;
    }
    builder.createBr(continueTarget);

    return nullptr;
}

InfoPtr IRVisitor::visitReturnStmt(ReturnStmt* node, ContextPtr context) {
    if (!node->getReturnExp()) {
        builder.createRet(nullptr);
        return nullptr;
    }

    auto returnIRInfo = asIRInfo(node->getReturnExp()->accept(*this, context));
    if (!returnIRInfo) {
        return nullptr;
    }
    auto returnValue = returnIRInfo->getValue();
    builder.createRet(returnValue);

    return nullptr;
}

InfoPtr IRVisitor::visitBlockStmt(BlockStmt* node, ContextPtr context) {
    for (size_t i = 0; i < node->getBlockItemsCount(); ++i) {
        node->getBlockItem(i)->accept(*this, context);
    }

    return nullptr;
}


