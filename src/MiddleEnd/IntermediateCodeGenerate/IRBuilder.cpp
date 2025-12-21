#include "../../../include/MiddleEnd/IntermediateCodeGenerate/IRBuilder.h"

IRBuilder::IRBuilder(LLVMContext& context) : llvmContext(context), currentBlock(nullptr), currentFunction(nullptr) {}

IRBuilder::~IRBuilder() {
    ensureTerminatorInCurrentFunction();
}

std::string IRBuilder::ensureName(const std::string& prefix, const std::string& name) {
    auto result = prefix + "." + name;
    auto& counter = nameCounters[result];
    ++counter;
    return result + "." + std::to_string(counter);
}

void IRBuilder::ensureTerminatorInCurrentFunction() {
    if (currentFunction) {
        for (size_t i = 0; i < currentFunction->getBasicBlocksCount(); ++i) {
            auto bb = currentFunction->getBasicBlock(i);
            if (bb->isTerminator()) {
                continue;
            } else {
                // 如果没有终止指令，添加一个ret void指令
                auto retInst = new RetInstr(nullptr, ensureName(bb->getName(), "ret"));
                bb->addInstruction(retInst);
                bb->setTerminator(true);
            }
        }
    }
}

void IRBuilder::setInsertPoint(BasicBlock* bb) {
    currentBlock = bb;
}
BasicBlock* IRBuilder::getInsertBlock() const {
    return currentBlock;
}

void IRBuilder::setInsertFunction(Function* func) {
    currentFunction = func;
}
Function* IRBuilder::getInsertFunction() const {
    return currentFunction;
}

// Const
Value* IRBuilder::createConstInt(LLVM::Type* type, int64_t value) {
    auto constInt = new ConstantInt(type, value, ensureName("Const", "Int"));
    ownedValue.push_back(std::unique_ptr<Value>(constInt));
    return constInt;
}
Value* IRBuilder::createConstArray(LLVM::Type* type, std::vector<Constant*> elements) {
    auto constDataArray = new ConstantArray(type, std::move(elements), ensureName("Const", "Array"));
    ownedValue.push_back(std::unique_ptr<Value>(constDataArray));
    return constDataArray;
}
Value* IRBuilder::createConstString(LLVM::Type* i8Type, const std::string& str) {
    auto constStr = new ConstString(i8Type, str, ensureName("Const", "String"));
    ownedValue.push_back(std::unique_ptr<Value>(constStr));
    return constStr;
}
// GlobalVariable
GlobalVariable* IRBuilder::createGlobalVariable(LLVM::Type* varType) {
    auto gv = new GlobalVariable(llvmContext.getPointerTyImpl(varType), ensureName("Global", "Variable"));
    ownedValue.push_back(std::unique_ptr<Value>(gv));
    return gv;
}
// Function
Function* IRBuilder::createFunction(LLVM::Type* funcType, const std::string& name, uint8_t mode) {
    auto func = new Function(funcType, name);
    if (mode == 0) { // definition
        // 确保当前函数的每个基本块有终止指令
        ensureTerminatorInCurrentFunction();
        // 创建入口基本块
        auto name = func->getName() + ".entry";
        auto entryBB = new BasicBlock(name);
        // 设置当前插入点为入口基本块
        currentBlock = entryBB;
        // 将入口基本块添加到函数中
        func->addBasicBlock(entryBB);
        
        // 设置为当前函数
        currentFunction = func;
    }else if (mode == 1) { // declaration
        // 不做任何操作
    } else {
        std::cout << "Error: Unknown function creation mode.\n";
        return nullptr;
    }
    ownedValue.push_back(std::unique_ptr<Value>(func));
    return func;
}

// Argument
Argument* IRBuilder::createArgument(LLVM::Type* type) {
    if (!currentFunction) {
        std::cout << "Error: No current function when creating argument.\n";
        return nullptr;
    }
    auto name = currentFunction->getName() + ".arg" + std::to_string(currentFunction->getArgumentsCount());
    auto arg = new Argument(type, currentFunction, currentFunction->getArgumentsCount(), name);
    // 将参数设置为函数的参数列表的一部分
    currentFunction->addArgument(arg);
    ownedValue.push_back(std::unique_ptr<Value>(arg));
    return arg;
}
// BasicBlock
// void
BasicBlock* IRBuilder::createBasicBlock() {
    if (!currentFunction) {
        std::cout << "Error: No current function when creating basic block.\n";
        return nullptr;
    }
    auto name = ensureName(currentFunction->getName(), "bb");
    auto bb = new BasicBlock(name);
    currentFunction->addBasicBlock(bb);
    // 设置为当前插入点
    currentBlock = bb;
    ownedValue.push_back(std::unique_ptr<Value>(bb));
    return bb;
}

// <result> = alloca <type>
// address
AllocaInst* IRBuilder::createAlloca(LLVM::Type* type) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating alloca instruction.\n";
        return nullptr;
    }
    auto allocaInst = new AllocaInst(llvmContext.getPointerTyImpl(type), ensureName(currentBlock->getName(), "alloca"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(allocaInst);
    ownedValue.push_back(std::unique_ptr<Value>(allocaInst));
    return allocaInst;
}

// <result> = load <type>, <type>* <ptr>
// value
LoadInst* IRBuilder::createLoad(Value* ptr) {
    if (!ptr->getType() || !ptr->getType()->isPointer()) {
        std::cout << "Error: Attempting to load from a non-pointer type.\n";
        return nullptr;
    }
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating load instruction.\n";
        return nullptr;
    }
    auto ptrType = static_cast<LLVM::PointerType*>(ptr->getType());
    auto loadInst = new LoadInst(ptrType->getPointedType(), ptr, ensureName(currentBlock->getName(), "load"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(loadInst);
    ownedValue.push_back(std::unique_ptr<Value>(loadInst));
    return loadInst;
}

// store <value>, <type>* <ptr>
// null
StoreInst* IRBuilder::createStore(Value* val, Value* ptr) {
    if (!ptr->getType() || !ptr->getType()->isPointer()) {
        std::cout << "Error: Attempting to store to a non-pointer type.\n";
        return nullptr;
    }
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating store instruction.\n";
        return nullptr;
    }
    auto storeInst = new StoreInst(val, ptr, ensureName(currentBlock->getName(), "store"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(storeInst);
    ownedValue.push_back(std::unique_ptr<Value>(storeInst));
    return storeInst;
}

// ret <type> <value>
// null
RetInstr* IRBuilder::createRet(Value* val) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating ret instruction.\n";
        return nullptr;
    }
    auto retInst = new RetInstr(val, ensureName(currentBlock->getName(), "ret"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(retInst);
    ownedValue.push_back(std::unique_ptr<Value>(retInst));
    return retInst;
}

// <result> = <BinaryOp> <ty> <op1>, <op2>
// value
AddInstr* IRBuilder::createAdd(Value* lhs, Value* rhs) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating add instruction.\n";
        return nullptr;
    }
    auto addInst = new AddInstr(llvmContext.getIntegerTyImpl(32), lhs, rhs, ensureName(currentBlock->getName(), "add"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(addInst);
    ownedValue.push_back(std::unique_ptr<Value>(addInst));
    return addInst;
}
SubInstr* IRBuilder::createSub(Value* lhs, Value* rhs) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating sub instruction.\n";
        return nullptr;
    }
    auto subInst = new SubInstr(llvmContext.getIntegerTyImpl(32), lhs, rhs, ensureName(currentBlock->getName(), "sub"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(subInst);
    ownedValue.push_back(std::unique_ptr<Value>(subInst));
    return subInst;
}
MulInstr* IRBuilder::createMul(Value* lhs, Value* rhs) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating mul instruction.\n";
        return nullptr;
    }
    auto mulInst = new MulInstr(llvmContext.getIntegerTyImpl(32), lhs, rhs, ensureName(currentBlock->getName(), "mul"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(mulInst);
    ownedValue.push_back(std::unique_ptr<Value>(mulInst));
    return mulInst;
}
SdivInstr* IRBuilder::createSdiv(Value* lhs, Value* rhs) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating sdiv instruction.\n";
        return nullptr;
    }
    auto sdivInst = new SdivInstr(llvmContext.getIntegerTyImpl(32), lhs, rhs, ensureName(currentBlock->getName(), "sdiv"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(sdivInst);
    ownedValue.push_back(std::unique_ptr<Value>(sdivInst));
    return sdivInst;
}

AndInstr* IRBuilder::createAnd(Value* lhs, Value* rhs) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating and instruction.\n";
        return nullptr;
    }
    auto andInst = new AndInstr(llvmContext.getIntegerTyImpl(1), lhs, rhs, ensureName(currentBlock->getName(), "and"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(andInst);
    ownedValue.push_back(std::unique_ptr<Value>(andInst));
    return andInst;
}
OrInstr* IRBuilder::createOr(Value* lhs, Value* rhs) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating or instruction.\n";
        return nullptr;
    }
    auto orInst = new OrInstr(llvmContext.getIntegerTyImpl(1), lhs, rhs, ensureName(currentBlock->getName(), "or"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(orInst);
    ownedValue.push_back(std::unique_ptr<Value>(orInst));
    return orInst;
}

// <result> = icmp <IcmpOp> <ty> <op1>, <op2>
// value
IcmpInstr* IRBuilder::createIcmp(IcmpInstr::IcmpOp op, Value* lhs, Value* rhs) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating icmp instruction.\n";
        return nullptr;
    }
    auto icmpInst = new IcmpInstr(op, llvmContext.getIntegerTyImpl(1), lhs, rhs, ensureName(currentBlock->getName(), "icmp"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(icmpInst);
    ownedValue.push_back(std::unique_ptr<Value>(icmpInst));
    return icmpInst;
}

// br <cond>, <trueBB>, <falseBB>
// null
BrInstr* IRBuilder::createBr(Value* cond, BasicBlock* trueBB, BasicBlock* falseBB) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating br instruction.\n";
        return nullptr;
    }
    auto brInst = new BrInstr(cond, trueBB, falseBB, ensureName(currentBlock->getName(), "br"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(brInst);
    ownedValue.push_back(std::unique_ptr<Value>(brInst));
    return brInst;
}

// br label <target>
// null
BrInstr* IRBuilder::createBr(BasicBlock* target) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating br instruction.\n";
        return nullptr;
    }
    auto brInst = new BrInstr(nullptr, target, nullptr, ensureName(currentBlock->getName(), "br"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(brInst);
    ownedValue.push_back(std::unique_ptr<Value>(brInst));
    return brInst;
}

// <result> = getelementptr <type>, <type>* <ptr>{, [inrange] <type> <index>}*
// address
GetElementPtrInst* IRBuilder::createGEP(Value* ptr, std::vector<Value*> indices) {
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating getelementptr instruction.\n";
        return nullptr;
    }
    auto type = ptr->getType();
    for (size_t i = 0; i < indices.size(); ++i) {
        if (!type->isPointer() && !type->isArray()) {
            std::cout << "Error: Attempting to get pointer from a non-pointer and non-array type";
            return nullptr;
        }
        type = type->isPointer() ? static_cast<LLVM::PointerType*>(type)->getPointedType()
                                 : static_cast<LLVM::ArrayType*>(type)->getElementType();
    }
    auto gepInst = new GetElementPtrInst(llvmContext.getPointerTyImpl(type), ptr, std::move(indices), ensureName(currentBlock->getName(), "gep"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(gepInst);
    ownedValue.push_back(std::unique_ptr<Value>(gepInst));
    return gepInst;
}

// <result> = call <type> <func>(<args>)
// value
CallInstr* IRBuilder::createCall(Function* func, std::vector<Value*> args) {
    if (!func->getType() || !func->getType()->isFunction()) {
        std::cout << "Error: Attempting to call a non-function type.\n";
        return nullptr;
    }
    if (!currentBlock) {
        std::cout << "Error: No current basic block when creating call instruction.\n";
        return nullptr;
    }
    auto funcType = static_cast<LLVM::FunctionType*>(func->getType());
    auto returnType = funcType->getReturnType();
    auto callInst = new CallInstr(returnType, func, std::move(args), ensureName(currentBlock->getName(), "call"));
    // 将指令添加到当前基本块
    currentBlock->addInstruction(callInst);
    ownedValue.push_back(std::unique_ptr<Value>(callInst));
    return callInst;
}