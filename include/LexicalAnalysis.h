#ifndef LEXICALANALYSIS_H
#define LEXICALANALYSIS_H
#include<unordered_map>
#include<deque>
#include"Token.h"

class LexicalAnalysis {
    private:
        std::istream& input; // 输入流
        int line; // 当前行号
        char currentChar; // 当前字符

        std::deque<Token> tokenBuffer; // token缓冲区

        void advance(); // 读取下一个字符
        char peek(); // 查看下一个字符

        void skipWhiteSpace(); // 跳过空白符
        Token number();
        Token identifier();
        Token string();

        Token getNextTokenInternal(); // 内部读取下一个token
    public:
        ~LexicalAnalysis() = default;
        LexicalAnalysis() = default;

        LexicalAnalysis(std::istream& in);
        Token getNextToken(); // 读取下一个token
        Token peekNextToken();  // 查看下一个token
        Token peekSecondToken(); // 查看第二个token
};
#endif