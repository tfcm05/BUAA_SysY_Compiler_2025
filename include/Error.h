#ifndef ERROR_H
#define ERROR_H

typedef enum {
    a, // illegal character
    b, // declared twice
    c, // undefined identifier
    d, // function call parameter mismatch
    e, // function call parameter type mismatch
    f, g,  // function return type mismatch
    h, // const variable assignment
    i, j, k, // missing right bracket
    l, // printf parameter mismatch
    m, // break or continue outside of loop
} ErrorCode;

void errorLog(ErrorCode errorCode, int line);

void dumpErrorsToFile();

#endif