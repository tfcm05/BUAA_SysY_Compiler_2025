#ifndef MIDDLEEND_OPTIMIZATION_ALGEBRAIC_SIMPLIFICATION_H
#define MIDDLEEND_OPTIMIZATION_ALGEBRAIC_SIMPLIFICATION_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"

#include <memory>
#include <unordered_map>
#include <cstdint>

// 代数化简 pass:
// 遍历 Module 中所有函数的指令, 对算术/逻辑/比较指令做保值化简。
// 该 pass 只做"引用替换": 将后续指令中对被化简指令结果的引用改为等价的新 Value,
// 不删除指令本身 (删除交给死指令消除 pass)。
//
// 规则:
//   add  x, 0 / add 0, x        -> x
//   sub  x, 0                   -> x
//   mul  x, 1 / mul 1, x        -> x
//   mul  x, 0 / mul 0, x        -> 0
//   sdiv x, 1                   -> x
//   and  x, 0 / and 0, x        -> 0
//   or   x, 0 / or  0, x        -> x
//   icmp EQ/SLE/SGE x, x        -> true (1)
//   icmp NE/SLT/SGT x, x        -> false (0)
class AlgebraicSimplification {
private:
    Module& module;
    LLVMContext& llvmContext;

    // 缓存本 pass 创建的常量 (管理所有权, 避免泄漏)
    std::unordered_map<LLVM::Type*, std::unique_ptr<Value>> zeroCache;
    std::unordered_map<LLVM::Type*, std::unique_ptr<Value>> oneCache;

    // v 是否为值为 val 的 ConstantInt
    bool isConstInt(Value* v, int64_t val) const;

    // 获取 (type, val) 的常量, 有缓存则复用
    Value* getConstInt(LLVM::Type* type, int64_t val);

    // 将 func 内所有指令中对 oldValue 的引用替换为 newValue (指针比较)
    // 返回是否实际发生了至少一处替换
    bool replaceUsesInFunction(Function* func, Value* oldValue, Value* newValue);

public:
    AlgebraicSimplification(Module& mod, LLVMContext& ctx);

    // 执行化简, 返回化简次数
    int run();
};

#endif // MIDDLEEND_OPTIMIZATION_ALGEBRAIC_SIMPLIFICATION_H
