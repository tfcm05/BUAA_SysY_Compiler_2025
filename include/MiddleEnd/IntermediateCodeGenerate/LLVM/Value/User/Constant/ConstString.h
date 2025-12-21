#ifndef CONST_STRING_H
#define CONST_STRING_H

#include "Constant.h"

#include <string>

class ConstString : public Constant {
private:
    std::string string;
public:
    ConstString(LLVM::Type* i8Type, const std::string& str, const std::string& name);

    const std::string& getString() const;
    void setString(const std::string& str);
};

#endif