#include<string>
#include"../../include/middle/Visitor.h"
#include"../../include/middle/SymbolTable.h"
#include"../../include/middle/Symbol.h"
#include <iostream>

void Visitor::visitCompUnit(const AstNode& node) {
    // 把传入的 AstNode 转为前端的 CompUnit
    auto cu = dynamic_cast<const CompUnit*>(&node);
    if (!cu) return;

    // 全局作用域
    curScope = std::make_shared<SymbolTable>();

    // 全局变量定义
    for (size_t i = 0; i < cu->getGlobalVarDefStmtsCount(); ++i) {
        visitVariableDefStmt(*cu->getGlobalVarDefStmt(i));
    }

    // 函数定义
    for (size_t i = 0; i < cu->getFuncDefStmtsCount(); ++i) {
        visitFunctionDefStmt(*cu->getFuncDefStmt(i));
    }

    // main 函数
    if (cu->getMainFuncDef()) {
        visitFunctionDefStmt(*cu->getMainFuncDef());
    }
}

void Visitor::visitVariableDefStmt(const VariableDefStmt& node) {
    bool isStatic = node.isStatic();
    bool isConst = node.isConst();

    for (size_t i = 0; i < node.getDefsCount(); ++i) {
        visitVariableDef(*node.getDef(i), isStatic, isConst);
    }
}

void Visitor::visitVariableDef(const VariableDef& node, bool isStatic, bool isConst) {
    std::string ident = node.getIdentifier();
    DataType dt = node.isArray() ? DataType::DATA_ARRAY : DataType::DATA_INT;

    auto var = std::make_shared<VarSymbol>(ident, dt, isStatic, isConst);

    // 生成类型名称（按题目统一映射）
    std::string typeName;
    bool isArray = node.isArray();
    if (isConst) {
        typeName = isArray ? "ConstIntArray" : "ConstInt";
    } else if (isStatic) {
        typeName = isArray ? "StaticIntArray" : "StaticInt";
    } else {
        typeName = isArray ? "IntArray" : "Int";
    }

    // 添加符号表并在成功时输出：作用域序号，单词文本，类型名称
    if (curScope->addSymbol(ident, var)) {
        std::cout << curScope->getId() << " " << ident << " " << typeName << "\n";
    }
}

void Visitor::visitFunctionDefStmt(const FunctionDefStmt& node) {
    // 先在当前作用域（一般为全局）加入函数符号
    DataType dt = (node.getFuncType() == BasicType::INT) ? DataType::DATA_INT : DataType::DATA_VOID;
    auto funcSym = std::make_shared<FuncSymbol>(node.getIdentifier(), dt);

    std::string typeName = (dt == DataType::DATA_VOID) ? "VoidFunc" : "IntFunc";

    if (curScope->addSymbol(node.getIdentifier(), funcSym)) {
        std::cout << curScope->getId() << " " << node.getIdentifier() << " " << typeName << "\n";
    }

    // 为函数体创建新的作用域（形参与局部变量在此）
    curScope = curScope->pushScope();

    // 形参
    for (size_t i = 0; i < node.getFormatParamsCount(); ++i) {
        visitParam(*node.getFormatParam(i));
    }

    // 函数体（BlockStmt）
    if (node.getStmt()) {
        visitBlockStmt(*dynamic_cast<const BlockStmt*>(node.getStmt()));
    }

    // 退出函数作用域
    curScope = curScope->popScope();
}

void Visitor::visitParam(const Param& node) {
    std::string ident = node.getIdentifier();
    DataType dt = node.getParamType() == BasicType::INT ? DataType::DATA_INT : DataType::DATA_VOID;
    if (node.isArray()) dt = DataType::DATA_ARRAY;
    if (dt == DataType::DATA_VOID) {
        // 一般不会出现 void 参数，按需要可忽略或处理
    }

    auto var = std::make_shared<VarSymbol>(ident, dt, false, false);

    // 参数类型按无 static/const 的普通参数处理
    std::string typeName = node.isArray() ? "IntArray" : "Int";

    if (curScope->addSymbol(ident, var)) {
        std::cout << curScope->getId() << " " << ident << " " << typeName << "\n";
    }
}

void Visitor::visitBlockStmt(const BlockStmt& node) {
    // 为 block 创建新的子作用域
    curScope = curScope->pushScope();

    for (size_t i = 0; i < node.getStmtsCount(); ++i) {
        const Stmt* s = node.getStmt(i);
        if (s) visitStmt(*s);
    }

    // 退出 block 作用域
    curScope = curScope->popScope();
}

void Visitor::visitStmt(const Stmt& node) {
    // 识别语句类型并递归处理可能出现的块或声明
    if (auto blk = dynamic_cast<const BlockStmt*>(&node)) {
        visitBlockStmt(*blk);
        return;
    }
    if (auto vds = dynamic_cast<const VariableDefStmt*>(&node)) {
        visitVariableDefStmt(*vds);
        return;
    }
    if (auto ifs = dynamic_cast<const IfStmt*>(&node)) {
        visitIfStmt(*ifs);
        return;
    }
    if (auto fs = dynamic_cast<const ForStmt*>(&node)) {
        visitForStmt(*fs);
        return;
    }
    // 其他语句（JumpStmt, ExpStmt 等）通常不影响符号表。
}

void Visitor::visitIfStmt(const IfStmt& node) {
    // 真分支
    if (node.getTrueStmt()) visitStmt(*node.getTrueStmt());
    // 假分支
    if (node.getFalseStmt()) visitStmt(*node.getFalseStmt());
}

void Visitor::visitForStmt(const ForStmt& node) {
    // for 的初始化项、条件、迭代可能包含表达式或内嵌声明（如果语法允许）
    // 这里只处理循环体语句（若循环体为块，会创建子作用域）
    if (node.getStmt()) visitStmt(*node.getStmt());
}