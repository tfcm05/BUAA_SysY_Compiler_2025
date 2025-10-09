#ifndef TOKEN_H
#define TOKEN_H
#include<string>

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
        Token() = default;

        Token(int line, TokenType tokenType, std::string value);

        int getLine();
        TokenType getTokenType();
        std::string getValue();
};
#endif