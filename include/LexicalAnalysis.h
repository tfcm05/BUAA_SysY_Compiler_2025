#ifndef LEXICALANALYSIS_H
#define LEXICALANALYSIS_H
#include<unordered_map>
#include"Token.h"

const char* getTypeCode(TokenType tokentype);

void viewTokens(int num, Token* tokens);

class LexicalAnalysis {
    private:
        Token tokens[12] = {};
    public:
        int lexicalAnalysis(std::string input, int lineNum);
        Token* getTokens();
};
#endif