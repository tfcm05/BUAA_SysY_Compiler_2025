#include "../../../include/MiddleEnd/SemanticAnalysis/ConstValue.h"
#include "../../../include/MiddleEnd/SemanticAnalysis/ClangContext.h"

// ConstValue implementation
Clang::ConstValue::ConstValue(ClangContext& context, ConstKind k) : context(context), kind(k) {}

Clang::ConstValue::ConstKind Clang::ConstValue::getKind() const { return kind; }

bool Clang::ConstValue::isConstInteger() const { return kind == ConstKind::CONST_INTEGER; }
bool Clang::ConstValue::isConstArray() const { return kind == ConstKind::CONST_ARRAY; }

// ConstValue derived classes
Clang::ConstInteger::ConstInteger(ClangContext& context, int64_t val)
    : ConstValue(context, ConstKind::CONST_INTEGER), intValue(val) {}
int64_t Clang::ConstInteger::getIntValue() const { return intValue; }
void Clang::ConstInteger::setIntValue(int64_t val) { intValue = val; }

Clang::ConstArray::ConstArray(ClangContext& context, std::vector<ConstValue*> elems)
    : ConstValue(context, ConstKind::CONST_ARRAY), elements(std::move(elems)) {}

const size_t Clang::ConstArray::getElementsSize() const { return elements.size(); }
Clang::ConstValue* Clang::ConstArray::getElement(size_t index) const { return elements[index]; }
void Clang::ConstArray::addElement(Clang::ConstValue* elem) { elements.push_back(elem); }
const std::vector<Clang::ConstValue*>& Clang::ConstArray::getElements() const { return elements; }