#ifndef CONST_EVAL_H
#define CONST_EVAL_H

#include "../../FrontEnd/SyntaxAnalysis/AstNode.h"
#include "ConstValue.h"
#include "ClangContext.h"

// Compile-time integer folding for unary/binary ops (wantConst paths).
namespace ConstEval {

// Returns folded ConstInteger, or nullptr if operand is not a const integer.
Clang::ConstValue* evalUnary(ClangContext& ctx, UnaryExp::UnaryOp op, Clang::ConstValue* operand);

// Returns folded ConstInteger, or nullptr if either side is not const integer.
// Sets *divByZero if DIV/MOD by zero (caller should errorLog).
Clang::ConstValue* evalBinary(ClangContext& ctx, BinaryExp::BinaryOp op,
                              Clang::ConstValue* lhs, Clang::ConstValue* rhs,
                              bool* divByZero);

} // namespace ConstEval

#endif
