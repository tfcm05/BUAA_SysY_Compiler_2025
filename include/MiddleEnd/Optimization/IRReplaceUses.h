#ifndef MIDDLEEND_OPTIMIZATION_IR_REPLACE_USES_H
#define MIDDLEEND_OPTIMIZATION_IR_REPLACE_USES_H

class Function;
class Value;

// Safe operand rewrite: Value::users / User::usees are not maintained by
// IRBuilder setters, so Passes must scan every operand slot in a Function.
// Returns true if at least one operand pointer was rewritten.
// Does not rewrite the instruction that *is* oldValue (avoids self-mutation).
bool replaceUsesInFunction(Function* function, Value* oldValue, Value* newValue);

// Collect Module functions plus MainFunction if it is not already listed.
#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include <vector>

inline std::vector<Function*> collectModuleFunctions(Module& module) {
    std::vector<Function*> funcs = module.getFunctions();
    if (Function* mainFunc = module.getMainFunction()) {
        bool found = false;
        for (Function* f : funcs) {
            if (f == mainFunc) {
                found = true;
                break;
            }
        }
        if (!found) {
            funcs.push_back(mainFunc);
        }
    }
    return funcs;
}

#endif
