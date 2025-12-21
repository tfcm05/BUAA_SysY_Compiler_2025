#include "../../../../../../../include/MiddleEnd/IntermediateCodeGenerate/LLVM/Value.h"

// global value
GlobalValue::GlobalValue(ValueKind kind, LLVM::Type* type, const std::string& name)
    : Constant(kind, type, name) {}

// function
Function::Function(LLVM::Type* funcType, const std::string& name) 
    : GlobalValue(FunctionVK, funcType, name), arguments(), basicBlocks() {}

size_t Function::getArgumentsCount() const { return arguments.size(); }
Argument* Function::getArgument(size_t index) const { return arguments.at(index); }
void Function::addArgument(Argument* arg) { arguments.push_back(arg); }
void Function::setArguments(std::vector<Argument*> newArguments) { arguments = std::move(newArguments); }
const std::vector<Argument*>& Function::getArguments() const { return arguments; }

size_t Function::getBasicBlocksCount() const { return basicBlocks.size(); }
BasicBlock* Function::getBasicBlock(size_t index) const { return basicBlocks.at(index); }
void Function::addBasicBlock(BasicBlock* bb) { basicBlocks.push_back(bb); }
void Function::setBasicBlocks(std::vector<BasicBlock*> newBasicBlocks) { basicBlocks = std::move(newBasicBlocks); }
const std::vector<BasicBlock*>& Function::getBasicBlocks() const { return basicBlocks; }

// global variable
GlobalVariable::GlobalVariable(LLVM::Type* type, const std::string& name)
    : GlobalValue(GlobalVariableVK, type, name), initValue(nullptr) {}

Constant* GlobalVariable::getInitializer() const { return initValue; }
void GlobalVariable::setInitializer(Constant* init) { initValue = init; }
