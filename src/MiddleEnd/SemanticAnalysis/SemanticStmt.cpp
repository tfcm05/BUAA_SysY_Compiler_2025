#include "../../../include/MiddleEnd/SemanticAnalysis/SemanticVisitor.h"
#include "../../../include/MiddleEnd/SemanticAnalysis/ScopeGuard.h"
#include "../../../include/MiddleEnd/SemanticAnalysis/ConstEval.h"

InfoPtr SemanticVisitor::visitExpStmt(ExpStmt* node, ContextPtr context) {
    if (node->getExp()) {
        node->getExp()->accept(*this, context);
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitForStmt(ForStmt* node, ContextPtr context) {
    // for 头声明的变量作用域覆盖整个 for 语句
    ScopeGuard guard(curScope);
    if (node->getInitial()) {
        node->getInitial()->accept(*this, context);
    }
    if (node->getCondition()) {
        node->getCondition()->accept(*this, context);
    }
    if (node->getIteration()) {
        node->getIteration()->accept(*this, context);
    }

    {
        SemanticContext& sem = asSem(context);
        SemanticContextGuard guard(sem);
        sem.setLoopFlag(true);
        if (node->getBody()) {
            node->getBody()->accept(*this, context);
        }
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitWhileStmt(WhileStmt* node, ContextPtr context) {
    if (node->getCondition()) {
        node->getCondition()->accept(*this, context);
    }

    {
        SemanticContext& sem = asSem(context);
        SemanticContextGuard guard(sem);
        sem.setLoopFlag(true);
        node->getBody()->accept(*this, context);
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitDoWhileStmt(DoWhileStmt* node, ContextPtr context) {
    if (node->getCondition()) {
        node->getCondition()->accept(*this, context);
    }

    {
        SemanticContext& sem = asSem(context);
        SemanticContextGuard guard(sem);
        sem.setLoopFlag(true);
        node->getBody()->accept(*this, context);
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitIfStmt(IfStmt* node, ContextPtr context) {
    if (node->getCondition()) {
        node->getCondition()->accept(*this, context);
    }
    if (node->getTrueStmt()) {
        node->getTrueStmt()->accept(*this, context);
    }
    if (node->getFalseStmt()) {
        node->getFalseStmt()->accept(*this, context);
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitBreakStmt(BreakStmt* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    if (!semanticContext.inLoop()) {
        errorLog(ErrorCode::BreakContinueOutsideLoop, node->getStartLine());
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitContinueStmt(ContinueStmt* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    if (!semanticContext.inLoop()) {
        errorLog(ErrorCode::BreakContinueOutsideLoop, node->getStartLine());
    }

    return nullptr;
}

InfoPtr SemanticVisitor::visitReturnStmt(ReturnStmt* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    auto returnType = semanticContext.getType();

    if (node->getReturnExp()) {
        node->getReturnExp()->accept(*this, context);
        // void 函数不得 return 表达式
        if (returnType->isVoidTy()) {
            errorLog(ErrorCode::ReturnTypeMismatch, node->getStartLine());
        }
    } else if (!returnType->isVoidTy()) {
        errorLog(ErrorCode::ReturnTypeMismatch, node->getStartLine());
    } 

    return nullptr;
}

// 判断语句是否保证以 return 结束
//   return 语句 → true; if 带 else 且两分支都保证 → true; 块语句最后一条保证 → true; 其余 false
static bool alwaysReturns(AstNode* node) {
    if (auto retStmt = dynamic_cast<ReturnStmt*>(node)) {
        (void)retStmt;
        return true;
    }
    if (auto ifStmt = dynamic_cast<IfStmt*>(node)) {
        if (!ifStmt->getFalseStmt() || !ifStmt->getTrueStmt()) {
            return false;   // 无 else: 条件为假时可能不执行
        }
        return alwaysReturns(ifStmt->getTrueStmt()) && alwaysReturns(ifStmt->getFalseStmt());
    }
    if (auto blockStmt = dynamic_cast<BlockStmt*>(node)) {
        // 遍历块中所有语句: 任何一条保证返回即可 (return 可在块中间, 后续为 unreachable)
        for (size_t i = 0; i < blockStmt->getBlockItemsCount(); ++i) {
            if (alwaysReturns(blockStmt->getBlockItem(i))) {
                return true;
            }
        }
        return false;
    }
    return false;
}

InfoPtr SemanticVisitor::visitBlockStmt(BlockStmt* node, ContextPtr context) {
    SemanticContext& semanticContext = asSem(context);
    if (semanticContext.functionBodyOnly()) {
        {
            SemanticContextGuard guard(semanticContext);
            semanticContext.clearModes();
            for (size_t i = 0; i < node->getBlockItemsCount(); ++i) {
                node->getBlockItem(i)->accept(*this, context);
            }
        }

        auto returnType = semanticContext.getType();
        if (!returnType->isVoidTy()) {
            // 函数体必须保证以 return 结束: 遍历所有语句, 任何一条保证返回即可
            if (!alwaysReturns(node)) {
                errorLog(ErrorCode::MissingReturn, node->getEndLine());
            }
        }
    } else {
        ScopeGuard guard(curScope);
        for (size_t i = 0; i < node->getBlockItemsCount(); ++i) {
            node->getBlockItem(i)->accept(*this, context);
        }
    }

    return nullptr;
}

