#include<string>
#include<vector>

typedef enum {
    DATA_VOID,
    DATA_INT,
    DATA_ARRAY,
} DataType;

typedef enum {
    SYMBOL_VAR,
    SYMBOL_FUNC,
} SymbolType;

struct TypeInfo {
    DataType dataType;
    int size; // 类型大小
    std::vector<TypeInfo> elements; // 数组元素
};

class Symbol {
    protected:
        std::string name;
        SymbolType symbolType; // 符号类型
        TypeInfo typeInfo; // 类型信息

        bool isConst; // 常量
        bool isGlobal; // 全局

        int scopeLevel;  // 作用域层级

        // llvm
};

class VarSymbol : Symbol {

};

class FuncSymbol : Symbol {
    private:
        TypeInfo returnType; // 函数返回类型
        std::vector<TypeInfo> params;   // 函数参数类型
};