#include "../../../include/MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/EnsureI1.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/ArrayInitIR.h"

InfoPtr IRVisitor::visitCompUnit(CompUnit* node, ContextPtr context) {
    IRContext& ir = asIR(context);
    for (size_t i = 0; i < node->getDeclCount(); ++i) {
        if (auto* varDecl = dynamic_cast<VariableDecl*>(node->getDecl(i))) {
            IRContextGuard guard(ir);
            ir.setCreateGlobal(true);
            ir.setUseLocation(false);
            varDecl->accept(*this, context);
        } else if (auto* funcDecl = dynamic_cast<FunctionDecl*>(node->getDecl(i))) {
            IRContextGuard guard(ir);
            ir.clearModes();
            funcDecl->accept(*this, context);
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

            std::shared_ptr<IRInfo> initializerInfo;
            {
                IRContextGuard guard(irContext);
                irContext.setCreateGlobal(true);
                irContext.setUseLocation(false);
                irContext.setType(llvmType);
                initializerInfo = asIRInfo(initDeclarator->getInitializer()->accept(*this, context));
            }
            if (!initializerInfo) {
                continue;
            }
            if (auto* constantValue = dynamic_cast<Constant*>(initializerInfo->getValue())) {
                globalVar->setInitializer(constantValue);
            }
        } else {
            // Local automatic: alloca + store
            auto allocaInst = builder.createAlloca(llvmType);
            bindings.set(symbol, allocaInst);

            auto initDeclarator = dynamic_cast<InitialDeclarator*>(declarator);
            if (!initDeclarator || !initDeclarator->getInitializer()) {
                continue;
            }

            IRContextGuard guard(irContext);
            irContext.setValue(allocaInst);
            irContext.setType(llvmType);
            initDeclarator->getInitializer()->accept(*this, context);
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
        std::vector<Constant*> scalars;
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            std::shared_ptr<IRInfo> initializerInfo;
            {
                IRContextGuard guard(irContext);
                irContext.setType(arrayType->getElementType());
                initializerInfo = asIRInfo(node->getInitializer(i)->accept(*this, context));
            }
            if (!initializerInfo) {
                return nullptr;
            }
            auto* constValue = dynamic_cast<Constant*>(initializerInfo->getValue());
            if (!constValue) {
                return nullptr;
            }
            flattenConstArray(constValue, scalars);
        }
        size_t offset = 0;
        auto arrayConst = buildArrayConstantFromScalars(arrayType, scalars, offset, builder, llvmContext);
        return std::make_shared<IRInfo>(arrayConst);
    } else {
        for (size_t i = 0; i < node->getInitializersCount(); ++i) {
            IRContextGuard guard(irContext);
            irContext.setType(arrayType->getElementType());
            irContext.addIndex(i);
            node->getInitializer(i)->accept(*this, context);
        }
        for (size_t i = node->getInitializersCount(); i < arraySize; ++i) {
            storeZeroElement(builder, llvmContext, irContext.getValue(), arrayType->getElementType(), {i});
        }
        return nullptr;
    }
}

