#include"LexicalAnalysis.h"
#include"AstNode.h"

#define CURRENT_TYPE currentToken.getTokenType()
#define NEXT_TYPE peekNext().getTokenType()
#define SECOND_TYPE peekSecond().getTokenType()

class Parse {
    private:
        LexicalAnalysis lexicalAnalysis;
        Token currentToken;

        // 编译单元相关
        std::unique_ptr<AstNode> parseCompUnit();
        std::unique_ptr<AstNode> parseDecl();
        std::unique_ptr<AstNode> parseConstDecl();
        std::unique_ptr<AstNode> parseVarDecl();
        std::unique_ptr<AstNode> parseFuncDef();
        std::unique_ptr<AstNode> parseMainFuncDef();

        // 类型和定义相关
        std::unique_ptr<AstNode> parseBType();
        std::unique_ptr<AstNode> parseConstDef();
        std::unique_ptr<AstNode> parseVarDef();
        std::unique_ptr<AstNode> parseConstInitVal();
        std::unique_ptr<AstNode> parseInitVal();

        // 函数相关
        std::unique_ptr<AstNode> parseFuncType();
        std::unique_ptr<AstNode> parseFuncFParams();
        std::unique_ptr<AstNode> parseFuncFParam();
    
        // 语句块和语句相关
        std::unique_ptr<AstNode> parseBlock();
        std::unique_ptr<AstNode> parseBlockItem();
        std::unique_ptr<AstNode> parseStmt();
        std::unique_ptr<AstNode> parseForStmt();

        // 表达式相关
        std::unique_ptr<AstNode> parseExp();
        std::unique_ptr<AstNode> parseCond();
        std::unique_ptr<AstNode> parseLVal();
        std::unique_ptr<AstNode> parsePrimaryExp();
        std::unique_ptr<AstNode> parseUnaryExp();
        std::unique_ptr<AstNode> parseUnaryOp();
        std::unique_ptr<AstNode> parseFuncRParams();
        std::unique_ptr<AstNode> parseMulExp();
        std::unique_ptr<AstNode> parseAddExp();
        std::unique_ptr<AstNode> parseRelExp();
        std::unique_ptr<AstNode> parseEqExp();
        std::unique_ptr<AstNode> parseLAndExp();
        std::unique_ptr<AstNode> parseLOrExp();
        std::unique_ptr<AstNode> parseConstExp();

        void advance();
        Token peekNext();
        Token peekSecond();

        bool match(TokenType expected, int mode);
        bool match(TokenType expected);

        void expect(TokenType expected, std::unique_ptr<AstNode>& node);
    public:
        Parse(LexicalAnalysis lexicalAnalysis);
        std::unique_ptr<AstNode> parse();
};