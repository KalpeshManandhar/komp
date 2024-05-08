#pragma once


enum TokenType{
    TOKEN_ERROR,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_NUMBER,
    TOKEN_EOF,

    TOKEN_COUNT
};

static const char *TOKEN_STRINGS[] = {
    "TOKEN_ERROR",
    "TOKEN_IDENTIFIER",
    "TOKEN_KEYWORD",
    "TOKEN_NUMBER",
    "TOKEN_EOF",

    "TOKEN_COUNT"
};


struct Token{
    int type;
    // int lineNo, charNo;
};






struct Tokenizer{
private:
    size_t cursor;

    char* buffer;
    size_t bufferSize;

    Token getIdentifierToken();
    Token getNumberToken();

    Token getIntegerToken_Hex();
    Token getIntegerToken_Binary();
    Token getIntegerToken_Decimal();



    void skipNonWhitespaces();
    void skipWhitespaces();
    bool isEOF();

public:    
    void loadFileToBuffer(const char *filepath);
    Token nextToken();
};






