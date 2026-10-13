#ifndef ENSURE_I1_H
#define ENSURE_I1_H

#include "IRBuilder.h"

// Convert an iN / constant integer Value* to i1 for branches and &&/||.
inline Value* ensureI1(IRBuilder& builder, LLVMContext& llvmContext, Value* value) {
    if (!value) {
        return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
    }
    if (auto constInt = dynamic_cast<ConstantInt*>(value)) {
        return builder.createConstInt(llvmContext.getIntegerTyImpl(1),
                                      constInt->getValue() != 0 ? 1 : 0);
    }
    auto valueType = value->getType();
    if (valueType && valueType->isInteger()) {
        auto integerType = static_cast<LLVM::IntegerType*>(valueType);
        if (integerType->getBitWidth() == 1) {
            return value;
        }
        auto zero = builder.createConstInt(
            llvmContext.getIntegerTyImpl(integerType->getBitWidth()), 0);
        return builder.createIcmp(IcmpInstr::IcmpOp::NE, value, zero);
    }
    return builder.createConstInt(llvmContext.getIntegerTyImpl(1), 0);
}

#endif
