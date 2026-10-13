#include "../../../include/MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/EnsureI1.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/ArrayInitIR.h"

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
    IRContext& irContext = asIR(context);
    {
        IRContextGuard guard(irContext);
        irContext.clearModes();
        for (size_t i = 0; i < node->getSubscriptsCount(); ++i) {
            auto indexInfo = asIRInfo(node->getSubscript(i)->accept(*this, context));
            if (!indexInfo) {
                return nullptr;
            }
            indices.push_back(indexInfo->getValue());
        }
    }

    auto countArrayDims = [](LLVM::Type* pointed) {
        size_t arrayDims = 0;
        LLVM::Type* dimType = pointed;
        while (dimType && dimType->isArray()) {
            arrayDims++;
            dimType = static_cast<LLVM::ArrayType*>(dimType)->getElementType();
        }
        return arrayDims;
    };

    if (irContext.useLocation()) {
        auto arrayIRInfo = asIRInfo(node->getArray()->accept(*this, context));
        if (!arrayIRInfo || !arrayIRInfo->getValue()) {
            return nullptr;
        }
        auto ptr = arrayIRInfo->getValue();
        auto ptrType = static_cast<LLVM::PointerType*>(ptr->getType());
        size_t arrayDims = countArrayDims(ptrType->getPointedType());
        if (arrayDims >= 1 && arrayDims >= node->getSubscriptsCount()) {
            indices.insert(indices.begin(), builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0));
        }
        return std::make_shared<IRInfo>(builder.createGEP(ptr, std::move(indices)));
    }

    std::shared_ptr<IRInfo> arrayIRInfo;
    {
        IRContextGuard guard(irContext);
        irContext.setUseLocation(true);
        irContext.setCreateGlobal(false);
        arrayIRInfo = asIRInfo(node->getArray()->accept(*this, context));
    }
    if (!arrayIRInfo || !arrayIRInfo->getValue()) {
        return nullptr;
    }
    auto ptr = arrayIRInfo->getValue();
    auto ptrType = static_cast<LLVM::PointerType*>(ptr->getType());
    size_t arrayDims = countArrayDims(ptrType->getPointedType());
    if (arrayDims >= 1 && arrayDims >= node->getSubscriptsCount()) {
        indices.insert(indices.begin(), builder.createConstInt(llvmContext.getIntegerTyImpl(32), 0));
    }
    auto elementPtr = builder.createGEP(ptr, std::move(indices));
    return std::make_shared<IRInfo>(builder.createLoad(elementPtr));
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
    std::shared_ptr<IRInfo> lhsIRInfo;
    {
        IRContext& ir = asIR(context);
        IRContextGuard guard(ir);
        ir.setUseLocation(true);
        ir.setCreateGlobal(false);
        lhsIRInfo = asIRInfo(node->getLhs()->accept(*this, context));
    }

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

