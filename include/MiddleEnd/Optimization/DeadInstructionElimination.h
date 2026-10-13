#ifndef MIDDLEEND_OPTIMIZATION_DEAD_INSTRUCTION_ELIMINATION_H
#define MIDDLEEND_OPTIMIZATION_DEAD_INSTRUCTION_ELIMINATION_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/Optimization/Pass.h"

#include <set>

// Delete unused side-effect-free instructions (whitelist).
// Use-def: scan operands (Value::users is not maintained).
// Load/Alloca removed only when unused; Store/Call/terminators/Phi/GEP kept.
class DeadInstructionElimination : public Pass {
private:
    Module& module;

    static bool isRemovable(Instruction* instr);
    static void collectOperands(Instruction* instr, std::set<Value*>& referenced);
    static int eliminateInFunction(Function* func);

public:
    explicit DeadInstructionElimination(Module& module);

    const char* getName() const override { return "DeadInstructionElimination"; }
    int runOnModule(Module& module, LLVMContext& context) override;
    int run();
};

#endif
