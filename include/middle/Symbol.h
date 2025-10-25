#ifndef SYMBOL_H
#define SYMBOL_H

#include"Value.h"
#include<string>
#include<vector>

enum class DataType {
    DATA_VOID,
    DATA_INT,
    DATA_ARRAY,
};

class Symbol {
    protected:
        std::string name;
        DataType dataType; // 类型

        // llvm
        ValuePtr value;
    public:
        // 初始化
        Symbol(const std::string& name, const DataType& dataType);
};

class VarSymbol : public Symbol {
    private:
        bool isStatic;
        bool isConst;
    public:
        VarSymbol(const std::string& name, const DataType& dataType, bool isStatic, bool isConst);
};

class FuncSymbol : public Symbol {
    private:
        std::vector<make_shared<VarSymbol>> parameters;
};

#endif