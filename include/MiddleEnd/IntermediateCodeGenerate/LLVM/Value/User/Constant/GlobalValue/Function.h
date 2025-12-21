#ifndef FUNCTION_H
#define FUNCTION_H

#include "GlobalValue.h"
#include "../../../Argument.h"
#include "../../../BasicBlock.h"
	
#include <string>
#include <vector>

class Function : public GlobalValue {
private:
	std::vector<Argument*> arguments;
	std::vector<BasicBlock*> basicBlocks;
public:
	Function(LLVM::Type* funcType, const std::string& name);

	size_t getArgumentsCount() const;
	Argument* getArgument(size_t index) const;
	void addArgument(Argument* arg);
	void setArguments(std::vector<Argument*> newArguments);
	const std::vector<Argument*>& getArguments() const;

	size_t getBasicBlocksCount() const;
	BasicBlock* getBasicBlock(size_t index) const;
	void addBasicBlock(BasicBlock* bb);
	void setBasicBlocks(std::vector<BasicBlock*> newBasicBlocks);
	const std::vector<BasicBlock*>& getBasicBlocks() const;
};

#endif
