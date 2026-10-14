#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value.h"

// constant
Constant::Constant(ValueKind kind, LLVM::Type* type, const std::string& name)
    : User(kind, type, name) {}

// constant int
ConstantInt::ConstantInt(LLVM::Type* type, int64_t val, const std::string& name)
    : Constant(ConstantIntVK, type, name), value(val) {}

int64_t ConstantInt::getValue() const { return value; }
void ConstantInt::setValue(int64_t newValue) { value = newValue; }

// constant array
ConstantArray::ConstantArray(LLVM::Type* type, std::vector<Constant*> elements, const std::string& name)
    : Constant(ConstantArrayVK, type, name), elementType(), elements(std::move(elements)) {}

LLVM::Type* ConstantArray::getElementType() const { return elementType; }
void ConstantArray::setElementType(LLVM::Type* newElementType) { elementType = newElementType; }

size_t ConstantArray::getElementsCount() const { return elements.size(); }
Constant* ConstantArray::getElement(size_t index) const { return elements.at(index); }
void ConstantArray::addElement(Constant* element) { elements.push_back(element); }
void ConstantArray::setElements(std::vector<Constant*> newElements) { elements = std::move(newElements); }
const std::vector<Constant*>& ConstantArray::getElements() const { return elements; }

// constant string
ConstString::ConstString(LLVM::Type* i8Type, const std::string& str, const std::string& name)
    : Constant(ConstantStringVK, i8Type, name), string(str) { }

const std::string& ConstString::getString() const { return string; }
void ConstString::setString(const std::string& str) { string = str; }
