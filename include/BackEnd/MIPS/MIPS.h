#ifndef MIPS_H
#define MIPS_H

#include <string>
#include <vector>
#include <cstdint>

// ============================================================
// MIPS 寄存器
// ============================================================
enum class MReg {
    // 常量
    Zero = 0,   // $zero
    // 汇编临时
    At = 1,     // $at
    // 函数返回 / 系统调用
    V0 = 2, V1 = 3,
    // 参数
    A0 = 4, A1 = 5, A2 = 6, A3 = 7,
    // 临时寄存器 (调用者保存)
    T0 = 8, T1 = 9, T2 = 10, T3 = 11,
    T4 = 12, T5 = 13, T6 = 14, T7 = 15,
    // 保存寄存器 (被调用者保存)
    S0 = 16, S1 = 17, S2 = 18, S3 = 19,
    S4 = 20, S5 = 21, S6 = 22, S7 = 23,
    // 其他临时
    T8 = 24, T9 = 25,
    // 内核
    K0 = 26, K1 = 27,
    // 特殊
    Gp = 28,    // $gp 全局指针
    Sp = 29,    // $sp 栈指针
    Fp = 30,    // $fp 帧指针
    Ra = 31,    // $ra 返回地址
};

// 获取寄存器的 MIPS 汇编名称 ($t0, $s0, $sp, ...)
std::string mregName(MReg reg);

// ============================================================
// MIPS 操作数
// ============================================================
struct MIPSOperand {
    enum Kind { REG, IMM, MEM, LABEL } kind;

    // REG
    MReg reg = MReg::Zero;

    // IMM
    int64_t imm = 0;

    // MEM: offset(base)
    int memOffset = 0;
    MReg memBase = MReg::Zero;

    // LABEL
    std::string label;

    MIPSOperand(MReg r) : kind(REG), reg(r) {}
    MIPSOperand(int64_t v) : kind(IMM), imm(v) {}
    MIPSOperand(int off, MReg base) : kind(MEM), memOffset(off), memBase(base) {}
    MIPSOperand(const std::string& lbl) : kind(LABEL), label(lbl) {}

    std::string toString() const;
};

// ============================================================
// MIPS 指令
// ============================================================
struct MIPSInstr {
    std::string mnemonic;               // 指令名 (add, lw, sw, li, j, ...)
    std::vector<MIPSOperand> operands;  // 操作数
    std::string comment;                // 可选注释 (# ...)
    bool isLabel = false;               // 是否为标签行 (如 "main:")

    // 构造普通指令
    MIPSInstr(const std::string& mn, const std::vector<MIPSOperand>& ops = {},
              const std::string& cmt = "")
        : mnemonic(mn), operands(ops), comment(cmt), isLabel(false) {}

    // 构造标签
    static MIPSInstr makeLabel(const std::string& lbl) {
        MIPSInstr i("", {}, "");
        i.isLabel = true;
        i.mnemonic = lbl;
        return i;
    }

    // 构造伪指令/注释行
    static MIPSInstr makeDirective(const std::string& dir) {
        return MIPSInstr(dir, {}, "");
    }

    // 构造纯注释
    static MIPSInstr makeComment(const std::string& cmt) {
        return MIPSInstr("", {}, cmt);
    }

    std::string toString() const;
};

// ============================================================
// MIPS 函数
// ============================================================
class MIPSFunction {
public:
    std::string name;
    std::vector<MIPSInstr> instructions;  // 函数体指令
    int frameSize = 0;                      // 栈帧大小 (字节)

    MIPSFunction(const std::string& n) : name(n) {}

    void addInstr(const MIPSInstr& instr) { instructions.push_back(instr); }
};

// ============================================================
// MIPS 数据段条目
// ============================================================
struct MIPSDataEntry {
    std::string label;
    std::string directive;  // .word, .byte, .asciiz, .space
    std::string value;      // 值或表达式
    bool isString = false;  // .asciiz 否则 .word/.byte

    MIPSDataEntry(const std::string& lbl, const std::string& dir,
                  const std::string& val, bool isStr = false)
        : label(lbl), directive(dir), value(val), isString(isStr) {}
};

// ============================================================
// MIPS 模块 (完整 MIPS 汇编程序)
// ============================================================
class MIPSModule {
public:
    std::vector<MIPSDataEntry> dataEntries;   // .data 段
    std::vector<MIPSFunction*> functions;     // .text 段函数
    std::vector<MIPSInstr> runtimeInit;        // main 前的运行时初始化

    ~MIPSModule() {
        for (auto* f : functions) delete f;
    }

    void addDataEntry(const MIPSDataEntry& entry) { dataEntries.push_back(entry); }
    void addFunction(MIPSFunction* func) { functions.push_back(func); }
};

// ============================================================
// MIPS 输出器
// ============================================================
class MIPSPrinter {
public:
    void print(const MIPSModule& module, std::ostream& os) const;
};

#endif