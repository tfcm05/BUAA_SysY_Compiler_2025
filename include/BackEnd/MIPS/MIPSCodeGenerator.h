#ifndef MIPS_CODE_GENERATOR_H
#define MIPS_CODE_GENERATOR_H

#include "MIPS.h"
#include "../../MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "../../MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"

#include <set>
#include <unordered_map>
#include <vector>

// ============================================================
// MIPS 代码生成器 (基本块级寄存器缓存)
//
// 遍历 LLVM IR Module, 生成 MIPS 汇编:
//   1. 全局变量 / 字符串常量 → .data 段
//   2. 每个函数 → .text 段, 含 prologue/epilogue
//   3. 每条 LLVM IR 指令 → 对应 MIPS 指令序列
//
// 寄存器分配策略 (基本块级缓存):
//   - T0-T7 作为 BB 级寄存器缓存池, 维护 Value* -> MReg 映射
//   - 每个 LLVM IR Value 仍分配栈帧槽位作为溢出目的地
//   - 指令结果优先留在寄存器中, BB 结束时统一写回栈帧
//   - 函数调用前清空缓存 (保护 caller-saved 寄存器)
//   - Phi 在代码生成前消除: 每个前驱 BB 的 terminator 前写 phi 栈槽
//
// 栈帧布局 (fp 指向帧底, offset 相对 fp):
//   $fp → [$ra]         offset 0
//          [$fp_saved]   offset 4
//          [参数/局部]   offset 8+
//   $sp → frame bottom (fp = sp)
// ============================================================
class MIPSCodeGenerator {
public:
    MIPSCodeGenerator(Module& module, LLVMContext& context)
        : module(module), context(context) {}

    // 生成 MIPS 代码, 返回完整模块
    std::unique_ptr<MIPSModule> run();

private:
    Module& module;
    LLVMContext& context;
    std::unique_ptr<MIPSModule> mipsModule;

    // ---- 数据段生成 ----
    void generateGlobalVariables();
    void generateStringConstants();
    void collectGlobalWords(const Constant* c, std::vector<int64_t>& out) const;

    // ---- 函数 / 基本块生成 ----
    void generateFunction(Function* func);
    void generateBasicBlock(MIPSFunction* mipsFunc, BasicBlock* bb);

    // ---- 指令生成 (分派) ----
    void generateInstruction(MIPSFunction* mipsFunc, Instruction* instr);
    void genArithmetic(MIPSFunction* mipsFunc, Instruction* instr);
    void genIcmp(MIPSFunction* mipsFunc, IcmpInstr* instr);
    void genBr(MIPSFunction* mipsFunc, BrInstr* instr);
    void genRet(MIPSFunction* mipsFunc, RetInstr* instr);
    void genCall(MIPSFunction* mipsFunc, CallInstr* instr);
    void genAlloca(MIPSFunction* mipsFunc, AllocaInst* instr);
    void genLoad(MIPSFunction* mipsFunc, LoadInst* instr);
    void genStore(MIPSFunction* mipsFunc, StoreInst* instr);
    void genGEP(MIPSFunction* mipsFunc, GetElementPtrInst* instr);
    void genZext(MIPSFunction* mipsFunc, ZextInstr* instr);

    // ---- 栈帧管理 ----
    struct FrameInfo {
        int frameSize = 0;       // 总栈帧大小
        int nextOffset = 8;      // 下一个可用 offset (跳过 $ra 和 $fp_saved)
        std::unordered_map<const Value*, int> valueOffset;  // Value* → 栈偏移
    };
    std::unordered_map<Function*, FrameInfo> frameMap;
    std::unordered_map<BasicBlock*, std::string> bbLabels;  // BB → MIPS 标签
    Function* currentFunc = nullptr;   // 当前正在生成的函数 (去掉 O(n) 扫描)

    void allocateFrame(Function* func);
    int getOffset(const Value* val) const;
    bool hasStackSlot(const Value* val) const;
    std::string getBBLabel(BasicBlock* bb) const;
    std::string getFuncLabel(const Function* func) const;
    std::string getGlobalLabel(const GlobalVariable* gv) const;
    size_t getTypeSize(LLVM::Type* type) const;
    LLVM::Type* gepAdvance(LLVM::Type* cur) const;
    bool isI8Type(LLVM::Type* type) const;

    // ---- 寄存器缓存 (BB 级) ----
    static constexpr int NUM_T_REGS = 8;   // T0-T7
    std::unordered_map<const Value*, MReg> regCache;  // Value → T 寄存器
    std::unordered_map<MReg, const Value*> regOwner;  // T 寄存器 → Value
    bool regBusy[NUM_T_REGS] = {false};               // T0-T7 占用标记
    MIPSFunction* curMipsFunc = nullptr;              // 当前输出函数

    int regIndex(MReg r) const {
        return static_cast<int>(r) - static_cast<int>(MReg::T0);
    }
    bool isTReg(MReg r) const {
        int v = static_cast<int>(r);
        return v >= static_cast<int>(MReg::T0) && v <= static_cast<int>(MReg::T7);
    }

    MReg allocReg(const std::set<MReg>& avoid = {});
    void flushRegCache();
    void cacheResult(const Value* val, MReg reg);
    MReg loadValue(const Value* val, const std::set<MReg>& avoid = {});
    MReg loadValue(const Value* val, MReg dst);   // 强制加载到指定寄存器 (不缓存)
    MReg loadAddress(const Value* val, const std::set<MReg>& avoid = {});
    void loadValueInto(const Value* val, MReg reg, bool isAddress);

    // ---- Phi 消除 ----
    // 前驱 BB → (phi 结果, incoming 值) 列表
    std::unordered_map<BasicBlock*, std::vector<std::pair<Value*, Value*>>> phiCopies;
    void collectPhiCopies(Function* func);

    // ---- 工具 ----
    void emitInstr(const MIPSInstr& instr) { curMipsFunc->addInstr(instr); }
    static bool isSmallImm(int64_t v) { return v >= -32768 && v <= 32767; }
    static bool isUnsignedImm16(int64_t v) { return v >= 0 && v <= 65535; }
    static bool isConstantIntVal(const Value* v, int64_t& out);
    void emitEpilogue(MIPSFunction* mipsFunc, int frameSize);
};

#endif
