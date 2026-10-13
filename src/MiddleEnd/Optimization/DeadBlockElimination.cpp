#include "MiddleEnd/Optimization/DeadBlockElimination.h"
#include "MiddleEnd/Optimization/IRReplaceUses.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/BasicBlock.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>

DeadBlockElimination::DeadBlockElimination(Module& module) : module(module) {}

int DeadBlockElimination::runOnModule(Module& mod, LLVMContext& ctx) {
    (void)mod;
    (void)ctx;
    return run();
}

void DeadBlockElimination::getSuccessors(BasicBlock* bb, std::vector<BasicBlock*>& succs) const {
    succs.clear();
    const auto& insts = bb->getInstructions();
    if (insts.empty()) {
        return;
    }
    auto* br = dynamic_cast<BrInstr*>(insts.back());
    if (!br) {
        return;
    }
    if (br->getCondition() == nullptr) {
        if (br->getTrueBlock() != nullptr) {
            succs.push_back(br->getTrueBlock());
        }
    } else {
        if (br->getTrueBlock() != nullptr) {
            succs.push_back(br->getTrueBlock());
        }
        if (br->getFalseBlock() != nullptr) {
            succs.push_back(br->getFalseBlock());
        }
    }
}

void DeadBlockElimination::markReachable(Function* func, std::vector<bool>& reachable) const {
    const auto& bbs = func->getBasicBlocks();
    reachable.assign(bbs.size(), false);
    if (bbs.empty()) {
        return;
    }
    std::unordered_set<BasicBlock*> visited;
    std::vector<BasicBlock*> worklist;
    worklist.push_back(bbs.front());
    while (!worklist.empty()) {
        BasicBlock* cur = worklist.back();
        worklist.pop_back();
        if (!visited.insert(cur).second) {
            continue;
        }
        std::vector<BasicBlock*> succs;
        getSuccessors(cur, succs);
        for (BasicBlock* s : succs) {
            if (visited.count(s) == 0) {
                worklist.push_back(s);
            }
        }
    }
    for (size_t i = 0; i < bbs.size(); ++i) {
        if (visited.count(bbs[i]) > 0) {
            reachable[i] = true;
        }
    }
}

void DeadBlockElimination::scrubPhiFromDeadBlocks(
    Function* func, const std::vector<BasicBlock*>& removed) const {
    if (removed.empty()) {
        return;
    }
    std::unordered_set<BasicBlock*> dead(removed.begin(), removed.end());
    for (BasicBlock* bb : func->getBasicBlocks()) {
        for (Instruction* inst : bb->getInstructions()) {
            auto* phi = dynamic_cast<PhiInstr*>(inst);
            if (!phi) {
                continue;
            }
            const auto& incomings = phi->getIncomingValues();
            std::vector<std::pair<Value*, BasicBlock*>> kept;
            kept.reserve(incomings.size());
            bool changed = false;
            for (const auto& in : incomings) {
                if (dead.count(in.second) > 0) {
                    changed = true;
                    continue;
                }
                kept.push_back(in);
            }
            if (changed) {
                phi->setIncoming(kept);
            }
        }
    }
}

int DeadBlockElimination::eliminateDeadBlocks(Function* func) {
    std::vector<bool> reachable;
    markReachable(func, reachable);
    const auto& bbs = func->getBasicBlocks();
    std::vector<BasicBlock*> alive;
    std::vector<BasicBlock*> removed;
    for (size_t i = 0; i < bbs.size(); ++i) {
        if (reachable[i]) {
            alive.push_back(bbs[i]);
        } else {
            removed.push_back(bbs[i]);
        }
    }
    int n = static_cast<int>(removed.size());
    if (n > 0) {
        // Scrub while dead blocks are still in the function list (phi may live on alive BBs).
        scrubPhiFromDeadBlocks(func, removed);
        func->setBasicBlocks(alive);
    }
    return n;
}

int DeadBlockElimination::mergeJumps(Function* func) {
    int merged = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        const auto& bbs = func->getBasicBlocks();
        if (bbs.empty()) {
            break;
        }
        std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> preds;
        for (BasicBlock* bb : bbs) {
            std::vector<BasicBlock*> succs;
            getSuccessors(bb, succs);
            for (BasicBlock* s : succs) {
                preds[s].push_back(bb);
            }
        }
        for (BasicBlock* b : bbs) {
            if (b == bbs.front()) {
                continue;
            }
            auto it = preds.find(b);
            if (it == preds.end() || it->second.size() != 1) {
                continue;
            }
            BasicBlock* a = it->second.front();
            if (a == b) {
                continue;
            }
            const auto& aInsts = a->getInstructions();
            if (aInsts.empty()) {
                continue;
            }
            auto* aBr = dynamic_cast<BrInstr*>(aInsts.back());
            if (!aBr || aBr->getCondition() != nullptr || aBr->getTrueBlock() != b) {
                continue;
            }
            const auto& bInsts = b->getInstructions();
            if (bInsts.empty() || !bInsts.back()->isTerminator()) {
                continue;
            }
            // Skip merge if B starts with Phi — moving Phis into A breaks SSA form.
            if (dynamic_cast<PhiInstr*>(bInsts.front())) {
                continue;
            }

            std::vector<Instruction*> newInsts;
            for (size_t i = 0; i + 1 < aInsts.size(); ++i) {
                newInsts.push_back(aInsts[i]);
            }
            for (Instruction* inst : bInsts) {
                newInsts.push_back(inst);
            }
            a->setInstructions(newInsts);
            a->setTerminator(!newInsts.empty() && newInsts.back()->isTerminator());

            for (BasicBlock* bb : bbs) {
                if (bb == b) {
                    continue;
                }
                for (Instruction* inst : bb->getInstructions()) {
                    auto* phi = dynamic_cast<PhiInstr*>(inst);
                    if (!phi) {
                        continue;
                    }
                    const auto& incomings = phi->getIncomingValues();
                    std::vector<std::pair<Value*, BasicBlock*>> newIncomings;
                    bool needUpdate = false;
                    for (const auto& in : incomings) {
                        if (in.second == b) {
                            newIncomings.push_back({in.first, a});
                            needUpdate = true;
                        } else {
                            newIncomings.push_back(in);
                        }
                    }
                    if (needUpdate) {
                        phi->setIncoming(newIncomings);
                    }
                }
            }

            std::vector<BasicBlock*> newBbs;
            for (BasicBlock* bb : bbs) {
                if (bb != b) {
                    newBbs.push_back(bb);
                }
            }
            func->setBasicBlocks(newBbs);
            ++merged;
            changed = true;
            break;
        }
    }
    return merged;
}

int DeadBlockElimination::run() {
    int total = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        for (Function* func : collectModuleFunctions(module)) {
            if (!func || func->getBasicBlocks().empty()) {
                continue;
            }
            int removed = eliminateDeadBlocks(func);
            if (removed > 0) {
                total += removed;
                changed = true;
            }
            int merged = mergeJumps(func);
            if (merged > 0) {
                total += merged;
                changed = true;
            }
        }
    }
    return total;
}
