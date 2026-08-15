#ifndef MIPS_PEEPHOLE_OPTIMIZER_H
#define MIPS_PEEPHOLE_OPTIMIZER_H

#include "MIPS.h"

#include <vector>

// ============================================================
// MIPS 窥孔优化器
//
// 纯后处理 pass: 在 MIPSCodeGenerator 生成 MIPSModule 之后运行,
// 只做局部 (peephole) 变换, 不依赖 IR / 语义信息。
//
// 优化项 (按优先级):
//   P1 立即数折叠   li $r, C + add/sub/and/or $d, $s, $r
//                    -> addi/andi/ori $d, $s, C
//   P2 冗余 load    sw $r, off($fp) 紧跟 lw $r, off($fp) -> 删除 lw
//   P3 跳转优化    跳转到紧邻标签的 j 删除;
//                   bnez/beqz $r, L1 + j L2 + L1: -> 反转条件为 beqz/bnez $r, L2;
//                   条件分支目标为下一条标签 -> 删除分支
//   P4 强度归约     li $r, 2^k + mul $d, $s, $r -> sll $d, $s, k
//                   li $r, 2^k + div $s, $r + mflo $d -> sra $d, $s, k
//   P5 死指令消除   li $r, C 且 $r 在窗口内不再使用/被重定义 -> 删除
//   P6 move 优化    move $r, $r -> 删除
//
// 每个函数内各 pass 迭代到不动点 (所有变换都严格减少指令数, 必然终止)。
// 标签行 (isLabel) 永远不会被删除。
// ============================================================
class MIPSPeepholeOptimizer {
public:
    explicit MIPSPeepholeOptimizer(MIPSModule& module) : module(module) {}

    // 对模块内所有函数 (以及 runtimeInit) 执行窥孔优化, 返回总优化次数
    int run();

private:
    MIPSModule& module;

    // 单函数优化主循环: 各 pass 迭代到不动点
    int optimizeFunction(MIPSFunction* func);

    // 对一段指令序列迭代执行所有窥孔 pass, 返回优化次数
    int runPasses(std::vector<MIPSInstr>& instrs);

    // 各窥孔 pass, 每个返回本次扫描的优化次数
    int peepholeImmediateFold(std::vector<MIPSInstr>& instrs);
    int peepholeLoadStore(std::vector<MIPSInstr>& instrs);
    int peepholeJump(std::vector<MIPSInstr>& instrs);
    int peepholeStrengthReduction(std::vector<MIPSInstr>& instrs);
    int peepholeDeadInstr(std::vector<MIPSInstr>& instrs);
    int peepholeMove(std::vector<MIPSInstr>& instrs);

    // ---- 工具 ----
    static bool isPowerOfTwo(int64_t v);
    static int log2OfPower(int64_t v);

    // 指令 instrs[idx] 之后, reg 在 (idx, idx+window] 范围内既不再被读取
    // (instrUsesReg) 也不被重定义 (instrModifiesReg) 时才返回 true。
    // 被重定义同样视为"原值已死", 可安全删除定义它的指令。
    static bool isLastUse(const std::vector<MIPSInstr>& instrs, size_t idx,
                          MReg reg, int window);

    // 指令是否把 reg 作为目的寄存器写入
    static bool instrModifiesReg(const MIPSInstr& instr, MReg reg);

    // 指令是否把 reg 作为源操作数读取 (含 MEM 基址, 不含目的寄存器)
    static bool instrUsesReg(const MIPSInstr& instr, MReg reg);
};

#endif
