#ifndef MIDDLEEND_OPTIMIZATION_DEAD_INSTRUCTION_ELIMINATION_H
#define MIDDLEEND_OPTIMIZATION_DEAD_INSTRUCTION_ELIMINATION_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"

#include <set>

// 死指令消除 pass:
//   遍历所有 Function 的所有 BasicBlock, 删除"结果无任何引用且无副作用"的指令。
//
// 可安全删除 (白名单, 纯计算 / 内存读 / 栈分配, 无副作用):
//   AddInstr / SubInstr / MulInstr / SdivInstr / AndInstr / OrInstr /
//   IcmpInstr / ZextInstr / LoadInst / AllocaInst
//   (LoadInst 有内存读语义, 但无别名分析的简单 pass 中保守假设只读;
//    AllocaInst 无 use 时同样删除)
//
// 不可删除 (有副作用 / 终止指令 / 保守保留):
//   StoreInst (写内存), CallInstr (可能被观察), RetInstr / BrInstr (终止),
//   PhiInstr (保持结构一致), GetElementPtrInst (指针计算, 保守保留)
//
// 实现策略:
//   1. 统计当前 Function 中所有指令作为操作数引用的 Value* 集合
//      (架构中 User::usees / Value::users 未维护, 需手工遍历操作数)
//   2. 删除不在引用集合中的可删指令 (仅从 BB 指令列表移除, 不 delete,
//      内存由 IRBuilder 统一管理)
//   3. 重复至 fixpoint (删除一条指令会使操作数引用数下降)
//
// 注意: 指令对象由 IRBuilder::ownedValue 持有, pass 只做列表级删除,
//       不能释放指令内存, 否则 IRBuilder 析构时 double-free。
class DeadInstructionElimination {
private:
    Module& module;

    // 判断指令是否为可安全删除类型 (白名单)。
    static bool isRemovable(Instruction* instr);

    // 收集一条指令的所有操作数 (被引用 Value*)。
    // 常量 / 参数 / 全局变量 / BasicBlock 也一并收集, 便于统一用
    // "指针是否在集合中"判断指令是否被引用。
    static void collectOperands(Instruction* instr, std::set<Value*>& referenced);

    // 对单个 Function 反复扫描至 fixpoint, 返回删除的指令数。
    static int eliminateInFunction(Function* func);

public:
    explicit DeadInstructionElimination(Module& module);

    // 对所有 Function (含 MainFunction, 若不在 functions 列表中) 执行死指令消除,
    // 返回总共删除的指令数。
    int run();
};

#endif
