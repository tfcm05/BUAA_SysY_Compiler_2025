#include "MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"
#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/IRBuilder.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"
#include "MiddleEnd/SemanticAnalysis/Type.h"
#include "MiddleEnd/SemanticAnalysis/Symbol.h"
#include "FrontEnd/SyntaxAnalysis/AstNodes.h"

// IRInfo Implementation
IRInfo::IRInfo(Value* val)
    : value(val) {}

Value* IRInfo::getValue() const { return value; }
void IRInfo::setValue(Value* val) { value = val; }

// IRContext Implementation — explicit mode flags
IRContext::IRContext()
    : createGlobalFlag(false), useLocationFlag(false), addressValue(nullptr),
      continueBB(nullptr), breakBB(nullptr), type(nullptr) {}

bool IRContext::createGlobal() const { return createGlobalFlag; }
void IRContext::setCreateGlobal(bool v) { createGlobalFlag = v; }
bool IRContext::useLocation() const { return useLocationFlag; }
void IRContext::setUseLocation(bool v) { useLocationFlag = v; }
void IRContext::clearModes() {
    createGlobalFlag = false;
    useLocationFlag = false;
}

Value* IRContext::getAddressValue() const { return addressValue; }
void IRContext::setAddressValue(Value* val) { addressValue = val; }

BasicBlock* IRContext::getContinueBB() const { return continueBB; }
void IRContext::setContinueBB(BasicBlock* bb) { continueBB = bb; }
BasicBlock* IRContext::getBreakBB() const { return breakBB; }
void IRContext::setBreakBB(BasicBlock* bb) { breakBB = bb; }

Value* IRContext::getValue() const { return addressValue; }
void IRContext::setValue(Value* val) { addressValue = val; }

LLVM::Type* IRContext::getType() const { return type; }
void IRContext::setType(LLVM::Type* ty) { type = ty; }

size_t IRContext::getIndicesCount() const { return indices.size(); }
size_t IRContext::getIndex(size_t index) const { return indices.at(index); }
void IRContext::addIndex(size_t index) { indices.push_back(index); }
void IRContext::clearIndices() { indices.clear(); }
void IRContext::setIndices(const std::vector<size_t>& idxs) { indices = idxs; }
const std::vector<size_t>& IRContext::getIndices() const { return indices; }

ContextPtr IRContext::clone() {
    auto cloned = std::make_shared<IRContext>();
    cloned->setCreateGlobal(createGlobalFlag);
    cloned->setUseLocation(useLocationFlag);
    cloned->addressValue = addressValue;
    cloned->continueBB = continueBB;
    cloned->breakBB = breakBB;
    cloned->setType(type);
    cloned->indices = indices;
    return cloned;
}

// IRVisitor Implementation
IRVisitor::IRVisitor(Module& mod, IRBuilder& bld, LLVMContext& ctx, const AnalysisResult& ar)
    : module(mod), builder(bld), llvmContext(ctx), analysis(ar) {}

Symbol* IRVisitor::resolveSymbol(AstNode* node) const {
    if (auto* sym = analysis.getSymbol(node)) {
        return sym;
    }
    // Migration fallback: Declarator / IdentifierExp may still carry Symbol*.
    if (auto* id = dynamic_cast<IdentifierExp*>(node)) {
        return id->getSymbol();
    }
    if (auto* decl = dynamic_cast<Declarator*>(node)) {
        return decl->getSymbol();
    }
    return nullptr;
}

void IRVisitor::finalize() {
    // Ensure every function basic block has a terminator before passes / print.
    builder.ensureTerminatorInCurrentFunction();
}

LLVM::Type* IRVisitor::convertType(Clang::Type* clangType) {
    if (!clangType) return llvmContext.getVoidTyImpl();

    if (auto it = typeCache.find(clangType); it != typeCache.end()) {
        return it->second;
    }

    LLVM::Type* llvmType = nullptr;
    switch (clangType->getTypeID()) {
        case Clang::Type::VoidTyID:
            llvmType = llvmContext.getVoidTyImpl();
            break;
        case Clang::Type::CharTyID:
            llvmType = llvmContext.getIntegerTyImpl(8);
            break;
        case Clang::Type::IntegerTyID:
            llvmType = llvmContext.getIntegerTyImpl(32);
            break;
        case Clang::Type::PointerTyID: {
            auto ptrType = static_cast<Clang::PointerType*>(clangType);
            auto elementType = convertType(ptrType->getPointedType());
            llvmType = llvmContext.getPointerTyImpl(elementType);
            break;
        }
        case Clang::Type::ArrayTyID: {
            auto arrayType = static_cast<Clang::ArrayType*>(clangType);
            auto elementType = convertType(arrayType->getElementType());
            llvmType = llvmContext.getArrayTyImpl(elementType, arrayType->getSize());
            break;
        }
        case Clang::Type::FunctionTyID: {
            auto funcType = static_cast<Clang::FunctionType*>(clangType);
            std::vector<LLVM::Type*> params;
            params.reserve(funcType->getParamTypesCount());
            for (size_t i = 0; i < funcType->getParamTypesCount(); ++i) {
                params.push_back(convertType(funcType->getParamType(i)));
            }
            auto retType = convertType(funcType->getReturnType());
            llvmType = llvmContext.getFunctionTyImpl(retType, params, funcType->isVariadic());
            break;
        }
        default:
            llvmType = llvmContext.getVoidTyImpl();
            break;
    }

    typeCache[clangType] = llvmType;
    return llvmType;
}

