#include "../../../../include/MiddleEnd/IntermediateCodeGenerate/LLVM/Value.h"

// value
Value::Value(ValueKind kind, LLVM::Type* type, std::string name)
	: kind(kind), type(type), name(std::move(name)) {}

Value::ValueKind Value::getKind() const {
	return kind;
}

bool Value::isConstantInt() const {
	return kind == ConstantIntVK;
}
bool Value::isConstantArray() const {
	return kind == ConstantArrayVK;
}
bool Value::isConstString() const {
	return kind == ConstantStringVK;
}
bool Value::isFunction() const {
	return kind == FunctionVK;
}
bool Value::isGlobalVariable() const {
	return kind == GlobalVariableVK;
}
bool Value::isInstruction() const {
	return kind == InstructionVK;
}
bool Value::isArgument() const {
	return kind == ArgumentVK;
}
bool Value::isBasicBlock() const {
	return kind == BasicBlockVK;
}

LLVM::Type* Value::getType() const {
	return type;
}
void Value::setType(LLVM::Type* newType) {
	type = newType;
}

const std::string& Value::getName() const {
	return name;
}
void Value::setName(const std::string& newName) {
	name = newName;
}

size_t Value::getUsersCount() const {
	return users.size();
}
User* Value::getUser(size_t index) const {
	return users.at(index);
}
void Value::addUser(User* user) {
	users.push_back(user);
}
const std::vector<User*>& Value::getUsers() const {
	return users;
}

// argument
Argument::Argument(LLVM::Type* type, Function* parentFunction, size_t argNumber, const std::string& name)
	: Value(ArgumentVK, type, name), parentFunction(parentFunction), argNumber(argNumber) {}

Function* Argument::getParentFunction() const {
	return parentFunction;
}
void Argument::setParentFunction(Function* newParentFunction) {
	parentFunction = newParentFunction;
}

size_t Argument::getArgNumber() const {
	return argNumber;
}
void Argument::setArgNumber(size_t newArgNumber) {
	argNumber = newArgNumber;
}

// basic block
BasicBlock::BasicBlock(const std::string& name)
	: Value(BasicBlockVK, nullptr, name), terminatorFlag(false) {}

bool BasicBlock::isTerminator() const {
	return terminatorFlag;
}
void BasicBlock::setTerminator(bool tf) {
	terminatorFlag = tf;
}

size_t BasicBlock::getInstructionsCount() const {
	return instructions.size();
}
Instruction* BasicBlock::getInstruction(size_t index) const {
	return instructions.at(index);
}
void BasicBlock::addInstruction(Instruction* instr) {
	instructions.push_back(instr);
	terminatorFlag = instr->isTerminator();
}
void BasicBlock::setInstructions(std::vector<Instruction*> newInstructions) {
	instructions = std::move(newInstructions);
}
const std::vector<Instruction*>& BasicBlock::getInstructions() const {
	return instructions;
}
