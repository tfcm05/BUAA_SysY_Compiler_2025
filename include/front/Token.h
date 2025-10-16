#ifndef TOKEN_H
#define TOKEN_H
#include<string>
#include <utility>

typedef enum {
    MAINTK, // main
    IDENFR, // 标识符
    INTCON, STRCON, 
    CONSTTK, STATICTK, 
    INTTK, VOIDTK, 
    IFTK, ELSETK, FORTK, BREAKTK, CONTINUETK, 
    RETURNTK, 
    ASSIGN, // =
    NOT, AND, OR, 
    PLUS, MINU, MULT, DIV, MOD, 
    LSS, LEQ, GRE, GEQ, EQL, NEQ, 
    SEMICN/*;*/, COMMA/*,*/,
    LPARENT/*(*/, RPARENT/*)*/, LBRACK/*[*/, RBRACK/*]*/, LBRACE/*{*/, RBRACE/*}*/, // 括号
    PRINTFTK,
    END, ERROR
} TokenType;

extern std::string getTypeCode(TokenType tokenType);

class Token {
    private:
        int line;
        TokenType tokenType;
        std::string value;
    public:
        ~Token() = default;
        Token() : line(0), tokenType(ERROR), value("") {}

        Token(int line, TokenType tokenType, std::string value);

        // 拷贝
        Token(const Token& other);
        Token& operator = (const Token& other);

        // 移动
        Token(Token&& other) noexcept;
        Token& operator = (Token&& other) noexcept;

        int getLine() const;
        TokenType getTokenType() const;
        std::string getValue() const;
};
#endif