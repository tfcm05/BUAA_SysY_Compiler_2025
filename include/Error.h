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

// 非标准 SysY 错误 (无标准错误码) 的计数: 仅递增错误总数, 不写入 error.txt
// 用于阻止 IR 生成 (如: 全局/static 初始化器非常量, 编译期除零等)
void incrementErrorCount();

std::string errorCodeToString(ErrorCode code);

void dumpErrorsToFile();

int getErrorCount();

#endif