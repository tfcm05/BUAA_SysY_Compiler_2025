#include<iostream>
#include<fstream>

#include"include/FrontEnd/LexicalAnalysis/LexicalAnalysis.h"
#include"include/FrontEnd/SyntaxAnalysis/SyntaxAnalysis.h"

#include "include/MiddleEnd/SemanticAnalysis/SemanticVisitor.h"
#include "include/MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"
#include "include/MiddleEnd/Optimization/AlgebraicSimplification.h"
#include "include/BackEnd/MIPS/MIPSCodeGenerator.h"
#include "include/BackEnd/MIPS/MIPSPeepholeOptimizer.h"

int main() {
    std::ifstream file("testfile.txt"); // 打开文件
    if (!file.is_open()) {
        std::cout << "Error: Could not open file\n" << std::endl;
        return 1;
    }

    // 词法 & 语法分析
    LexicalAnalysis lexicalAnalysis = LexicalAnalysis(file);
    Parser parse = Parser(lexicalAnalysis);
    auto root = parse.parse();
    parse.parserLog(root.get());

    // 语义分析
    ClangContext clangContext;
    SemanticVisitor semanticVisitor(clangContext);
    root->accept(semanticVisitor, std::make_shared<SemanticContext>());

    dumpErrorsToFile();

    if (getErrorCount() > 0) {
        return 0;
    }

    // 中间代码生成 (IR 打印在 IRVisitor 析构时写入 llvm_ir.txt)
    LLVMContext llvmContext;
    Module module;
    IRBuilder builder(llvmContext);
    IRVisitor irVisitor(module, builder, llvmContext);
    root->accept(irVisitor, std::make_shared<IRContext>());

    // IR 优化 pass
    AlgebraicSimplification algSim(module, llvmContext);
    algSim.run();

    // MIPS 后端: 代码生成 + 窥孔优化
    MIPSCodeGenerator mipsGen(module, llvmContext);
    auto mipsModule = mipsGen.run();

    MIPSPeepholeOptimizer peephole(*mipsModule);
    peephole.run();

    // 输出 MIPS 汇编
    std::ofstream mipsFile("mips.s");
    if (mipsFile.is_open()) {
        MIPSPrinter printer;
        printer.print(*mipsModule, mipsFile);
    } else {
        std::cout << "Error: Could not open mips.s for writing\n" << std::endl;
    }

    return 0;
}
