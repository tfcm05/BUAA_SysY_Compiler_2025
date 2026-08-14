#ifndef IR_BUILDER_H
#define IR_BUILDER_H

#include "LLVM/Value.h"
#include "LLVMContext.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <cstdint>

class IRBuilder {
private:
    LLVMContext& llvmContext;

    BasicBlock* currentBlock;
    Function* currentFunction;

    std::vector<std::unique_ptr<Value>> ownedValue;

    std::unordered_map<std::string, uint32_t> nameCounters;

    std::string ensureName(const std::string& prefix, const std::string& name);
public:
    IRBuilder(LLVMContext& context);
    ~IRBuilder();

    // 确保 currentFunction 的所有基本块都有终止指令 (调用/析构/打印前)
    void ensureTerminatorInCurrentFunction();

    void setInsertPoint(BasicBlock* bb);
    BasicBlock* getInsertBlock() const;

    void setInsertFunction(Function* func);
    Function* getInsertFunction() const;

    // Const
    Value* createConstInt(LLVM::Type* type, int64_t value);
    Value* createConstArray(LLVM::Type* type, std::vector<Constant*> elements);
    Value* createConstString(LLVM::Type* i8Type, const std::string& str);

    // GlobalVariable
    GlobalVariable* createGlobalVariable(LLVM::Type* type);
    // Function
    Function* createFunction(LLVM::Type* funcType, const std::string& name, uint8_t mode);
    
    // Argument
    Argument* createArgument(LLVM::Type* type);
    // BasicBlock
    BasicBlock* createBasicBlock();

    // <result> = alloca <type>
    AllocaInst* createAlloca(LLVM::Type* type);

    // <result> = load <type>, <type>* <ptr>
    LoadInst* createLoad(Value* ptr);

    // store <value>, <type>* <ptr>
    StoreInst* createStore(Value* val, Value* ptr);

    // ret <type> <value>
    RetInstr* createRet(Value* val);

    // <result> = <BinaryOp> <ty> <op1>, <op2>
    AddInstr* createAdd(Value* lhs, Value* rhs);
    SubInstr* createSub(Value* lhs, Value* rhs);
    MulInstr* createMul(Value* lhs, Value* rhs);
    SdivInstr* createSdiv(Value* lhs, Value* rhs);
    AndInstr* createAnd(Value* lhs, Value* rhs);
    OrInstr* createOr(Value* lhs, Value* rhs);

    // <result> = icmp <IcmpOp> <ty> <op1>, <op2>
    IcmpInstr* createIcmp(IcmpInstr::IcmpOp op, Value* lhs, Value* rhs);

    // zero-extend <operand> to <type>
    ZextInstr* createZext(LLVM::Type* toType, Value* operand);

    // br <cond>, <trueBB>, <falseBB>
    BrInstr* createBr(Value* cond, BasicBlock* trueBB, BasicBlock* falseBB = nullptr);
    BrInstr* createBr(BasicBlock* target);

    // <result> = phi <type> [<value>, <bb>]...
    PhiInstr* createPhi(LLVM::Type* type, std::vector<std::pair<Value*, BasicBlock*>> incomings);

    // <result> = getelementptr <type>, <type>* <ptr>, <index>...
    GetElementPtrInst* createGEP(Value* ptr, std::vector<Value*> indices);

    // <result> = call <type> <func>(<args>)
    CallInstr* createCall(Function* func, std::vector<Value*> args);
};

#endif