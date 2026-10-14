#ifndef GRAMMATICALANALYSIS_H
#define GRAMMATICALANALYSIS_H

#include "AstNodes.h"
#include "../LexicalAnalysis/LexicalAnalysis.h"

#define CURRENT_TYPE currentToken.getTokenType()
#define NEXT_TYPE peekNextRef().getTokenType()
#define SECOND_TYPE peekSecondRef().getTokenType()

#define CURRENT_LINE currentToken.getLine()
#define CURRENT_VALUE currentToken.getValue()

class Parser {
    private:
        LexicalAnalysis& lexicalAnalysis;
        Token currentToken;
        int preLine;

        // 编译单元
        CompUnitPtr parseCompUnit();

        // 声明相关
        DeclSpec parseDeclSpec();

        DeclaratorPtr parseDeclarator();
        DeclaratorPtr parseDeclaratorNoInit();
        InitializerPtr parseInitializer();

        VariableDeclPtr parseVarDecl();
        FunctionDeclPtr parseFuncDecl();
        ParameterDeclPtr parseParameterDecl();

        FunctionDeclPtr parseMainFuncDecl();
    
        // 语句相关
        StmtPtr parseStmt();
        BlockStmtPtr parseBlockStmt();
        ExpStmtPtr parseExpStmt();
        IfStmtPtr parseIfStmt();
        ForStmtPtr parseForStmt();
        WhileStmtPtr parseWhileStmt();
        DoWhileStmtPtr parseDoWhileStmt();
        ReturnStmtPtr parseReturnStmt();
        BreakStmtPtr parseBreakStmt();
        ContinueStmtPtr parseContinueStmt();

        // 表达式相关
        ExpPtr parseLiteralExp();
        ExpPtr parseIdentifierExp();
        ExpPtr parseFunctionCallExp();
        ExpPtr parseSubscriptExp();

        ExpPtr parsePrimaryExp();
        ExpPtr parsePostfixExp();
        ExpPtr parseUnaryExp();
        ExpPtr parseMulExp();
        ExpPtr parseAddExp();
        ExpPtr parseRelExp();
        ExpPtr parseEqExp();
        ExpPtr parseLAndExp();
        ExpPtr parseLOrExp();
        ExpPtr parseAssignExp();
        ExpPtr parseCommaExp();

        void initial();
        void advance();
        Token peekNext();
        Token peekSecond();
        const Token& peekNextRef();
        const Token& peekSecondRef();

        bool match(TokenType expected, int mode);
        bool match(TokenType expected);
        // offset 0 = current token; 1 = next, ...
        TokenType tokenTypeAt(int offset);
        // Local/top-level variable decl starting at current (allows int **p).
        bool looksLikeVarDecl();
        // Function decl/def at current (int f(...), void g(), int *f()).
        bool looksLikeFuncDecl();

        void expect(TokenType expected);
    public:
        ~Parser() = default;
        Parser(LexicalAnalysis& lexicalAnalysis);
        std::unique_ptr<AstNode> parse();
        
        // 将构建出的语法树以文本形式输出到 parser.txt，便于调试
        void parserLog(const AstNode* root) const;
};

#endif