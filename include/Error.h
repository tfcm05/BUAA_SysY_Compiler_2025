#ifndef ERROR_H
#define ERROR_H

#include <string>

typedef enum {
    a, // illegal character
    b, // declared twice
    c, // undefined identifier
    d, // function call parameter mismatch
    e, // function call parameter type mismatch
    f, g,  // function return type mismatch
    h, // const variable assignment
    i, j, k, // missing right bracket
    l, // printf parameter mismatch
    m, // break or continue outside of loop
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