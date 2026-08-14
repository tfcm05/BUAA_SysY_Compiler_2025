#include "../../../include/MiddleEnd/IntermediateCodeGenerate/IRPrinter.h"

void IRPrinter::printModule(const Module& module) const {
    std::ofstream os;
    os.open("llvm_ir.txt");
    if (!os.is_open()) {
        return;
    }

    os << "; 声明\n";
    os << "\n";
    

    os << "; global variables -----------------\n";
    for (const auto* gv : module.getGlobalVariables()) {
        os << "@" << gv->getName() << " = dso_local global " << formatType(static_cast<LLVM::PointerType*>(gv->getType())->getPointedType());
        if (gv->getInitializer()) {
            os << " " << formatConst(gv->getInitializer());
        } else {
            os << " zeroinitializer";
        }
        os << "\n";
    }
    if (!module.getGlobalVariables().empty()) {
        os << "\n";
    }

    os << "; global constant strings --------\n";
    for (const auto* gv : module.getGlobalConstStrs()) {
        os << "@" << gv->getName() << " = private unnamed_addr constant " << formatType(static_cast<LLVM::PointerType*>(gv->getType())->getPointedType());
        os << " " << formatConst(gv->getInitializer());
        os << "\n";
    }
    if (!module.getGlobalConstStrs().empty()) {
        os << "\n";
    }

    os << "; functions -----------------------\n";
    for (const auto* func : module.getFunctions()) {
        printFunction(func, os);
        os << "\n";
    }
    if (!module.getFunctions().empty()) {
        os << "\n";
    }

    os << "; main function -------------------\n";
    printFunction(module.getMainFunction(), os);
    os<< "\n";
}

void IRPrinter::printFunction(const Function* func, std::ostream& os) const {
    if (!func || !func->getType() || !func->getType()->isFunction()) return;
    auto funcTy = static_cast<LLVM::FunctionType*>(func->getType());

    if (func->getBasicBlocks().empty()) {
        // 函数声明 (无函数体, 如 int getint();), 链接时由运行库提供实现
        os << "declare " << formatType(funcTy->getReturnType()) << " @" << func->getName() << "(";
        for (size_t i = 0; i < funcTy->getParamsCount(); ++i) {
            if (i) os << ", ";
            os << formatType(funcTy->getParamType(i));
        }
        if (funcTy->isVariadicFunc()) {
            if (funcTy->getParamsCount()) os << ", ";
            os << "...";
        }
        os << ")\n";
        return;
    }

    os << "define dso_local " << formatType(funcTy->getReturnType()) << " @" << func->getName() << "(";
    for (size_t i = 0; i < funcTy->getParamsCount(); ++i) {
        if (i) os << ", ";
        os << formatType(funcTy->getParamType(i));
        os << " %" << func->getArgument(i)->getName();
    }
    if (funcTy->isVariadicFunc()) {
        if (funcTy->getParamsCount()) os << ", ";
        os << "...";
    }
    os << ") {\n";

    for (const auto* bb : func->getBasicBlocks()) {
        printBasicBlock(bb, os);
    }

    os << "}\n";
}

void IRPrinter::printBasicBlock(const BasicBlock* bb, std::ostream& os) const {
    if (!bb) return;
    os << bb->getName() << ":\n";
    for (const auto* inst : bb->getInstructions()) {
        os << "  ";
        printInstruction(inst, os);
        os << "\n";
    }
}

std::string IRPrinter::formatType(const LLVM::Type* type) const {
    if (!type) return "<nullty>";
    switch (type->getTypeID()) {
    case LLVM::Type::VoidTyID:
        return "void";
    case LLVM::Type::IntegerTyID: {
        auto intTy = static_cast<const LLVM::IntegerType*>(type);
        return "i" + std::to_string(intTy->getBitWidth());
    }
    case LLVM::Type::PointerTyID: {
        auto ptrTy = static_cast<const LLVM::PointerType*>(type);
        return formatType(ptrTy->getPointedType()) + "*";
    }
    case LLVM::Type::ArrayTyID: {
        auto arrTy = static_cast<const LLVM::ArrayType*>(type);
        std::ostringstream oss;
        oss << "[" << arrTy->getSize() << " x " << formatType(arrTy->getElementType()) << "]";
        return oss.str();
    }
    case LLVM::Type::FunctionTyID: {
        auto fnTy = static_cast<const LLVM::FunctionType*>(type);
        std::ostringstream oss;
        oss << formatType(fnTy->getReturnType()) << " (";
        for (size_t i = 0; i < fnTy->getParamsCount(); ++i) {
            if (i) oss << ", ";
            oss << formatType(fnTy->getParamType(i));
        }
        if (fnTy->isVariadicFunc()) {
            if (fnTy->getParamsCount()) oss << ", ";
            oss << "...";
        }
        oss << ")";
        return oss.str();
    }
    default:
        return "<unkty>";
    }
}

std::string IRPrinter::formatConst(const Constant* constant) const {
    if (!constant) return "<nullconst>";
    if (auto ci = dynamic_cast<const ConstantInt*>(constant)) {
        return std::to_string(ci->getValue());
    }
    if (auto ca = dynamic_cast<const ConstantArray*>(constant)) {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < ca->getElementsCount(); ++i) {
            if (i) oss << ", ";
            oss << formatType(ca->getElement(i)->getType()) << " ";
            oss << formatConst(ca->getElement(i));
        }
        oss << "]";
        return oss.str();
    }
    if (auto cs = dynamic_cast<const ConstString*>(constant)) {
        std::ostringstream oss;
        oss << "c\"";
        for (char c : cs->getString()) {
            if (c == '\n') {
                oss << "\\0A";
            } else if (c == '\0') {
                oss << "\\00";
            } else {
                oss << c;
            }
        }
        oss << "\"";
        return oss.str();
    }
    return "<const>";
}

std::string IRPrinter::formatValue(const Value* value) const {
    if (!value) return "<null>";
    if (value->isConstantInt()) {
        return formatConst(static_cast<const Constant*>(value));
    }
    if (value->isConstantArray()) {
        return formatConst(static_cast<const Constant*>(value));
    }
    if (value->isConstString()) {
        return formatConst(static_cast<const Constant*>(value));
    }
    if (value->isFunction() || value->isGlobalVariable()) {
        return "@" + value->getName();
    }
    if (value->isBasicBlock()) {
        return value->getName();
    }
    return "%" + value->getName();
}

std::string IRPrinter::formatIcmpOp(IcmpInstr::IcmpOp op) const {
    switch (op) {
    case IcmpInstr::IcmpOp::EQ: return "eq";
    case IcmpInstr::IcmpOp::NE: return "ne";
    case IcmpInstr::IcmpOp::SLT: return "slt";
    case IcmpInstr::IcmpOp::SLE: return "sle";
    case IcmpInstr::IcmpOp::SGT: return "sgt";
    case IcmpInstr::IcmpOp::SGE: return "sge";
    default: return "icmp";
    }
}

void IRPrinter::printInstruction(const Instruction* inst, std::ostream& os) const {
    if (!inst) {
        os << "<null inst>";
        return;
    }

    // Memory
    if (auto allocaI = dynamic_cast<const AllocaInst*>(inst)) {
        os << formatValue(allocaI) << " = alloca " << formatType(static_cast<LLVM::PointerType*>(allocaI->getType())->getPointedType());
        return;
    }
    if (auto loadI = dynamic_cast<const LoadInst*>(inst)) {
        os << formatValue(loadI) << " = load " << formatType(loadI->getType())
           << ", " << formatType(loadI->getPointer()->getType()) << " " << formatValue(loadI->getPointer());
        return;
    }
    if (auto storeI = dynamic_cast<const StoreInst*>(inst)) {
        os << "store " << formatType(storeI->getValue()->getType()) << " " << formatValue(storeI->getValue())
           << ", " << formatType(storeI->getPointer()->getType()) << " " << formatValue(storeI->getPointer());
        return;
    }
    if (auto gepI = dynamic_cast<const GetElementPtrInst*>(inst)) {
        
        os << formatValue(gepI) << " = getelementptr " << formatType(static_cast<LLVM::PointerType*>(gepI->getPointer()->getType())->getPointedType());
        os << ", " << formatType(gepI->getPointer()->getType()) << " " << formatValue(gepI->getPointer());
        for (size_t i = 0; i < gepI->getIndicesCount(); ++i) {
            os << ", " << formatType(gepI->getIndex(i)->getType()) << " " << formatValue(gepI->getIndex(i));
        }
        return;
    }

    // Arithmetic and logical
    if (auto addI = dynamic_cast<const AddInstr*>(inst)) {
        os << formatValue(addI) << " = add " << formatType(addI->getType()) << " "
           << formatValue(addI->getLHS()) << ", " << formatValue(addI->getRHS());
        return;
    }
    if (auto subI = dynamic_cast<const SubInstr*>(inst)) {
        os << formatValue(subI) << " = sub " << formatType(subI->getType()) << " "
           << formatValue(subI->getLHS()) << ", " << formatValue(subI->getRHS());
        return;
    }
    if (auto mulI = dynamic_cast<const MulInstr*>(inst)) {
        os << formatValue(mulI) << " = mul " << formatType(mulI->getType()) << " "
           << formatValue(mulI->getLHS()) << ", " << formatValue(mulI->getRHS());
        return;
    }
    if (auto divI = dynamic_cast<const SdivInstr*>(inst)) {
        os << formatValue(divI) << " = sdiv " << formatType(divI->getType()) << " "
           << formatValue(divI->getLHS()) << ", " << formatValue(divI->getRHS());
        return;
    }
    if (auto andI = dynamic_cast<const AndInstr*>(inst)) {
        os << formatValue(andI) << " = and " << formatType(andI->getType()) << " "
           << formatValue(andI->getLHS()) << ", " << formatValue(andI->getRHS());
        return;
    }
    if (auto orI = dynamic_cast<const OrInstr*>(inst)) {
        os << formatValue(orI) << " = or " << formatType(orI->getType()) << " "
           << formatValue(orI->getLHS()) << ", " << formatValue(orI->getRHS());
        return;
    }
    if (auto icmpI = dynamic_cast<const IcmpInstr*>(inst)) {
        os << formatValue(icmpI) << " = icmp " << formatIcmpOp(icmpI->getPredicate()) << " "
           << formatType(icmpI->getLHS()->getType()) << " " << formatValue(icmpI->getLHS())
           << ", " << formatValue(icmpI->getRHS());
        return;
    }
    if (auto zextI = dynamic_cast<const ZextInstr*>(inst)) {
        os << formatValue(zextI) << " = zext " << formatType(zextI->getOperand()->getType()) << " " << formatValue(zextI->getOperand())
           << " to " << formatType(zextI->getType());
        return;
    }

    // Call
    if (auto callI = dynamic_cast<const CallInstr*>(inst)) {
        auto funcType = static_cast<LLVM::FunctionType*>(callI->getFunction()->getType());
        if (funcType->getReturnType()->isVoid()) {
            os << "call " << formatType(funcType->getReturnType()) << " @" << callI->getFunction()->getName() << "(";
        } else {
            os << formatValue(callI) << " = call " << formatType(callI->getType()) << " @" << callI->getFunction()->getName() << "(";
        }
        for (size_t i = 0; i < callI->getArgumentsCount(); ++i) {
            if (i) os << ", ";
            auto arg = callI->getArgument(i);
            os << formatType(arg->getType()) << " " << formatValue(arg);
        }
        os << ")";
        return;
    }

    // Control flow
    if (auto brI = dynamic_cast<const BrInstr*>(inst)) {
        if (brI->getCondition()) {
            os << "br i1 " << formatValue(brI->getCondition()) << ", label %" << brI->getTrueBlock()->getName()
               << ", label %" << brI->getFalseBlock()->getName();
        } else {
            os << "br label %" << brI->getTrueBlock()->getName();
        }
        return;
    }
    if (auto retI = dynamic_cast<const RetInstr*>(inst)) {
        if (retI->getReturnValue()) {
            os << "ret " << formatType(retI->getReturnValue()->getType()) << " " << formatValue(retI->getReturnValue());
        } else {
            os << "ret void";
        }
        return;
    }
    if (auto phiI = dynamic_cast<const PhiInstr*>(inst)) {
        os << formatValue(phiI) << " = phi " << formatType(phiI->getType()) << " ";
        for (size_t i = 0; i < phiI->getIncomingCount(); ++i) {
            if (i) os << ", ";
            auto [val, bb] = phiI->getIncoming(i);
            os << "[" << formatValue(val) << ", %" << bb->getName() << "]";
        }
        return;
    }

    os << "<unhandled inst>";
}