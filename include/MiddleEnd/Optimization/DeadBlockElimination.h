#ifndef MIDDLEEND_OPTIMIZATION_DEAD_BLOCK_ELIMINATION_H
#define MIDDLEEND_OPTIMIZATION_DEAD_BLOCK_ELIMINATION_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/BasicBlock.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/GlobalValue/Function.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"

#include <vector>

// 死基本块消除 pass:
//   1. 死基本块消除: 从 entry BB 出发 BFS 做可达性标记, 删除不可达基本块。
//   2. 无条件跳转链合并: 若 BB A 的最后一条是无条件 br 到 BB B,
//      且 B 的唯一前驱是 A 且 B 不是 entry:
//       - 将 B 的所有指令(含其终止指令)并入 A, 替换掉 A 末尾的无条件 br
//       - 把其他 BB 中 PhiInstr 指向 B 的 incoming 改写为 A
//       - 从函数中删除 B
//   run() 反复执行(死块消除 -> 跳转链合并)直到不动点, 返回消除的基本块总数。
//   与常量分支化简配合使用: 常量折叠 -> 常量分支化简 -> 死基本块消除。
class DeadBlockElimination {
private:
    Module& module;

    // 计算 bb 的后继 (基于最后一条 BrInstr), 写入 succs
    void getSuccessors(BasicBlock* bb, std::vector<BasicBlock*>& succs) const;

    // 从 entry 出发 BFS 标记可达性, reachable[i] 对应 func 的第 i 个 BB
    void markReachable(Function* func, std::vector<bool>& reachable) const;

    // 删除函数内不可达基本块, 返回删除数量
    int eliminateDeadBlocks(Function* func);

    // 无条件跳转链合并, 返回合并数量
    int mergeJumps(Function* func);

public:
    explicit DeadBlockElimination(Module& module);

    // 执行死基本块消除, 返回消除的基本块总数
    int run();
};

#endif
