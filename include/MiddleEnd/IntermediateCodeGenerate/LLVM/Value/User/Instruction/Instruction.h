#ifndef INSTRUCTION_H
#define INSTRUCTION_H
#include "../User.h"

#include <string>

class Instruction : public User {
private:
    bool terminatorFlag;
protected:
	Instruction(LLVM::Type* type, const std::string& name, bool terminatorFlag);
public:
	bool isTerminator() const;
	void setTerminator(bool tf);
};

#endif