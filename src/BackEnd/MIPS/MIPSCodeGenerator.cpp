#include "../../../include/BackEnd/MIPS/MIPS.h"
#include "../../../include/BackEnd/MIPS/MIPSCodeGenerator.h"

#include <sstream>
#include <iomanip>

// ============================================================
// MIPS 寄存器名称
// ============================================================
std::string mregName(MReg reg) {
    static const char* names[] = {
        "$zero", "$at", "$v0", "$v1",
        "$a0", "$a1", "$a2", "$a3",
        "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
        "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
        "$t8", "$t9", "$k0", "$k1",
        "$gp", "$sp", "$fp", "$ra"
    };
    return names[static_cast<int>(reg)];
}

// ============================================================
// MIPS Operand
// ============================================================
std::string MIPSOperand::toString() const {
    switch (kind) {
    case REG:
        return mregName(reg);
    case IMM:
        return std::to_string(imm);
    case MEM:
        return std::to_string(memOffset) + "(" + mregName(memBase) + ")";
    case LABEL:
        return label;
    }
    return "?";
}

// ============================================================
// MIPS Instruction
// ============================================================
std::string MIPSInstr::toString() const {
    if (isLabel) {
        return mnemonic + ":";
    }
    std::ostringstream oss;
    oss << "    " << mnemonic;
    for (size_t i = 0; i < operands.size(); ++i) {
        if (i > 0) oss << ", ";
        else oss << " ";
        oss << operands[i].toString();
    }
    if (!comment.empty()) {
        oss << "    # " << comment;
    }
    return oss.str();
}

// ============================================================
// MIPSPrinter
// ============================================================
void MIPSPrinter::print(const MIPSModule& module, std::ostream& os) const {
    // 数据段
    if (!module.dataEntries.empty()) {
        os << ".data\n\n";
        for (const auto& entry : module.dataEntries) {
            os << entry.label << ":\n";
            os << "    " << entry.directive << "  " << entry.value << "\n";
        }
        os << "\n";
    }

    // 文本段
    os << ".text\n\n";
    for (const auto* func : module.functions) {
        os << "    .globl " << func->name << "\n";
        os << func->name << ":\n";
        for (const auto& instr : func->instructions) {
            os << instr.toString() << "\n";
        }
        os << "\n";
    }
}

// ============================================================
// MIPSCodeGenerator
// ============================================================
std::unique_ptr<MIPSModule> MIPSCodeGenerator::run() {
    mipsModule = std::make_unique<MIPSModule>();

    // 1. 生成数据段 (全局变量 + 字符串常量)
    generateGlobalVariables();
    generateStringConstants();

    // 2. 为每个函数分配栈帧
    for (const auto* func : module.getFunctions()) {
        allocateFrame(const_cast<Function*>(func));
    }

    // 3. 生成代码段
    for (const auto* func : module.getFunctions()) {
        generateFunction(const_cast<Function*>(func));
    }

    return std::move(mipsModule);
}

// ============================================================
// 数据段生成
// ============================================================
void MIPSCodeGenerator::generateGlobalVariables() {
    for (const auto* gv : module.getGlobalVariables()) {
        std::string label = getGlobalLabel(const_cast<GlobalVariable*>(gv));

        // 根据初始化器类型生成
        const auto* init = gv->getInitializer();
        if (!init) {
            // 未初始化: .space
            size_t size = getTypeSize(gv->getType());
            mipsModule->addDataEntry({label, ".space", std::to_string(size)});
            continue;
        }

        if (init->isConstantInt()) {
            // 整数全局变量: .word
            int64_t val = static_cast<const ConstantInt*>(init)->getValue();
            mipsModule->addDataEntry({label, ".word", std::to_string(val)});
        } else {
            // 数组或其他: .space (简化)
            size_t size = getTypeSize(gv->getType());
            mipsModule->addDataEntry({label, ".space", std::to_string(size)});
        }
    }
}

void MIPSCodeGenerator::generateStringConstants() {
    for (const auto* gv : module.getGlobalConstStrs()) {
        std::string label = getGlobalLabel(const_cast<GlobalVariable*>(gv));
        const auto* init = gv->getInitializer();
        if (init && init->isConstString()) {
            std::string str = static_cast<const ConstString*>(init)->getString();
            // 转义
            std::string escaped;
            for (char c : str) {
                switch (c) {
                case '\n': escaped += "\\n"; break;
                case '\t': escaped += "\\t"; break;
                case '\\': escaped += "\\\\"; break;
                case '"':  escaped += "\\\""; break;
                case '\0': escaped += "\\0"; break;
                default:   escaped += c; break;
                }
            }
            mipsModule->addDataEntry({label, ".asciiz", "\"" + escaped + "\"", true});
        }
    }
}

// ============================================================
// 栈帧管理
// ============================================================
void MIPSCodeGenerator::allocateFrame(Function* func) {
    FrameInfo info;
    info.nextOffset = 8;  // 0: $ra, 4: $fp_saved

    // 为参数分配栈槽
    for (size_t i = 0; i < func->getArgumentsCount(); ++i) {
        Argument* arg = func->getArgument(i);
        info.valueOffset[arg] = info.nextOffset;
        info.nextOffset += 4;
    }

    // 为所有指令的结果分配栈槽
    for (size_t i = 0; i < func->getBasicBlocksCount(); ++i) {
        BasicBlock* bb = func->getBasicBlock(i);
        for (size_t j = 0; j < bb->getInstructionsCount(); ++j) {
            Instruction* instr = bb->getInstruction(j);
            // 所有 Instruction 都是 Value, 都可能被其他指令引用
            info.valueOffset[instr] = info.nextOffset;
            info.nextOffset += 4;  // 简化: 每个值 4 字节
        }
    }

    // 栈帧大小对齐到 8 字节
    info.frameSize = ((info.nextOffset + 4 + 7) / 8) * 8;  // +4 for alignment padding
    frameMap[func] = info;

    // 为 BB 生成标签
    for (size_t i = 0; i < func->getBasicBlocksCount(); ++i) {
        BasicBlock* bb = func->getBasicBlock(i);
        bbLabels[bb] = getFuncLabel(func) + "." + bb->getName();
    }
}

int MIPSCodeGenerator::getOffset(const Value* val, Function* func) {
    auto& info = frameMap[func];
    auto it = info.valueOffset.find(val);
    if (it != info.valueOffset.end()) {
        return it->second;
    }
    return 0;  // 不应发生
}

std::string MIPSCodeGenerator::getBBLabel(BasicBlock* bb) {
    return bbLabels[bb];
}

std::string MIPSCodeGenerator::getFuncLabel(Function* func) {
    return func->getName();
}

std::string MIPSCodeGenerator::getGlobalLabel(GlobalVariable* gv) {
    return gv->getName();
}

size_t MIPSCodeGenerator::getTypeSize(LLVM::Type* type) {
    if (!type) return 0;
    switch (type->getTypeID()) {
    case LLVM::Type::VoidTyID:   return 0;
    case LLVM::Type::IntegerTyID: return 4;  // i32 or i8 (简化)
    case LLVM::Type::PointerTyID: return 4;   // 32 位指针
    case LLVM::Type::ArrayTyID: {
        auto* arrType = static_cast<LLVM::ArrayType*>(type);
        return arrType->getSize() * getTypeSize(arrType->getElementType());
    }
    case LLVM::Type::FunctionTyID: return 4;  // 函数指针
    default: return 4;
    }
}

int MIPSCodeGenerator::getElementSize(LLVM::Type* type) {
    if (!type) return 4;
    switch (type->getTypeID()) {
    case LLVM::Type::IntegerTyID: {
        auto* intType = static_cast<LLVM::IntegerType*>(type);
        // i8 = 1 byte, i32 = 4 bytes
        return intType->getBitWidth() == 8 ? 1 : 4;
    }
    case LLVM::Type::PointerTyID: return 4;
    case LLVM::Type::ArrayTyID:
    case LLVM::Type::VoidTyID:
    case LLVM::Type::FunctionTyID:
    default: return 4;
    }
}

// ============================================================
// 值加载与存储
// ============================================================
void MIPSCodeGenerator::loadValue(MIPSFunction* mipsFunc, const Value* val,
                                   MReg dst, Function* func) {
    if (!val) {
        mipsFunc->addInstr(MIPSInstr("li", {MReg(0)}, "null value"));
        return;
    }

    if (val->isConstantInt()) {
        int64_t imm = static_cast<const ConstantInt*>(val)->getValue();
        mipsFunc->addInstr(MIPSInstr("li", {MReg(dst), MIPSOperand(imm)}));
        return;
    }

    if (val->isBasicBlock()) {
        // BB 作为跳转目标
        return;
    }

    if (val->isFunction()) {
        auto* callee = const_cast<Function*>(static_cast<const Function*>(val));
        mipsFunc->addInstr(MIPSInstr("la", {MReg(dst),
            MIPSOperand(getFuncLabel(callee))}));
        return;
    }

    // 从栈帧加载
    int offset = getOffset(val, func);
    mipsFunc->addInstr(MIPSInstr("lw", {MReg(dst),
        MIPSOperand(offset, MReg(Fp))}));
}

void MIPSCodeGenerator::storeValue(MIPSFunction* mipsFunc, MReg src,
                                     const Value* val, Function* func) {
    if (!val) return;

    if (val->isGlobalVariable()) {
        auto* gv = const_cast<GlobalVariable*>(static_cast<const GlobalVariable*>(val));
        std::string label = getGlobalLabel(gv);
        mipsFunc->addInstr(MIPSInstr("la", {MReg(T2), MIPSOperand(label)}));
        mipsFunc->addInstr(MIPSInstr("sw", {MReg(src),
            MIPSOperand(0, MReg(T2))}));
        return;
    }

    int offset = getOffset(val, func);
    mipsFunc->addInstr(MIPSInstr("sw", {MReg(src),
        MIPSOperand(offset, MReg(Fp))}));
}

void MIPSCodeGenerator::loadAddress(MIPSFunction* mipsFunc, const Value* val,
                                      MReg dst, Function* func) {
    if (val->isGlobalVariable()) {
        auto* gv = const_cast<GlobalVariable*>(static_cast<const GlobalVariable*>(val));
        mipsFunc->addInstr(MIPSInstr("la", {MReg(dst),
            MIPSOperand(getGlobalLabel(gv))}));
        return;
    }

    // 局部变量的栈地址
    int offset = getOffset(val, func);
    mipsFunc->addInstr(MIPSInstr("addiu", {MReg(dst),
        MReg(Fp), MIPSOperand(offset)}));
}

// ============================================================
// 函数生成
// ============================================================
void MIPSCodeGenerator::generateFunction(Function* func) {
    auto* mipsFunc = new MIPSFunction(getFuncLabel(func));
    const auto& info = frameMap[func];

    // ---- Prologue ----
    mipsFunc->addInstr(MIPSInstr::makeComment("=== prologue ==="));
    mipsFunc->addInstr(MIPSInstr("addiu", {MReg(Sp), MReg(Sp),
        MIPSOperand(-info.frameSize)}));
    mipsFunc->addInstr(MIPSInstr("sw", {MReg(Ra),
        MIPSOperand(0, MReg(Sp))}));
    mipsFunc->addInstr(MIPSInstr("sw", {MReg(Fp),
        MIPSOperand(4, MReg(Sp))}));
    mipsFunc->addInstr(MIPSInstr("addiu", {MReg(Fp), MReg(Sp),
        MIPSOperand(info.frameSize)}));

    // 将参数从 $a0-$a3 存到栈帧
    for (size_t i = 0; i < func->getArgumentsCount() && i < 4; ++i) {
        Argument* arg = func->getArgument(i);
        MReg argReg = static_cast<MReg>(static_cast<int>(MReg::A0) + i);
        int offset = getOffset(arg, func);
        mipsFunc->addInstr(MIPSInstr("sw", {MReg(argReg),
            MIPSOperand(offset, MReg(Fp))}));
    }
    // 超过 4 个参数: 从调用者的栈帧获取 (简化: 暂不支持)

    // ---- 生成基本块 ----
    for (size_t i = 0; i < func->getBasicBlocksCount(); ++i) {
        BasicBlock* bb = func->getBasicBlock(i);
        generateBasicBlock(mipsFunc, bb);
    }

    // ---- Epilogue (保底) ----
    // 如果最后一条不是 ret, 加一个默认 epilogue
    if (!func->getBasicBlocks().empty()) {
        auto* lastBB = func->getBasicBlock(func->getBasicBlocksCount() - 1);
        if (!lastBB->isTerminator() || lastBB->getInstructionsCount() == 0) {
            mipsFunc->addInstr(MIPSInstr::makeComment("=== default epilogue ==="));
            mipsFunc->addInstr(MIPSInstr("lw", {MReg(Ra),
                MIPSOperand(0, MReg(Fp))}));
            mipsFunc->addInstr(MIPSInstr("lw", {MReg(Fp),
                MIPSOperand(4, MReg(Fp))}));
            // 注意: sp 恢复需要在 fp 恢复前用 fp 计算
            // sp = fp + frameSize - frameSize = fp (原始) ... 简化处理
            mipsFunc->addInstr(MIPSInstr("addiu", {MReg(Sp), MReg(Fp),
                MIPSOperand(info.frameSize)}));
            mipsFunc->addInstr(MIPSInstr("jr", {MReg(Ra)}));
        }
    }

    mipsModule->addFunction(mipsFunc);
}

// ============================================================
// 基本块生成
// ============================================================
void MIPSCodeGenerator::generateBasicBlock(MIPSFunction* mipsFunc, BasicBlock* bb) {
    // BB 标签
    mipsFunc->addInstr(MIPSInstr::makeLabel(getBBLabel(bb)));

    // 逐条指令生成
    for (size_t i = 0; i < bb->getInstructionsCount(); ++i) {
        Instruction* instr = bb->getInstruction(i);
        generateInstruction(mipsFunc, instr);
    }
}

// ============================================================
// 指令分派
// ============================================================
void MIPSCodeGenerator::generateInstruction(MIPSFunction* mipsFunc, Instruction* instr) {
    // 根据 ValueKind 分派
    // Instruction 子类: AddInstr, SubInstr, MulInstr, SdivInstr,
    //   AndInstr, OrInstr, IcmpInstr, ZextInstr, BrInstr, PhiInstr,
    //   RetInstr, CallInstr, AllocaInst, LoadInst, StoreInst, GetElementPtrInst

    // 算术逻辑指令 (都有 getLHS/getRHS)
    if (dynamic_cast<AddInstr*>(instr) ||
        dynamic_cast<SubInstr*>(instr) ||
        dynamic_cast<MulInstr*>(instr) ||
        dynamic_cast<SdivInstr*>(instr) ||
        dynamic_cast<AndInstr*>(instr) ||
        dynamic_cast<OrInstr*>(instr)) {
        genArithmetic(mipsFunc, instr);
    } else if (auto* icmp = dynamic_cast<IcmpInstr*>(instr)) {
        genIcmp(mipsFunc, icmp);
    } else if (auto* br = dynamic_cast<BrInstr*>(instr)) {
        genBr(mipsFunc, br);
    } else if (auto* ret = dynamic_cast<RetInstr*>(instr)) {
        genRet(mipsFunc, ret);
    } else if (auto* call = dynamic_cast<CallInstr*>(instr)) {
        genCall(mipsFunc, call);
    } else if (auto* alloca = dynamic_cast<AllocaInst*>(instr)) {
        genAlloca(mipsFunc, alloca);
    } else if (auto* load = dynamic_cast<LoadInst*>(instr)) {
        genLoad(mipsFunc, load);
    } else if (auto* store = dynamic_cast<StoreInst*>(instr)) {
        genStore(mipsFunc, store);
    } else if (auto* gep = dynamic_cast<GetElementPtrInst*>(instr)) {
        genGEP(mipsFunc, gep);
    } else if (auto* zext = dynamic_cast<ZextInstr*>(instr)) {
        genZext(mipsFunc, zext);
    } else if (auto* phi = dynamic_cast<PhiInstr*>(instr)) {
        genPhi(mipsFunc, phi);
    } else {
        mipsFunc->addInstr(MIPSInstr::makeComment(
            "TODO: unhandled instruction type"));
    }
}

// ============================================================
// 算术逻辑指令
// ============================================================
void MIPSCodeGenerator::genArithmetic(MIPSFunction* mipsFunc, Instruction* instr) {
    // 获取操作数 (所有算术指令都有 getLHS/getRHS)
    Value* lhs = nullptr;
    Value* rhs = nullptr;
    const char* mnem = nullptr;

    if (auto* a = dynamic_cast<AddInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "add";
    } else if (auto* a = dynamic_cast<SubInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "sub";
    } else if (auto* a = dynamic_cast<MulInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "mul";
    } else if (auto* a = dynamic_cast<SdivInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "div";
    } else if (auto* a = dynamic_cast<AndInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "and";
    } else if (auto* a = dynamic_cast<OrInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "or";
    } else {
        return;
    }

    // 获取当前函数 (通过栈帧信息反查 — 简化: 使用第一个包含该 instr 的函数)
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) {
            func = f;
            break;
        }
    }
    if (!func) return;

    // 加载操作数到 $t0, $t1
    loadValue(mipsFunc, lhs, MReg(T0), func);
    loadValue(mipsFunc, rhs, MReg(T1), func);

    // 生成指令
    if (strcmp(mnem, "div") == 0) {
        // MIPS div: 结果在 LO 寄存器, 用 mflo 取出
        mipsFunc->addInstr(MIPSInstr("div", {MReg(T0), MReg(T1)},
            "div $t0 / $t1"));
        mipsFunc->addInstr(MIPSInstr("mflo", {MReg(T2)}));
    } else {
        mipsFunc->addInstr(MIPSInstr(mnem,
            {MReg(T2), MReg(T0), MReg(T1)}));
    }

    // 存储结果
    storeValue(mipsFunc, MReg(T2), instr, func);
}

// ============================================================
// Icmp 指令
// ============================================================
void MIPSCodeGenerator::genIcmp(MIPSFunction* mipsFunc, IcmpInstr* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    loadValue(mipsFunc, instr->getLHS(), MReg(T0), func);
    loadValue(mipsFunc, instr->getRHS(), MReg(T1), func);

    // icmp 结果: 0 (false) 或 1 (true)
    // slt $t2, $t0, $t1  → $t2 = ($t0 < $t1) ? 1 : 0
    std::string op;
    switch (instr->getPredicate()) {
    case IcmpInstr::IcmpOp::EQ:
        mipsFunc->addInstr(MIPSInstr("xor", {MReg(T2), MReg(T0),
            MReg(T1)}));
        mipsFunc->addInstr(MIPSInstr("sltiu", {MReg(T2), MReg(T2),
            MIPSOperand(1)}));
        break;
    case IcmpInstr::IcmpOp::NE:
        mipsFunc->addInstr(MIPSInstr("xor", {MReg(T2), MReg(T0),
            MReg(T1)}));
        mipsFunc->addInstr(MIPSInstr("sltu", {MReg(T2), MReg(Zero),
            MReg(T2)}));
        break;
    case IcmpInstr::IcmpOp::SLT:
        mipsFunc->addInstr(MIPSInstr("slt", {MReg(T2), MReg(T0),
            MReg(T1)}));
        break;
    case IcmpInstr::IcmpOp::SLE:
        mipsFunc->addInstr(MIPSInstr("slt", {MReg(T2), MReg(T1),
            MReg(T0)}));
        mipsFunc->addInstr(MIPSInstr("xori", {MReg(T2), MReg(T2),
            MIPSOperand(1)}));
        break;
    case IcmpInstr::IcmpOp::SGT:
        mipsFunc->addInstr(MIPSInstr("slt", {MReg(T2), MReg(T1),
            MReg(T0)}));
        break;
    case IcmpInstr::IcmpOp::SGE:
        mipsFunc->addInstr(MIPSInstr("slt", {MReg(T2), MReg(T0),
            MReg(T1)}));
        mipsFunc->addInstr(MIPSInstr("xori", {MReg(T2), MReg(T2),
            MIPSOperand(1)}));
        break;
    }

    storeValue(mipsFunc, MReg(T2), instr, func);
}

// ============================================================
// 分支指令
// ============================================================
void MIPSCodeGenerator::genBr(MIPSFunction* mipsFunc, BrInstr* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    if (instr->getCondition()) {
        // 条件分支: br i1 cond, trueBB, falseBB
        loadValue(mipsFunc, instr->getCondition(), MReg(T0), func);
        std::string trueLabel = getBBLabel(instr->getTrueBlock());
        std::string falseLabel = getBBLabel(instr->getFalseBlock());
        mipsFunc->addInstr(MIPSInstr("bnez", {MReg(T0),
            MIPSOperand(trueLabel)}));
        mipsFunc->addInstr(MIPSInstr("j",
            {MIPSOperand(falseLabel)}));
    } else {
        // 无条件分支: br targetBB
        std::string target = getBBLabel(instr->getTrueBlock());
        mipsFunc->addInstr(MIPSInstr("j",
            {MIPSOperand(target)}));
    }
}

// ============================================================
// 返回指令
// ============================================================
void MIPSCodeGenerator::genRet(MIPSFunction* mipsFunc, RetInstr* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    // 加载返回值到 $v0
    if (instr->getReturnValue()) {
        loadValue(mipsFunc, instr->getReturnValue(), MReg(V0), func);
    }

    // Epilogue
    mipsFunc->addInstr(MIPSInstr::makeComment("=== epilogue ==="));
    mipsFunc->addInstr(MIPSInstr("lw", {MReg(Ra),
        MIPSOperand(0, MReg(Fp))}));
    mipsFunc->addInstr(MIPSInstr("lw", {MReg(MReg::T8),
        MIPSOperand(4, MReg(Fp))}));
    mipsFunc->addInstr(MIPSInstr("addiu", {MReg(Sp), MReg(Fp),
        MIPSOperand(frameMap[func].frameSize)}));
    mipsFunc->addInstr(MIPSInstr("move", {
        MReg(Fp), MReg(MReg::T8)}));
    mipsFunc->addInstr(MIPSInstr("jr", {MReg(Ra)}));
}

// ============================================================
// 函数调用
// ============================================================
void MIPSCodeGenerator::genCall(MIPSFunction* mipsFunc, CallInstr* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    // 将实参加载到 $a0-$a3
    size_t argCount = instr->getArgumentsCount();
    for (size_t i = 0; i < argCount && i < 4; ++i) {
        loadValue(mipsFunc, instr->getArgument(i),
            static_cast<MReg>(static_cast<int>(MReg::A0) + i), func);
    }

    // jal
    Function* callee = instr->getFunction();
    mipsFunc->addInstr(MIPSInstr("jal",
        {MIPSOperand(getFuncLabel(callee))}));

    // 存储返回值
    if (instr->getType() && !instr->getType()->isVoid()) {
        storeValue(mipsFunc, MReg(V0), instr, func);
    }
}

// ============================================================
// Alloca 指令
// ============================================================
void MIPSCodeGenerator::genAlloca(MIPSFunction* mipsFunc, AllocaInst* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    // alloca: 取地址 (栈帧中的偏移即为地址)
    int offset = getOffset(instr, func);
    mipsFunc->addInstr(MIPSInstr("addiu", {MReg(T0),
        MReg(Fp), MIPSOperand(offset)},
        "alloca result address"));
    storeValue(mipsFunc, MReg(T0), instr, func);
}

// ============================================================
// Load 指令
// ============================================================
void MIPSCodeGenerator::genLoad(MIPSFunction* mipsFunc, LoadInst* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    // 加载指针值到 $t0
    loadValue(mipsFunc, instr->getPointer(), MReg(T0), func);
    // 从内存加载
    mipsFunc->addInstr(MIPSInstr("lw", {MReg(T1),
        MIPSOperand(0, MReg(T0))}));
    // 存储结果
    storeValue(mipsFunc, MReg(T1), instr, func);
}

// ============================================================
// Store 指令
// ============================================================
void MIPSCodeGenerator::genStore(MIPSFunction* mipsFunc, StoreInst* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    // 加载值到 $t0, 指针到 $t1
    loadValue(mipsFunc, instr->getValue(), MReg(T0), func);
    loadValue(mipsFunc, instr->getPointer(), MReg(T1), func);
    // 存储
    mipsFunc->addInstr(MIPSInstr("sw", {MReg(T0),
        MIPSOperand(0, MReg(T1))}));
}

// ============================================================
// GEP 指令
// ============================================================
void MIPSCodeGenerator::genGEP(MIPSFunction* mipsFunc, GetElementPtrInst* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    // GEP: 计算元素地址
    // 简化: base + sum(indices[i] * elementSize[i])
    loadValue(mipsFunc, instr->getPointer(), MReg(T0), func);

    // 第一个 index 通常是 0 (数组首地址)
    // 后续 index 乘以元素大小累加
    MReg addrReg = MReg(T0);
    for (size_t i = 0; i < instr->getIndicesCount(); ++i) {
        Value* indexVal = instr->getIndex(i);
        loadValue(mipsFunc, indexVal, MReg(T1), func);

        // 计算这一维的元素大小 (简化: 假设 4 字节)
        // TODO: 根据 GEP 的类型结构精确计算元素大小
        int elemSize = 4;

        mipsFunc->addInstr(MIPSInstr("li", {MReg(T2),
            MIPSOperand(elemSize)}));
        mipsFunc->addInstr(MIPSInstr("mul", {MReg(T1),
            MReg(T1), MReg(T2)}));
        mipsFunc->addInstr(MIPSInstr("addu", {addrReg,
            addrReg, MReg(T1)}));
    }

    // 存储结果地址
    storeValue(mipsFunc, addrReg, instr, func);
}

// ============================================================
// Zext 指令
// ============================================================
void MIPSCodeGenerator::genZext(MIPSFunction* mipsFunc, ZextInstr* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    // zext i1 → i32: 加载并保证非 0/1
    loadValue(mipsFunc, instr->getOperand(), MReg(T0), func);
    // andi $t0, $t0, 1 (确保只有最低位)
    mipsFunc->addInstr(MIPSInstr("andi", {MReg(T0), MReg(T0),
        MIPSOperand(1)}));
    storeValue(mipsFunc, MReg(T0), instr, func);
}

// ============================================================
// Phi 指令
// ============================================================
void MIPSCodeGenerator::genPhi(MIPSFunction* mipsFunc, PhiInstr* instr) {
    Function* func = nullptr;
    for (auto& [f, info] : frameMap) {
        if (info.valueOffset.count(instr)) { func = f; break; }
    }
    if (!func) return;

    // Phi: 在每个前驱块结束时将对应的 incoming 值复制到 phi 结果
    // 简化实现: 在当前块开头从第一个 incoming 加载
    // TODO: 正确实现需要在每个前驱块末尾插入 copy 指令
    if (instr->getIncomingCount() > 0) {
        auto [val, bb] = instr->getIncoming(0);
        loadValue(mipsFunc, val, MReg(T0), func);
        storeValue(mipsFunc, MReg(T0), instr, func);
    }
    mipsFunc->addInstr(MIPSInstr::makeComment(
        "TODO: Phi - only loaded first incoming"));
}