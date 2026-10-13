#ifndef ARRAY_INIT_IR_H
#define ARRAY_INIT_IR_H

#include "IRBuilder.h"
#include <vector>

// Flatten nested ConstArray into a scalar Constant* sequence.
inline void flattenConstArray(Constant* value, std::vector<Constant*>& out) {
    if (auto constArray = dynamic_cast<ConstantArray*>(value)) {
        for (size_t i = 0; i < constArray->getElementsCount(); ++i) {
            flattenConstArray(constArray->getElement(i), out);
        }
    } else {
        out.push_back(value);
    }
}

// Rebuild nested array constant from a flat scalar sequence (pad with zeros).
inline Constant* buildArrayConstantFromScalars(LLVM::ArrayType* arrayType,
                                               const std::vector<Constant*>& scalars,
                                               size_t& offset,
                                               IRBuilder& builder,
                                               LLVMContext& ctx) {
    auto elementType = arrayType->getElementType();
    std::vector<Constant*> elements;
    elements.reserve(arrayType->getSize());
    for (size_t i = 0; i < arrayType->getSize(); ++i) {
        if (elementType->isArray()) {
            elements.push_back(buildArrayConstantFromScalars(
                static_cast<LLVM::ArrayType*>(elementType), scalars, offset, builder, ctx));
        } else if (offset < scalars.size()) {
            elements.push_back(scalars[offset++]);
        } else {
            elements.push_back(dynamic_cast<Constant*>(builder.createConstInt(elementType, 0)));
        }
    }
    return dynamic_cast<Constant*>(builder.createConstArray(arrayType, std::move(elements)));
}

inline void storeZeroElement(IRBuilder& builder, LLVMContext& ctx, Value* base,
                             LLVM::Type* elementType, const std::vector<size_t>& indices) {
    if (elementType->isArray()) {
        auto arrayType = static_cast<LLVM::ArrayType*>(elementType);
        for (size_t i = 0; i < arrayType->getSize(); ++i) {
            auto childIndices = indices;
            childIndices.push_back(i);
            storeZeroElement(builder, ctx, base, arrayType->getElementType(), childIndices);
        }
        return;
    }
    std::vector<Value*> gepIndices;
    gepIndices.reserve(1 + indices.size());
    gepIndices.push_back(builder.createConstInt(ctx.getIntegerTyImpl(32), 0));
    for (size_t index : indices) {
        gepIndices.push_back(builder.createConstInt(ctx.getIntegerTyImpl(32), index));
    }
    auto ptr = builder.createGEP(base, std::move(gepIndices));
    builder.createStore(builder.createConstInt(elementType, 0), ptr);
}

#endif
