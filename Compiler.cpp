#include<iostream>
#include<fstream>

#include"include/front/LexicalAnalysis.h"
#include"include/front/GrammaticalAnalysis.h"
#include"include/middle/Visitor.h"

int main() {
    std::ifstream file("testfile.txt"); // 打开文件
    if (!file.is_open()) {
        std::cerr << "无法打开文件" << std::endl;
        return 1;
    }
    LexicalAnalysis lexicalAnalysis = LexicalAnalysis(file);
    Parser parse = Parser(lexicalAnalysis);
    auto root = parse.parse();
    parse.parserLog(root.get());

    std::cout << "\n--- Starting Semantic Analysis ---\n" << std::endl;

    SemanticVisitor visitor;
    visitor.visitCompUnit(root.get());

    std::cout << "\n--- Semantic Analysis Finished ---\n" << std::endl;
    return 0;
}
