#include"LlvmContext.h"
#include"../front/AstNode.h"

class Module {
    private:
        LlvmContext context;
    public:
        Module(const AstNode& root);
};