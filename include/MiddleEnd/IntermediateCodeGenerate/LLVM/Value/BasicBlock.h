#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#include "Value.h"
#include "User/Instruction/Instruction.h"

#include <string>
#include <vector>

class BasicBlock : public Value {
private:
    bool terminatorFlag;
    std::vector<Instruction*> instructions;
public:
    BasicBlock(const std::string& name);

    bool isTerminator() const;
    void setTerminator(bool tf);

    size_t getInstructionsCount() const;
    Instruction* getInstruction(size_t index) const;
    void addInstruction(Instruction* instr);
    void setInstructions(std::vector<Instruction*> newInstructions);
    const std::vector<Instruction*>& getInstructions() const;
};

#endif