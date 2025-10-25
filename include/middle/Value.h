#include<string>
#include<vector>
#include<memory>

class Value;
class User;
class Function;
class BasicBlock;
class Instruction;

using ValuePtr = std::shared_ptr<Value>;
using UserPtr = std::shared_ptr<User>;
using FunctionPtr = std::shared_ptr<Function>;
using BasicBlockPtr = std::shared_ptr<BasicBlock>;
using InstructionPtr = std::shared_ptr<Instruction>;

struct Use {
    UserPtr user;
    ValuePtr usee;
};

using UsePtr = std::shared_ptr<Use>;
using UseRefList = std::vector<UsePtr>;

enum class ReturnType {
    VoidReTy,       // 空返回值
    LabelReTy,      // 标签类型

    IntegerReTy,    // 整数类型
    FunctionReTy,   // 函数类型
};

enum class ValueType {
    // 函数内
    ArgumentTy, //参数
    BasicBlockTy, //基本块

    // 常量
    ConstantTy, //常量标识符
    ConstantDataTy, //字面量

    // 全局量
    FunctionTy,
    GlobalVariableTy,

    // 指令
    BinaryOperatorTy,
    CompareInstTy,
    BranchInstTy,
    ReturnInstTy,
    StoreInstTy,
    CallInstTy,
    InputInstTy,
    OutputInstTy,
    AllocaInstTy,
    LoadInstTy,
    UnaryOperatorTy,
};

class Value {
    protected:
        ReturnType returnType;
        std::string name;

        UseRefList pre; // user - this
        UseRefList next; // this - usee

    private:
        ValueType valueType;

    public:
        Value(ReturnType reTy, ValueType vaTy, std::string name);
        ~Value();
        void addPre(UserPtr use);
        void addNext(UserPtr use);

};

class User : public Value {

};

class Function : public User {

};

class BasicBlock : public User {

};

class Instruction : public User {

};