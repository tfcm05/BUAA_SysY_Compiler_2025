#include<fstream>
#include"include/Error.h"
#include"include/Token.h"

void errorLog(ErrorCode errorCode, int line) {
    std::ofstream error("error.txt", std::ios::app);
    error <<  line << " " << char (errorCode + 'a') << std::endl;
    error.close();
}

void lexerLog(Token token) {
    std::ofstream lexer("lexer.txt", std::ios::app);
    if (token.getTokenType() == END) return;
    lexer << getTypeCode(token.getTokenType()) << " " << token.getValue() << std::endl;
    lexer.close();
}