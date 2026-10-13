#include "MiddleEnd/SemanticAnalysis/ConstEval.h"

namespace ConstEval {

Clang::ConstValue* evalUnary(ClangContext& ctx, UnaryExp::UnaryOp op, Clang::ConstValue* operand) {
    if (!operand || !operand->isConstInteger()) {
        return nullptr;
    }
    auto expIntValue = static_cast<Clang::ConstInteger*>(operand)->getIntValue();
    int64_t constValue = 0;
    switch (op) {
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
    return ctx.getConstIntegerImpl(constValue);
}

Clang::ConstValue* evalBinary(ClangContext& ctx, BinaryExp::BinaryOp op,
                              Clang::ConstValue* lhs, Clang::ConstValue* rhs,
                              bool* divByZero) {
    if (divByZero) {
        *divByZero = false;
    }
    if (!lhs || !lhs->isConstInteger() || !rhs || !rhs->isConstInteger()) {
        return nullptr;
    }
    auto lhsIntValue = static_cast<Clang::ConstInteger*>(lhs)->getIntValue();
    auto rhsIntValue = static_cast<Clang::ConstInteger*>(rhs)->getIntValue();

    if ((op == BinaryExp::BinaryOp::DIV || op == BinaryExp::BinaryOp::MOD) && rhsIntValue == 0) {
        if (divByZero) {
            *divByZero = true;
        }
        return nullptr;
    }

    int64_t constValue = 0;
    switch (op) {
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
            return nullptr;
    }
    return ctx.getConstIntegerImpl(constValue);
}

} // namespace ConstEval
