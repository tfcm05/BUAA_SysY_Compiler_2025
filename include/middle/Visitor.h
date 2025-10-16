#include"AstNode.h"

class Visitor {
    private:
        void _visitFuncDef(const AstNode& node);
        void _visitVarDef(const AstNode& node);

    public:
        void visitCompUnit(const AstNode& node);
};