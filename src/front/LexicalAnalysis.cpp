#include<iostream>
#include"../../include/front/LexicalAnalysis.h"
#include"../../include/Error.h"

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
    char preChar = currentChar;
    if (input.get(currentChar)) {
        if (preChar == '\n') {
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

void LexicalAnalysis::skipComment() {
    // 单行注释 //
    if (currentChar == '/' && peek() == '/') {
        advance();
        advance();
        // 跳过直到行尾或文件结束
        while (currentChar != '\n' && currentChar != '\0') {
            advance();
        }
        if (currentChar == '\n') {
            advance();
        }
    }
    // 多行注释 /* */
    else if (currentChar == '/' && peek() == '*') {
        advance();
        advance();

        while (currentChar != '\0') {
            if (currentChar == '*' && peek() == '/') {
                advance();
                advance();
                break;
            }
            advance(); // 跳过注释内容
        }
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
    while (true) {
        skipWhiteSpace();
        
        // 检查注释
        if (currentChar == '/' && (peek() == '/' || peek() == '*')) {
            skipComment();
            continue; // 跳过注释后继续处理下一个token
        }
        
        break;
    }

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

// 确保缓冲区大小
void LexicalAnalysis::ensureBufferSize(size_t size) {
    while (tokenBuffer.size() < size) {
        tokenBuffer.emplace_back(getNextTokenInternal());
    }
}

Token LexicalAnalysis::getNextToken() {
    // 如果缓冲区有Token，先从缓冲区取
    if (!tokenBuffer.empty()) {
        Token token = std::move(tokenBuffer.front());  // 使用移动而不是拷贝
        tokenBuffer.pop_front();
        return token;
    }
    
    // 否则直接从输入流获取
    return getNextTokenInternal();
}

Token LexicalAnalysis::peekNextToken() {
    ensureBufferSize(1);
    return tokenBuffer.front();  // 拷贝
}

Token LexicalAnalysis::peekSecondToken() {
    ensureBufferSize(2);
    return tokenBuffer[1];  // 拷贝
}

const Token& LexicalAnalysis::peekNextTokenRef() {
    ensureBufferSize(1);
    return tokenBuffer.front();  // 返回引用，无拷贝
}

const Token& LexicalAnalysis::peekSecondTokenRef() {
    ensureBufferSize(2);
    return tokenBuffer[1];  // 返回引用，无拷贝
}

bool LexicalAnalysis::isAssign() {
    int num = tokenBuffer.size();
    for (int i = 0; i < num; i++) {
        if (tokenBuffer[i].getTokenType() == ASSIGN) {
            return true;
        } else if (tokenBuffer[i].getTokenType() == SEMICN) {
            return false;
        }
    }

    Token temp;
    do {
        temp = getNextTokenInternal();
        tokenBuffer.emplace_back(temp);
        if (temp.getTokenType() == ASSIGN) {
            return true;
        }
    } while (temp.getTokenType() != SEMICN && temp.getTokenType() != END);

    return false;
}