#ifndef GLOBAL_VALUE_H
#define GLOBAL_VALUE_H

#include "../Constant.h"

#include <string>

class GlobalValue : public Constant {
protected:
    GlobalValue(ValueKind kind, LLVM::Type* type, const std::string& name);
public:
    virtual ~GlobalValue() = default;
};

#endif