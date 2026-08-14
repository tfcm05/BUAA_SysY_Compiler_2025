#ifndef MIPS_CODE_GENERATOR_H
#define MIPS_CODE_GENERATOR_H

#include "../MIPS.h"
#include "../../MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "../../MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"

#include <unordered_map>

// ============================================================
// MIPS 代码生成器
//
// 遍历 LLVM IR Module, 生成 MIPS 汇编:
//   1. 全局变量 / 字符串常量 → .data 段
//   2. 每个函数 → .text 段, 含 prologue/epilogue
//   3. 每条 LLVM IR 指令 → 对应 MIPS 指令序列
//
// 寄存器分配策略 (栈式):
//   - 每个 LLVM IR Value 分配一个栈帧槽位 (4 字节)
//   - $t0-$t2 作为临时寄存器, 按需 load/store
//   - 函数参数: $a0-$a3 传入, prologue 中存到栈帧
//   - 函数返回: $v0
//   - $ra 在 prologue 中保存, epilogue 中恢复
//
// 栈帧布局 (高地址 → 低地址):
//   $fp → [$ra]         offset 0
//          [$fp_saved]   offset 4
//          [local var 0]  offset 8
//          [local var 1]  offset 12
//          ...
//   $sp → frame bottom
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

    // ---- 全局变量与字符串 ----
    void generateGlobalVariables();
    void generateStringConstants();

    // ---- 函数生成 ----
    void generateFunction(Function* func);

    // ---- 基本块生成 ----
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
    void genPhi(MIPSFunction* mipsFunc, PhiInstr* instr);

    // ---- 栈帧管理 ----
    struct FrameInfo {
        int frameSize = 0;       // 总栈帧大小
        int nextOffset = 8;      // 下一个可用 offset (跳过 $ra 和 $fp)
        std::unordered_map<const Value*, int> valueOffset;  // Value* → 栈偏移
    };
    std::unordered_map<Function*, FrameInfo> frameMap;
    std::unordered_map<BasicBlock*, std::string> bbLabels;  // BB → MIPS 标签

    // 为函数内所有 Value 分配栈槽
    void allocateFrame(Function* func);
    // 获取 Value 在栈帧中的偏移
    int getOffset(const Value* val, Function* func);
    // 获取 BB 对应的 MIPS 标签
    std::string getBBLabel(BasicBlock* bb);
    // 获取函数对应的 MIPS 标签
    std::string getFuncLabel(Function* func);
    // 获取全局变量的 MIPS 标签
    std::string getGlobalLabel(GlobalVariable* gv);
    // 获取数组类型占用的字节数
    size_t getTypeSize(LLVM::Type* type);
    // 获取数组元素的对齐大小
    int getElementSize(LLVM::Type* type);

    // ---- 临时寄存器管理 ----
    // 将 Value 的值加载到指定寄存器
    void loadValue(MIPSFunction* mipsFunc, const Value* val, MReg dst, Function* func);
    // 将寄存器值存储到 Value 的栈槽
    void storeValue(MIPSFunction* mipsFunc, MReg src, const Value* val, Function* func);
    // 将地址 (栈槽地址 or 全局地址) 加载到寄存器
    void loadAddress(MIPSFunction* mipsFunc, const Value* val, MReg dst, Function* func);
};

#endif