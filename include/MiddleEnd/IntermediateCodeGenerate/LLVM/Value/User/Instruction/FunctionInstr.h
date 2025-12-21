#ifndef FUNCTION_INSTR_H
#define FUNCTION_INSTR_H
#include "Instruction.h"
#include "../Constant/GlobalValue/Function.h"

#include <vector>
#include <string>

class  CallInstr : public Instruction {
private:
    Function* function;
    std::vector<Value*> arguments;
public:
    CallInstr(LLVM::Type* type, Function* function, std::vector<Value*> arguments, const std::string& name);

    Function* getFunction() const;
    void setFunction(Function* newFunction);

    size_t getArgumentsCount() const;
    Value* getArgument(size_t index) const;
    void addArgument(Value* argument);
    void setArguments(std::vector<Value*> newArguments);
    const std::vector<Value*>& getArguments() const;
};

#endif