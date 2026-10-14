#include "MiddleEnd/Optimization/Mem2Reg.h"
#include "MiddleEnd/Optimization/IRReplaceUses.h"

#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/FunctionInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/MemoryAccessInstr.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/ConstInt.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/BasicBlock.h"
#include "MiddleEnd/IntermediateCodeGenerate/Type.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>

Mem2Reg::Mem2Reg(Module& module, LLVMContext& context)
    : module(module), context(context) {}

std::vector<std::unique_ptr<Value>>& Mem2Reg::ownedPool() {
    static std::vector<std::unique_ptr<Value>> pool;
    return pool;
}

Value* Mem2Reg::getUndef(LLVM::Type* ty) {
    auto* ci = new ConstantInt(ty, 0, "mem2reg.undef");
    ownedPool().push_back(std::unique_ptr<Value>(ci));
    return ci;
}

LLVM::Type* Mem2Reg::allocatedType(AllocaInst* alloca) const {
    if (!alloca || !alloca->getType() || !alloca->getType()->isPointer()) {
        return nullptr;
    }
    return static_cast<LLVM::PointerType*>(alloca->getType())->getPointedType();
}

void Mem2Reg::getSuccessors(BasicBlock* bb, BBList& succs) const {
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
        if (br->getTrueBlock()) {
            succs.push_back(br->getTrueBlock());
        }
    } else {
        if (br->getTrueBlock()) {
            succs.push_back(br->getTrueBlock());
        }
        if (br->getFalseBlock()) {
            succs.push_back(br->getFalseBlock());
        }
    }
}

void Mem2Reg::buildCFG(Function* func, PredMap& preds, SuccMap& succs) const {
    preds.clear();
    succs.clear();
    for (BasicBlock* bb : func->getBasicBlocks()) {
        preds[bb];
        succs[bb];
    }
    for (BasicBlock* bb : func->getBasicBlocks()) {
        BBList s;
        getSuccessors(bb, s);
        succs[bb] = s;
        for (BasicBlock* t : s) {
            preds[t].push_back(bb);
        }
    }
}

bool Mem2Reg::collectUses(AllocaInst* alloca, Function* func,
                          std::vector<Instruction*>& loads,
                          std::vector<Instruction*>& stores,
                          std::unordered_set<BasicBlock*>& useBlocks) const {
    loads.clear();
    stores.clear();
    useBlocks.clear();

    LLVM::Type* ty = allocatedType(alloca);
    if (!ty || ty->isArray() || ty->isFunction() || ty->isVoid()) {
        return false;
    }
    if (!ty->isInteger() && !ty->isPointer()) {
        return false;
    }

    for (BasicBlock* bb : func->getBasicBlocks()) {
        for (Instruction* inst : bb->getInstructions()) {
            if (inst == alloca) {
                useBlocks.insert(bb);
                continue;
            }
            if (auto* load = dynamic_cast<LoadInst*>(inst)) {
                if (load->getPointer() == alloca) {
                    if (load->getType() != ty) {
                        return false;
                    }
                    loads.push_back(inst);
                    useBlocks.insert(bb);
                }
                continue;
            }
            if (auto* store = dynamic_cast<StoreInst*>(inst)) {
                if (store->getPointer() == alloca) {
                    if (!store->getValue() || store->getValue()->getType() != ty) {
                        return false;
                    }
                    stores.push_back(inst);
                    useBlocks.insert(bb);
                    continue;
                }
                if (store->getValue() == alloca) {
                    return false;
                }
                continue;
            }
            if (auto* gep = dynamic_cast<GetElementPtrInst*>(inst)) {
                if (gep->getPointer() == alloca) {
                    return false;
                }
                for (size_t i = 0; i < gep->getIndicesCount(); ++i) {
                    if (gep->getIndex(i) == alloca) {
                        return false;
                    }
                }
                continue;
            }
            if (auto* call = dynamic_cast<CallInstr*>(inst)) {
                for (size_t i = 0; i < call->getArgumentsCount(); ++i) {
                    if (call->getArgument(i) == alloca) {
                        return false;
                    }
                }
                continue;
            }
            auto mentions = [&](Value* v) { return v == alloca; };
            if (auto* bin = dynamic_cast<AddInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<SubInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<MulInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<SdivInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<AndInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<OrInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* bin = dynamic_cast<IcmpInstr*>(inst)) {
                if (mentions(bin->getLHS()) || mentions(bin->getRHS())) return false;
            } else if (auto* z = dynamic_cast<ZextInstr*>(inst)) {
                if (mentions(z->getOperand())) return false;
            } else if (auto* br = dynamic_cast<BrInstr*>(inst)) {
                if (mentions(br->getCondition())) return false;
            } else if (auto* ret = dynamic_cast<RetInstr*>(inst)) {
                if (mentions(ret->getReturnValue())) return false;
            } else if (auto* phi = dynamic_cast<PhiInstr*>(inst)) {
                for (size_t i = 0; i < phi->getIncomingCount(); ++i) {
                    if (mentions(phi->getIncoming(i).first)) return false;
                }
            }
        }
    }
    return !useBlocks.empty();
}

void Mem2Reg::computeDominators(
    Function* func, const PredMap& preds,
    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& dom) const {
    const auto& bbs = func->getBasicBlocks();
    if (bbs.empty()) {
        return;
    }
    BasicBlock* entry = bbs.front();
    std::unordered_set<BasicBlock*> all(bbs.begin(), bbs.end());

    for (BasicBlock* bb : bbs) {
        if (bb == entry) {
            dom[bb] = {entry};
        } else {
            dom[bb] = all;
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (BasicBlock* bb : bbs) {
            if (bb == entry) {
                continue;
            }
            std::unordered_set<BasicBlock*> neu = all;
            const auto& predList = preds.at(bb);
            if (!predList.empty()) {
                neu = dom[predList.front()];
                for (size_t i = 1; i < predList.size(); ++i) {
                    const auto& pd = dom[predList[i]];
                    std::unordered_set<BasicBlock*> inter;
                    for (BasicBlock* x : neu) {
                        if (pd.count(x)) {
                            inter.insert(x);
                        }
                    }
                    neu.swap(inter);
                }
            }
            neu.insert(bb);
            if (neu != dom[bb]) {
                dom[bb] = std::move(neu);
                changed = true;
            }
        }
    }
}

void Mem2Reg::computeIdom(
    Function* func,
    const std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& dom,
    std::unordered_map<BasicBlock*, BasicBlock*>& idom) const {
    idom.clear();
    const auto& bbs = func->getBasicBlocks();
    if (bbs.empty()) {
        return;
    }
    BasicBlock* entry = bbs.front();
    idom[entry] = nullptr;

    // Immediate dominator = strict dominator with the largest dominator set
    // (closest to bb on the dominator chain).
    for (BasicBlock* bb : bbs) {
        if (bb == entry) {
            continue;
        }
        BasicBlock* best = nullptr;
        size_t bestSize = 0;
        for (BasicBlock* d : dom.at(bb)) {
            if (d == bb) {
                continue;
            }
            if (dom.at(d).size() > bestSize) {
                bestSize = dom.at(d).size();
                best = d;
            }
        }
        idom[bb] = best;
    }
}

void Mem2Reg::computeDF(
    Function* func, const PredMap& preds,
    const std::unordered_map<BasicBlock*, BasicBlock*>& idom,
    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>>& df) const {
    df.clear();
    for (BasicBlock* bb : func->getBasicBlocks()) {
        df[bb];
    }
    for (BasicBlock* bb : func->getBasicBlocks()) {
        const auto& predList = preds.at(bb);
        if (predList.size() < 2) {
            continue;
        }
        for (BasicBlock* p : predList) {
            BasicBlock* runner = p;
            BasicBlock* idomBb = idom.at(bb);
            while (runner && runner != idomBb) {
                df[runner].insert(bb);
                runner = idom.at(runner);
            }
        }
    }
}

void Mem2Reg::prependPhi(BasicBlock* bb, PhiInstr* phi) const {
    std::vector<Instruction*> insts = bb->getInstructions();
    size_t insertAt = 0;
    while (insertAt < insts.size() && dynamic_cast<PhiInstr*>(insts[insertAt])) {
        ++insertAt;
    }
    insts.insert(insts.begin() + static_cast<std::ptrdiff_t>(insertAt), phi);
    bb->setInstructions(insts);
    bb->setTerminator(!insts.empty() && insts.back()->isTerminator());
}

void Mem2Reg::removeInstructions(
    BasicBlock* bb, const std::unordered_set<Instruction*>& doomed) const {
    if (!bb || doomed.empty()) {
        return;
    }
    std::vector<Instruction*> kept;
    kept.reserve(bb->getInstructionsCount());
    for (Instruction* inst : bb->getInstructions()) {
        if (doomed.count(inst) == 0) {
            kept.push_back(inst);
        }
    }
    bb->setInstructions(kept);
    bb->setTerminator(!kept.empty() && kept.back()->isTerminator());
}

int Mem2Reg::promoteSingleBlock(AllocaInst* alloca, Function* func,
                                BasicBlock* bb,
                                const std::vector<Instruction*>& loads,
                                const std::vector<Instruction*>& stores) {
    LLVM::Type* ty = allocatedType(alloca);
    Value* cur = getUndef(ty);

    std::unordered_set<Instruction*> doomed;
    doomed.insert(alloca);
    for (Instruction* s : stores) {
        doomed.insert(s);
    }
    for (Instruction* l : loads) {
        doomed.insert(l);
    }

    for (Instruction* inst : bb->getInstructions()) {
        if (auto* store = dynamic_cast<StoreInst*>(inst)) {
            if (store->getPointer() == alloca) {
                cur = store->getValue();
            }
        } else if (auto* load = dynamic_cast<LoadInst*>(inst)) {
            if (load->getPointer() == alloca) {
                replaceUsesInFunction(func, load, cur);
            }
        }
    }

    removeInstructions(bb, doomed);
    return 1;
}

int Mem2Reg::promoteMultiBlock(AllocaInst* alloca, Function* func,
                               const std::vector<Instruction*>& loads,
                               const std::vector<Instruction*>& stores,
                               const PredMap& preds, const SuccMap& succs) {
    LLVM::Type* ty = allocatedType(alloca);
    Value* undef = getUndef(ty);

    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>> dom;
    computeDominators(func, preds, dom);
    std::unordered_map<BasicBlock*, BasicBlock*> idom;
    computeIdom(func, dom, idom);
    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>> df;
    computeDF(func, preds, idom, df);

    // Definition blocks = blocks with a store (alloca itself is not a value def).
    std::unordered_set<BasicBlock*> defBlocks;
    for (Instruction* s : stores) {
        for (BasicBlock* bb : func->getBasicBlocks()) {
            for (Instruction* inst : bb->getInstructions()) {
                if (inst == s) {
                    defBlocks.insert(bb);
                }
            }
        }
    }

    // Iterated dominance frontier → Phi sites.
    std::unordered_set<BasicBlock*> phiSites;
    std::vector<BasicBlock*> work(defBlocks.begin(), defBlocks.end());
    while (!work.empty()) {
        BasicBlock* b = work.back();
        work.pop_back();
        for (BasicBlock* d : df[b]) {
            if (phiSites.insert(d).second) {
                work.push_back(d);
            }
        }
    }

    std::unordered_map<BasicBlock*, PhiInstr*> phis;
    // Unique names: multiple allocas may place phis in the same block.
    static uint32_t phiSerial = 0;
    const std::string allocaTag = alloca->getName().empty()
                                      ? ("a" + std::to_string(++phiSerial))
                                      : alloca->getName();
    for (BasicBlock* bb : phiSites) {
        auto* phi = new PhiInstr(
            ty, {}, bb->getName() + "." + allocaTag + ".m2r." + std::to_string(++phiSerial));
        ownedPool().push_back(std::unique_ptr<Value>(phi));
        prependPhi(bb, phi);
        phis[bb] = phi;
    }

    // DomTree children
    std::unordered_map<BasicBlock*, BBList> children;
    BasicBlock* entry = func->getBasicBlocks().front();
    for (BasicBlock* bb : func->getBasicBlocks()) {
        BasicBlock* p = idom[bb];
        if (p) {
            children[p].push_back(bb);
        }
    }

    std::unordered_set<Instruction*> doomed;
    doomed.insert(alloca);
    for (Instruction* s : stores) {
        doomed.insert(s);
    }
    for (Instruction* l : loads) {
        doomed.insert(l);
    }

    std::vector<Value*> stack;
    stack.push_back(undef);

    std::function<void(BasicBlock*)> rename = [&](BasicBlock* bb) {
        size_t stackSize = stack.size();

        if (auto it = phis.find(bb); it != phis.end()) {
            stack.push_back(it->second);
        }

        for (Instruction* inst : bb->getInstructions()) {
            if (doomed.count(inst) && dynamic_cast<AllocaInst*>(inst) == alloca) {
                continue;
            }
            if (auto* store = dynamic_cast<StoreInst*>(inst)) {
                if (store->getPointer() == alloca) {
                    stack.push_back(store->getValue());
                }
            } else if (auto* load = dynamic_cast<LoadInst*>(inst)) {
                if (load->getPointer() == alloca) {
                    replaceUsesInFunction(func, load, stack.back());
                }
            }
        }

        Value* outVal = stack.back();
        for (BasicBlock* succ : succs.at(bb)) {
            if (auto it = phis.find(succ); it != phis.end()) {
                it->second->addIncoming(outVal, bb);
            }
        }

        for (BasicBlock* child : children[bb]) {
            rename(child);
        }

        stack.resize(stackSize);
    };

    rename(entry);

    // Dedup phi incomings (one per pred) and ensure every pred has an edge.
    for (auto& [bb, phi] : phis) {
        const auto& predList = preds.at(bb);
        std::unordered_map<BasicBlock*, Value*> byPred;
        for (const auto& in : phi->getIncomingValues()) {
            byPred[in.second] = in.first;
        }
        std::vector<std::pair<Value*, BasicBlock*>> filled;
        filled.reserve(predList.size());
        for (BasicBlock* p : predList) {
            Value* v = undef;
            if (auto it = byPred.find(p); it != byPred.end()) {
                v = it->second;
            }
            filled.push_back({v, p});
        }
        phi->setIncoming(std::move(filled));
    }

    // Remove alloca / load / store from all blocks that may contain them.
    std::unordered_set<BasicBlock*> touch;
    for (BasicBlock* bb : func->getBasicBlocks()) {
        for (Instruction* inst : bb->getInstructions()) {
            if (doomed.count(inst)) {
                touch.insert(bb);
            }
        }
    }
    for (BasicBlock* bb : touch) {
        removeInstructions(bb, doomed);
    }
    return 1;
}

int Mem2Reg::promoteFunction(Function* func) {
    if (!func || func->getBasicBlocks().empty()) {
        return 0;
    }

    PredMap preds;
    SuccMap succs;
    buildCFG(func, preds, succs);

    std::vector<AllocaInst*> allocas;
    for (BasicBlock* bb : func->getBasicBlocks()) {
        for (Instruction* inst : bb->getInstructions()) {
            if (auto* alloca = dynamic_cast<AllocaInst*>(inst)) {
                allocas.push_back(alloca);
            }
        }
    }

    int promoted = 0;
    for (AllocaInst* alloca : allocas) {
        std::vector<Instruction*> loads;
        std::vector<Instruction*> stores;
        std::unordered_set<BasicBlock*> useBlocks;
        if (!collectUses(alloca, func, loads, stores, useBlocks)) {
            continue;
        }

        // All load/store (and alloca) in one block → no Phi needed.
        // Exclude blocks that only contain the alloca with zero load/store from
        // deciding multi-block when loads/stores share one block.
        std::unordered_set<BasicBlock*> memBlocks;
        auto markBlock = [&](Instruction* target) {
            for (BasicBlock* bb : func->getBasicBlocks()) {
                for (Instruction* inst : bb->getInstructions()) {
                    if (inst == target) {
                        memBlocks.insert(bb);
                        return;
                    }
                }
            }
        };
        for (Instruction* l : loads) {
            markBlock(l);
        }
        for (Instruction* s : stores) {
            markBlock(s);
        }
        // Alloca's block counts only when there are no loads/stores elsewhere —
        // if loads+stores are empty, still promote by deleting dead alloca.
        if (loads.empty() && stores.empty()) {
            // Dead alloca: just remove it from its block.
            for (BasicBlock* bb : func->getBasicBlocks()) {
                std::unordered_set<Instruction*> doomed{alloca};
                for (Instruction* inst : bb->getInstructions()) {
                    if (inst == alloca) {
                        removeInstructions(bb, doomed);
                        ++promoted;
                        break;
                    }
                }
            }
            continue;
        }

        if (memBlocks.size() <= 1) {
            BasicBlock* sole = *memBlocks.begin();
            // Loads/stores share one block; alloca may still live in entry.
            promoted += promoteSingleBlock(alloca, func, sole, loads, stores);
            for (BasicBlock* bb : func->getBasicBlocks()) {
                if (bb == sole) {
                    continue;
                }
                for (Instruction* inst : bb->getInstructions()) {
                    if (inst == alloca) {
                        removeInstructions(bb, {alloca});
                        break;
                    }
                }
            }
            continue;
        }

        promoted += promoteMultiBlock(alloca, func, loads, stores, preds, succs);
        // CFG may be unchanged; rebuild not required between allocas in same func
        // unless we altered terminators (we don't). Phi insertion doesn't change CFG.
    }
    return promoted;
}

int Mem2Reg::runOnModule(Module& mod, LLVMContext& ctx) {
    (void)mod;
    (void)ctx;
    return run();
}

int Mem2Reg::run() {
    int total = 0;
    for (Function* func : collectModuleFunctions(module)) {
        total += promoteFunction(func);
    }
    return total;
}
