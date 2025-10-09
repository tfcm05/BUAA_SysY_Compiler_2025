#include"Token.h"

typedef enum {
    a, // 非法符号
    b, c, d, e, f, g, h,
    i, j, k, // 缺失分号，右小括号，右中括号
} ErrorCode;

void errorLog(ErrorCode errorCode, int line);

void lexerLog(Token token);