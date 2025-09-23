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
    SEMICN, COMMA, // ; ,
    LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE, // 括号
    PRINTFTK,
    NOTHING
} TokenType;

class Token {
    private:
        int lineNum = 0;
        TokenType tokenType = NOTHING;
        std::string value = "";
    public:
        void setToken(int lineNum, TokenType tokenType, std::string value);

        int getLineNum();
        TokenType getTokenType();
        std::string getValue();
};
#endif