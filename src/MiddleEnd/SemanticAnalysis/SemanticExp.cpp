#include "MiddleEnd/SemanticAnalysis/SemanticVisitor.h"
#include "MiddleEnd/SemanticAnalysis/ScopeGuard.h"
#include "MiddleEnd/SemanticAnalysis/ConstEval.h"
#include "FrontEnd/SyntaxAnalysis/AstNodes.h"
#include "Error.h"

InfoPtr SemanticVisitor::visitLiteralExp(LiteralExp* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    auto info = std::make_shared<SemanticInfo>();

    if (node->getLiteralType() == LiteralExp::LiteralType::NUMBER) {
        info->setType(clangContext.getIntegerTyImpl());
        if (semanticContext.wantConst()) {
            info->setConstValue(clangContext.getConstIntegerImpl(node->getNumberValue()));
        }
    } else if (node->getLiteralType() == LiteralExp::LiteralType::STRING) {
        // const char* = pointer to const char
        info->setType(clangContext.getPointerTyImpl(
            clangContext.getQualifiedTyImpl(clangContext.getCharTyImpl(), 1)));
    }

    analysisResult.setExpType(node, info->getType());
    if (info->getConstValue()) {
        analysisResult.setExpConst(node, info->getConstValue());
    }
    return info;
}

InfoPtr SemanticVisitor::visitIdentifierExp(IdentifierExp* node, ContextPtr context) {
    auto symbol = curScope->lookup(node->getIdentifier());

    if (!symbol) {
        errorLog(ErrorCode::UndefinedName, node->getStartLine(), node->getIdentifier());
        return nullptr;
    }
    // AnalysisResult is the primary binding; AST setSymbol is dual-write during migration.
    bindSymbol(node, symbol);

    auto info = std::make_shared<SemanticInfo>();
    info->setType(symbol->getType());
    info->setLValueFlag(true);
    analysisResult.setExpType(node, symbol->getType());

    SemanticContext& semanticContext = asSem(context);
    if (semanticContext.wantConst()) {
        info->setConstValue(symbol->getConstValue());
        if (symbol->getConstValue()) {
            analysisResult.setExpConst(node, symbol->getConstValue());
        }
    }

    return info;
}

InfoPtr SemanticVisitor::visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) {
    if (!node->getFunction()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    int errCountBefore = getErrorCount();
    auto funcInfo = asSemInfo(node->getFunction()->accept(*this, context));
    
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
        // 变参函数: 实参个数 >= 形参个数, 多余实参做默认类型提升
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
            auto argInfo = asSemInfo(node->getArgument(i)->accept(*this, context));
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
            auto argInfo = asSemInfo(node->getArgument(i)->accept(*this, context));
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
    auto arrayInfo = asSemInfo(node->getArray()->accept(*this, context));
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

    SemanticContext& semanticContext = asSem(context);
    if (semanticContext.wantConst()) {

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

            auto subscriptInfo = asSemInfo(node->getSubscript(i)->accept(*this, context));
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
    auto expInfo = asSemInfo(node->getExp()->accept(*this, context));
    if (!expInfo) {
        return nullptr;
    }

    auto info = std::make_shared<SemanticInfo>();
    info->setType(expInfo->getType());
    info->setLValueFlag(false);

    SemanticContext& semanticContext = asSem(context);
    if (semanticContext.wantConst()) {
        auto folded = ConstEval::evalUnary(clangContext, node->getUnaryOp(), expInfo->getConstValue());
        if (folded) {
            info->setConstValue(folded);
        }
        // Non-const operand: soft-fail (no error); upper layer decides.
    }

    return info;
}

InfoPtr SemanticVisitor::visitBinaryExp(BinaryExp* node, ContextPtr context) {
    if (!node->getLhs() || !node->getRhs()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto lhsInfo = asSemInfo(node->getLhs()->accept(*this, context));
    if (!lhsInfo) {
        return nullptr;
    }

    auto info = std::make_shared<SemanticInfo>();
    info->setType(clangContext.getIntegerTyImpl());
    info->setLValueFlag(false);

    SemanticContext& semanticContext = asSem(context);
    if (semanticContext.wantConst()) {
        auto lhsConstValue = lhsInfo->getConstValue();
        if (!lhsConstValue || !lhsConstValue->isConstInteger()) {
            return info;
        }
        auto lhsIntValue = static_cast<Clang::ConstInteger*>(lhsConstValue)->getIntValue();

        // Short-circuit: && with 0 / || with non-zero skips RHS.
        if (node->getBinaryOp() == BinaryExp::BinaryOp::AND && lhsIntValue == 0) {
            info->setConstValue(clangContext.getConstIntegerImpl(0));
            return info;
        }
        if (node->getBinaryOp() == BinaryExp::BinaryOp::OR && lhsIntValue != 0) {
            info->setConstValue(clangContext.getConstIntegerImpl(1));
            return info;
        }

        auto rhsInfo = asSemInfo(node->getRhs()->accept(*this, context));
        if (!rhsInfo) {
            return nullptr;
        }
        bool divByZero = false;
        auto folded = ConstEval::evalBinary(clangContext, node->getBinaryOp(),
                                            lhsConstValue, rhsInfo->getConstValue(),
                                            &divByZero);
        if (divByZero) {
            errorLog(ErrorCode::ConstDivByZero, node->getStartLine());
            return nullptr;
        }
        if (folded) {
            info->setConstValue(folded);
        }
    } else {
        node->getRhs()->accept(*this, context);
    }

    return info;
}

InfoPtr SemanticVisitor::visitAssignExp(AssignExp* node, ContextPtr context) {
    if (!node->getLhs() || !node->getRhs()) {
        // 语法错误恢复后可能产生空子节点
        return nullptr;
    }
    auto lhsInfo = asSemInfo(node->getLhs()->accept(*this, context));
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

