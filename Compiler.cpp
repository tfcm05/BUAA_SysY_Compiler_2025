#include<iostream>
#include<fstream>

#include"include/FrontEnd/LexicalAnalysis/LexicalAnalysis.h"
#include"include/FrontEnd/SyntaxAnalysis/SyntaxAnalysis.h"

#include "include/MiddleEnd/SemanticAnalysis/SemanticVisitor.h"
#include "include/MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"
#include "include/MiddleEnd/IntermediateCodeGenerate/IRPrinter.h"
#include "include/MiddleEnd/Optimization/Pass.h"
#include "include/MiddleEnd/Optimization/AlgebraicSimplification.h"
#include "include/BackEnd/MIPS/MIPSCodeGenerator.h"
#include "include/BackEnd/MIPS/MIPSPeepholeOptimizer.h"

#include <vector>
#include <memory>

int main(int argc, char* argv[]) {
    const char* inputPath = (argc > 1) ? argv[1] : "testfile.txt";
    std::ifstream file(inputPath);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file: " << inputPath << std::endl;
        return 1;
    }

    // 词法 & 语法分析
    LexicalAnalysis lexicalAnalysis = LexicalAnalysis(file);
    Parser parse = Parser(lexicalAnalysis);
    auto root = parse.parse();
    parse.parserLog(root.get());

    // 语义分析 → AnalysisResult (Symbol/type/const annotations)
    ClangContext clangContext;
    SemanticVisitor semanticVisitor(clangContext);
    root->accept(semanticVisitor, std::make_shared<SemanticContext>());

    dumpErrorsToFile();

    if (getErrorCount() > 0) {
        return 0;
    }

    // 中间代码生成 (reads AnalysisResult; IRBindings session map for Value*)
    LLVMContext llvmContext;
    Module module;
    IRBuilder builder(llvmContext);
    IRVisitor irVisitor(module, builder, llvmContext, semanticVisitor.analysis());
    root->accept(irVisitor, std::make_shared<IRContext>());
    irVisitor.finalize();

    // IR 优化 pipeline (Module-level Passes only; no AST re-walk)
    std::vector<std::unique_ptr<Pass>> passes;
    passes.push_back(std::make_unique<AlgebraicSimplification>(module, llvmContext));
    for (auto& pass : passes) {
        pass->runOnModule(module, llvmContext);
    }

    // Print IR after passes (not in IRVisitor destructor)
    {
        IRPrinter printer;
        printer.printModule(module);
    }

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
