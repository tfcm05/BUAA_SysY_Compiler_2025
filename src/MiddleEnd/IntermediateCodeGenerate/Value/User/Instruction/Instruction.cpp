#include "../../../../../../include/MiddleEnd/IntermediateCodeGenerate/LLVM/Value.h"

// instruction
Instruction::Instruction(LLVM::Type* type, const std::string& name, bool terminatorFlag)
	: User(InstructionVK, type, name), terminatorFlag(terminatorFlag) {}

bool Instruction::isTerminator() const { return terminatorFlag; }
void Instruction::setTerminator(bool tf) { terminatorFlag = tf; }

// ===--------------------------------------------------------------------===//
// memort access instructions
// ===--------------------------------------------------------------------===//

// alloca instruction
AllocaInst::AllocaInst(LLVM::Type* type, const std::string& name)
	: Instruction(type, name, false) {}

// load instruction
LoadInst::LoadInst(LLVM::Type* type, Value* ptr, const std::string& name)
	: Instruction(type, name, false), ptr(ptr) {}

Value* LoadInst::getPointer() const { return ptr; }
void LoadInst::setPointer(Value* newPtr) { ptr = newPtr; }

// store instruction
StoreInst::StoreInst(Value* value, Value* ptr, const std::string& name)
	: Instruction(nullptr, name, false), value(value), ptr(ptr) {}

Value* StoreInst::getValue() const { return value; }
void StoreInst::setValue(Value* newValue) { value = newValue; }

Value* StoreInst::getPointer() const { return ptr; }
void StoreInst::setPointer(Value* newPtr) { ptr = newPtr; }

// getelementptr instruction
GetElementPtrInst::GetElementPtrInst(LLVM::Type* type, Value* ptr, std::vector<Value*> indices, const std::string& name)
	: Instruction(type, name, false), ptr(ptr), indices(std::move(indices)) {}

Value* GetElementPtrInst::getPointer() const { return ptr; }
void GetElementPtrInst::setPointer(Value* newPtr) { ptr = newPtr; }

size_t GetElementPtrInst::getIndicesCount() const { return indices.size(); }
Value* GetElementPtrInst::getIndex(size_t index) const { return indices.at(index); }
void GetElementPtrInst::addIndex(Value* index) { indices.push_back(index); }
void GetElementPtrInst::setIndices(std::vector<Value*> newIndices) { indices = std::move(newIndices); }
const std::vector<Value*>& GetElementPtrInst::getIndices() const { return indices; }

// ===--------------------------------------------------------------------===//
// arithmetic and logical instructions
// ===--------------------------------------------------------------------===//

// addition instruction
AddInstr::AddInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name)
	: Instruction(type, name, false), lhs(lhs), rhs(rhs) {}

Value* AddInstr::getLHS() const { return lhs; }
void AddInstr::setLHS(Value* newLHS) { lhs = newLHS; }

Value* AddInstr::getRHS() const { return rhs; }
void AddInstr::setRHS(Value* newRHS) { rhs = newRHS; }

// subtraction instruction
SubInstr::SubInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name)
	: Instruction(type, name, false), lhs(lhs), rhs(rhs) {}

Value* SubInstr::getLHS() const { return lhs; }
void SubInstr::setLHS(Value* newLHS) { lhs = newLHS; }

Value* SubInstr::getRHS() const { return rhs; }
void SubInstr::setRHS(Value* newRHS) { rhs = newRHS; }

// multiplication instruction
MulInstr::MulInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name)
	: Instruction(type, name, false), lhs(lhs), rhs(rhs) {}

Value* MulInstr::getLHS() const { return lhs; }
void MulInstr::setLHS(Value* newLHS) { lhs = newLHS; }

Value* MulInstr::getRHS() const { return rhs; }
void MulInstr::setRHS(Value* newRHS) { rhs = newRHS; }

// signed division instruction
SdivInstr::SdivInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name)
	: Instruction(type, name, false), lhs(lhs), rhs(rhs) {}

Value* SdivInstr::getLHS() const { return lhs; }
void SdivInstr::setLHS(Value* newLHS) { lhs = newLHS; }

Value* SdivInstr::getRHS() const { return rhs; }
void SdivInstr::setRHS(Value* newRHS) { rhs = newRHS; }

// and instruction
AndInstr::AndInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name)
	: Instruction(type, name, false), lhs(lhs), rhs(rhs) {}

Value* AndInstr::getLHS() const { return lhs; }
void AndInstr::setLHS(Value* newLHS) { lhs = newLHS; }

Value* AndInstr::getRHS() const { return rhs; }
void AndInstr::setRHS(Value* newRHS) { rhs = newRHS; }

// or instruction
OrInstr::OrInstr(LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name)
	: Instruction(type, name, false), lhs(lhs), rhs(rhs) {}

Value* OrInstr::getLHS() const { return lhs; }
void OrInstr::setLHS(Value* newLHS) { lhs = newLHS; }

Value* OrInstr::getRHS() const { return rhs; }
void OrInstr::setRHS(Value* newRHS) { rhs = newRHS; }

// integer comparison instruction
IcmpInstr::IcmpInstr(IcmpOp op, LLVM::Type* type, Value* lhs, Value* rhs, const std::string& name)
	: Instruction(type, name, false), op(op), lhs(lhs), rhs(rhs) {}

IcmpInstr::IcmpOp IcmpInstr::getPredicate() const { return op; }
void IcmpInstr::setPredicate(IcmpOp newOp) { op = newOp; }

Value* IcmpInstr::getLHS() const { return lhs; }
void IcmpInstr::setLHS(Value* newLHS) { lhs = newLHS; }

Value* IcmpInstr::getRHS() const { return rhs; }
void IcmpInstr::setRHS(Value* newRHS) { rhs = newRHS; }

// ===--------------------------------------------------------------------===//
// control flow instructions
// ===--------------------------------------------------------------------===//

// branch instruction
BrInstr::BrInstr(Value* condition, BasicBlock* trueBB, BasicBlock* falseBB, const std::string& name)
	: Instruction(nullptr, name, true), condition(condition), trueBB(trueBB), falseBB(falseBB) {}

Value* BrInstr::getCondition() const { return condition; }
void BrInstr::setCondition(Value* newCondition) { condition = newCondition; }

BasicBlock* BrInstr::getTrueBlock() const { return trueBB; }
void BrInstr::setTrueBlock(BasicBlock* newTrueBlock) { trueBB = newTrueBlock; }

BasicBlock* BrInstr::getFalseBlock() const { return falseBB; }
void BrInstr::setFalseBlock(BasicBlock* newFalseBlock) { falseBB = newFalseBlock; }

// phi instruction
PhiInstr::PhiInstr(LLVM::Type* type, std::vector<std::pair<Value*, BasicBlock*>> incoming, const std::string& name)
	: Instruction(type, name, false), incomingValues(std::move(incoming)) {}

void PhiInstr::addIncoming(Value* value, BasicBlock* block) { incomingValues.emplace_back(value, block); }
size_t PhiInstr::getIncomingCount() const { return incomingValues.size(); }
std::pair<Value*, BasicBlock*> PhiInstr::getIncoming(size_t index) const { return incomingValues.at(index); }
void PhiInstr::setIncoming(std::vector<std::pair<Value*, BasicBlock*>> newIncomingValues) { incomingValues = std::move(newIncomingValues); }
const std::vector<std::pair<Value*, BasicBlock*>>& PhiInstr::getIncomingValues() const { return incomingValues; }

// return instruction
RetInstr::RetInstr(Value* returnValue, const std::string& name)
	: Instruction(returnValue ? returnValue->getType() : nullptr, name, true), returnValue(returnValue) {}

Value* RetInstr::getReturnValue() const { return returnValue; }
void RetInstr::setReturnValue(Value* newReturnValue) { returnValue = newReturnValue; }

// ===--------------------------------------------------------------------===//
// function call instruction
// ===--------------------------------------------------------------------===//

// call instruction
CallInstr::CallInstr(LLVM::Type* type,Function* function, std::vector<Value*> arguments, const std::string& name)
	: Instruction(type, name, false), function(function), arguments(std::move(arguments)) {}

Function* CallInstr::getFunction() const { return function; }
void CallInstr::setFunction(Function* newFunction) { function = newFunction; }

size_t CallInstr::getArgumentsCount() const { return arguments.size(); }
Value* CallInstr::getArgument(size_t index) const {return arguments.at(index); }
void CallInstr::addArgument(Value* argument) { arguments.push_back(argument); }
void CallInstr::setArguments(std::vector<Value*> newArguments) { arguments = std::move(newArguments); }
const std::vector<Value*>& CallInstr::getArguments() const { return arguments; }
