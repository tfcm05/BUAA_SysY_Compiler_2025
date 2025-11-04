#include <fstream>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include "../include/Error.h"
#include "../include/front/Token.h"
#include "../include/front/AstNode.h"

namespace {
    struct ErrorInfo {
        int line;
        ErrorCode code;
    };

    std::vector<ErrorInfo> errorList;
}

void errorLog(ErrorCode errorCode, int line) {
    errorList.push_back({line, errorCode});
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
        errorFile << err.line << " " << static_cast<char>(err.code + 'a') << std::endl;
    }
    errorFile.close();
}