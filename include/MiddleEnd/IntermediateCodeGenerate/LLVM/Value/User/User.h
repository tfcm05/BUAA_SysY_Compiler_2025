#ifndef USER_H
#define USER_H

#include "../Value.h"

#include <vector>
#include <string>

class User : public Value {
private:
    std::vector<Value*> usees;
protected:
    User(ValueKind kind, LLVM::Type* type, const std::string& name);

    size_t getUseesCount() const;
    Value* getUsee(size_t index) const;
    void addUsee(Value* usee);
    const std::vector<Value*>& getUsees() const;
};

#endif