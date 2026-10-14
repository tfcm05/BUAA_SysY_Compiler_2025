#ifndef IR_PRINTER_H
#define IR_PRINTER_H

#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVM/Value.h"

#include <iosfwd>
#include <string>

class IRPrinter {
public:
    void printModule(const Module& module) const;
private:
    // helpers
    std::string formatType(const LLVM::Type* type) const;
    std::string formatValue(const Value* value) const;
    std::string formatConst(const Constant* constant) const;
    std::string formatIcmpOp(IcmpInstr::IcmpOp op) const;

    void printFunction(const Function* func, std::ostream& os) const;
    void printBasicBlock(const BasicBlock* bb, std::ostream& os) const;
    void printInstruction(const Instruction* inst, std::ostream& os) const;
};

#endif
