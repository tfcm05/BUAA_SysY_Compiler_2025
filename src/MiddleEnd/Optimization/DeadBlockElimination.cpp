#include "MiddleEnd/Optimization/DeadBlockElimination.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>

DeadBlockElimination::DeadBlockElimination(Module& module) : module(module) {}

void DeadBlockElimination::getSuccessors(BasicBlock* bb, std::vector<BasicBlock*>& succs) const {
    succs.clear();
    const auto& insts = bb->getInstructions();
    if (insts.empty()) {
        return;
    }
    auto* br = dynamic_cast<BrInstr*>(insts.back());
    if (!br) {
        return; // ret 等无后继
    }
    if (br->getCondition() == nullptr) {
        // 无条件 br: 目标是 trueBlock
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
    worklist.push_back(bbs.front()); // entry BB 可达
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

int DeadBlockElimination::eliminateDeadBlocks(Function* func) {
    std::vector<bool> reachable;
    markReachable(func, reachable);
    const auto& bbs = func->getBasicBlocks();
    std::vector<BasicBlock*> alive;
    for (size_t i = 0; i < bbs.size(); ++i) {
        if (reachable[i]) {
            alive.push_back(bbs[i]);
        }
    }
    int removed = static_cast<int>(bbs.size() - alive.size());
    if (removed > 0) {
        func->setBasicBlocks(alive);
    }
    return removed;
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
        // 构建前驱关系: preds[bb] = 所有能跳转到 bb 的 BB
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
                continue; // B 不能是 entry
            }
            auto it = preds.find(b);
            if (it == preds.end() || it->second.size() != 1) {
                continue; // 必须只有一个前驱
            }
            BasicBlock* a = it->second.front();
            if (a == b) {
                continue; // 自环不合并
            }
            // A 的最后一条必须是无条件 br 到 B
            const auto& aInsts = a->getInstructions();
            if (aInsts.empty()) {
                continue;
            }
            auto* aBr = dynamic_cast<BrInstr*>(aInsts.back());
            if (!aBr || aBr->getCondition() != nullptr || aBr->getTrueBlock() != b) {
                continue;
            }
            // B 必须非空且以终止指令结尾
            const auto& bInsts = b->getInstructions();
            if (bInsts.empty() || !bInsts.back()->isTerminator()) {
                continue;
            }
            // 合并: A 去掉末尾的无条件 br, 接上 B 的全部指令
            std::vector<Instruction*> newInsts;
            for (size_t i = 0; i + 1 < aInsts.size(); ++i) {
                newInsts.push_back(aInsts[i]);
            }
            for (Instruction* inst : bInsts) {
                newInsts.push_back(inst);
            }
            a->setInstructions(newInsts);
            // 更新其他 BB 中 PhiInstr 对 B 的 incoming 引用为 A
            for (BasicBlock* bb : bbs) {
                if (bb == b) {
                    continue; // B 即将被删除
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
            // 从函数中删除 B
            std::vector<BasicBlock*> newBbs;
            for (BasicBlock* bb : bbs) {
                if (bb != b) {
                    newBbs.push_back(bb);
                }
            }
            func->setBasicBlocks(newBbs);
            ++merged;
            changed = true;
            break; // 列表已变化, 重新开始扫描
        }
    }
    return merged;
}

int DeadBlockElimination::run() {
    int total = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        for (Function* func : module.getFunctions()) {
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
