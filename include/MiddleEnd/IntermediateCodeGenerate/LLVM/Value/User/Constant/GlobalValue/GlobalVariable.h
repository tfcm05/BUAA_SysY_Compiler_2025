#ifndef GLOBAL_VARIABLE_H
#define GLOBAL_VARIABLE_H

#include "GlobalValue.h"

#include <string>

class GlobalVariable : public GlobalValue {
private:
    Constant* initValue;
public:
    GlobalVariable(LLVM::Type* type, const std::string& name);

    Constant* getInitializer() const;
    void setInitializer(Constant* init);
};

#endif