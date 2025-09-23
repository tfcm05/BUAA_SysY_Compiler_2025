#include<stdio.h>
#include<fstream>
#include"include/LexicalAnalysis.h"

int main() {
    std::ifstream file("source.txt"); // 打开文件
    if (!file.is_open()) {
        printf("无法打开文件");
        return 1;
    }
 
    std::string input;
    LexicalAnalysis lexicalAnalysis;
    int lineNum = 1;
    while (std::getline(file, input)) { // 逐行读取
        int tokenNumPerLine = lexicalAnalysis.lexicalAnalysis(input, lineNum++);
        Token* tokens = lexicalAnalysis.getTokens();
    }
}