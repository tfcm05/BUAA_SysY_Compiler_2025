#include <fstream>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include "Error.h"

namespace {
    struct ErrorInfo {
        int line;
        ErrorCode code;
        std::string msg;
    };

    std::vector<ErrorInfo> errorList;
}

void errorLog(ErrorCode errorCode, int line, const std::string& msg) {
    errorList.push_back({line, errorCode, msg});
}

int getErrorCount() {
    return (int)errorList.size();
}

std::string errorCodeToString(ErrorCode code) {
    switch (code) {
    case IllegalChar:               return "非法字符";
    case MissingSemicolon:          return "缺少分号";
    case MissingRightParen:         return "缺少右小括号";
    case MissingRightBracket:       return "缺少右中括号";
    case RedefinedName:             return "名字重定义";
    case UndefinedName:             return "未定义的名字";
    case ParamCountMismatch:        return "函数参数个数不匹配";
    case ParamTypeMismatch:         return "函数参数类型不匹配";
    case CallNonFunction:           return "对非函数类型进行调用";
    case ReturnTypeMismatch:        return "return语句与函数返回类型不匹配";
    case MissingReturn:             return "有返回值的函数缺少return语句";
    case ConstAssignment:           return "不能改变常量的值";
    case GlobalStaticNonConstInit:  return "全局或静态变量初始化必须为常量表达式";
    case ConstDivByZero:            return "常量表达式除以零";
    case ArraySizeNotConst:         return "数组大小不是常量";
    case ArraySizeNotPositive:      return "数组大小必须为正";
    case SubscriptNonArray:         return "对非数组或非指针类型使用下标";
    case AssignmentToNonLValue:     return "赋值左侧不是左值";
    case AddressOfNonLValue:       return "对非左值取地址";
    case DerefNonPointer:          return "对非指针类型解引用";
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