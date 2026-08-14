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

int getErrorCount() {
    return (int)errorList.size() + nonStandardErrorCount;
}

std::string errorCodeToString(ErrorCode code) {
    switch (code) {
    case a: return "非法字符";
    case b: return "名字重定义";
    case c: return "未定义的名字";
    case d: return "函数参数个数不匹配";
    case e: return "函数参数类型不匹配";
    case f: return "无返回值的函数存在不匹配的return语句";
    case g: return "有返回值的函数缺少return语句";
    case h: return "不能改变常量的值";
    case i: return "缺少分号";
    case j: return "缺少右小括号";
    case k: return "缺少右中括号";
    case l: return "printf中格式字符与表达式个数不匹配";
    case m: return "非循环块中使用break和continue语句";
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