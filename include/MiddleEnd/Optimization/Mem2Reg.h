#ifndef MIDDLEEND_OPTIMIZATION_MEM2REG_H
#define MIDDLEEND_OPTIMIZATION_MEM2REG_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"
#include "MiddleEnd/Optimization/Pass.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class AllocaInst;
class BasicBlock;
class Function;
class Value;
class Instruction;
class PhiInstr;

// Mem2Reg: promote scalar allocas with no escaped address.
// - Single-block uses: linear rewrite, no Phi
// - Multi-block: iterated dominance-frontier Phi + DomTree rename
// Limits: no array/escaped addr; store type must match alloca; undef = iN 0
class Mem2Reg : public Pass {
private:
    Module& module;
    LLVMContext& context;

    using BBList = std::vector<BasicBlock*>;
    using PredMap = std::unordered_map<BasicBlock*, BBList>;
    using SuccMap = std::unordered_map<BasicBlock*, BBList>;

    static std::vector<std::unique_ptr<Value>>& ownedPool();
    Value* getUndef(LLVM::Type* ty);
    LLVM::Type* allocatedType(AllocaInst* alloca) const;

    void getSuccessors(BasicBlock* bb, BBList& succs) const;
    void buildCFG(Function* func, PredMap& preds, SuccMap& succs) const;

    bool collectUses(AllocaInst* alloca, Function* func,
                     std::vector<Instruction*>& loads,
                     std::vector<Instruction*>& stores,
                     std::unordered_set<BasicBlock*>& useBlocks) const;

    void computeDominators(Function* func, const PredMap& preds,
                           std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& dom) const;
    void computeIdom(Function* func,
                     const std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& dom,
                     std::unordered_map<BasicBlock*, BasicBlock*>& idom) const;
    void computeDF(Function* func, const PredMap& preds,
                   const std::unordered_map<BasicBlock*, BasicBlock*>& idom,
                   std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& df) const;

    void prependPhi(BasicBlock* bb, PhiInstr* phi) const;
    void removeInstructions(BasicBlock* bb,
                            const std::unordered_set<Instruction*>& doomed) const;

    int promoteSingleBlock(AllocaInst* alloca, Function* func,
                           BasicBlock* bb,
                           const std::vector<Instruction*>& loads,
                           const std::vector<Instruction*>& stores);
    int promoteMultiBlock(AllocaInst* alloca, Function* func,
                          const std::vector<Instruction*>& loads,
                          const std::vector<Instruction*>& stores,
                          const PredMap& preds, const SuccMap& succs);
    int promoteFunction(Function* func);

public:
    Mem2Reg(Module& module, LLVMContext& context);

    const char* getName() const override { return "Mem2Reg"; }
    int runOnModule(Module& module, LLVMContext& context) override;
    int run();
};

#endif
