#include "MiddleEnd/IntermediateCodeGenerate/Module.h"

void Module::addFunction(Function* func) { functions.push_back(func); }
const std::vector<Function*>& Module::getFunctions() const { return functions; }

void Module::addGlobalVariable(GlobalVariable* gv) { globalVariables.push_back(gv); }
const std::vector<GlobalVariable*>& Module::getGlobalVariables() const { return globalVariables; }

void Module::addGlobalConstStr(GlobalVariable* gv) { globalConstStrs.push_back(gv); }
const std::vector<GlobalVariable*>& Module::getGlobalConstStrs() const { return globalConstStrs; }

Function* Module::getMainFunction() const { return MainFunction; }
void Module::setMainFunction(Function* func) { MainFunction = func; }