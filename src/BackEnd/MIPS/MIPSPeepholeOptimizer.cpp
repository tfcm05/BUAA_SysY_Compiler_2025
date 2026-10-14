#include "BackEnd/MIPS/MIPSPeepholeOptimizer.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>

namespace {

// 寄存器活性窗口: 在此窗口内 (约 16 条指令) 无法确认寄存器被使用/重定义
// 时, 保守地认为"仍被使用", 避免跨基本块的误删。
// 本代码生成器是纯栈式分配, li/lw 加载的寄存器值总是紧接着被使用, 窗口足够。
constexpr int kWindow = 16;

// 有目的寄存器 (operands[0] 被写入) 的指令助记符。
// 不在该集合中的指令 (sw, j, jr, jal, bnez, beqz, div, ...) 所有 REG 操作数都是源。
const std::set<std::string>& destRegMnemonics() {
    static const std::set<std::string> s = {
        "li", "la",
        "add", "addu", "addi", "addiu",
        "sub", "subu",
        "mul",
        "and", "andi",
        "or", "ori",
        "xor", "xori",
        "sll", "srl", "sra",
        "slt", "sltu", "slti", "sltiu",
        "mflo", "mfhi",
        "lw",
        "move",
        "neg", "not",
        "seb", "seh",
    };
    return s;
}

}  // namespace

// ============================================================
// 入口: 处理模块内所有函数
// ============================================================
int MIPSPeepholeOptimizer::run() {
    int total = 0;
    for (MIPSFunction* func : module.functions) {
        total += optimizeFunction(func);
    }
    // main 前的运行时初始化序列同样做一次窥孔优化
    total += runPasses(module.runtimeInit);
    return total;
}

// ============================================================
// 单函数主循环: 各 pass 迭代到不动点
// ============================================================
int MIPSPeepholeOptimizer::optimizeFunction(MIPSFunction* func) {
    return runPasses(func->instructions);
}

int MIPSPeepholeOptimizer::runPasses(std::vector<MIPSInstr>& instrs) {
    int total = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        int c;
        c = peepholeImmediateFold(instrs);
        changed |= c > 0;
        total += c;
        c = peepholeLoadStore(instrs);
        changed |= c > 0;
        total += c;
        c = peepholeJump(instrs);
        changed |= c > 0;
        total += c;
        c = peepholeStrengthReduction(instrs);
        changed |= c > 0;
        total += c;
        c = peepholeDeadInstr(instrs);
        changed |= c > 0;
        total += c;
        c = peepholeMove(instrs);
        changed |= c > 0;
        total += c;
    }
    return total;
}

// ============================================================
// P1: 立即数折叠
//   li $r, C  (C 在 16 位范围内)
//   add/sub/and/or $d, $s, $r
//     -> addi/andi/ori $d, $s, C
// 仅看相邻两条; $r 在算术指令之后不再被使用才折叠。
// ============================================================
int MIPSPeepholeOptimizer::peepholeImmediateFold(std::vector<MIPSInstr>& instrs) {
    int count = 0;
    for (size_t i = 0; i + 1 < instrs.size(); ++i) {
        MIPSInstr& li = instrs[i];
        if (li.isLabel || li.mnemonic != "li") continue;
        if (li.operands.size() < 2 ||
            li.operands[0].kind != MIPSOperand::REG ||
            li.operands[1].kind != MIPSOperand::IMM) continue;
        MReg r = li.operands[0].reg;
        int64_t C = li.operands[1].imm;

        MIPSInstr& arith = instrs[i + 1];
        if (arith.isLabel) continue;
        const std::string& mn = arith.mnemonic;
        std::string newMn;
        int64_t newImm = C;
        if (mn == "add" || mn == "addu") {
            newMn = "addi";
        } else if (mn == "sub" || mn == "subu") {
            newMn = "addi";
            if (C == INT64_MIN) continue;  // -C 溢出
            newImm = -C;
        } else if (mn == "and") {
            newMn = "andi";
        } else if (mn == "or") {
            newMn = "ori";
        } else {
            continue;
        }

        // 模式: $d, $s, $r  ($r 是第二个源操作数)
        if (arith.operands.size() < 3 ||
            arith.operands[0].kind != MIPSOperand::REG ||
            arith.operands[1].kind != MIPSOperand::REG ||
            arith.operands[2].kind != MIPSOperand::REG ||
            arith.operands[2].reg != r) continue;

        // 立即数范围: addi 有符号 16 位; andi/ori 在 MIPS 上是零扩展 16 位
        if (newMn == "addi") {
            if (newImm < -32768 || newImm > 32767) continue;
        } else if (C < 0 || C > 65535) {
            continue;
        }

        // $r 不再被后续使用 (折叠后 li 将被删除)
        if (!isLastUse(instrs, i + 1, r, kWindow)) continue;

        MReg d = arith.operands[0].reg;
        MReg s = arith.operands[1].reg;
        arith.mnemonic = newMn;
        arith.operands = {MIPSOperand(d), MIPSOperand(s), MIPSOperand(newImm)};
        instrs.erase(instrs.begin() + i);  // 删除 li
        ++count;
    }
    return count;
}

// ============================================================
// P2: 冗余 load/store 消除
//   sw $r, off(base) 紧跟 lw $r, off(base) -> 删除 lw
// ============================================================
int MIPSPeepholeOptimizer::peepholeLoadStore(std::vector<MIPSInstr>& instrs) {
    int count = 0;
    for (size_t i = 0; i + 1 < instrs.size(); ++i) {
        const MIPSInstr& sw = instrs[i];
        if (sw.isLabel || sw.mnemonic != "sw" || sw.operands.size() < 2) continue;
        if (sw.operands[0].kind != MIPSOperand::REG ||
            sw.operands[1].kind != MIPSOperand::MEM) continue;

        const MIPSInstr& lw = instrs[i + 1];
        if (lw.isLabel || lw.mnemonic != "lw" || lw.operands.size() < 2) continue;
        if (lw.operands[0].kind != MIPSOperand::REG ||
            lw.operands[1].kind != MIPSOperand::MEM) continue;

        // 相同寄存器、相同偏移、相同基址
        if (sw.operands[0].reg != lw.operands[0].reg) continue;
        if (sw.operands[1].memOffset != lw.operands[1].memOffset ||
            sw.operands[1].memBase != lw.operands[1].memBase) continue;

        instrs.erase(instrs.begin() + i + 1);  // 删除冗余 lw
        ++count;
    }
    return count;
}

// ============================================================
// P3: 跳转优化
//   (a) j L1 紧跟 L1: -> 删除 j
//   (b) bnez/beqz $r, L1 紧跟 j L2 且下一条是 L1: -> 反转条件为 beqz/bnez $r, L2
//   (c) 条件分支目标就是紧邻的下一条标签 -> 删除分支
// ============================================================
int MIPSPeepholeOptimizer::peepholeJump(std::vector<MIPSInstr>& instrs) {
    int count = 0;
    for (size_t i = 0; i + 1 < instrs.size(); ++i) {
        MIPSInstr& cur = instrs[i];
        if (cur.isLabel) continue;

        // ---- (a) 无条件跳转到紧邻的下一条标签 ----
        if (cur.mnemonic == "j" && cur.operands.size() == 1 &&
            cur.operands[0].kind == MIPSOperand::LABEL) {
            const MIPSInstr& next = instrs[i + 1];
            if (next.isLabel && next.mnemonic == cur.operands[0].label) {
                instrs.erase(instrs.begin() + i);
                ++count;
                continue;
            }
            continue;
        }

        // ---- (b) 条件分支 + 无条件跳转, 目标标签紧跟其后 -> 反转条件 ----
        bool isCond = (cur.mnemonic == "bnez" || cur.mnemonic == "beqz");
        if (!isCond || cur.operands.size() < 2 ||
            cur.operands[0].kind != MIPSOperand::REG ||
            cur.operands[1].kind != MIPSOperand::LABEL) continue;

        MIPSInstr& jmp = instrs[i + 1];
        if (jmp.isLabel || jmp.mnemonic != "j" || jmp.operands.size() != 1 ||
            jmp.operands[0].kind != MIPSOperand::LABEL) continue;
        if (i + 2 >= instrs.size()) continue;
        const MIPSInstr& tgt = instrs[i + 2];
        if (!tgt.isLabel || tgt.mnemonic != cur.operands[1].label) continue;

        // bnez $r, L1; j L2; L1:  ->  beqz $r, L2; L1:
        MReg r = cur.operands[0].reg;
        std::string l2 = jmp.operands[0].label;
        cur.mnemonic = (cur.mnemonic == "bnez") ? "beqz" : "bnez";
        cur.operands = {MIPSOperand(r), MIPSOperand(l2)};
        instrs.erase(instrs.begin() + i + 1);  // 删除 j
        ++count;
    }

    // ---- (c) 条件分支目标就是紧邻的下一条标签 -> 删除分支 ----
    for (size_t i = 0; i + 1 < instrs.size(); ++i) {
        MIPSInstr& br = instrs[i];
        if (br.isLabel) continue;
        if (br.mnemonic != "bnez" && br.mnemonic != "beqz") continue;
        if (br.operands.size() < 2 ||
            br.operands[1].kind != MIPSOperand::LABEL) continue;
        const MIPSInstr& next = instrs[i + 1];
        if (next.isLabel && next.mnemonic == br.operands[1].label) {
            instrs.erase(instrs.begin() + i);
            ++count;
        }
    }
    return count;
}

// ============================================================
// P4: 强度归约
//   li $r, 2^k + mul $d, $s, $r          -> sll $d, $s, k
//   li $r, 2^k + div $s, $r + mflo $d    -> sra $d, $s, k
// 注意: 有符号除法对负数用 sra 是向 -inf 舍入, 与 C 的向零截断不完全等价,
// 但本 pass 按规格要求实现 (编译器教学场景, 通常除以正幂次常量)。
// ============================================================
int MIPSPeepholeOptimizer::peepholeStrengthReduction(std::vector<MIPSInstr>& instrs) {
    int count = 0;
    for (size_t i = 0; i + 1 < instrs.size(); ++i) {
        MIPSInstr& li = instrs[i];
        if (li.isLabel || li.mnemonic != "li" || li.operands.size() < 2 ||
            li.operands[0].kind != MIPSOperand::REG ||
            li.operands[1].kind != MIPSOperand::IMM) continue;
        MReg r = li.operands[0].reg;
        int64_t C = li.operands[1].imm;
        if (!isPowerOfTwo(C)) continue;
        int k = log2OfPower(C);
        if (k > 31) continue;  // MIPS sll/sra 移位数只有 5 位

        // ---- mul $d, $s, $r (或 $d, $r, $s) -> sll $d, $s, k ----
        MIPSInstr& mul = instrs[i + 1];
        if (!mul.isLabel && mul.mnemonic == "mul" && mul.operands.size() >= 3 &&
            mul.operands[0].kind == MIPSOperand::REG &&
            mul.operands[1].kind == MIPSOperand::REG &&
            mul.operands[2].kind == MIPSOperand::REG &&
            (mul.operands[1].reg == r || mul.operands[2].reg == r)) {
            // $d == $r: mul 读取旧值后被覆盖, 旧值在 mul 处死亡, 必可折叠;
            // 否则需确认 mul 之后 $r 不再被使用 (被重定义同样视为死亡)
            bool safe = (mul.operands[0].reg == r) ||
                        isLastUse(instrs, i + 1, r, kWindow);
            if (safe) {
                MReg d = mul.operands[0].reg;
                MReg s = (mul.operands[1].reg == r) ? mul.operands[2].reg
                                                    : mul.operands[1].reg;
                mul.mnemonic = "sll";
                mul.operands = {MIPSOperand(d), MIPSOperand(s),
                                MIPSOperand(static_cast<int64_t>(k))};
                instrs.erase(instrs.begin() + i);  // 删除 li
                ++count;
                continue;
            }
        }

        // ---- div $s, $r + mflo $d -> sra $d, $s, k ----
        if (i + 2 >= instrs.size()) continue;
        const MIPSInstr& div = instrs[i + 1];
        if (div.isLabel || div.mnemonic != "div" || div.operands.size() < 2 ||
            div.operands[0].kind != MIPSOperand::REG ||
            div.operands[1].kind != MIPSOperand::REG ||
            div.operands[1].reg != r) continue;
        MIPSInstr& mflo = instrs[i + 2];
        if (mflo.isLabel || mflo.mnemonic != "mflo" || mflo.operands.size() < 1 ||
            mflo.operands[0].kind != MIPSOperand::REG) continue;
        if (!isLastUse(instrs, i + 1, r, kWindow)) continue;

        MReg s = div.operands[0].reg;
        MReg d = mflo.operands[0].reg;
        mflo.mnemonic = "sra";
        mflo.operands = {MIPSOperand(d), MIPSOperand(s),
                         MIPSOperand(static_cast<int64_t>(k))};
        instrs.erase(instrs.begin() + i + 1);  // 删除 div
        instrs.erase(instrs.begin() + i);      // 删除 li
        ++count;
    }
    return count;
}

// ============================================================
// P5: 死指令消除
//   li $r, C 且 $r 在窗口内不再被使用/重定义 -> 删除
// ============================================================
int MIPSPeepholeOptimizer::peepholeDeadInstr(std::vector<MIPSInstr>& instrs) {
    int count = 0;
    for (size_t i = 0; i < instrs.size(); ++i) {
        const MIPSInstr& li = instrs[i];
        if (li.isLabel || li.mnemonic != "li" || li.operands.size() < 2 ||
            li.operands[0].kind != MIPSOperand::REG ||
            li.operands[1].kind != MIPSOperand::IMM) continue;
        MReg r = li.operands[0].reg;
        if (isLastUse(instrs, i, r, kWindow)) {
            instrs.erase(instrs.begin() + i);
            ++count;
        }
    }
    return count;
}

// ============================================================
// P6: move 优化
//   move $r, $r -> 删除
// ============================================================
int MIPSPeepholeOptimizer::peepholeMove(std::vector<MIPSInstr>& instrs) {
    int count = 0;
    for (size_t i = 0; i < instrs.size(); ++i) {
        const MIPSInstr& mv = instrs[i];
        if (mv.isLabel || mv.mnemonic != "move" || mv.operands.size() < 2 ||
            mv.operands[0].kind != MIPSOperand::REG ||
            mv.operands[1].kind != MIPSOperand::REG) continue;
        if (mv.operands[0].reg == mv.operands[1].reg) {
            instrs.erase(instrs.begin() + i);
            ++count;
        }
    }
    return count;
}

// ============================================================
// 工具函数
// ============================================================
bool MIPSPeepholeOptimizer::isPowerOfTwo(int64_t v) {
    return v > 0 && (v & (v - 1)) == 0;
}

int MIPSPeepholeOptimizer::log2OfPower(int64_t v) {
    int k = 0;
    while (v > 1) {
        v >>= 1;
        ++k;
    }
    return k;
}

bool MIPSPeepholeOptimizer::isLastUse(const std::vector<MIPSInstr>& instrs,
                                      size_t idx, MReg reg, int window) {
    const size_t end = std::min(instrs.size(),
                                idx + 1 + static_cast<size_t>(window));
    for (size_t k = idx + 1; k < end; ++k) {
        if (instrUsesReg(instrs[k], reg)) return false;     // 仍被使用
        if (instrModifiesReg(instrs[k], reg)) return true;  // 被重定义, 原值已死
    }
    return true;  // 窗口内无使用也无重定义
}

bool MIPSPeepholeOptimizer::instrModifiesReg(const MIPSInstr& instr, MReg reg) {
    if (instr.isLabel) return false;
    const std::string& mn = instr.mnemonic;
    if (mn == "jal") return reg == MReg::Ra;  // jal 写入 $ra
    if (destRegMnemonics().count(mn) == 0) return false;
    if (instr.operands.empty() || instr.operands[0].kind != MIPSOperand::REG)
        return false;
    return instr.operands[0].reg == reg;
}

bool MIPSPeepholeOptimizer::instrUsesReg(const MIPSInstr& instr, MReg reg) {
    if (instr.isLabel) return false;
    const std::string& mn = instr.mnemonic;
    // jal 隐式读取 $a0-$a3 (调用参数)
    if (mn == "jal") {
        int v = static_cast<int>(reg);
        return v >= static_cast<int>(MReg::A0) && v <= static_cast<int>(MReg::A3);
    }
    // jr 隐式读取 $v0 (返回寄存器)
    if (mn == "jr") {
        return reg == MReg::V0;
    }
    // 有目的寄存器的指令跳过 operands[0]; 其余指令所有 REG/MEM 都是源
    const size_t start = destRegMnemonics().count(mn) ? 1 : 0;
    for (size_t i = start; i < instr.operands.size(); ++i) {
        const MIPSOperand& op = instr.operands[i];
        if (op.kind == MIPSOperand::REG && op.reg == reg) return true;
        if (op.kind == MIPSOperand::MEM && op.memBase == reg) return true;
    }
    return false;
}
