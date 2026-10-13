#ifndef IR_VISITOR_H
#define IR_VISITOR_H

#include "../../FrontEnd/SyntaxAnalysis/AstVisitor.h"
#include "../../FrontEnd/SyntaxAnalysis/AstNode.h"
#include "../SemanticAnalysis/SymbolTable.h"
#include "../SemanticAnalysis/AnalysisResult.h"
#include "IRBuilder.h"
#include "IRPrinter.h"
#include "IRBindings.h"

#include <string>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <memory>

class BasicBlock;

class IRInfo : public Info {
private:
    Value* value;
public:
    IRInfo(Value* val);
    Value* getValue() const;
    void setValue(Value* val);
};

// Explicit mode flags (no visitPurpose DSL).
class IRContext : public Context {
private:
    bool createGlobalFlag;
    bool useLocationFlag;
    Value* addressValue;
    BasicBlock* continueBB;
    BasicBlock* breakBB;
    LLVM::Type* type;
    std::vector<size_t> indices;
public:
    IRContext();

    bool createGlobal() const;
    void setCreateGlobal(bool v);
    bool useLocation() const;
    void setUseLocation(bool v);
    void clearModes();

    Value* getAddressValue() const;
    void setAddressValue(Value* val);

    BasicBlock* getContinueBB() const;
    void setContinueBB(BasicBlock* bb);
    BasicBlock* getBreakBB() const;
    void setBreakBB(BasicBlock* bb);

    // Back-compat aliases used by initializer paths (address only).
    Value* getValue() const;
    void setValue(Value* val);

    LLVM::Type* getType() const;
    void setType(LLVM::Type* ty);

    size_t getIndicesCount() const;
    size_t getIndex(size_t index) const;
    void addIndex(size_t index);
    void clearIndices();
    void setIndices(const std::vector<size_t>& idxs);
    const std::vector<size_t>& getIndices() const;

    ContextPtr clone() override;
};

class IRContextGuard {
private:
    IRContext& ctx;
    bool oldCreateGlobal;
    bool oldUseLocation;
    Value* oldAddress;
    BasicBlock* oldContinueBB;
    BasicBlock* oldBreakBB;
    LLVM::Type* oldType;
    std::vector<size_t> oldIndices;
public:
    explicit IRContextGuard(IRContext& c)
        : ctx(c),
          oldCreateGlobal(c.createGlobal()),
          oldUseLocation(c.useLocation()),
          oldAddress(c.getAddressValue()),
          oldContinueBB(c.getContinueBB()),
          oldBreakBB(c.getBreakBB()),
          oldType(c.getType()),
          oldIndices(c.getIndices()) {}

    ~IRContextGuard() {
        ctx.setCreateGlobal(oldCreateGlobal);
        ctx.setUseLocation(oldUseLocation);
        ctx.setAddressValue(oldAddress);
        ctx.setContinueBB(oldContinueBB);
        ctx.setBreakBB(oldBreakBB);
        ctx.setType(oldType);
        ctx.setIndices(oldIndices);
    }

    IRContextGuard(const IRContextGuard&) = delete;
    IRContextGuard& operator=(const IRContextGuard&) = delete;
};

inline IRContext& asIR(const ContextPtr& context) {
    return *static_cast<IRContext*>(context.get());
}

inline std::shared_ptr<IRInfo> asIRInfo(const InfoPtr& info) {
    return std::static_pointer_cast<IRInfo>(info);
}

class IRVisitor : public AstVisitor {
private:
    Module& module;
    IRBuilder& builder;
    LLVMContext& llvmContext;
    const AnalysisResult& analysis;
    IRBindings bindings;

    std::unordered_map<Clang::Type*, LLVM::Type*> typeCache;

    LLVM::Type* convertType(Clang::Type* clangType);
    Symbol* resolveSymbol(AstNode* node) const;
public:
    IRVisitor(Module& mod, IRBuilder& bld, LLVMContext& ctx, const AnalysisResult& ar);
    ~IRVisitor() override = default;

    void finalize();

    InfoPtr visitLiteralExp(LiteralExp* node, ContextPtr context) override;
    InfoPtr visitIdentifierExp(IdentifierExp* node, ContextPtr context) override;
    InfoPtr visitFunctionCallExp(FunctionCallExp* node, ContextPtr context) override;
    InfoPtr visitSubscriptExp(SubscriptExp* node, ContextPtr context) override;
    InfoPtr visitUnaryExp(UnaryExp* node, ContextPtr context) override;
    InfoPtr visitBinaryExp(BinaryExp* node, ContextPtr context) override;
    InfoPtr visitAssignExp(AssignExp* node, ContextPtr context) override;
    InfoPtr visitCommaExp(CommaExp* node, ContextPtr context) override;

    InfoPtr visitExpStmt(ExpStmt* node, ContextPtr context) override;
    InfoPtr visitForStmt(ForStmt* node, ContextPtr context) override;
    InfoPtr visitWhileStmt(WhileStmt* node, ContextPtr context) override;
    InfoPtr visitDoWhileStmt(DoWhileStmt* node, ContextPtr context) override;
    InfoPtr visitIfStmt(IfStmt* node, ContextPtr context) override;
    InfoPtr visitBreakStmt(BreakStmt* node, ContextPtr context) override;
    InfoPtr visitContinueStmt(ContinueStmt* node, ContextPtr context) override;
    InfoPtr visitReturnStmt(ReturnStmt* node, ContextPtr context) override;
    InfoPtr visitBlockStmt(BlockStmt* node, ContextPtr context) override;

    InfoPtr visitExpInitializer(ExpInitializer* node, ContextPtr context) override;
    InfoPtr visitListInitializer(ListInitializer* node, ContextPtr context) override;

    // Declarators: default empty on AstVisitor; IR does not walk them.
    InfoPtr visitVariableDecl(VariableDecl* node, ContextPtr context) override;
    InfoPtr visitFunctionDecl(FunctionDecl* node, ContextPtr context) override;
    InfoPtr visitParameterDecl(ParameterDecl* node, ContextPtr context) override;

    InfoPtr visitCompUnit(CompUnit* node, ContextPtr context) override;
};

#endif
