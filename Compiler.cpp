#include<iostream>
#include<fstream>

#include"include/front/LexicalAnalysis.h"
#include"include/front/GrammaticalAnalysis.h"
#include"include/middle/Visitor.h"

int main() {
    std::ifstream file("testfile.txt"); // 打开文件
    if (!file.is_open()) {
        std::cout << "Error: Could not open file\n" << std::endl;
        return 1;
    }

    std::cout << "--- Starting Lexical & Syntax Analysis ---\n" << std::endl;
    LexicalAnalysis lexicalAnalysis = LexicalAnalysis(file);
    Parser parse = Parser(lexicalAnalysis);
    auto root = parse.parse();
    parse.parserLog(root.get());
    std::cout << "\n--- Lexical & Syntax Analysis Finished ---\n" << std::endl;

    std::cout << "...\n" << std::endl;
    std::cout << "..\n" << std::endl;
    std::cout << ".\n" << std::endl;

    std::cout << "\n--- Starting Semantic Analysis ---\n" << std::endl;
    SemanticVisitor visitor;
    visitor.visitCompUnit(root.get());
    std::cout << "\n--- Semantic Analysis Finished ---\n" << std::endl;
    
    return 0;
}