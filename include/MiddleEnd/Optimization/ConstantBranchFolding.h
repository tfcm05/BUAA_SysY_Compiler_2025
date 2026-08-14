#ifndef MIDDLEEND_OPTIMIZATION_CONSTANT_BRANCH_FOLDING_H
#define MIDDLEEND_OPTIMIZATION_CONSTANT_BRANCH_FOLDING_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/ConstInt.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Constant/GlobalValue/Function.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/User/Instruction/ControlFlow.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value/BasicBlock.h"

// 常量条件分支化简 pass:
//   对每个 BrInstr, 若 condition 为 ConstantInt:
//     - 值非 0: 化简为无条件 br trueBB
//     - 值为 0: 化简为无条件 br falseBB
// 不处理化简后不可达的基本块(由死基本块消除 pass 负责)。
// 与常量折叠、死基本块消除配合使用:
//   常量折叠 -> 常量分支化简 -> 死基本块消除
class ConstantBranchFolding {
private:
    Module& module;
public:
    explicit ConstantBranchFolding(Module& module);
    // 遍历 Module 中所有基本块的 BrInstr 做常量条件化简,
    // 返回化简的分支数量。
    int run();
};

#endif
