#include<iostream>
#include"include/LexicalAnalysis.h"

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
    {'{', LBRACE}, {'}', RBRACE}
};

// 词法分析，返回token数
int LexicalAnalysis::lexicalAnalysis(std::string input, int lineNum) {
    int num = 0;
    
    int index = 0;
    int length = input.length();
    
    while (index < length && num < 12) {
        // 跳过空白字符
        if (isspace(input[index])) {
            index++;
            continue;
        }
        
        // 检查是否为标识符或关键字（以字母或下划线开头）
        if (isalpha(input[index]) || input[index] == '_') {
            std::string identifier;
            while (index < length && (isalnum(input[index]) || input[index] == '_')) {
                identifier += input[index];
                index++;
            }
            
            // 检查是否为关键字
            if (keywords.find(identifier) != keywords.end()) {
                tokens[num].setToken(lineNum, keywords[identifier], identifier);
            } else {
                tokens[num].setToken(lineNum, IDENFR, identifier);
            }
            num++;
        }
        // 检查是否为整数常量
        else if (isdigit(input[index])) {
            std::string number;
            while (index < length && isdigit(input[index])) {
                number += input[index];
                index++;
            }
            tokens[num].setToken(lineNum, INTCON, number);
            num++;
        }
        // 检查是否为字符串常量
        else if (input[index] == '"') {
            std::string strConst = "\"";
            index++; // 跳过开头的双引号
            
            while (index < length && input[index] != '"') {
                strConst += input[index];
                index++;
            }
            
            if (index < length && input[index] == '"') {
                strConst += '"';
                index++; // 跳过结尾的双引号
                tokens[num].setToken(lineNum, STRCON, strConst);
                num++;
            }
        }
        // 检查双字符运算符
        else if (index + 1 < length) {
            std::string twoChar = input.substr(index, 2);
            if (twoChar == "&&") {
                tokens[num].setToken(lineNum, AND, "&&");
                index += 2;
                num++;
            }
            else if (twoChar == "||") {
                tokens[num].setToken(lineNum, OR, "||");
                index += 2;
                num++;
            }
            else if (twoChar == "<=") {
                tokens[num].setToken(lineNum, LEQ, "<=");
                index += 2;
                num++;
            }
            else if (twoChar == ">=") {
                tokens[num].setToken(lineNum, GEQ, ">=");
                index += 2;
                num++;
            }
            else if (twoChar == "==") {
                tokens[num].setToken(lineNum, EQL, "==");
                index += 2;
                num++;
            }
            else if (twoChar == "!=") {
                tokens[num].setToken(lineNum, NEQ, "!=");
                index += 2;
                num++;
            }
            // 单字符运算符
            else if (singleCharOps.find(input[index]) != singleCharOps.end()) {
                std::string opStr(1, input[index]);
                tokens[num].setToken(lineNum, singleCharOps[input[index]], opStr);
                index++;
                num++;
            }
            else {
                // 无法识别的字符，跳过
                index++;
            }
        }
        // 单字符运算符（在字符串末尾的情况）
        else if (singleCharOps.find(input[index]) != singleCharOps.end()) {
            std::string opStr(1, input[index]);
            tokens[num].setToken(lineNum, singleCharOps[input[index]], opStr);
            index++;
            num++;
        }
        else {
            // 无法识别的字符，进行处理
            index++;
        }
    }

    // 查看tokens
    viewTokens(num, tokens);

    return num;
}

Token* LexicalAnalysis::getTokens() {
    return tokens; 
}

void viewTokens(int num, Token* tokens) {
    for (int i = 0; i < num; i++) {
        std::cout << getTypeCode((tokens + i)->getTokenType()) << " " << (tokens + i)->getValue() << std::endl;
    }
}

// 获取类型码
const char* getTypeCode(TokenType tokentype) {
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