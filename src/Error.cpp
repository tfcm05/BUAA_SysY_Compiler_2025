#include <fstream>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include "../include/Error.h"

namespace {
    struct ErrorInfo {
        int line;
        ErrorCode code;
        std::string msg;
    };

    std::vector<ErrorInfo> errorList;
    int nonStandardErrorCount = 0;  // 非标准错误计数 (不写入 error.txt, 仅阻止 IR gen)
}

void errorLog(ErrorCode errorCode, int line, const std::string& msg) {
    errorList.push_back({line, errorCode, msg});
}

void incrementErrorCount() {
    // 不写入 error.txt (无标准错误码), 但递增错误计数以阻止 IR 生成
    nonStandardErrorCount++;
}

void semanticError(int line, const std::string& message) {
    std::cout << "Error: " << message << ", line: " << line << std::endl;
    incrementErrorCount();
}

int getErrorCount() {
    return (int)errorList.size() + nonStandardErrorCount;
}

std::string errorCodeToString(ErrorCode code) {
    switch (code) {
    case IllegalChar: return "非法字符";
    case RedefinedName: return "名字重定义";
    case UndefinedName: return "未定义的名字";
    case ParamCountMismatch: return "函数参数个数不匹配";
    case ParamTypeMismatch: return "函数参数类型不匹配";
    case ReturnTypeMismatch: return "return语句与函数返回类型不匹配";
    case MissingReturn: return "有返回值的函数缺少return语句";
    case ConstAssignment: return "不能改变常量的值";
    case MissingSemicolon: return "缺少分号";
    case MissingRightParen: return "缺少右小括号";
    case MissingRightBracket: return "缺少右中括号";
    case PrintfFormatMismatch: return "printf中格式字符与表达式个数不匹配";
    case BreakContinueOutsideLoop: return "非循环块中使用break和continue语句";
    default: return "未知错误";
    }
}

void dumpErrorsToFile() {
    std::sort(errorList.begin(), errorList.end(), [](const ErrorInfo& a, const ErrorInfo& b) {
        return a.line < b.line;
    });

    std::ofstream errorFile("error.txt");
    if (!errorFile.is_open()) {
        std::cout << "Error: Could not open error.txt for writing." << std::endl;
        return;
    }
    for (const auto& err : errorList) {
        errorFile << err.line << " " << errorCodeToString(err.code);
        if (!err.msg.empty()) {
            errorFile << " '" << err.msg << "'";
        }
        errorFile << std::endl;
    }
    errorFile.close();
}