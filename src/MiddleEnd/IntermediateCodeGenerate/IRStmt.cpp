#include "MiddleEnd/IntermediateCodeGenerate/IRVisitor.h"
#include "MiddleEnd/IntermediateCodeGenerate/IRBuilder.h"
#include "MiddleEnd/IntermediateCodeGenerate/Module.h"
#include "MiddleEnd/IntermediateCodeGenerate/LLVMContext.h"
#include "MiddleEnd/IntermediateCodeGenerate/EnsureI1.h"
#include "MiddleEnd/IntermediateCodeGenerate/ArrayInitIR.h"
#include "MiddleEnd/SemanticAnalysis/Symbol.h"
#include "MiddleEnd/SemanticAnalysis/Type.h"
#include "FrontEnd/SyntaxAnalysis/AstNodes.h"

InfoPtr IRVisitor::visitExpStmt(ExpStmt* node, ContextPtr context) {
    if (node->getExp()) {
        node->getExp()->accept(*this, context);
    }
    return nullptr;
}

InfoPtr IRVisitor::visitForStmt(ForStmt* node, ContextPtr context) {
    if (node->getInitial()) {
        node->getInitial()->accept(*this, context);
    }

    auto preheader = builder.getInsertBlock();

    auto condBlock = builder.createBasicBlock();
    auto bodyBlock = builder.createBasicBlock();
    auto iterBlock = builder.createBasicBlock();
    auto exitBlock = builder.createBasicBlock();

    builder.setInsertPoint(preheader);
    builder.createBr(condBlock);

    builder.setInsertPoint(condBlock);
    if (node->getCondition()) {
        auto condInfo = asIRInfo(node->getCondition()->accept(*this, context));
        if (!condInfo) {
            return nullptr;
        }
        auto condValue = ensureI1(builder, llvmContext, condInfo->getValue());
        builder.createBr(condValue, bodyBlock, exitBlock);
    } else {
        builder.createBr(bodyBlock);
    }

    builder.setInsertPoint(bodyBlock);
    if (node->getBody()) {
        IRContext& ir = asIR(context);
        IRContextGuard guard(ir);
        ir.setContinueBB(iterBlock);
        ir.setBreakBB(exitBlock);
        node->getBody()->accept(*this, context);
    }
    // body 可能以 return/break 结束 (已含终止指令), 此时不再追加跳转
    if (!builder.getInsertBlock()->isTerminator()) {
        builder.createBr(iterBlock);
    }

    builder.setInsertPoint(iterBlock);
    if (node->getIteration()) {
        node->getIteration()->accept(*this, context);
    }
    builder.createBr(condBlock);

    builder.setInsertPoint(exitBlock);
    return nullptr;
}

InfoPtr IRVisitor::visitWhileStmt(WhileStmt* node, ContextPtr context) {
    auto preheader = builder.getInsertBlock();

    auto condBlock = builder.createBasicBlock();
    auto bodyBlock = builder.createBasicBlock();
    auto exitBlock = builder.createBasicBlock();

    builder.setInsertPoint(preheader);
    builder.createBr(condBlock);

    builder.setInsertPoint(condBlock);
    if (node->getCondition()) {
        auto condInfo = asIRInfo(node->getCondition()->accept(*this, context));
        if (!condInfo) {
            return nullptr;
        }
        auto condValue = ensureI1(builder, llvmContext, condInfo->getValue());
        builder.createBr(condValue, bodyBlock, exitBlock);
    } else {
        builder.createBr(exitBlock);
    }

    builder.setInsertPoint(bodyBlock);
    if (node->getBody()) {
        IRContext& ir = asIR(context);
        IRContextGuard guard(ir);
        ir.setContinueBB(condBlock);
        ir.setBreakBB(exitBlock);
        node->getBody()->accept(*this, context);
    }
    if (!builder.getInsertBlock()->isTerminator()) {
        builder.createBr(condBlock);
    }

    builder.setInsertPoint(exitBlock);
    return nullptr;
}

InfoPtr IRVisitor::visitDoWhileStmt(DoWhileStmt* node, ContextPtr context) {
    auto preheader = builder.getInsertBlock();

    auto bodyBlock = builder.createBasicBlock();
    auto condBlock = builder.createBasicBlock();
    auto exitBlock = builder.createBasicBlock();

    builder.setInsertPoint(preheader);
    builder.createBr(bodyBlock);

    builder.setInsertPoint(bodyBlock);
    if (node->getBody()) {
        IRContext& ir = asIR(context);
        IRContextGuard guard(ir);
        ir.setContinueBB(condBlock);
        ir.setBreakBB(exitBlock);
        node->getBody()->accept(*this, context);
    }
    if (!builder.getInsertBlock()->isTerminator()) {
        builder.createBr(condBlock);
    }

    builder.setInsertPoint(condBlock);
    if (node->getCondition()) {
        auto condInfo = asIRInfo(node->getCondition()->accept(*this, context));
        if (!condInfo) {
            return nullptr;
        }
        auto condValue = ensureI1(builder, llvmContext, condInfo->getValue());
        builder.createBr(condValue, bodyBlock, exitBlock);
    } else {
        builder.createBr(exitBlock);
    }

    builder.setInsertPoint(exitBlock);
    return nullptr;
}

InfoPtr IRVisitor::visitIfStmt(IfStmt* node, ContextPtr context) {
    auto entryBlock = builder.getInsertBlock();
    auto trueBlock = builder.createBasicBlock();
    BasicBlock* falseBlock = nullptr;
    if (node->getFalseStmt()) {
        falseBlock = builder.createBasicBlock();
    }
    auto mergeBlock = builder.createBasicBlock();

    builder.setInsertPoint(entryBlock);
    if (node->getCondition()) {
        auto condInfo = asIRInfo(node->getCondition()->accept(*this, context));
        if (!condInfo) {
            return nullptr;
        }
        auto condValue = ensureI1(builder, llvmContext, condInfo->getValue());
        builder.createBr(condValue, trueBlock, falseBlock ? falseBlock : mergeBlock);
    } else {
        builder.createBr(falseBlock ? falseBlock : mergeBlock);
    }

    builder.setInsertPoint(trueBlock);
    if (node->getTrueStmt()) {
        node->getTrueStmt()->accept(*this, context);
    }
    {
        auto afterTrueBlock = builder.getInsertBlock();
        if (afterTrueBlock && !afterTrueBlock->isTerminator()) {
            builder.createBr(mergeBlock);
        }
    }

    if (falseBlock) {
        builder.setInsertPoint(falseBlock);
        node->getFalseStmt()->accept(*this, context);
        {
            auto afterFalseBlock = builder.getInsertBlock();
            if (afterFalseBlock && !afterFalseBlock->isTerminator()) {
                builder.createBr(mergeBlock);
            }
        }
    }

    builder.setInsertPoint(mergeBlock);
    return nullptr;
}

InfoPtr IRVisitor::visitBreakStmt(BreakStmt* node, ContextPtr context) {
    IRContext& breakContext = asIR(context);
    auto breakTarget = breakContext.getBreakBB();
    if (!breakTarget) {
        return nullptr;
    }
    builder.createBr(breakTarget);

    return nullptr;
}

InfoPtr IRVisitor::visitContinueStmt(ContinueStmt* node, ContextPtr context) {
    IRContext& continueContext = asIR(context);
    auto continueTarget = continueContext.getContinueBB();
    if (!continueTarget) {
        return nullptr;
    }
    builder.createBr(continueTarget);

    return nullptr;
}

InfoPtr IRVisitor::visitReturnStmt(ReturnStmt* node, ContextPtr context) {
    if (!node->getReturnExp()) {
        builder.createRet(nullptr);
        return nullptr;
    }

    auto returnIRInfo = asIRInfo(node->getReturnExp()->accept(*this, context));
    if (!returnIRInfo) {
        return nullptr;
    }
    auto returnValue = returnIRInfo->getValue();
    builder.createRet(returnValue);

    return nullptr;
}

InfoPtr IRVisitor::visitBlockStmt(BlockStmt* node, ContextPtr context) {
    for (size_t i = 0; i < node->getBlockItemsCount(); ++i) {
        node->getBlockItem(i)->accept(*this, context);
    }

    return nullptr;
}

