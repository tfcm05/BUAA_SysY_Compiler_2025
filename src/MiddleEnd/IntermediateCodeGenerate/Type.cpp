#include "../../../include/MiddleEnd/IntermediateCodeGenerate/Type.h"
#include "../../../include/MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"

namespace LLVM {

//===----------------------------------------------------------------------===//
// Type 基类实现
//===----------------------------------------------------------------------===//
Type::Type(LLVMContext& context, TypeID id)
	: typeIdentifier(id), context(context) {}

Type::TypeID Type::getTypeID() const {
	return typeIdentifier;
}

LLVMContext& Type::getContext() const {
	return context;
}

bool Type::isVoid() const {
	return typeIdentifier == VoidTyID;
}

bool Type::isInteger() const {
	return typeIdentifier == IntegerTyID;
}

bool Type::isPointer() const {
	return typeIdentifier == PointerTyID;
}

bool Type::isArray() const {
	return typeIdentifier == ArrayTyID;
}

bool Type::isFunction() const {
	return typeIdentifier == FunctionTyID;
}

Type* Type::getVoidTy(LLVMContext& context) {
	return context.getVoidTyImpl();
}

Type* Type::getInt1Ty(LLVMContext& context) {
	return context.getIntegerTyImpl(1);
}

Type* Type::getInt8Ty(LLVMContext& context) {
	return context.getIntegerTyImpl(8);
}

Type* Type::getInt32Ty(LLVMContext& context) {
	return context.getIntegerTyImpl(32);
}

//===----------------------------------------------------------------------===//
// VoidType 实现
//===----------------------------------------------------------------------===//
VoidType::VoidType(LLVMContext& context)
	: Type(context, VoidTyID) {}

VoidType* VoidType::get(LLVMContext& context) {
	return context.getVoidTyImpl();
}

//===----------------------------------------------------------------------===//
// IntegerType 实现
//===----------------------------------------------------------------------===//
IntegerType::IntegerType(LLVMContext& context, unsigned int bw)
	: Type(context, IntegerTyID), bitWidth(bw) {}

IntegerType* IntegerType::get(LLVMContext& context, unsigned int bw) {
	return context.getIntegerTyImpl(bw);
}

unsigned int IntegerType::getBitWidth() const {
	return bitWidth;
}

//===----------------------------------------------------------------------===////
// PointerType 实现
//===----------------------------------------------------------------------===//
PointerType::PointerType(Type* elementType)
	: Type(elementType->getContext(), PointerTyID), pointedType(elementType) {}

PointerType* PointerType::get(Type* elementType) {
	auto& context = elementType->getContext();
	return context.getPointerTyImpl(elementType);
}

Type* PointerType::getPointedType() const {
	return pointedType;
}

//===----------------------------------------------------------------------===//
// ArrayType 实现
//===----------------------------------------------------------------------===//
ArrayType::ArrayType(Type* elemType, size_t sz)
	: Type(elemType->getContext(), ArrayTyID), elementType(elemType), size(sz) {}

ArrayType* ArrayType::get(Type* elemType, size_t sz) {
	auto& context = elemType->getContext();
	return context.getArrayTyImpl(elemType, sz);
}

Type* ArrayType::getElementType() const {
	return elementType;
}

size_t ArrayType::getSize() const {
	return size;
}

//===----------------------------------------------------------------------===////
// FunctionType 实现
//===----------------------------------------------------------------------===//
FunctionType::FunctionType(Type* retType, const std::vector<Type*>& params, bool vf)
	: Type(retType->getContext(), FunctionTyID), returnType(retType), paramTypes(params), variadicFlag(vf) {}

FunctionType* FunctionType::get(Type* retType, const std::vector<Type*>& params, bool vf) {
	auto& context = retType->getContext();
	return context.getFunctionTyImpl(retType, params, vf);
}

Type* FunctionType::getReturnType() const {
	return returnType;
}

size_t FunctionType::getParamsCount() const {
	return paramTypes.size();
}

Type* FunctionType::getParamType(size_t index) const {
	if (index >= paramTypes.size()) {
		return nullptr;
	}
	return paramTypes[index];
}

const std::vector<Type*>& FunctionType::getParamTypes() const {
	return paramTypes;
}

bool FunctionType::isVariadicFunc() const {
	return variadicFlag;
}

bool FunctionType::setVariadicFlag(bool vf) {
	bool previous = variadicFlag;
	variadicFlag = vf;
	return previous;
}

}