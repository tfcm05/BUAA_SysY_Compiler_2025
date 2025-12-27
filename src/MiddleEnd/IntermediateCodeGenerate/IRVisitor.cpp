#include "../../../include/MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"

// IRInfo Implementation
IRInfo::IRInfo(Value* val)
    : value(val) {}

Value* IRInfo::getValue() const { return value; }
void IRInfo::setValue(Value* val) { value = val; }

// IRContext Implementation
IRContext::IRContext()
    : purpose(visitPurpose::DEFAULT), type(nullptr) {}

IRContext::visitPurpose IRContext::getVisitPurpose() const { return purpose; }
void IRContext::setVisitPurpose(visitPurpose vp) { purpose = vp; }

Value* IRContext::getValue() const { return values[0]; }
void IRContext::setValue(Value* val) { values[0] = val; }

Value* IRContext::getValue(size_t index) const { return values[index]; }
void IRContext::setValue(size_t index, Value* val) { values[index] = val; }

LLVM::Type* IRContext::getType() const { return type; }
void IRContext::setType(LLVM::Type* ty) { type = ty; }

size_t IRContext::getIndicesCount() const { return indices.size(); }
size_t IRContext::getIndex(size_t index) const { return indices.at(index); }
void IRContext::addIndex(size_t index) { indices.push_back(index); }
const std::vector<size_t>& IRContext::getIndices() const { return indices; }

ContextPtr IRContext::clone() {
    auto cloned = std::make_shared<IRContext>();
    cloned->setVisitPurpose(purpose);
    
    cloned->values[0] = values[0];
    cloned->values[1] = values[1];
    
    cloned->setType(type);
    cloned->indices = indices;
    return cloned;
}

// IRVisitor Implementation
IRVisitor::IRVisitor(Module& mod, IRBuilder& bld, LLVMContext& ctx)
    : module(mod), builder(bld), llvmContext(ctx) {
    {
        auto putintType = llvmContext.getFunctionTyImpl(
            llvmContext.getVoidTyImpl(),
            std::vector<LLVM::Type*>{ llvmContext.getIntegerTyImpl(32) },
            false
        );
        auto putintFunction = builder.createFunction(putintType, "putint", 1);
        functionCache["putint"] = putintFunction;
    }

    {
        auto putStrType = llvmContext.getFunctionTyImpl(
            llvmContext.getVoidTyImpl(),
            std::vector<LLVM::Type*>{ llvmContext.getPointerTyImpl(llvmContext.getIntegerTyImpl(8)) },
            false
        );
        auto putStrFunction = builder.createFunction(putStrType, "putstr", 1);
        functionCache["putstr"] = putStrFunction;
    }
}

IRVisitor::~IRVisitor() {
    IRPrinter printer;
    printer.printModule(module);
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
    auto globalVarContext = std::dynamic_pointer_cast<IRContext>(context->clone());
    globalVarContext->setVisitPurpose(IRContext::CREATE_GLOBAL_VARIABLE);
    for (size_t i = 0; i < node->getVarDeclsCount(); ++i) {
        node->getVarDecl(i)->accept(*this, globalVarContext);
    }

    for (size_t i = 0; i < node->getFuncDeclsCount(); ++i) {
        node->getFuncDecl(i)->accept(*this, context);
    }

    node->getMainFuncDecl()->accept(*this, context);

    return nullptr;
}

InfoPtr IRVisitor::visitVariableDecl(VariableDecl* node, ContextPtr context) {
    for (size_t i = 0; i < node->getDeclaratorsCount(); ++i) {
        auto declarator = node->getDeclarator(i);
        auto symbol = declarator->getSymbol();
        if (!symbol) {
            std::cout << "Error: Missing symbol for variable declarator.\n";
            continue;
        }
        if (symbol->getKind() != Symbol::VARIABLE) {
            std::cout << "Error: Symbol is not of variable kind.\n";
            continue;
        }
        auto llvmType = convertType(symbol->getType());

        auto irContext = std::dynamic_pointer_cast<IRContext>(context);
        if (irContext->getVisitPurpose() == IRContext::CREATE_GLOBAL_VARIABLE) {
            auto globalVar = builder.createGlobalVariable(llvmType);
            module.addGlobalVariable(globalVar);
            symbol->setValue(globalVar);

            auto initDeclarator = dynamic_cast<InitialDeclarator*>(declarator);
            if (!initDeclarator || !initDeclarator->getInitializer()) {
                continue;
            }

            auto initializerContext = std::dynamic_pointer_cast<IRContext>(context->clone());
            initializerContext->setType(llvmType);

            auto initializerInfo = std::dynamic_pointer_cast<IRInfo>(initDeclarator->getInitializer()->accept(*this, initializerContext));
            if (auto constantValue = dynamic_cast<Constant*>(initializerInfo->getValue())) {
                globalVar->setInitializer(constantValue);
            }
        } else {
            if (symbol->isStatic()) {
                auto globalVar = builder.createGlobalVariable(llvmType);
                module.addGlobalVariable(globalVar);
                symbol->setValue(globalVar);

                auto initDeclarator = dynamic_cast<InitialDeclarator*>(declarator);
                if (!initDeclarator || !initDeclarator->getInitializer()) {
                    continue;
                }

                auto staticContext = std::dynamic_pointer_cast<IRContext>(context->clone());
                staticContext->setVisitPurpose(IRContext::CREATE_GLOBAL_VARIABLE);
                staticContext->setType(llvmType);

                auto initializerInfo = std::dynamic_pointer_cast<IRInfo>(initDeclarator->getInitializer()->accept(*this, staticContext));
                if (auto constantValue = dynamic_cast<Constant*>(initializerInfo->getValue())) {
                    globalVar->setInitializer(constantValue);
                }
            } else {
                auto allocaInst = builder.createAlloca(llvmType);
                symbol->setValue(allocaInst);

                auto addressContext = std::dynamic_pointer_cast<IRContext>(context->clone());
                addressContext->setValue(allocaInst);
                addressContext->setType(llvmType);

                auto initDeclarator = dynamic_cast<InitialDeclarator*>(declarator);
                if (!initDeclarator || !initDeclarator->getInitializer()) {
                    continue;
                }

                initDeclarator->getInitializer()->accept(*this, addressContext);
            }
        }
    }

    return nullptr;
}

InfoPtr IRVisitor::visitFunctionDecl(FunctionDecl* node, ContextPtr context) {
    auto funcSymbol = node->getDeclarator()->getSymbol();
    if (!funcSymbol) {
        std::cout << "Error: Missing symbol for function declaration.\n";
        return nullptr;
    }
    if (funcSymbol->getKind() != Symbol::FUNCTION) {
        std::cout << "Error: Symbol is not of function kind.\n";
        return nullptr;
    }
    auto llvmFuncType = convertType(funcSymbol->getType());
    auto function = builder.createFunction(llvmFuncType, funcSymbol->getName(), 0);
    
    if (funcSymbol->getName() == "main") {
        module.setMainFunction(function);
    } else {
        module.addFunction(function);
    }

    funcSymbol->setValue(function);

    auto funcDeclarator = dynamic_cast<FuncDeclarator*>(node->getDeclarator());
    for (size_t i = 0; i < funcDeclarator->getParametersCount(); ++i) {
        auto paramDecl = funcDeclarator->getParameter(i);
        auto paramSymbol = paramDecl->getDeclarator()->getSymbol();
        if (!paramSymbol) {
            std::cout << "Error: Missing symbol for function parameter.\n";
            continue;
        }
        if (paramSymbol->getKind() != Symbol::PARAMETER) {
            std::cout << "Error: Symbol is not of parameter kind.\n";
            continue;
        }
        auto arg = builder.createArgument(convertType(paramSymbol->getType()));
        paramSymbol->setValue(arg);
    }

    node->getBody()->accept(*this, context);

    return nullptr;
}

InfoPtr IRVisitor::visitParameterDecl(ParameterDecl* node, ContextPtr context) {
    return nullptr;
}

InfoPtr IRVisitor::visitExpInitializer(ExpInitializer* node, ContextPtr context) {
    auto irContext = std::dynamic_pointer_cast<IRContext>(context);
    if (irContext->getVisitPurpose() == IRContext::CREATE_GLOBAL_VARIABLE) {
        auto constExpValue = builder.createConstInt(llvmContext.getIntegerTyImpl(32), node->getConstValue());
        return std::make_shared<IRInfo>(constExpValue);
    } else {
        auto expInfo = std::dynamic_pointer_cast<IRInfo>(node->getExp()->accept(*this, context));
        auto expValue = expInfo->getValue();

        auto address = irContext->getValue();
        auto indices = irContext->getIndices();

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
    auto irContext = std::dynamic_pointer_cast<IRContext>(context);
    auto type = irContext->getType();
    LLVM::ArrayType* arrayType = nullptr;
    size_t arraySize = 0;
    if (type->isArray()) {
        arrayType = static_cast<LLVM::ArrayType*>(type);
        arraySize = arrayType->getSize();
    } else {
        std::cout << "Error: ListInitializer used for non-array type in global variable initialization.\n";
        return nullptr;
    }
    if (irContext->getVisitPurpose() == IRContext::CREATE_GLOBAL_VARIABLE) {
        auto initializerContext = std::dynamic_pointer_cast<IRContext>(context->clone());
        initializerContext->setType(arrayType->getElementType());
        std::vector<Constant*> constElements;
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            auto initializerInfo = std::dynamic_pointer_cast<IRInfo>(node->getInitializer(i)->accept(*this, initializerContext));
            constElements.push_back(dynamic_cast<Constant*>(initializerInfo->getValue()));
        }
        for (size_t i = node->getInitializersCount(); i < arraySize; ++i) {
            constElements.push_back(dynamic_cast<Constant*>(builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0)));
        }

        return std::make_shared<IRInfo>(builder.createConstArray(irContext->getType(), std::move(constElements)));
    } else {
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            auto initializerContext = std::dynamic_pointer_cast<IRContext>(context->clone());
            initializerContext->setType(arrayType->getElementType());
            initializerContext->addIndex(i);

            node->getInitializer(i)->accept(*this, initializerContext);
        }
        return nullptr;
    }
}

InfoPtr IRVisitor::visitIdentifierExp(IdentifierExp* node, ContextPtr context) {
    auto symbol = node->getSymbol();
    if (!symbol) {
        std::cout << "Error: IdentifierExp has no associated symbol.\n";
        std::cout << "Line: " << node->getStartLine() << std::endl;
        std::cout << "Identifier: " << node->getIdentifier() << std::endl;
        return nullptr;
    }

    auto value = symbol->getValue();
    if (!value) {
        // builtin functions
        if (symbol->getKind() == Symbol::FUNCTION) {
            if (symbol->getName() == "getint") {
                auto getintType = llvmContext.getFunctionTyImpl(
                    llvmContext.getIntegerTyImpl(32),
                    std::vector<LLVM::Type*>{},
                    false
                );
                auto getintFunc = builder.createFunction(getintType, "getint", 1);
                symbol->setValue(getintFunc);
                value = getintFunc;
                return std::make_shared<IRInfo>(value);
            } else if (symbol->getName() == "printf") {
                auto printintType = llvmContext.getFunctionTyImpl(
                    llvmContext.getIntegerTyImpl(32),
                    std::vector<LLVM::Type*>{ llvmContext.getPointerTyImpl(llvmContext.getIntegerTyImpl(8)) },
                    true
                );
                auto printintFunc = builder.createFunction(printintType, "printf", 1);
                symbol->setValue(printintFunc);
                value = printintFunc;
                return std::make_shared<IRInfo>(value);
            } 
        }
        std::cout << "Error: Symbol has no associated value for IR generation.\n";
        return nullptr;
    }

    if (symbol->getKind() == Symbol::FUNCTION || symbol->getKind() == Symbol::PARAMETER) {
        return std::make_shared<IRInfo>(value);
    } 
    
    auto irContext = std::dynamic_pointer_cast<IRContext>(context);
    if (irContext->getVisitPurpose() == IRContext::USE_LOCATION_VALUE) {
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
            auto string = node->getStringValue();
            // 对字符串进行转义处理
            for (size_t i = 0; i < string.size(); ++i) {
                char c = string[i];
                if (c == '\\') {
                    char cNext = i + 1 < string.size() ? string[i + 1] : '\0';
                    if (cNext == 'n') {
                        string[i] = '\n';
                        string.erase(i + 1, 1);
                    }
                }
            }
            // 最后添加字符串结束符
            string.push_back('\0');
            auto value = builder.createConstString(llvmContext.getIntegerTyImpl(8), string);
            auto gloalConstStr = builder.createGlobalVariable(llvmContext.getArrayTyImpl(llvmContext.getIntegerTyImpl(8), string.size()));
            module.addGlobalConstStr(gloalConstStr);
            gloalConstStr->setInitializer(dynamic_cast<Constant*>(value));

            auto irContext = std::dynamic_pointer_cast<IRContext>(context);
            if (irContext->getVisitPurpose() == IRContext::USE_LOCATION_VALUE) {
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
    auto functionIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getFunction()->accept(*this, context));
    auto value = functionIRInfo->getValue();
    if (!value) {
        std::cout << "Error: FunctionCallExp has no associated value.\n";
        return nullptr;
    }
    auto function = dynamic_cast<Function*>(value);
    if (!function) {
        std::cout << "Error: FunctionCallExp refers to a non-function value.\n";
        return nullptr;
    }

    auto funcType = static_cast<LLVM::FunctionType*>(function->getType());
    if (funcType->isVariadicFunc()) {
        // printf
        auto printfNode = dynamic_cast<IdentifierExp*>(node->getFunction());
        if (!printfNode || printfNode->getSymbol()->getName() != "printf") {
            std::cout << "Error: Only 'printf' variadic function is supported.\n";
            return nullptr;
        }

        auto formatString = dynamic_cast<LiteralExp*>(node->getArgument(0))->getStringValue();
        // 对formatString进行转义处理
        for (size_t i = 0; i < formatString.size(); ++i) {
            char c = formatString[i];
            if (c == '\\') {
                char cNext = i + 1 < formatString.size() ? formatString[i + 1] : '\0';
                if (cNext == 'n') {
                    formatString[i] = '\n';
                    formatString.erase(i + 1, 1);
                }
            }
        }

        // 对其他参数进行处理
        std::vector<Value*> argValues;
        for (size_t i = 1; i < node->getArgumentsCount(); ++i) {
            auto argIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getArgument(i)->accept(*this, context));
            auto argValue = argIRInfo->getValue();
            argValues.push_back(argValue);
        }

        // 对formatString进行分割，创建多个常量字符串
        std::string currentPart;
        for (size_t i = 0; i < formatString.size(); ++i) {
            if (formatString[i] == '%') {
                if (i + 1 < formatString.size() && formatString[i + 1] == 'd') {
                    if (!currentPart.empty()) {
                        currentPart.push_back('\0');
                        auto constStr = builder.createConstString(llvmContext.getIntegerTyImpl(8), currentPart);
                        auto globalConstStr = builder.createGlobalVariable(llvmContext.getArrayTyImpl(llvmContext.getIntegerTyImpl(8), currentPart.size()));
                        globalConstStr->setInitializer(dynamic_cast<Constant*>(constStr));
                        module.addGlobalConstStr(globalConstStr);
                        auto gepInst = builder.createGEP(globalConstStr, {
                            builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0),
                            builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0)
                        });
                        builder.createCall(functionCache["putstr"], { gepInst });
                        currentPart.clear();
                    }
                    ++i; // Skip 'd'
                    // putint
                    builder.createCall(functionCache["putint"], { argValues.front() });
                    argValues.erase(argValues.begin());
                } else {
                    currentPart += formatString[i];
                }
            } else {
                currentPart += formatString[i];
            }
        }
        if (!currentPart.empty()) {
            currentPart.push_back('\0');
            auto constStr = builder.createConstString(llvmContext.getIntegerTyImpl(8), currentPart);
            auto globalConstStr = builder.createGlobalVariable(llvmContext.getArrayTyImpl(llvmContext.getIntegerTyImpl(8), currentPart.size()));
            globalConstStr->setInitializer(dynamic_cast<Constant*>(constStr));
            module.addGlobalConstStr(globalConstStr);
            auto gepInst = builder.createGEP(globalConstStr, {
                builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0),
                builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0)
            });
            builder.createCall(functionCache["putstr"], { gepInst });
            currentPart.clear();
        }

        return nullptr;
    }

    std::vector<Value*> args;
    for (size_t i = 0; i < node->getArgumentsCount(); ++i) {
        auto argIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getArgument(i)->accept(*this, context));
        auto arg = argIRInfo->getValue();
        args.push_back(arg);
    }

    auto callInst = builder.createCall(function, std::move(args));
    return std::make_shared<IRInfo>(callInst);
}

InfoPtr IRVisitor::visitSubscriptExp(SubscriptExp* node, ContextPtr context) {
    std::vector<Value*> indices;
    for (size_t i = 0; i < node->getSubscriptsCount(); ++i) {
        auto indexInfo = std::dynamic_pointer_cast<IRInfo>(node->getSubscript(i)->accept(*this, context));
        auto index = indexInfo->getValue();
        indices.push_back(index);
    }

    auto irContext = std::dynamic_pointer_cast<IRContext>(context);
    if (irContext->getVisitPurpose() == IRContext::USE_LOCATION_VALUE) {
        auto arrayIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getArray()->accept(*this, context));
        auto ptr = arrayIRInfo->getValue();
        if (!ptr) {
            std::cout << "Error: SubscriptExp array has no associated value.\n";
            return nullptr;
        }
        auto ptrType = static_cast<LLVM::PointerType*>(ptr->getType());
        if (ptrType->getPointedType()->isArray()) {
            indices.insert(indices.begin(), builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0));
        }
        auto elementPtr = builder.createGEP(ptr, std::move(indices));
        return std::make_shared<IRInfo>(elementPtr);
    } else {
        auto arrayContext = std::dynamic_pointer_cast<IRContext>(context->clone());
        arrayContext->setVisitPurpose(IRContext::USE_LOCATION_VALUE);
        auto arrayIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getArray()->accept(*this, arrayContext));
        auto ptr = arrayIRInfo->getValue();
        if (!ptr) {
            std::cout << "Error: SubscriptExp array has no associated value.\n";
            return nullptr;
        }
        auto ptrType = static_cast<LLVM::PointerType*>(ptr->getType());
        if (ptrType->getPointedType()->isArray()) {
            indices.insert(indices.begin(), builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0));
        }
        auto elementPtr = builder.createGEP(ptr, std::move(indices));
        auto loadInst = builder.createLoad(elementPtr);
        return std::make_shared<IRInfo>(loadInst);
    }
}

InfoPtr IRVisitor::visitUnaryExp(UnaryExp* node, ContextPtr context) {
    auto expIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getExp()->accept(*this, context));
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
            auto intResult = builder.createZext(llvmContext.getIntegerTyImpl(32), boolValue);
            return std::make_shared<IRInfo>(intResult);
        }
        default:
            return nullptr;
    }
}

InfoPtr IRVisitor::visitBinaryExp(BinaryExp* node, ContextPtr context) {
    auto ensureBoolean = [&](Value* value) -> Value* {
        if (!value) {
            std::cout << "Error: Null value encountered during boolean coercion.\n";
            return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
        }
        if (auto constInt = dynamic_cast<ConstantInt*>(value)) {
            if (constInt->getValue() != 0) {
                return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 1);
            } else {
                return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
            }
        }
        auto valueType = value->getType();
        if (valueType && valueType->isInteger()) {
            auto integerType = static_cast<LLVM::IntegerType*>(valueType);
            if (integerType->getBitWidth() == 1) {
                return value;
            }
            auto zero = builder.createConstInt(llvmContext.getIntegerTyImpl(integerType->getBitWidth()), 0);
            return builder.createIcmp(IcmpInstr::IcmpOp::NE, value, zero);
        }
        std::cout << "Error: Unsupported value type for boolean coercion.\n";
        return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
    };

    auto lhsInfo = std::dynamic_pointer_cast<IRInfo>(node->getLhs()->accept(*this, context));
    auto lhsValue = lhsInfo ? lhsInfo->getValue() : nullptr;

    switch (node->getBinaryOp()) {
        case BinaryExp::BinaryOp::AND: {
            auto currentBlock = builder.getInsertBlock();
            auto rhsBlock = builder.createBasicBlock();
            auto mergeBlock = builder.createBasicBlock();

            builder.setInsertPoint(currentBlock);
            auto lhsBool = ensureBoolean(lhsValue);
            builder.createBr(lhsBool, rhsBlock, mergeBlock);

            builder.setInsertPoint(rhsBlock);
            auto rhsInfo = std::dynamic_pointer_cast<IRInfo>(node->getRhs()->accept(*this, context));
            auto rhsBool = ensureBoolean(rhsInfo ? rhsInfo->getValue() : nullptr);
            builder.createBr(mergeBlock);

            builder.setInsertPoint(mergeBlock);
            std::vector<std::pair<Value*, BasicBlock*>> incomings;
            incomings.emplace_back(builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0), currentBlock);
            incomings.emplace_back(rhsBool, rhsBlock);
            auto phi = builder.createPhi(llvmContext.getIntegerTyImpl(1), std::move(incomings));
            return std::make_shared<IRInfo>(phi);
        }
        case BinaryExp::BinaryOp::OR: {
            auto currentBlock = builder.getInsertBlock();
            auto rhsBlock = builder.createBasicBlock();
            auto mergeBlock = builder.createBasicBlock();

            builder.setInsertPoint(currentBlock);
            auto lhsBool = ensureBoolean(lhsValue);
            builder.createBr(lhsBool, mergeBlock, rhsBlock);

            builder.setInsertPoint(rhsBlock);
            auto rhsInfo = std::dynamic_pointer_cast<IRInfo>(node->getRhs()->accept(*this, context));
            auto rhsBool = ensureBoolean(rhsInfo ? rhsInfo->getValue() : nullptr);
            builder.createBr(mergeBlock);

            builder.setInsertPoint(mergeBlock);
            std::vector<std::pair<Value*, BasicBlock*>> incomings;
            incomings.emplace_back(builder.createConstInt(llvmContext.getIntegerTyImpl(1), 1), currentBlock);
            incomings.emplace_back(rhsBool, rhsBlock);
            auto phi = builder.createPhi(llvmContext.getIntegerTyImpl(1), std::move(incomings));
            return std::make_shared<IRInfo>(phi);
        }
        default:
            break;
    }

    auto rhsInfo = std::dynamic_pointer_cast<IRInfo>(node->getRhs()->accept(*this, context));
    auto rhsValue = rhsInfo ? rhsInfo->getValue() : nullptr;

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
    auto lhsContext = std::dynamic_pointer_cast<IRContext>(context->clone());
    lhsContext->setVisitPurpose(IRContext::USE_LOCATION_VALUE);
    auto lhsIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getLhs()->accept(*this, lhsContext));

    auto rhsIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getRhs()->accept(*this, context));

    auto lhsValue = lhsIRInfo->getValue();
    auto rhsValue = rhsIRInfo->getValue();
    if (!lhsValue || !rhsValue) {
        std::cout << "Error: Invalid LHS or RHS in assignment expression.\n";
        return nullptr;
    }

    builder.createStore(rhsValue, lhsValue);
    return std::make_shared<IRInfo>(rhsValue);
}

InfoPtr IRVisitor::visitCommaExp(CommaExp* node, ContextPtr context) {
    for (size_t i = 0; i < node->getExpsCount(); ++i) {
        node->getExp(i)->accept(*this, context);
    }

    return nullptr;
}

InfoPtr IRVisitor::visitExpStmt(ExpStmt* node, ContextPtr context) {
    node->getExp()->accept(*this, context);
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
        auto ensureBoolean = [&](Value* value) -> Value* {
            if (!value) {
                std::cout << "Error: Null for-loop condition value.\n";
                return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
            }
            if (auto constInt = dynamic_cast<ConstantInt*>(value)) {
                if (constInt->getValue() != 0) {
                    return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 1);
                } else {
                    return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
                }
            }
            auto valueType = value->getType();
            if (valueType && valueType->isInteger()) {
                auto integerType = static_cast<LLVM::IntegerType*>(valueType);
                if (integerType->getBitWidth() == 1) {
                    return value;
                }
                auto zero = builder.createConstInt(llvmContext.getIntegerTyImpl(integerType->getBitWidth()), 0);
                return builder.createIcmp(IcmpInstr::IcmpOp::NE, value, zero);
            }
            std::cout << "Error: Unsupported for-loop condition type.\n";
            return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
        };

        auto condContext = context ? context->clone() : std::make_shared<IRContext>();
        auto condInfo = std::dynamic_pointer_cast<IRInfo>(node->getCondition()->accept(*this, condContext));
        auto condValue = ensureBoolean(condInfo ? condInfo->getValue() : nullptr);
        builder.createBr(condValue, bodyBlock, exitBlock);
    } else {
        builder.createBr(bodyBlock);
    }


    auto bodyContext = std::dynamic_pointer_cast<IRContext>(context->clone());
    bodyContext->setValue(0, iterBlock);
    bodyContext->setValue(1, exitBlock);

    builder.setInsertPoint(bodyBlock);
    if (node->getBody()) {
        node->getBody()->accept(*this, bodyContext);
    }
    builder.createBr(iterBlock);

    builder.setInsertPoint(iterBlock);
    if (node->getIteration()) {
        node->getIteration()->accept(*this, context);
    }
    builder.createBr(condBlock);

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
    auto ensureBoolean = [&](Value* value) -> Value* {
        if (!value) {
            std::cout << "Error: Null if-condition value.\n";
            return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
        }
        auto valueType = value->getType();
        if (valueType && valueType->isInteger()) {
            auto integerType = static_cast<LLVM::IntegerType*>(valueType);
            if (integerType->getBitWidth() == 1) {
                return value;
            }
            auto zero = builder.createConstInt(llvmContext.getIntegerTyImpl(integerType->getBitWidth()), 0);
            return builder.createIcmp(IcmpInstr::IcmpOp::NE, value, zero);
        }
        std::cout << "Error: Unsupported if-condition type.\n";
        return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
    };

    auto condContext = context ? context->clone() : std::make_shared<IRContext>();
    auto condInfo = std::dynamic_pointer_cast<IRInfo>(node->getCondition()->accept(*this, condContext));
    auto condValue = ensureBoolean(condInfo ? condInfo->getValue() : nullptr);
    builder.createBr(condValue, trueBlock, falseBlock ? falseBlock : mergeBlock);

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
    auto breakContext = std::dynamic_pointer_cast<IRContext>(context);
    auto breakTarget = dynamic_cast<BasicBlock*>(breakContext->getValue(1));
    builder.createBr(breakTarget);

    return nullptr;
}

InfoPtr IRVisitor::visitContinueStmt(ContinueStmt* node, ContextPtr context) {
    auto continueContext = std::dynamic_pointer_cast<IRContext>(context);
    auto continueTarget = dynamic_cast<BasicBlock*>(continueContext->getValue(0));
    builder.createBr(continueTarget);

    return nullptr;
}

InfoPtr IRVisitor::visitReturnStmt(ReturnStmt* node, ContextPtr context) {
    if (!node->getReturnExp()) {
        builder.createRet(nullptr);
        return nullptr;
    }

    auto returnIRInfo = std::dynamic_pointer_cast<IRInfo>(node->getReturnExp()->accept(*this, context));
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

InfoPtr IRVisitor::visitIdentDeclarator(IdentDeclarator* node, ContextPtr context) {
    return nullptr;
}
InfoPtr IRVisitor::visitArrayDeclarator(ArrayDeclarator* node, ContextPtr context) {
    return nullptr;
}
InfoPtr IRVisitor::visitFuncDeclarator(FuncDeclarator* node, ContextPtr context) {
    return nullptr;
}
InfoPtr IRVisitor::visitInitialDeclarator(InitialDeclarator* node, ContextPtr context) {
    return nullptr;
}

