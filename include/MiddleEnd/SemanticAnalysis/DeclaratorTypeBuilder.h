#ifndef DECLARATOR_TYPE_BUILDER_H
#define DECLARATOR_TYPE_BUILDER_H

#include "ClangContext.h"
#include "Type.h"
#include <cstddef>
#include <vector>

// Pure type-wrapping helpers for C declarators (pointer / array / function).
// Array size evaluation stays in SemanticVisitor (needs wantConst accept).
namespace DeclaratorTypeBuilder {

inline Clang::Type* wrapPointer(ClangContext& ctx, Clang::Type* pointee) {
    return ctx.getPointerTyImpl(pointee);
}

inline Clang::Type* wrapArray(ClangContext& ctx, Clang::Type* element, int64_t size) {
    return ctx.getArrayTyImpl(element, size);
}

inline Clang::Type* wrapFunction(ClangContext& ctx, Clang::Type* ret,
                                 const std::vector<Clang::Type*>& params, bool variadic) {
    return ctx.getFunctionTyImpl(ret, params, variadic);
}

} // namespace DeclaratorTypeBuilder

#endif
