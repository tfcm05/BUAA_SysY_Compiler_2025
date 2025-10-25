#include<vector>
#include"value.h"

class LlvmContext {
    std::vector<FunctionPtr> functions;

    std::vector<ValuePtr> values;
    std::vector<UsePtr> uses;
};