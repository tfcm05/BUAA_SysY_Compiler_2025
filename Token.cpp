#include"include/Token.h"

Token::Token(int line, TokenType tokenType, std::string value) {
    Token::line = line;
    Token::tokenType = tokenType;
    Token::value = value;
}

int Token::getLine() {
    return line;
}

TokenType Token::getTokenType() {
    return tokenType;
}

std::string Token::getValue() {
    return value;
}

// 获取类型码
std::string getTypeCode(TokenType tokentype) {
    switch (tokentype) {
        case MAINTK: return "MAINTK";
        case IDENFR: return "IDENFR";
        case INTCON: return "INTCON";
        case STRCON: return "STRCON";
        case CONSTTK: return "CONSTTK";
        case STATICTK: return "STATICTK";
        case INTTK: return "INTTK";
        case VOIDTK: return "VOIDTK";
        case IFTK: return "IFTK";
        case ELSETK: return "ELSETK";
        case FORTK: return "FORTK";
        case BREAKTK: return "BREAKTK";
        case CONTINUETK: return "CONTINUETK";
        case RETURNTK: return "RETURNTK";
        case ASSIGN: return "ASSIGN";
        case NOT: return "NOT";
        case AND: return "AND";
        case OR: return "OR";
        case PLUS: return "PLUS";
        case MINU: return "MINU";
        case MULT: return "MULT";
        case DIV: return "DIV";
        case MOD: return "MOD";
        case LSS: return "LSS";
        case LEQ: return "LEQ";
        case GRE: return "GRE";
        case GEQ: return "GEQ";
        case EQL: return "EQL";
        case NEQ: return "NEQ";
        case SEMICN: return "SEMICN";
        case COMMA: return "COMMA";
        case LPARENT: return "LPARENT";
        case RPARENT: return "RPARENT";
        case LBRACK: return "LBRACK";
        case RBRACK: return "RBRACK";
        case LBRACE: return "LBRACE";
        case RBRACE: return "RBRACE";
        case PRINTFTK: return "PRINTFTK";
        default: return "UNKNOWN";
    }
}