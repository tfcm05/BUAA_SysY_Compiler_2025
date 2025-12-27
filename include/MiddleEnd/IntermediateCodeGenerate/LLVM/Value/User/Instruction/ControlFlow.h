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

class PhiInstr : public Instruction {
private:
    std::vector<std::pair<Value*, BasicBlock*>> incomingValues;
public:
    PhiInstr(LLVM::Type* type, std::vector<std::pair<Value*, BasicBlock*>> incoming, const std::string& name);

    void addIncoming(Value* value, BasicBlock* block);
    size_t getIncomingCount() const;
    std::pair<Value*, BasicBlock*> getIncoming(size_t index) const;
    void setIncoming(std::vector<std::pair<Value*, BasicBlock*>> newIncomingValues);
    const std::vector<std::pair<Value*, BasicBlock*>>& getIncomingValues() const;
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
