#ifndef CONSTANT_H
#define CONSTANT_H

#include "../User.h"

#include <string>

class Constant : public User {
protected:
    Constant(ValueKind kind, LLVM::Type* type, const std::string& name);
public:
    virtual ~Constant() = default;
};

#endif

