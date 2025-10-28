#include"../front/AstNode.h"
/*
class Visitor {
    private:
        // 针对前端 AST 的访问接口
        void visitFunctionDefStmt(const FunctionDefStmt& node);
        void visitVariableDefStmt(const VariableDefStmt& node);
        void visitVariableDef(const VariableDef& node, bool isStatic, bool isConst);
        void visitParam(const Param& node);
        void visitBlockStmt(const BlockStmt& node);
        void visitStmt(const Stmt& node); // 递归处理语句（查找嵌套声明或块）
        void visitIfStmt(const IfStmt& node);
        void visitForStmt(const ForStmt& node);

    public:
        void visitCompUnit(const AstNode& node);
};*/