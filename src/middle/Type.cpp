#include "../../include/middle/Type.h"

TypePtr Type::getVoidTy() {
    static TypePtr voidTy = std::make_shared<VoidType>();
    return voidTy;
}

TypePtr Type::getIntegerTy() {
    static TypePtr intTy = std::make_shared<IntegerType>();
    return intTy;
}

TypePtr convertToType(BasicType basicType) {
    if (basicType == BasicType::INT) {
        return Type::getIntegerTy();
    }
    return Type::getVoidTy();
}