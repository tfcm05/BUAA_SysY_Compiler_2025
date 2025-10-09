#include<iostream>
#include"include/LexicalAnalysis.h"
#include"include/Error.h"

std::unordered_map<std::string, TokenType> keywords = {
    {"main", MAINTK},
    {"const", CONSTTK},
    {"static", STATICTK},
    {"int", INTTK},
    {"void", VOIDTK},
    {"if", IFTK},
    {"else", ELSETK},
    {"for", FORTK},
    {"break", BREAKTK},
    {"continue", CONTINUETK},
    {"return", RETURNTK},
    {"printf", PRINTFTK}
};

std::unordered_map<char, TokenType> singleCharOps = {
    {'+', PLUS}, {'-', MINU}, {'*', MULT}, {'/', DIV}, {'%', MOD},
    {'<', LSS}, {'>', GRE}, {'=', ASSIGN}, {'!', NOT},
    {';', SEMICN}, {',', COMMA},
    {'(', LPARENT}, {')', RPARENT}, {'[', LBRACK}, {']', RBRACK}, 
    {'{', LBRACE}, {'}', RBRACE},
};

LexicalAnalysis::LexicalAnalysis(std::istream& in) : input(in), line(1) {
    advance();
}

void LexicalAnalysis::advance() {
    if (input.get(currentChar)) {
        if (currentChar == '\n') {
            line++;
        }
    } else {
        currentChar = '\0';
    }
}

char LexicalAnalysis::peek() {
    return input.peek();
}

void LexicalAnalysis::skipWhiteSpace() {
    while (std::isspace(currentChar)) {
        advance();
    }
}

Token LexicalAnalysis::number() {
    std::string number;

    while(std::isdigit(currentChar)) {
        number += currentChar;
        advance();
    }

    return Token(line, TokenType::INTCON, number);
}

Token LexicalAnalysis::identifier() {
    std::string identifier;

    while (std::isalnum(currentChar) || currentChar == '_') {
        identifier += currentChar;
        advance();
    }

    // 关键字
    if (keywords.find(identifier) != keywords.end()) {
        return Token(line, keywords[identifier], identifier);
    }

    return Token(line, TokenType::IDENFR, identifier);
}

Token LexicalAnalysis::string() {
    std::string string;

    do {
        string += currentChar;
        advance();
    }  while (currentChar != '"');

    string += currentChar;
    advance();

    return Token(line, TokenType::STRCON, string);
}

Token LexicalAnalysis::getNextTokenInternal() {
    skipWhiteSpace();

    // 文件终止
    if (currentChar == '\0') {
        return Token(line, TokenType::END, "");
    }
    // 数字 
    if (std::isdigit(currentChar)) {
        return number();
    }
    // 标识符
    if (std::isalnum(currentChar) || currentChar == '_') {
        return identifier();
    }
    // 字符串
    if (currentChar == '"') {
        return string();
    }

    // 单符
    if (singleCharOps.find(currentChar) != singleCharOps.end()) {
        char temp = currentChar;
        advance();
        // 双符
        if (temp == '>' && currentChar == '=') {
            advance();
            return Token(line, TokenType::GEQ, ">=");
        } else if (temp == '<' && currentChar == '=') {
            advance();
            return Token(line, TokenType::LEQ, "<=");
        } else if (temp == '=' && currentChar == '=') {
            advance();
            return Token(line, TokenType::EQL, "==");
        } else if (temp == '!' && currentChar == '=') {
            advance();
            return Token(line, TokenType::NEQ, "!=");
        }
        return Token(line, singleCharOps[temp], std::string (1, temp));
    }

    // 双符
    if (currentChar == '&') {
        if (peek() == '&') {
            advance();
            return Token(line, TokenType::AND, "&&");
        } else {
            // 错误处理
            errorLog(ErrorCode::a, line);
            advance();
            return Token(line, TokenType::AND, "&&");
        }
    } else if (currentChar == '|') {
        if (peek() == '|') {
            advance();
            return Token(line, TokenType::OR, "||");
        } else {
            // 错误处理
            errorLog(ErrorCode::a, line);
            advance();
            return Token(line, TokenType::OR, "||");
        }
    }

    // 处理
    return Token(line, TokenType::ERROR, "");
}

Token LexicalAnalysis::getNextToken() {
    // 如果缓冲区有Token，先从缓冲区取
    if (!tokenBuffer.empty()) {
        Token token = tokenBuffer.front();
        tokenBuffer.pop_front();
        return token;
    }
    
    // 否则直接从输入流获取
    return getNextTokenInternal();
}

Token LexicalAnalysis::peekNextToken() {
    // 如果缓冲区为空，先获取一个Token放入缓冲区
    if (tokenBuffer.empty()) {
        tokenBuffer.push_back(getNextTokenInternal());
    }
    
    // 返回缓冲区的第一个Token
    return tokenBuffer.front();
}

Token LexicalAnalysis::peekSecondToken() {
    // 确保缓冲区至少有2个Token
    while (tokenBuffer.size() < 2) {
        tokenBuffer.push_back(getNextTokenInternal());
    }
    
    // 返回缓冲区的第二个Token
    return tokenBuffer[1];
}