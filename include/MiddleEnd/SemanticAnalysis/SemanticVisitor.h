#ifndef SEMANTIC_VISITOR_H
#define SEMANTIC_VISITOR_H
#include "../../FrontEnd/SyntaxAnalysis/AstVisitor.h"
#include "../../FrontEnd/SyntaxAnalysis/AstNode.h"
#include "../../Error.h"
#include "SymbolTable.h"
#include "ClangContext.h"
#include "ConstValue.h"
#include "AnalysisResult.h"
#include <fstream>
#include <iostream>
#include <map>
#include <memory>

class SemanticInfo : public Info {
private:
    Clang::Type* type;
    bool lValueFlag;
    Clang::ConstValue* constValue;
    std::string name;
public:
    SemanticInfo();

    Clang::Type* getType() const;
    void setType(Clang::Type* ty);

    bool isLValue() const;
    void setLValueFlag(bool lvf);
    Clang::ConstValue* getConstValue() const;
    void setConstValue(Clang::ConstValue* cv);

    std::string getName() const;
    void setName(const std::string& n);
};

// Explicit mode flags (no VisitPurpose DSL).
class SemanticContext : public Context {
private:
    bool evaluateConstFlag;
    bool generateParamSymbolsFlag;
    bool functionBodyOnlyFlag;
    Clang::Type* type;
    bool loopFlag;
    bool staticStorageRegion;
public:
    SemanticContext();
    ContextPtr clone() override;

    bool wantConst() const;
    void setWantConst(bool v);
    bool generateParamSymbols() const;
    void setGenerateParamSymbols(bool v);
    bool functionBodyOnly() const;
    void setFunctionBodyOnly(bool v);

    // Clear all mode flags (DEFAULT).
    void clearModes();

    Clang::Type* getType() const;
    void setType(Clang::Type* ty);

    bool inLoop() const;
    void setLoopFlag(bool lf);

    bool isStaticStorageRegion() const;
    void setStaticStorageRegion(bool ssr);
};

// RAII: mutate SemanticContext and restore on scope exit (prefer over clone).
class SemanticContextGuard {
private:
    SemanticContext& ctx;
    bool oldWantConst;
    bool oldGenerateParamSymbols;
    bool oldFunctionBodyOnly;
    Clang::Type* oldType;
    bool oldLoopFlag;
    bool oldStaticStorageRegion;
public:
    explicit SemanticContextGuard(SemanticContext& c)
        : ctx(c),
          oldWantConst(c.wantConst()),
          oldGenerateParamSymbols(c.generateParamSymbols()),
          oldFunctionBodyOnly(c.functionBodyOnly()),
          oldType(c.getType()),
          oldLoopFlag(c.inLoop()),
          oldStaticStorageRegion(c.isStaticStorageRegion()) {}

    ~SemanticContextGuard() {
        ctx.setWantConst(oldWantConst);
        ctx.setGenerateParamSymbols(oldGenerateParamSymbols);
        ctx.setFunctionBodyOnly(oldFunctionBodyOnly);
        ctx.setType(oldType);
        ctx.setLoopFlag(oldLoopFlag);
        ctx.setStaticStorageRegion(oldStaticStorageRegion);
    }

    SemanticContextGuard(const SemanticContextGuard&) = delete;
    SemanticContextGuard& operator=(const SemanticContextGuard&) = delete;
};

inline SemanticContext& asSem(const ContextPtr& context) {
    return *static_cast<SemanticContext*>(context.get());
}

inline std::shared_ptr<SemanticInfo> asSemInfo(const InfoPtr& info) {
    return std::static_pointer_cast<SemanticInfo>(info);
}

class SemanticVisitor : public AstVisitor {
private:
    SymbolTablePtr curScope;
    ClangContext& clangContext;
    AnalysisResult analysisResult;
    std::ofstream outputFile;
    std::map<int, std::vector<std::unique_ptr<Symbol>>> symbolRecords;

    Clang::Type* convertToType(TypeSpecifier ts, unsigned int tq);
    bool canAssign(Clang::Type* destType, Clang::Type* srcType);
    Symbol* createSymbol(const std::string& name, StorageClass sc, Clang::Type* type,
                         Symbol::SymbolKind kind, Clang::ConstValue* constValue);

    // Bind symbol into AnalysisResult and dual-write AST during migration.
    void bindSymbol(AstNode* node, Symbol* symbol);
public:
    SemanticVisitor(ClangContext& cc);
    ~SemanticVisitor() override;

    AnalysisResult& analysis() { return analysisResult; }
    const AnalysisResult& analysis() const { return analysisResult; }

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

    InfoPtr visitIdentDeclarator(IdentDeclarator* node, ContextPtr context) override;
    InfoPtr visitPointerDeclarator(PointerDeclarator* node, ContextPtr context) override;
    InfoPtr visitArrayDeclarator(ArrayDeclarator* node, ContextPtr context) override;
    InfoPtr visitFuncDeclarator(FuncDeclarator* node, ContextPtr context) override;
    InfoPtr visitInitialDeclarator(InitialDeclarator* node, ContextPtr context) override;

    InfoPtr visitVariableDecl(VariableDecl* node, ContextPtr context) override;
    InfoPtr visitFunctionDecl(FunctionDecl* node, ContextPtr context) override;
    InfoPtr visitParameterDecl(ParameterDecl* node, ContextPtr context) override;

    InfoPtr visitCompUnit(CompUnit* node, ContextPtr context) override;
};

#endif
