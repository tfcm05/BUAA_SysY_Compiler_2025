#ifndef LLVM_MODULE_H
#define LLVM_MODULE_H

#include "LLVM/Value.h"

#include <vector>

class Module {
private:
    std::vector<Function*> functions;
    std::vector<GlobalVariable*> globalVariables;
    std::vector<GlobalVariable*> globalConstStrs;
    Function* MainFunction;
public:
    Module() = default;
 
    void addFunction(Function* func);
    const std::vector<Function*>& getFunctions() const;
    
    void addGlobalVariable(GlobalVariable* gv);
    const std::vector<GlobalVariable*>& getGlobalVariables() const;

    void addGlobalConstStr(GlobalVariable* gv);
    const std::vector<GlobalVariable*>& getGlobalConstStrs() const;

    void setMainFunction(Function* func);
    Function* getMainFunction() const;
};

#endif