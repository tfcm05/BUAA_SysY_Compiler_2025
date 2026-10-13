#ifndef MIDDLEEND_OPTIMIZATION_PASS_H
#define MIDDLEEND_OPTIMIZATION_PASS_H

class Module;
class LLVMContext;

// Thin Module-level pass interface. Optimizations run on a frozen Module
// after IRVisitor finishes; they must not re-traverse the SysY AST.
class Pass {
public:
    virtual ~Pass() = default;
    virtual const char* getName() const = 0;
    // Returns a pass-specific metric (e.g. number of simplifications).
    virtual int runOnModule(Module& module, LLVMContext& context) = 0;
};

#endif
