#ifndef ERROR_H
#define ERROR_H

#include <string>

typedef enum {
    IllegalChar,               // a: 非法字符
    RedefinedName,             // b: 名字重定义
    UndefinedName,             // c: 未定义的名字
    ParamCountMismatch,        // d: 函数参数个数不匹配
    ParamTypeMismatch,         // e: 函数参数类型不匹配
    ReturnTypeMismatch,        // f: return 语句与函数返回类型不匹配
    MissingReturn,             // g: 有返回值的函数缺少 return 语句
    ConstAssignment,           // h: 不能改变常量的值
    MissingSemicolon,          // i: 缺少分号
    MissingRightParen,         // j: 缺少右小括号
    MissingRightBracket,       // k: 缺少右中括号
    PrintfFormatMismatch,      // l: printf 格式不匹配 (已无调用, 保留)
    BreakContinueOutsideLoop,  // m: 非循环块中使用 break/continue
} ErrorCode;

void errorLog(ErrorCode errorCode, int line, const std::string& msg = "");

// 统一语义错误报告: 输出到 stdout + 递增错误计数 (阻止 IR 生成)
// 用于无标准 SysY 错误码的语义错误 (全局/static 初始化器非常量、编译期除零等)
void semanticError(int line, const std::string& message);

// 仅递增错误计数, 不输出 (配合自定义错误输出使用)
void incrementErrorCount();

std::string errorCodeToString(ErrorCode code);

void dumpErrorsToFile();

int getErrorCount();

#endif