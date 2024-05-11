#pragma once

#include "str.h"


enum TokenPrimaryType{
    TOKEN_ERROR,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_NUMBER,
    TOKEN_EOF,

    TOKEN_PRIMARY_COUNT
};

static const char *TOKEN_PRIMARY_TYPE_STRING[] = {
    "TOKEN_ERROR",
    "TOKEN_IDENTIFIER",
    "TOKEN_KEYWORD",
    "TOKEN_NUMBER",
    "TOKEN_EOF",

    "TOKEN_COUNT"
};



enum TokenSecondaryType{
    TOKEN_NUMERIC_HEX,
    TOKEN_NUMERIC_BIN,
    TOKEN_NUMERIC_DEC,
    TOKEN_NUMERIC_OCT,
    TOKEN_NUMERIC_DOUBLE,


    TOKEN_SECONDARY_COUNT
};


struct Token{
    TokenPrimaryType type;
    TokenSecondaryType type2;
    int lineNo, charNo;
    
    Splice string;
};