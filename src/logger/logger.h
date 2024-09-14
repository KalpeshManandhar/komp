#pragma once

#include <stdio.h>


#define logErrorMessage(errorAtToken, message, ...)  \
        fprintf(stderr, "[Error] %3d:%-3d " message "\n", errorAtToken.lineNo, errorAtToken.charNo, __VA_ARGS__);



enum ErrorCode{
    ERROR_UNKNOWN,

    ERROR_COUNT
};


static const char * ERROR_STRINGS[ERROR_COUNT] = {
    "Something is wrong here. Goodluck finding it!",
};


#define logErrorCode(file, line, charNo, code) logError(file, line, charNo, ERROR_STRINGS[code])

inline void logError(const char *file, int lineNo, int charNo, const char * message){
    fprintf(stderr, "%s:\n",file);
    fprintf(stderr, "%d:%d [ERROR] %s\n ", lineNo, charNo, message);
}
