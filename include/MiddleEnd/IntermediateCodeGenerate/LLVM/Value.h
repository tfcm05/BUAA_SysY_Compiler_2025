#ifndef LLVM_VALUE_H
#define LLVM_VALUE_H

#include "Value/Value.h"

#include "Value/Argument.h"
#include "Value/BasicBlock.h"

#include "Value/User/Constant/ConstInt.h"
#include "Value/User/Constant/ConstArray.h"
#include "Value/User/Constant/ConstString.h"

#include "Value/User/Constant/GlobalValue/Function.h"
#include "Value/User/Constant/GlobalValue/GlobalVariable.h"

#include "Value/User/Instruction/MemoryAccessInstr.h"
#include "Value/User/Instruction/ArithmeticAndLogicalInstr.h"
#include "Value/User/Instruction/ControlFlow.h"
#include "Value/User/Instruction/FunctionInstr.h"

#endif