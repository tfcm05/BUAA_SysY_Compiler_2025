#ifndef IR_CONSTANT_FOLDING_H
#define IR_CONSTANT_FOLDING_H

#include "../IntermediateCodeGenerate/Module.h"
#include "../IntermediateCodeGenerate/LLVMContext.h"
#include "../IntermediateCodeGenerate/LLVM/Value/User/Constant/ConstInt.h"

#include <memory>
#include <vector>

// IR 常量折叠 pass
//
// 遍历 Module 中所有 Function 的所有 BasicBlock 的指令,
// 对两个操作数均为 ConstantInt 的纯运算指令 (add/sub/mul/sdiv/and/or/icmp)
// 以及操作数为 ConstantInt 的 zext 指令, 在编译期计算结果,
// 将结果替换为新的 ConstantInt, 并把该指令从基本块指令列表中移除。
//
// 替换引用采用"指针匹配 + 全函数扫描"的方式: 对同一 Function 内所有
// 指令的各个操作数位置 (lhs/rhs/operand/condition/returnValue/phi 前驱/
// call 实参/gep 索引/store 值等) 检查是否等于被折叠指令的指针。
//
// 注意 (生命周期约定):
//   本类通过 ownedConstants 持有运行期间新建的 ConstantInt。
//   调用方应保证本 pass 对象在 IRPrinter 完成打印 (或所有下游消费方
//   完成对 IR 的引用) 之后再析构, 否则被折叠结果替换后的 IR 中会
//   出现悬垂指针。被移除的算术指令对象本身由 IRBuilder 管理, pass
//   只从指令列表中摘除, 不负责释放。
class IRConstantFolding {
private:
    Module& module;
    LLVMContext& context;

    // 新建折叠常量的所有权容器, 析构时统一释放
    std::vector<std::unique_ptr<ConstantInt>> ownedConstants;

    // 尝试折叠单条指令:
    //   可折叠且安全 -> 创建并返回新的 ConstantInt (加入 ownedConstants)
    //   否则         -> 返回 nullptr
    ConstantInt* tryFold(Instruction* inst);

    // 创建 (并托管) 一个 ConstantInt
    ConstantInt* createConstantInt(LLVM::Type* type, int64_t value);

    // 将 function 内所有指令中指向 oldValue 的操作数替换为 newValue
    void replaceUses(Function* function, Value* oldValue, Value* newValue);

    // 折叠单个函数, 返回折叠条数
    int foldFunction(Function* function);
public:
    IRConstantFolding(Module& module, LLVMContext& context);
    ~IRConstantFolding();

    // 折叠整个 Module, 返回折叠的指令总条数
    int run();
};

#endif
