#include "BackEnd/MIPS/MIPS.h"
#include "BackEnd/MIPS/MIPSCodeGenerator.h"

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
// MIPSCodeGenerator: 常量判断
// ============================================================
bool MIPSCodeGenerator::isConstantIntVal(const Value* v, int64_t& out) {
    if (v && v->isConstantInt()) {
        out = static_cast<const ConstantInt*>(v)->getValue();
        return true;
    }
    return false;
}

// ============================================================
// MIPSCodeGenerator: 入口
// ============================================================
std::unique_ptr<MIPSModule> MIPSCodeGenerator::run() {
    mipsModule = std::make_unique<MIPSModule>();

    // 1. 数据段 (全局变量 + 字符串常量)
    generateGlobalVariables();
    generateStringConstants();

    // 2. 为每个函数分配栈帧 (main 单独存放于 MainFunction)
    for (const auto* func : module.getFunctions()) {
        if (func->getBasicBlocksCount() > 0) {  // 跳过外部声明
            allocateFrame(const_cast<Function*>(func));
        }
    }
    if (auto* mainFunc = module.getMainFunction()) {
        allocateFrame(mainFunc);
    }

    // 3. 代码段
    for (const auto* func : module.getFunctions()) {
        if (func->getBasicBlocksCount() > 0) {  // 外部声明由运行库提供
            generateFunction(const_cast<Function*>(func));
        }
    }
    if (auto* mainFunc = module.getMainFunction()) {
        generateFunction(mainFunc);
    }

    return std::move(mipsModule);
}

// ============================================================
// 数据段生成
// ============================================================
void MIPSCodeGenerator::collectGlobalWords(const Constant* c,
                                           std::vector<int64_t>& out) const {
    if (!c) return;
    if (c->isConstantInt()) {
        out.push_back(static_cast<const ConstantInt*>(c)->getValue());
    } else if (c->isConstantArray()) {
        auto* ca = static_cast<const ConstantArray*>(c);
        for (size_t i = 0; i < ca->getElementsCount(); ++i) {
            collectGlobalWords(ca->getElement(i), out);
        }
    }
}

void MIPSCodeGenerator::generateGlobalVariables() {
    for (const auto* gv : module.getGlobalVariables()) {
        std::string label = getGlobalLabel(gv);
        LLVM::Type* pointedType =
            static_cast<LLVM::PointerType*>(gv->getType())->getPointedType();

        const auto* init = gv->getInitializer();
        if (!init) {
            // 未初始化: .space
            mipsModule->addDataEntry(
                {label, ".space", std::to_string(getTypeSize(pointedType))});
            continue;
        }

        if (init->isConstantInt()) {
            // 标量整数: .word
            int64_t val = static_cast<const ConstantInt*>(init)->getValue();
            mipsModule->addDataEntry({label, ".word", std::to_string(val)});
            continue;
        }

        if (init->isConstantArray()) {
            // 数组: 展开为 .word / .byte 列表 (元素全部为常量时)
            std::vector<int64_t> elems;
            collectGlobalWords(init, elems);
            bool isByte = pointedType && pointedType->isArray() &&
                          isI8Type(static_cast<LLVM::ArrayType*>(pointedType)
                                       ->getElementType());
            if (!elems.empty()) {
                std::string v;
                for (size_t i = 0; i < elems.size(); ++i) {
                    if (i) v += ", ";
                    v += std::to_string(elems[i]);
                }
                mipsModule->addDataEntry(
                    {label, isByte ? ".byte" : ".word", v});
                continue;
            }
        }

        // 其他情况: .space
        mipsModule->addDataEntry(
            {label, ".space", std::to_string(getTypeSize(pointedType))});
    }
}

void MIPSCodeGenerator::generateStringConstants() {
    for (const auto* gv : module.getGlobalConstStrs()) {
        std::string label = getGlobalLabel(gv);
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
            mipsModule->addDataEntry(
                {label, ".asciiz", "\"" + escaped + "\"", true});
        }
    }
}

// ============================================================
// 栈帧管理
// ============================================================
void MIPSCodeGenerator::allocateFrame(Function* func) {
    FrameInfo info;
    info.nextOffset = 8;  // 0: $ra, 4: $fp_saved

    // 参数
    for (size_t i = 0; i < func->getArgumentsCount(); ++i) {
        Argument* arg = func->getArgument(i);
        info.valueOffset[arg] = info.nextOffset;
        info.nextOffset += 4;
    }

    // 指令结果 (store/br/ret 无结果不分配; alloca 按内容区大小分配)
    for (size_t i = 0; i < func->getBasicBlocksCount(); ++i) {
        BasicBlock* bb = func->getBasicBlock(i);
        for (size_t j = 0; j < bb->getInstructionsCount(); ++j) {
            Instruction* instr = bb->getInstruction(j);
            if (instr->isTerminator()) continue;  // br / ret
            if (dynamic_cast<StoreInst*>(instr)) continue;
            if (auto* call = dynamic_cast<CallInstr*>(instr)) {
                if (call->getType() && call->getType()->isVoid()) continue;
            }
            if (auto* alloca = dynamic_cast<AllocaInst*>(instr)) {
                // alloca 栈槽即内容区
                size_t sz = getTypeSize(
                    static_cast<LLVM::PointerType*>(alloca->getType())
                        ->getPointedType());
                sz = (sz + 3) / 4 * 4;
                info.valueOffset[alloca] = info.nextOffset;
                info.nextOffset += static_cast<int>(sz);
                continue;
            }
            info.valueOffset[instr] = info.nextOffset;
            info.nextOffset += 4;
        }
    }

    // 对齐到 8 字节
    info.frameSize = ((info.nextOffset + 7) / 8) * 8;
    frameMap[func] = info;

    // BB 标签
    for (size_t i = 0; i < func->getBasicBlocksCount(); ++i) {
        BasicBlock* bb = func->getBasicBlock(i);
        bbLabels[bb] = getFuncLabel(func) + "." + bb->getName();
    }
}

int MIPSCodeGenerator::getOffset(const Value* val) const {
    auto it = frameMap.find(currentFunc);
    if (it == frameMap.end()) return 0;
    auto vit = it->second.valueOffset.find(val);
    if (vit == it->second.valueOffset.end()) return 0;
    return vit->second;
}

bool MIPSCodeGenerator::hasStackSlot(const Value* val) const {
    auto it = frameMap.find(currentFunc);
    if (it == frameMap.end()) return false;
    return it->second.valueOffset.count(val) != 0;
}

std::string MIPSCodeGenerator::getBBLabel(BasicBlock* bb) const {
    auto it = bbLabels.find(bb);
    return it != bbLabels.end() ? it->second : std::string();
}

std::string MIPSCodeGenerator::getFuncLabel(const Function* func) const {
    return func->getName();
}

std::string MIPSCodeGenerator::getGlobalLabel(const GlobalVariable* gv) const {
    return gv->getName();
}

size_t MIPSCodeGenerator::getTypeSize(LLVM::Type* type) const {
    if (!type) return 0;
    switch (type->getTypeID()) {
    case LLVM::Type::VoidTyID:   return 0;
    case LLVM::Type::IntegerTyID: {
        auto* intType = static_cast<LLVM::IntegerType*>(type);
        return intType->getBitWidth() == 8 ? 1 : 4;
    }
    case LLVM::Type::PointerTyID: return 4;
    case LLVM::Type::ArrayTyID: {
        auto* arrType = static_cast<LLVM::ArrayType*>(type);
        return arrType->getSize() * getTypeSize(arrType->getElementType());
    }
    case LLVM::Type::FunctionTyID: return 4;
    }
    return 4;
}

// GEP 剥掉一层
LLVM::Type* MIPSCodeGenerator::gepAdvance(LLVM::Type* cur) const {
    if (!cur) return nullptr;
    if (cur->isArray()) {
        return static_cast<LLVM::ArrayType*>(cur)->getElementType();
    }
    if (cur->isPointer()) {
        return static_cast<LLVM::PointerType*>(cur)->getPointedType();
    }
    return cur;
}

bool MIPSCodeGenerator::isI8Type(LLVM::Type* type) const {
    return type && type->isInteger() &&
           static_cast<LLVM::IntegerType*>(type)->getBitWidth() == 8;
}

// ============================================================
// 寄存器缓存 (BB 级)
// ============================================================
MReg MIPSCodeGenerator::allocReg(const std::set<MReg>& avoid) {
    // 1. 空闲寄存器
    for (int i = 0; i < NUM_T_REGS; ++i) {
        MReg r = static_cast<MReg>(static_cast<int>(MReg::T0) + i);
        if (!regBusy[i] && !avoid.count(r)) {
            regBusy[i] = true;
            return r;
        }
    }
    // 2. 全部占用: 淘汰一个非 avoid 的寄存器
    for (int i = 0; i < NUM_T_REGS; ++i) {
        MReg r = static_cast<MReg>(static_cast<int>(MReg::T0) + i);
        if (avoid.count(r)) continue;
        auto it = regOwner.find(r);
        if (it != regOwner.end()) {
            // 缓存项: 写回栈帧
            const Value* v = it->second;
            if (hasStackSlot(v)) {
                emitInstr(MIPSInstr(
                    "sw", {r, MIPSOperand(getOffset(v), MReg(MReg::Fp))},
                    "spill " + v->getName()));
            }
            regCache.erase(v);
            regOwner.erase(it);
        }
        regBusy[i] = true;
        return r;
    }
    // 3. 不可达 (avoid 集合至多 3 个, 寄存器 8 个)
    return MReg::T0;
}

void MIPSCodeGenerator::flushRegCache() {
    for (auto& [val, reg] : regCache) {
        if (hasStackSlot(val)) {
            emitInstr(MIPSInstr(
                "sw", {reg, MIPSOperand(getOffset(val), MReg(MReg::Fp))},
                "flush " + val->getName()));
        }
    }
    regCache.clear();
    regOwner.clear();
    for (int i = 0; i < NUM_T_REGS; ++i) regBusy[i] = false;
}

void MIPSCodeGenerator::cacheResult(const Value* val, MReg reg) {
    // reg 已有其他 owner: 先写回旧值
    auto it = regOwner.find(reg);
    if (it != regOwner.end() && it->second != val) {
        const Value* oldVal = it->second;
        if (hasStackSlot(oldVal)) {
            emitInstr(MIPSInstr(
                "sw", {reg, MIPSOperand(getOffset(oldVal), MReg(MReg::Fp))},
                "spill " + oldVal->getName()));
        }
        regCache.erase(oldVal);
    }
    // val 已映射到其他寄存器: 移除旧映射 (旧寄存器值未变, 无需写回)
    auto vit = regCache.find(val);
    if (vit != regCache.end() && vit->second != reg) {
        regOwner.erase(vit->second);
    }
    regCache[val] = reg;
    regOwner[reg] = val;
    if (isTReg(reg)) regBusy[regIndex(reg)] = true;
}

void MIPSCodeGenerator::loadValueInto(const Value* val, MReg reg,
                                      bool isAddress) {
    if (!val) {
        emitInstr(MIPSInstr("li", {reg, MIPSOperand(0)}));
        return;
    }
    if (val->isConstantInt()) {
        int64_t imm = static_cast<const ConstantInt*>(val)->getValue();
        emitInstr(MIPSInstr("li", {reg, MIPSOperand(imm)}));
        return;
    }
    if (dynamic_cast<const AllocaInst*>(val)) {
        // alloca 的值即内容区地址 = fp + offset
        emitInstr(MIPSInstr("addiu",
                            {reg, MReg(MReg::Fp),
                             MIPSOperand(getOffset(val))}));
        return;
    }
    if (val->isGlobalVariable()) {
        auto* gv = const_cast<GlobalVariable*>(
            static_cast<const GlobalVariable*>(val));
        if (isAddress) {
            emitInstr(MIPSInstr("la", {reg, MIPSOperand(getGlobalLabel(gv))}));
        } else {
            emitInstr(MIPSInstr("la", {reg, MIPSOperand(getGlobalLabel(gv))}));
            emitInstr(MIPSInstr("lw", {reg, MIPSOperand(0, reg)}));
        }
        return;
    }
    if (val->isFunction()) {
        auto* fn = const_cast<Function*>(static_cast<const Function*>(val));
        emitInstr(MIPSInstr("la", {reg, MIPSOperand(getFuncLabel(fn))}));
        return;
    }
    if (val->isBasicBlock()) {
        auto* bb = const_cast<BasicBlock*>(static_cast<const BasicBlock*>(val));
        emitInstr(MIPSInstr("la", {reg, MIPSOperand(getBBLabel(bb))}));
        return;
    }
    // 栈帧加载
    emitInstr(MIPSInstr("lw",
                        {reg, MIPSOperand(getOffset(val), MReg(MReg::Fp))}));
}

MReg MIPSCodeGenerator::loadValue(const Value* val,
                                  const std::set<MReg>& avoid) {
    // 缓存命中
    auto it = regCache.find(val);
    if (it != regCache.end()) return it->second;

    MReg reg = allocReg(avoid);
    loadValueInto(val, reg, false);

    // 仅对有栈槽的指令结果/参数进行缓存;
    // 常量/全局/函数/BB/alloca 每次按需重新计算
    bool cacheable = val && !val->isConstantInt() && !val->isGlobalVariable() &&
                     !val->isFunction() && !val->isBasicBlock() &&
                     !dynamic_cast<const AllocaInst*>(val);
    if (cacheable) cacheResult(val, reg);
    return reg;
}

MReg MIPSCodeGenerator::loadValue(const Value* val, MReg dst) {
    // 缓存命中: move 到 dst
    auto it = regCache.find(val);
    if (it != regCache.end()) {
        if (it->second != dst) {
            emitInstr(MIPSInstr("move", {dst, it->second}));
        }
        return dst;
    }
    loadValueInto(val, dst, false);
    return dst;
}

MReg MIPSCodeGenerator::loadAddress(const Value* val,
                                    const std::set<MReg>& avoid) {
    auto it = regCache.find(val);
    if (it != regCache.end()) return it->second;
    if (val && val->isGlobalVariable()) {
        MReg reg = allocReg(avoid);
        loadValueInto(val, reg, true);  // la
        return reg;
    }
    // 其他 (alloca / GEP 结果 / 参数指针): 值即地址
    return loadValue(val, avoid);
}

// ============================================================
// 函数生成
// ============================================================
void MIPSCodeGenerator::emitEpilogue(MIPSFunction* mipsFunc, int frameSize) {
    mipsFunc->addInstr(MIPSInstr::makeComment("=== epilogue ==="));
    mipsFunc->addInstr(MIPSInstr("lw", {MReg(MReg::Ra),
        MIPSOperand(0, MReg(MReg::Fp))}));
    mipsFunc->addInstr(MIPSInstr("lw", {MReg(MReg::T8),
        MIPSOperand(4, MReg(MReg::Fp))}));
    mipsFunc->addInstr(MIPSInstr("addiu", {MReg(MReg::Sp),
        MReg(MReg::Fp), MIPSOperand(frameSize)}));
    mipsFunc->addInstr(MIPSInstr("move", {MReg(MReg::Fp),
        MReg(MReg::T8)}));
    mipsFunc->addInstr(MIPSInstr("jr", {MReg(MReg::Ra)}));
}

void MIPSCodeGenerator::generateFunction(Function* func) {
    if (func->getBasicBlocksCount() == 0) return;  // 外部声明, 不生成定义
    currentFunc = func;
    auto* mipsFunc = new MIPSFunction(getFuncLabel(func));
    curMipsFunc = mipsFunc;
    const auto& info = frameMap[func];
    int frameSize = info.frameSize;

    // ---- Prologue ----
    mipsFunc->addInstr(MIPSInstr::makeComment("=== prologue ==="));
    mipsFunc->addInstr(MIPSInstr("addiu", {MReg(MReg::Sp), MReg(MReg::Sp),
        MIPSOperand(-frameSize)}));
    // 保存旧 fp (t8 为 caller-saved, prologue 中可自由使用)
    mipsFunc->addInstr(MIPSInstr("move", {MReg(MReg::T8), MReg(MReg::Fp)}));
    mipsFunc->addInstr(MIPSInstr("move", {MReg(MReg::Fp), MReg(MReg::Sp)}));
    mipsFunc->addInstr(MIPSInstr("sw", {MReg(MReg::Ra),
        MIPSOperand(0, MReg(MReg::Fp))}));
    mipsFunc->addInstr(MIPSInstr("sw", {MReg(MReg::T8),
        MIPSOperand(4, MReg(MReg::Fp))}));

    // ---- 参数: 前 4 个来自 $a0-$a3, 其余从调用者栈帧读取 ----
    for (size_t i = 0; i < func->getArgumentsCount(); ++i) {
        Argument* arg = func->getArgument(i);
        int off = getOffset(arg);
        if (i < 4) {
            MReg argReg =
                static_cast<MReg>(static_cast<int>(MReg::A0) + i);
            mipsFunc->addInstr(MIPSInstr("sw", {argReg,
                MIPSOperand(off, MReg(MReg::Fp))}));
        } else {
            // 第 i 个参数 (i>=4) 在调用者栈: fp + frameSize + 16 + 4*(i-4)
            int64_t srcOff = frameSize + 16 + 4 * (int64_t)(i - 4);
            mipsFunc->addInstr(MIPSInstr("lw", {MReg(MReg::T8),
                MIPSOperand((int)srcOff, MReg(MReg::Fp))}));
            mipsFunc->addInstr(MIPSInstr("sw", {MReg(MReg::T8),
                MIPSOperand(off, MReg(MReg::Fp))}));
        }
    }

    // ---- Phi 消除分析 ----
    collectPhiCopies(func);

    // ---- 基本块 ----
    for (size_t i = 0; i < func->getBasicBlocksCount(); ++i) {
        generateBasicBlock(mipsFunc, func->getBasicBlock(i));
    }

    // ---- 保底 epilogue (最后 BB 不以 ret 结尾) ----
    bool lastIsRet = false;
    if (func->getBasicBlocksCount() > 0) {
        BasicBlock* lastBB = func->getBasicBlock(func->getBasicBlocksCount() - 1);
        if (lastBB->getInstructionsCount() > 0) {
            Instruction* last =
                lastBB->getInstruction(lastBB->getInstructionsCount() - 1);
            if (dynamic_cast<RetInstr*>(last)) lastIsRet = true;
        }
    }
    if (!lastIsRet) {
        emitEpilogue(mipsFunc, frameSize);
    }

    // 清空生成状态
    flushRegCache();
    curMipsFunc = nullptr;
    currentFunc = nullptr;

    mipsModule->addFunction(mipsFunc);
}

// ============================================================
// 基本块生成
// ============================================================
void MIPSCodeGenerator::generateBasicBlock(MIPSFunction* mipsFunc,
                                           BasicBlock* bb) {
    mipsFunc->addInstr(MIPSInstr::makeLabel(getBBLabel(bb)));

    // 1. 生成非 terminator 指令 (跳过 Phi)
    Instruction* terminator = nullptr;
    for (size_t i = 0; i < bb->getInstructionsCount(); ++i) {
        Instruction* instr = bb->getInstruction(i);
        if (instr->isTerminator()) {
            terminator = instr;
            break;
        }
        if (dynamic_cast<PhiInstr*>(instr)) continue;
        generateInstruction(mipsFunc, instr);
    }

    // 2. Phi 消除: 本 BB 作为前驱时, 在 terminator 前把 incoming 值写入 phi 栈槽
    auto it = phiCopies.find(bb);
    if (it != phiCopies.end()) {
        for (auto& [phiResult, incomingVal] : it->second) {
            MReg reg = loadValue(incomingVal);
            emitInstr(MIPSInstr("sw", {reg,
                MIPSOperand(getOffset(phiResult), MReg(MReg::Fp))},
                "phi copy"));
        }
    }

    // 3. BB 结束: 清空寄存器缓存 (所有值写回栈帧)
    flushRegCache();

    // 4. terminator
    if (terminator) generateInstruction(mipsFunc, terminator);

    // 5. terminator 可能加载/缓存条件值, 清空保证 BB 边界缓存干净
    flushRegCache();
}

// ============================================================
// 指令分派
// ============================================================
void MIPSCodeGenerator::generateInstruction(MIPSFunction* mipsFunc,
                                            Instruction* instr) {
    if (dynamic_cast<AddInstr*>(instr) || dynamic_cast<SubInstr*>(instr) ||
        dynamic_cast<MulInstr*>(instr) || dynamic_cast<SdivInstr*>(instr) ||
        dynamic_cast<AndInstr*>(instr) || dynamic_cast<OrInstr*>(instr)) {
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
    } else {
        mipsFunc->addInstr(MIPSInstr::makeComment(
            "TODO: unhandled instruction type"));
    }
}

// ============================================================
// 算术逻辑指令
// ============================================================
void MIPSCodeGenerator::genArithmetic(MIPSFunction* mipsFunc,
                                      Instruction* instr) {
    (void)mipsFunc;
    Value* lhs = nullptr;
    Value* rhs = nullptr;
    const char* mnem = nullptr;
    bool isDiv = false;

    if (auto* a = dynamic_cast<AddInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "add";
    } else if (auto* a = dynamic_cast<SubInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "sub";
    } else if (auto* a = dynamic_cast<MulInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "mul";
    } else if (auto* a = dynamic_cast<SdivInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "div"; isDiv = true;
    } else if (auto* a = dynamic_cast<AndInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "and";
    } else if (auto* a = dynamic_cast<OrInstr*>(instr)) {
        lhs = a->getLHS(); rhs = a->getRHS(); mnem = "or";
    } else {
        return;
    }

    int64_t rhsImm = 0;
    bool rhsConst = isConstantIntVal(rhs, rhsImm);
    int64_t lhsImm = 0;
    bool lhsConst = isConstantIntVal(lhs, lhsImm);

    // 可交换运算: 常量放右侧以利用立即数指令
    bool commutative = (std::string(mnem) == "add" ||
                        std::string(mnem) == "mul" ||
                        std::string(mnem) == "and" ||
                        std::string(mnem) == "or");
    if (commutative && !rhsConst && lhsConst) {
        std::swap(lhs, rhs);
        rhsConst = isConstantIntVal(rhs, rhsImm);
    }

    // 立即数优化: add->addi, sub->addiu(负值), and->andi, or->ori
    bool immOpt = false;
    const char* immMnem = nullptr;
    int64_t immVal = 0;
    if (!isDiv && rhsConst) {
        if (std::string(mnem) == "add" && isSmallImm(rhsImm)) {
            immOpt = true; immMnem = "addi"; immVal = rhsImm;
        } else if (std::string(mnem) == "sub" && isSmallImm(-rhsImm)) {
            immOpt = true; immMnem = "addiu"; immVal = -rhsImm;
        } else if (std::string(mnem) == "and" && isUnsignedImm16(rhsImm)) {
            immOpt = true; immMnem = "andi"; immVal = rhsImm;
        } else if (std::string(mnem) == "or" && isUnsignedImm16(rhsImm)) {
            immOpt = true; immMnem = "ori"; immVal = rhsImm;
        }
    }

    if (immOpt) {
        MReg lhsReg = loadValue(lhs);
        MReg rd = allocReg({lhsReg});
        emitInstr(MIPSInstr(immMnem, {rd, lhsReg, MIPSOperand(immVal)}));
        cacheResult(instr, rd);
        return;
    }

    MReg lhsReg = loadValue(lhs);
    MReg rhsReg = loadValue(rhs, std::set<MReg>{lhsReg});
    MReg rd = allocReg({lhsReg, rhsReg});
    if (isDiv) {
        emitInstr(MIPSInstr("div", {lhsReg, rhsReg}, "div " + lhs->getName() +
                                                        " / " + rhs->getName()));
        emitInstr(MIPSInstr("mflo", {rd}));
    } else {
        emitInstr(MIPSInstr(mnem, {rd, lhsReg, rhsReg}));
    }
    cacheResult(instr, rd);
}

// ============================================================
// Icmp 指令
// ============================================================
void MIPSCodeGenerator::genIcmp(MIPSFunction* mipsFunc, IcmpInstr* instr) {
    (void)mipsFunc;
    MReg lhsReg = loadValue(instr->getLHS());
    MReg rhsReg = loadValue(instr->getRHS(), std::set<MReg>{lhsReg});
    MReg rd = allocReg({lhsReg, rhsReg});

    switch (instr->getPredicate()) {
    case IcmpInstr::IcmpOp::EQ:
        emitInstr(MIPSInstr("xor", {rd, lhsReg, rhsReg}));
        emitInstr(MIPSInstr("sltiu", {rd, rd, MIPSOperand(1)}));
        break;
    case IcmpInstr::IcmpOp::NE:
        emitInstr(MIPSInstr("xor", {rd, lhsReg, rhsReg}));
        emitInstr(MIPSInstr("sltu", {rd, MReg(MReg::Zero), rd}));
        break;
    case IcmpInstr::IcmpOp::SLT:
        emitInstr(MIPSInstr("slt", {rd, lhsReg, rhsReg}));
        break;
    case IcmpInstr::IcmpOp::SLE:
        emitInstr(MIPSInstr("slt", {rd, rhsReg, lhsReg}));
        emitInstr(MIPSInstr("xori", {rd, rd, MIPSOperand(1)}));
        break;
    case IcmpInstr::IcmpOp::SGT:
        emitInstr(MIPSInstr("slt", {rd, rhsReg, lhsReg}));
        break;
    case IcmpInstr::IcmpOp::SGE:
        emitInstr(MIPSInstr("slt", {rd, lhsReg, rhsReg}));
        emitInstr(MIPSInstr("xori", {rd, rd, MIPSOperand(1)}));
        break;
    }

    cacheResult(instr, rd);
}

// ============================================================
// 分支指令
// ============================================================
void MIPSCodeGenerator::genBr(MIPSFunction* mipsFunc, BrInstr* instr) {
    (void)mipsFunc;
    if (instr->getCondition()) {
        // 条件加载到临时寄存器 (不缓存, 避免 BB 边界残留)
        MReg cond = allocReg();
        loadValue(instr->getCondition(), cond);
        emitInstr(MIPSInstr("bnez", {cond,
            MIPSOperand(getBBLabel(instr->getTrueBlock()))}));
        emitInstr(MIPSInstr("j", {MIPSOperand(getBBLabel(instr->getFalseBlock()))}));
    } else {
        emitInstr(MIPSInstr("j", {MIPSOperand(getBBLabel(instr->getTrueBlock()))}));
    }
}

// ============================================================
// 返回指令
// ============================================================
void MIPSCodeGenerator::genRet(MIPSFunction* mipsFunc, RetInstr* instr) {
    if (instr->getReturnValue()) {
        loadValue(instr->getReturnValue(), MReg(MReg::V0));
    }
    emitEpilogue(mipsFunc, frameMap[currentFunc].frameSize);
}

// ============================================================
// 函数调用
// ============================================================
void MIPSCodeGenerator::genCall(MIPSFunction* mipsFunc, CallInstr* instr) {
    (void)mipsFunc;
    // 调用前清空寄存器缓存 (保护 caller-saved $t0-$t7)
    flushRegCache();

    size_t argCount = instr->getArgumentsCount();

    // 第 5+ 个参数压栈 (O32 风格: 调用者预留 16 字节参数保存区 + 栈参数)
    if (argCount > 4) {
        int64_t stackArgBytes = 16 + 4 * (int64_t)(argCount - 4);
        emitInstr(MIPSInstr("addiu", {MReg(MReg::Sp), MReg(MReg::Sp),
            MIPSOperand(-stackArgBytes)}));
    }

    for (size_t i = 0; i < argCount; ++i) {
        if (i < 4) {
            MReg argReg =
                static_cast<MReg>(static_cast<int>(MReg::A0) + i);
            loadValue(instr->getArgument(i), argReg);
        } else {
            MReg reg = loadValue(instr->getArgument(i), MReg(MReg::T8));
            int64_t off = 16 + 4 * (int64_t)(i - 4);
            emitInstr(MIPSInstr("sw", {reg,
                MIPSOperand((int)off, MReg(MReg::Sp))}));
        }
    }

    emitInstr(MIPSInstr("jal", {MIPSOperand(getFuncLabel(instr->getFunction()))}));

    if (argCount > 4) {
        int64_t stackArgBytes = 16 + 4 * (int64_t)(argCount - 4);
        emitInstr(MIPSInstr("addiu", {MReg(MReg::Sp), MReg(MReg::Sp),
            MIPSOperand(stackArgBytes)}));
    }

    // 返回值
    if (instr->getType() && !instr->getType()->isVoid()) {
        MReg rd = allocReg();
        emitInstr(MIPSInstr("move", {rd, MReg(MReg::V0)}));
        cacheResult(instr, rd);
    }
}

// ============================================================
// Alloca 指令: 不生成指令, 地址按需计算 (fp + offset)
// ============================================================
void MIPSCodeGenerator::genAlloca(MIPSFunction* mipsFunc, AllocaInst* instr) {
    mipsFunc->addInstr(MIPSInstr::makeComment(
        "alloca " + instr->getName() + " @ fp+" + std::to_string(getOffset(instr))));
}

// ============================================================
// Load 指令
// ============================================================
void MIPSCodeGenerator::genLoad(MIPSFunction* mipsFunc, LoadInst* instr) {
    (void)mipsFunc;
    MReg ptrReg = loadAddress(instr->getPointer());
    MReg rd = allocReg({ptrReg});
    if (isI8Type(instr->getType())) {
        emitInstr(MIPSInstr("lbu", {rd, MIPSOperand(0, ptrReg)}));
    } else {
        emitInstr(MIPSInstr("lw", {rd, MIPSOperand(0, ptrReg)}));
    }
    cacheResult(instr, rd);
}

// ============================================================
// Store 指令
// ============================================================
void MIPSCodeGenerator::genStore(MIPSFunction* mipsFunc, StoreInst* instr) {
    (void)mipsFunc;
    MReg valReg = loadValue(instr->getValue());
    MReg ptrReg = loadAddress(instr->getPointer(), {valReg});
    if (isI8Type(instr->getValue()->getType())) {
        emitInstr(MIPSInstr("sb", {valReg, MIPSOperand(0, ptrReg)}));
    } else {
        emitInstr(MIPSInstr("sw", {valReg, MIPSOperand(0, ptrReg)}));
    }
}

// ============================================================
// GEP 指令: 按类型结构精确计算偏移
// ============================================================
void MIPSCodeGenerator::genGEP(MIPSFunction* mipsFunc,
                               GetElementPtrInst* instr) {
    (void)mipsFunc;
    MReg addrReg = loadAddress(instr->getPointer());

    LLVM::Type* curType = nullptr;
    if (instr->getPointer()->getType() &&
        instr->getPointer()->getType()->isPointer()) {
        curType = static_cast<LLVM::PointerType*>(
                      instr->getPointer()->getType())
                      ->getPointedType();
    }

    for (size_t i = 0; i < instr->getIndicesCount(); ++i) {
        Value* idx = instr->getIndex(i);
        // 第 k 个索引的步长 = 当前层类型的完整大小 (sizeof(T_k)):
        //   指针 | [2 x [3 x i32]]*  索引 [0,1,1] -> 0 + 1*12 + 1*4 = 16
        //   即 LLVM GEP 语义: 第 0 个索引用 sizeof(指针指向类型),
        //   之后每个索引用"当前数组类型的元素大小" (= sizeof(剥一层后的类型))。
        int step = static_cast<int>(getTypeSize(curType));

        int64_t constIdx = 0;
        bool isConst = isConstantIntVal(idx, constIdx);

        if (isConst && constIdx == 0) {
            // 0 索引: 跳过 (常见于 GEP a, 0, i, j 的第一维)
        } else if (isConst) {
            int64_t off = constIdx * step;
            if (isSmallImm(off)) {
                MReg newAddr = allocReg({addrReg});
                emitInstr(MIPSInstr("addiu", {newAddr, addrReg,
                    MIPSOperand(off)}));
                addrReg = newAddr;
            } else {
                MReg tmp = allocReg({addrReg});
                emitInstr(MIPSInstr("li", {tmp, MIPSOperand(off)}));
                MReg newAddr = allocReg({addrReg, tmp});
                emitInstr(MIPSInstr("addu", {newAddr, addrReg, tmp}));
                addrReg = newAddr;
            }
        } else {
            MReg idxReg = loadValue(idx, std::set<MReg>{addrReg});
            MReg tmp = allocReg({addrReg, idxReg});
            emitInstr(MIPSInstr("li", {tmp, MIPSOperand(step)}));
            emitInstr(MIPSInstr("mul", {tmp, idxReg, tmp}));
            MReg newAddr = allocReg({addrReg, idxReg, tmp});
            emitInstr(MIPSInstr("addu", {newAddr, addrReg, tmp}));
            addrReg = newAddr;
        }

        curType = gepAdvance(curType);
    }

    cacheResult(instr, addrReg);
}

// ============================================================
// Zext 指令
// ============================================================
void MIPSCodeGenerator::genZext(MIPSFunction* mipsFunc, ZextInstr* instr) {
    (void)mipsFunc;
    MReg reg = loadValue(instr->getOperand());
    MReg rd = allocReg({reg});
    emitInstr(MIPSInstr("andi", {rd, reg, MIPSOperand(1)}));
    cacheResult(instr, rd);
}

// ============================================================
// Phi 消除: 收集每个前驱 BB 需要写入的 phi 值
// ============================================================
void MIPSCodeGenerator::collectPhiCopies(Function* func) {
    phiCopies.clear();
    for (size_t i = 0; i < func->getBasicBlocksCount(); ++i) {
        BasicBlock* bb = func->getBasicBlock(i);
        for (size_t j = 0; j < bb->getInstructionsCount(); ++j) {
            if (auto* phi = dynamic_cast<PhiInstr*>(bb->getInstruction(j))) {
                for (size_t k = 0; k < phi->getIncomingCount(); ++k) {
                    auto [val, predBB] = phi->getIncoming(k);
                    phiCopies[predBB].push_back({phi, val});
                }
            }
        }
    }
}
