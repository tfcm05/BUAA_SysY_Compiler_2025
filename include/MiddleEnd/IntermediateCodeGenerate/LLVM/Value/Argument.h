#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "Value.h"

#include <string>

class Function;

class Argument : public Value {
private:
    Function* parentFunction;
    size_t argNumber;
public:
    Argument(LLVM::Type* type, Function* parentFunction, size_t argNumber, const std::string& name);

    Function* getParentFunction() const;
    void setParentFunction(Function* newParentFunction);

    size_t getArgNumber() const;
    void setArgNumber(size_t newArgNumber);
};

#endif