#include "../../../../../include/MiddleEnd/IntermediateCodeGenerate/LLVM/Value.h"

// user
User::User(ValueKind kind, LLVM::Type* type, const std::string& name)
	: Value(kind, type, name) {}

size_t User::getUseesCount() const {
    return usees.size();
}
Value* User::getUsee(size_t index) const {
    return usees.at(index);
}
void User::addUsee(Value* usee) {
    usees.push_back(usee);
}
const std::vector<Value*>& User::getUsees() const {
    return usees;
}

