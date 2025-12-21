#ifndef MEMORY_ACCESS_INSTR_H
#define MEMORY_ACCESS_INSTR_H
#include "Instruction.h"

#include <vector>
#include <string>

class AllocaInst : public Instruction {
public:
    AllocaInst(LLVM::Type* type, const std::string& name);
};

class LoadInst : public Instruction {
private:
    Value* ptr;
public:
    LoadInst(LLVM::Type* type, Value* ptr, const std::string& name);

    Value* getPointer() const;
    void setPointer(Value* newPtr);
};

class StoreInst : public Instruction {
private:
    Value* value;
    Value* ptr;
public:
    StoreInst(Value* value, Value* ptr, const std::string& name);

    Value* getValue() const;
    void setValue(Value* newValue);

    Value* getPointer() const;
    void setPointer(Value* newPtr);
};

class GetElementPtrInst : public Instruction {
private:
    Value* ptr;
    std::vector<Value*> indices;
public:
    GetElementPtrInst(LLVM::Type* type, Value* ptr, std::vector<Value*> indices, const std::string& name);

    Value* getPointer() const;
    void setPointer(Value* newPtr);

    size_t getIndicesCount() const;
    Value* getIndex(size_t index) const;
    void addIndex(Value* index);
    void setIndices(std::vector<Value*> newIndices);
    const std::vector<Value*>& getIndices() const;
};

#endif