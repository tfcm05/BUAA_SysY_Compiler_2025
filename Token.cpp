#include"include/Token.h"

void Token:: setToken(int lineNum, TokenType tokenType, std::string value) {
    Token::lineNum = lineNum;
    Token::tokenType = tokenType;
    Token::value = value;
}

int Token::getLineNum() {
    return lineNum;
}

TokenType Token::getTokenType() {
    return tokenType;
}

std::string Token::getValue() {
    return value;
}