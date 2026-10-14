#ifndef ERROR_H
#define ERROR_H

#include <string>

// 错误码按编译阶段分类
//   词法: IllegalChar
//   语法: MissingSemicolon / MissingRightParen / MissingRightBracket
//   语义: 声明与作用域 / 函数调用 / 返回语句 / 常量与初始化 / 数组 / 赋值 / 控制流
typedef enum {
    // ===== 词法错误 =====
    IllegalChar,                // 非法字符

    // ===== 语法错误 =====
    MissingSemicolon,           // 缺少分号
    MissingRightParen,         // 缺少右小括号
    MissingRightBracket,       // 缺少右中括号

    // ===== 语义错误: 声明与作用域 =====
    RedefinedName,             // 名字重定义
    UndefinedName,             // 未定义的名字

    // ===== 语义错误: 函数调用 =====
    ParamCountMismatch,        // 函数参数个数不匹配
    ParamTypeMismatch,         // 函数参数类型不匹配
    CallNonFunction,           // 对非函数类型进行调用

    // ===== 语义错误: 返回语句 =====
    ReturnTypeMismatch,        // return语句与函数返回类型不匹配
    MissingReturn,             // 有返回值的函数缺少return语句

    // ===== 语义错误: 常量与初始化 =====
    ConstAssignment,           // 不能改变常量的值
    GlobalStaticNonConstInit,  // 全局/static变量初始化必须为常量表达式
    ConstDivByZero,            // 常量表达式除以零

    // ===== 语义错误: 数组 =====
    ArraySizeNotConst,         // 数组大小不是常量
    ArraySizeNotPositive,      // 数组大小必须为正
    SubscriptNonArray,         // 对非数组/非指针类型使用下标

    // ===== 语义错误: 赋值 =====
    AssignmentToNonLValue,     // 赋值左侧不是左值
    AddressOfNonLValue,       // 对非左值取地址
    DerefNonPointer,          // 对非指针类型解引用

    // ===== 语义错误: 控制流 =====
    BreakContinueOutsideLoop,  // 非循环块中使用break/continue
} ErrorCode;

void errorLog(ErrorCode errorCode, int line, const std::string& msg = "");

std::string errorCodeToString(ErrorCode code);

void dumpErrorsToFile();

int getErrorCount();

#endif