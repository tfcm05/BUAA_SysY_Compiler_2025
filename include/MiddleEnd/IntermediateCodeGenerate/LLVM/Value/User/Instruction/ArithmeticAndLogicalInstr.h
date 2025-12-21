#ifndef ARITHMETIC_AND_LOGICAL_INSTR_H
#define ARITHMETIC_AND_LOGICAL_INSTR_H
#include "Instruction.h"

#include <string>

class AddInstr : public Instruction {
private:
    Value* lhs;
    Value* rhs;
public:
    AddInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name);

    Value* getLHS() const;
    void setLHS(Value* newLHS);
    Value* getRHS() const;
    void setRHS(Value* newRHS);
};

class SubInstr : public Instruction {
private:
    Value* lhs;
    Value* rhs;
public:
    SubInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name);

    Value* getLHS() const;
    void setLHS(Value* newLHS);
    Value* getRHS() const;
    void setRHS(Value* newRHS);
};

class MulInstr : public Instruction {
private:
    Value* lhs;
    Value* rhs;
public:
    MulInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name);

    Value* getLHS() const;
    void setLHS(Value* newLHS);
    Value* getRHS() const;
    void setRHS(Value* newRHS);
};


class SdivInstr : public Instruction {
private:
    Value* lhs;
    Value* rhs;
public:
    SdivInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name);

    Value* getLHS() const;
    void setLHS(Value* newLHS);
    Value* getRHS() const;
    void setRHS(Value* newRHS);
};


class AndInstr : public Instruction {
private:
    Value* lhs;
    Value* rhs;
public:
    AndInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name);

    Value* getLHS() const;
    void setLHS(Value* newLHS);
    Value* getRHS() const;
    void setRHS(Value* newRHS);
};


class OrInstr : public Instruction {
private:
    Value* lhs;
    Value* rhs;
public:
    OrInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name);

    Value* getLHS() const;
    void setLHS(Value* newLHS);
    Value* getRHS() const;
    void setRHS(Value* newRHS);
};

class IcmpInstr : public Instruction {
public:
    enum class IcmpOp {
        EQ, // equal
        NE, // not equal
        SLT, // signed less than
        SLE, // signed less equal
        SGT, // signed greater than
        SGE  // signed greater equal
    };
private:
    IcmpOp op;
    Value* lhs;
    Value* rhs;
public:
    IcmpInstr(IcmpOp op, LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name);

    IcmpOp getPredicate() const;
    void setPredicate(IcmpOp newOp);
    Value* getLHS() const;
    void setLHS(Value* newLHS);
    Value* getRHS() const;
    void setRHS(Value* newRHS);
};

#endif