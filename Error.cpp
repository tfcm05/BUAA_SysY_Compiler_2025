#include<fstream>
#include"include/Error.h""

void errorLog(ErrorCode errorCode, int line) {
    std::ofstream error("error.txt", std::ios::app);
    error <<  line << " " << char (errorCode + 'a') << std::endl;
    error.close();
}