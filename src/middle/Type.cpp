#include "../../include/middle/Type.h"
#include <map>

TypePtr Type::getVoidTy() {
    static TypePtr voidTy = std::make_shared<VoidType>();
    return voidTy;
}

TypePtr Type::getIntegerTy() {
    static TypePtr intTy = std::make_shared<IntegerType>();
    return intTy;
}

TypePtr Type::getCharTy() {
    static TypePtr charTy = std::make_shared<CharType>();
    return charTy;
}

TypePtr Type::getConstIntegerTy() {
    static TypePtr constIntTy = [] {
        auto ty = std::make_shared<IntegerType>();
        ty->setTypeQualifier(TypeQualifier::CONST);
        return ty;
    }();
    return constIntTy;
}

TypePtr Type::getConstCharTy() {
    static TypePtr constCharTy = [] {
        auto ty = std::make_shared<CharType>();
        ty->setTypeQualifier(TypeQualifier::CONST);
        return ty;
    }();
    return constCharTy;
}

TypePtr convertToType(BasicType basicType) {
    if (basicType == BasicType::INT) {
        return Type::getIntegerTy();
    }
    return Type::getVoidTy();
}

TypePtr convertToType(BasicType basicType, TypeQualifier tq) {
    if (basicType == BasicType::INT) {
        if (tq == TypeQualifier::CONST) {
            return Type::getConstIntegerTy();
        }
        return Type::getIntegerTy();
    }
    return Type::getVoidTy();
}