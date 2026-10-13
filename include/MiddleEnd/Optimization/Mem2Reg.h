#ifndef MIDDLEEND_OPTIMIZATION_MEM2REG_H
#define MIDDLEEND_OPTIMIZATION_MEM2REG_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"
#include "MiddleEnd/Optimization/Pass.h"

#include <memory>
#include <unordered_set>
#include <vector>

class AllocaInst;
class BasicBlock;
class Function;
class Value;
class Instruction;

// Conservative Mem2Reg: promote only scalar allocas whose Load/Store uses all
// live in a single basic block (linear rewrite, no Phi). Cross-block SSA
// promotion is deferred until ConstBranch/DBE maintain phi CFG edges reliably.
//
// Limits:
// - Single-block uses only (no Phi insertion)
// - No array / escaped address (GEP, call arg, store-as-value)
// - Store value type must match allocated type (skips ill-typed i1→i32 stores)
// - Uninitialized reads become iN 0
// - Uses replaceUsesInFunction (scan-based)
class Mem2Reg : public Pass {
private:
    Module& module;
    LLVMContext& context;

    static std::vector<std::unique_ptr<Value>>& ownedPool();
    Value* getUndef(LLVM::Type* ty);
    LLVM::Type* allocatedType(AllocaInst* alloca) const;

    bool isPromotable(AllocaInst* alloca, Function* func,
                      std::vector<Instruction*>& loads,
                      std::vector<Instruction*>& stores,
                      BasicBlock*& soleBlock) const;
    void removeInstructions(BasicBlock* bb,
                            const std::unordered_set<Instruction*>& doomed) const;
    int promoteSingleBlock(AllocaInst* alloca, Function* func,
                           BasicBlock* bb,
                           const std::vector<Instruction*>& loads,
                           const std::vector<Instruction*>& stores);
    int promoteFunction(Function* func);

public:
    Mem2Reg(Module& module, LLVMContext& context);

    const char* getName() const override { return "Mem2Reg"; }
    int runOnModule(Module& module, LLVMContext& context) override;
    int run();
};

#endif
