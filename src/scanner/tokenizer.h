#pragma once

#include "str.h"
#include "fa.h"

#include "token_types.h"

struct Token{
    int type;
    int type2;
    int lineNo, charNo;
    
    Splice string;
};






struct Tokenizer{
private:
    size_t cursor;

    char* buffer;
    size_t bufferSize;

    NumConstDFA numDFA;


    Token getIdentifierToken();
    Token getNumberToken();
    Token getPunctuatorToken();

    Token getIntegerToken_Hex();
    Token getIntegerToken_Binary();
    Token getIntegerToken_Decimal();



    void skipNonWhitespaces();
    void skipWhitespaces();
    bool isEOF();

public:    
    void init();
    void loadFileToBuffer(const char *filepath);
    Token nextToken();
};






