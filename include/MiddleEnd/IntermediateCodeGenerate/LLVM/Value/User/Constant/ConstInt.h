#ifndef CONST_INT_H
#define CONST_INT_H

#include <cstdint>
#include "Constant.h"

#include <string>

class ConstantInt : public Constant {
private:
    int64_t value;
public:
    ConstantInt(LLVM::Type* type, int64_t value, const std::string& name);

    int64_t getValue() const;
    void setValue(int64_t newValue);
};

#endif