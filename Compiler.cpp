#include<stdio.h>
#include<fstream>

#include"include/front/LexicalAnalysis.h"
#include"include/front/GrammaticalAnalysis.h"
#include"include/middle/Visitor.h"

int main() {
    std::ifstream file("testfile.txt"); // 打开文件
    if (!file.is_open()) {
        printf("无法打开文件");
        return 1;
    }
    LexicalAnalysis lexicalAnalysis = LexicalAnalysis(file);
    Parse parse = Parse(lexicalAnalysis);
    auto root = parse.parse();
    parse.parserLog(root.get());
    //Visitor visitor;
    //visitor.visitCompUnit(*root);
    return 0;
}