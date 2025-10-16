#include<stdio.h>
#include<fstream>

#include"include/front/LexicalAnalysis.h"
#include"include/front/GrammaticalAnalysis.h"

int main() {
    std::ifstream file("testfile.txt"); // 打开文件
    if (!file.is_open()) {
        printf("无法打开文件");
        return 1;
    }
    LexicalAnalysis lexicalAnalysis = LexicalAnalysis(file);
    Parse parse = Parse(lexicalAnalysis);
    auto root = parse.parse();
    root->print();

    return 0;
}