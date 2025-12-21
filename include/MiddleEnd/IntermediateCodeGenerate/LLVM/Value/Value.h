#ifndef MY_VALUE_H
#define MY_VALUE_H

#include "../../Type.h"

class User;

#include <string>
#include <vector>

class Value {
public:
    enum ValueKind {
        // value->user->constant
        ConstantIntVK,
        ConstantArrayVK,
        ConstantStringVK,

        // value->user->constant->globalvalue
        FunctionVK,
        GlobalVariableVK,

        // value->user
        InstructionVK,
        
        // value
        ArgumentVK,
        BasicBlockVK,
    };
private:
    ValueKind kind;
    LLVM::Type* type;
    std::string name;
    std::vector<User*> users;
protected:
    Value(ValueKind kind, LLVM::Type* type, std::string name);
public:
    virtual ~Value() = default;

    ValueKind getKind() const;

    bool isConstantInt() const;
    bool isConstantArray() const;
    bool isConstString() const;
    
    bool isFunction() const;
    bool isGlobalVariable() const;

    bool isInstruction() const;

    bool isArgument() const;
    bool isBasicBlock() const;

    LLVM::Type* getType() const;
    void setType(LLVM::Type* newType);

    const std::string& getName() const;
    void setName(const std::string& newName);

    
    size_t getUsersCount() const;
    User* getUser(size_t index) const;
    void addUser(User* user);
    const std::vector<User*>& getUsers() const;
};

#endif
