#include"../../include/front/Token.h"

Token::Token(int line, TokenType tokenType, std::string value) : line(line), tokenType(tokenType), value(value) {}

// 拷贝构造函数实现
Token::Token(const Token& other)
    : line(other.line), tokenType(other.tokenType), value(other.value) {}

// 拷贝赋值运算符实现
Token& Token::operator=(const Token& other) {
    if (this != &other) {
        line = other.line;
        tokenType = other.tokenType;
        value = other.value;
    }
    return *this;
}

// 移动构造函数实现
Token::Token(Token&& other) noexcept
    : line(other.line), tokenType(other.tokenType), value(std::move(other.value)) {
    other.line = 0;
    other.tokenType = TokenType::ERROR;
}

// 移动赋值运算符实现
Token& Token::operator=(Token&& other) noexcept {
    if (this != &other) {
        line = other.line;
        tokenType = other.tokenType;
        value = std::move(other.value);
        
        other.line = 0;
        other.tokenType = TokenType::ERROR;
    }
    return *this;
}

int Token::getLine() const {
    return line;
}

TokenType Token::getTokenType() const {
    return tokenType;
}

std::string Token::getValue() const {
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