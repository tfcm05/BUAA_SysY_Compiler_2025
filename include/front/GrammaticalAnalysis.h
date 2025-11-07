#ifndef GRAMMATICALANALYSIS_H
#define GRAMMATICALANALYSIS_H

#include<memory>
#include<string>
#include<vector>
#include"LexicalAnalysis.h"
#include"AstNode.h"

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
        std::unique_ptr<CompUnit> parseCompUnit();
        std::unique_ptr<FunctionDecl> parseMainFuncDecl();

        // 声明说明符相关
        TypeSpecifier parseTypeSpecifier();
        TypeQualifier parseTypeQualifier();
        StorageClass parseStorageClass();

        // 声明相关
        std::unique_ptr<VariableDecl> parseVarDecl();
        std::unique_ptr<FunctionDecl> parseFuncDecl();

        std::unique_ptr<VariableDeclarator> parseVarDeclarator();
        std::vector<std::unique_ptr<Exp>> parseInitVal();

        std::vector<std::unique_ptr<FormatParam>> parseFuncFParams();
        std::unique_ptr<FormatParam> parseFuncFParam();
        std::vector<std::unique_ptr<Exp>> parseFuncRParams();
    
        // 语句相关
        std::unique_ptr<Stmt> parseStmt();
        std::unique_ptr<BlockStmt> parseBlock();
        std::unique_ptr<ExpStmt> parseExpStmt();
        std::unique_ptr<IfStmt> parseIfStmt();
        std::unique_ptr<ForStmt> parseForStmt();
        std::unique_ptr<ReturnStmt> parseReturnStmt();
        std::unique_ptr<BreakStmt> parseBreakStmt();
        std::unique_ptr<ContinueStmt> parseContinueStmt();

        // 表达式相关
        std::unique_ptr<Exp> parseLiteralExp();
        std::unique_ptr<Exp> parseIdentifierExp();
        std::unique_ptr<Exp> parsePostfixExp();
        std::unique_ptr<Exp> parsePrimaryExp();
        std::unique_ptr<Exp> parseUnaryExp();
        std::unique_ptr<Exp> parseMulExp();
        std::unique_ptr<Exp> parseAddExp();
        std::unique_ptr<Exp> parseRelExp();
        std::unique_ptr<Exp> parseEqExp();
        std::unique_ptr<Exp> parseLAndExp();
        std::unique_ptr<Exp> parseLOrExp();
        std::unique_ptr<Exp> parseAssignExp();
        std::unique_ptr<Exp> parseCommaExp();

        void initial();
        void advance();
        Token peekNext();
        Token peekSecond();
        const Token& peekNextRef();
        const Token& peekSecondRef();

        bool match(TokenType expected, int mode);
        bool match(TokenType expected);

        void expect(TokenType expected);
    public:
        ~Parser() = default;
        Parser(LexicalAnalysis& lexicalAnalysis);
        std::unique_ptr<AstNode> parse();
        
        // 将构建出的语法树以文本形式输出到 parser.txt，便于调试
        void parserLog(const AstNode* root) const;
};

#endif