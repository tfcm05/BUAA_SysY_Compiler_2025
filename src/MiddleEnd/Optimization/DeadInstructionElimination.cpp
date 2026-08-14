#include "MiddleEnd/Optimization/DeadInstructionElimination.h"

DeadInstructionElimination::DeadInstructionElimination(Module& module) : module(module) {}

bool DeadInstructionElimination::isRemovable(Instruction* instr) {
    // 白名单: 纯计算 / 内存读 / 栈分配, 无副作用。
    // LoadInst 有内存读语义, 但无别名分析时保守假设其只读;
    // AllocaInst 无 use 时同样删除 (其内存写者 store 会保留)。
    return dynamic_cast<AddInstr*>(instr) || dynamic_cast<SubInstr*>(instr) ||
           dynamic_cast<MulInstr*>(instr) || dynamic_cast<SdivInstr*>(instr) ||
           dynamic_cast<AndInstr*>(instr) || dynamic_cast<OrInstr*>(instr) ||
           dynamic_cast<ZextInstr*>(instr) || dynamic_cast<IcmpInstr*>(instr) ||
           dynamic_cast<LoadInst*>(instr) || dynamic_cast<AllocaInst*>(instr);
}

void DeadInstructionElimination::collectOperands(Instruction* instr, std::set<Value*>& referenced) {
    auto add = [&referenced](Value* v) {
        if (v) {
            referenced.insert(v);
        }
    };

    // 二元算术 / 逻辑 / 比较
    if (auto* i = dynamic_cast<AddInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<SubInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<MulInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<SdivInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<AndInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<OrInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<IcmpInstr*>(instr)) {
        add(i->getLHS());
        add(i->getRHS());
    } else if (auto* i = dynamic_cast<ZextInstr*>(instr)) {
        add(i->getOperand());
    }
    // 内存访问
    else if (auto* i = dynamic_cast<LoadInst*>(instr)) {
        add(i->getPointer());
    } else if (auto* i = dynamic_cast<StoreInst*>(instr)) {
        add(i->getValue());
        add(i->getPointer());
    } else if (auto* i = dynamic_cast<GetElementPtrInst*>(instr)) {
        add(i->getPointer());
        for (size_t k = 0; k < i->getIndicesCount(); ++k) {
            add(i->getIndex(k));
        }
    }
    // 控制流
    else if (auto* i = dynamic_cast<BrInstr*>(instr)) {
        add(i->getCondition()); // trueBB/falseBB 为 BasicBlock, 本 pass 不删 BB
    } else if (auto* i = dynamic_cast<RetInstr*>(instr)) {
        add(i->getReturnValue());
    } else if (auto* i = dynamic_cast<PhiInstr*>(instr)) {
        for (size_t k = 0; k < i->getIncomingCount(); ++k) {
            add(i->getIncoming(k).first);
        }
    }
    // 函数调用
    else if (auto* i = dynamic_cast<CallInstr*>(instr)) {
        for (size_t k = 0; k < i->getArgumentsCount(); ++k) {
            add(i->getArgument(k));
        }
    }
    // AllocaInst 无操作数
}

int DeadInstructionElimination::eliminateInFunction(Function* func) {
    int removed = 0;
    bool changed = true;
    while (changed) {
        changed = false;

        // 统计当前状态下所有被引用的 Value* (含常量/参数/全局/指令)
        std::set<Value*> referenced;
        const auto& bbs = func->getBasicBlocks();
        for (auto* bb : bbs) {
            for (auto* inst : bb->getInstructions()) {
                collectOperands(inst, referenced);
            }
        }

        // 删除无引用且可安全删除的指令
        for (auto* bb : bbs) {
            const auto& insts = bb->getInstructions();
            if (insts.empty()) {
                continue;
            }
            std::vector<Instruction*> kept;
            kept.reserve(insts.size());
            for (auto* inst : insts) {
                if (isRemovable(inst) && referenced.find(inst) == referenced.end()) {
                    ++removed;
                    changed = true;
                } else {
                    kept.push_back(inst);
                }
            }
            if (kept.size() != insts.size()) {
                bb->setInstructions(kept);
                // setInstructions 不维护 terminatorFlag, 手动恢复
                bb->setTerminator(!kept.empty() && kept.back()->isTerminator());
            }
        }
    }
    return removed;
}

int DeadInstructionElimination::run() {
    int total = 0;

    // 处理 functions 列表中的全部函数 (声明无 basicBlocks, 自动跳过)
    for (auto* func : module.getFunctions()) {
        if (!func || func->getBasicBlocks().empty()) {
            continue;
        }
        total += eliminateInFunction(func);
    }

    // MainFunction 单独注册, 不在 functions 列表时也需处理 (重复处理无副作用)
    if (auto* main = module.getMainFunction()) {
        if (!main->getBasicBlocks().empty()) {
            total += eliminateInFunction(main);
        }
    }

    return total;
}
