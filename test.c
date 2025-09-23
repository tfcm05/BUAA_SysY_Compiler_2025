#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOKEN_LEN 100

// 定义符号编码
typedef enum {
    IDSY, INTSY, PLUSSY, STARSY, COMMASY, LPARSY, RPARSY, COLONSY, ASSIGNSY,
    BEGINSY, ENDSY, IFSY, THENSY, WHILESY, DOSY, CONSTSY, VARSY, PROCEDURESY
} TokenType;

// 全局变量
char token[MAX_TOKEN_LEN];
int token_len = 0;
char ch;
FILE *source_file;

// 函数声明
void getch();
void ungetch();
void getnbc();
void cat();
int isLetter();
int isDigit();
int reserve();
void error();

// 获取下一个字符
void getch() {
    ch = fgetc(source_file);
}

// 回退一个字符
void ungetch() {
    ungetc(ch, source_file);
}

// 跳过空白字符
void getnbc() {
    while (isspace(ch)) {
        getch();
    }
}

// 将当前字符添加到token中
void cat() {
    if (token_len < MAX_TOKEN_LEN - 1) {
        token[token_len++] = ch;
        token[token_len] = '\0';
    }
}

// 判断是否为字母
int isLetter() {
    return isalpha(ch);
}

// 判断是否为数字
int isDigit() {
    return isdigit(ch);
}

// 查找保留字
int reserve() {
    if (strcmp(token, "BEGIN") == 0) return BEGINSY;
    if (strcmp(token, "END") == 0) return ENDSY;
    if (strcmp(token, "IF") == 0) return IFSY;
    if (strcmp(token, "THEN") == 0) return THENSY;
    if (strcmp(token, "WHILE") == 0) return WHILESY;
    if (strcmp(token, "DO") == 0) return DOSY;
    if (strcmp(token, "CONST") == 0) return CONSTSY;
    if (strcmp(token, "VAR") == 0) return VARSY;
    if (strcmp(token, "PROCEDURE") == 0) return PROCEDURESY;
    return 0;
}

// 错误处理
void error() {
    printf("Lexical error: invalid character '%c'\n", ch);
}

// 获取token类型名称
const char* get_token_name(TokenType type) {
    switch (type) {
        case IDSY: return "IDSY";
        case INTSY: return "INTSY";
        case PLUSSY: return "PLUSSY";
        case STARSY: return "STARSY";
        case COMMASY: return "COMMASY";
        case LPARSY: return "LPARSY";
        case RPARSY: return "RPARSY";
        case COLONSY: return "COLONSY";
        case ASSIGNSY: return "ASSIGNSY";
        case BEGINSY: return "BEGINSY";
        case ENDSY: return "ENDSY";
        case IFSY: return "IFSY";
        case THENSY: return "THENSY";
        case WHILESY: return "WHILESY";
        case DOSY: return "DOSY";
        case CONSTSY: return "CONSTSY";
        case VARSY: return "VARSY";
        case PROCEDURESY: return "PROCEDURESY";
        default: return "UNKNOWN";
    }
}

// 词法分析主函数
void get_token(TokenType *token_type, int *value, char *token_str) {
    token_len = 0;
    token[0] = '\0';
    
    getnbc();
    getch();

    if (ch == EOF) {
        *token_type = -1;
        return;
    }
    
    if (isLetter()) {
        while (isLetter() || isDigit()) {
            cat();
            getch();
        }
        ungetch();
        int c = reserve();
        if (c == 0) {
            *token_type = IDSY;
            strcpy(token_str, token);
        } else {
            *token_type = c;
            token_str[0] = '\0';
        }
    } else if (isDigit()) {
        while (isDigit()) {
            cat();
            getch();
        }
        ungetch();
        *token_type = INTSY;
        *value = atoi(token);
        token_str[0] = '\0';
    } else {
        switch (ch) {
            case '+': 
                *token_type = PLUSSY;
                break;
            case '*': 
                *token_type = STARSY;
                break;
            case ',': 
                *token_type = COMMASY;
                break;
            case '(': 
                *token_type = LPARSY;
                break;
            case ')': 
                *token_type = RPARSY;
                break;
            case ':': 
                getch();
                if (ch == '=') {
                    *token_type = ASSIGNSY;
                } else {
                    ungetch();
                    *token_type = COLONSY;
                }
                break;
            default:
                error();
        }
        *value = 0;
        token_str[0] = '\0';
    }
}

int main() {
    source_file = fopen("source.txt", "r");
    if (!source_file) {
        printf("Cannot open source file\n");
        return 1;
    }
    
    TokenType token_type;
    int value;
    char token_str[MAX_TOKEN_LEN];
    
    while (1) {
        get_token(&token_type, &value, token_str);

        // 检查是否到达文件末尾
        if (token_type == -1) {
            break;
        }
        
        if (token_type == IDSY) {
            printf("(%s, %s)\n", get_token_name(token_type), token_str);
        } else if (token_type == INTSY) {
            printf("(%s, %d)\n", get_token_name(token_type), value);
        } else if (token_type >= BEGINSY && token_type <= PROCEDURESY) {
            printf("(%d, -)\n", token_type);
        } else {
            printf("(%s, -)\n", get_token_name(token_type));
        }
    }
    
    fclose(source_file);
    return 0;
}