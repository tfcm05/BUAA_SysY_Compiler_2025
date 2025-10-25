#include"../../include/middle/Symbol.h"

Symbol::Symbol(const std::string& name, const DataType& dataType) : name(name), dataType(dataType) { }

VarSymbol::VarSymbol(const std::string& name, const DataType& dataType, bool isStatic, bool isConst) : Symbol(name, dataType), isStatic(isStatic), isConst(isConst) { }

FuncSymbol::FuncSymbol()