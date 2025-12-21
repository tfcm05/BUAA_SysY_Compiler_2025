#include<iostream>
#include<fstream>

#include"include/FrontEnd/LexicalAnalysis/LexicalAnalysis.h"
#include"include/FrontEnd/SyntaxAnalysis/SyntaxAnalysis.h"

#include "include/MiddleEnd/SemanticAnalysis/SemanticVisitor.h"
#include "include/MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"

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

    
    std::cout << "--- Starting Semantic Analysis ---\n" << std::endl;
    ClangContext clangContext;
    SemanticVisitor semanticVisitor(clangContext);
    root->accept(semanticVisitor, std::make_shared<SemanticContext>());
    std::cout << "\n--- Semantic Analysis Finished ---\n" << std::endl;
    
    dumpErrorsToFile();

    std::cout << "...\n" << std::endl;
    std::cout << "..\n" << std::endl;
    std::cout << ".\n" << std::endl;

    
    std::cout << "--- Starting Intermediate Code Generation ---\n" << std::endl;
    LLVMContext llvmContext;
    Module module;
    IRBuilder builder(llvmContext);
    IRVisitor irVisitor(module, builder, llvmContext);
    root->accept(irVisitor, std::make_shared<IRContext>());
    std::cout << "\n--- Intermediate Code Generation Finished ---\n" << std::endl;
    
    return 0;
}