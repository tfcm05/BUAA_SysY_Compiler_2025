#ifndef MIDDLEEND_OPTIMIZATION_DEAD_BLOCK_ELIMINATION_H
#define MIDDLEEND_OPTIMIZATION_DEAD_BLOCK_ELIMINATION_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/Optimization/Pass.h"

#include <vector>

// Reachability from entry + unconditional jump-chain merge.
// Scrubs Phi incoming edges that pointed at removed blocks.
class DeadBlockElimination : public Pass {
private:
    Module& module;

    void getSuccessors(BasicBlock* bb, std::vector<BasicBlock*>& succs) const;
    void markReachable(Function* func, std::vector<bool>& reachable) const;
    void scrubPhiFromDeadBlocks(Function* func,
                                const std::vector<BasicBlock*>& removed) const;
    // Drop Phi incomings whose block is no longer a CFG predecessor.
    int scrubPhiToCFG(Function* func) const;
    int eliminateDeadBlocks(Function* func);
    int mergeJumps(Function* func);

public:
    explicit DeadBlockElimination(Module& module);

    const char* getName() const override { return "DeadBlockElimination"; }
    int runOnModule(Module& module, LLVMContext& context) override;
    int run();
};

#endif
