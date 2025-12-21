#ifndef CONTROL_FLOW_INSTRUCTION_H
#define CONTROL_FLOW_INSTRUCTION_H
#include "Instruction.h"
#include "../../BasicBlock.h"

#include <string>

class BrInstr : public Instruction {
private:
    Value* condition;
    BasicBlock* trueBB;
    BasicBlock* falseBB;
public:
    BrInstr(Value* condition, BasicBlock* trueBB, BasicBlock* falseBB, const std::string& name);

    Value* getCondition() const;
    void setCondition(Value* newCondition);
    BasicBlock* getTrueBlock() const;
    void setTrueBlock(BasicBlock* newTrueBlock);
    BasicBlock* getFalseBlock() const;
    void setFalseBlock(BasicBlock* newFalseBlock);
};

class RetInstr : public Instruction {
private:
    Value* returnValue;
public:
    RetInstr(Value* returnValue, const std::string& name);

    Value* getReturnValue() const;
    void setReturnValue(Value* newReturnValue);
};

#endif
