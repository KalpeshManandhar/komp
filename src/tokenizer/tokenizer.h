#pragma once

#include "fa.h"

#include "token.h"




struct Tokenizer{
private:
    size_t cursor;

    char* buffer;
    size_t bufferSize;

    NumConstDFA numDFA;
    PunctuatorDFA puncDFA;
    StringLitDFA strDFA;



    Token getIdentifierToken();
    Token getNumberToken();
    Token getCharLiteralToken();
    Token getStringLiteralToken();
    Token getPunctuatorToken();


    bool checkForComments();
    void skipComments();
    
    void skipUntil(char c);
    void skipNonWhitespaces();
    void skipWhitespaces();
    bool isEOF();

    char peekChar();
    char consumeChar();


public:    

    int lineNo;
    int charNo;
    char fileName[100];
    size_t errors;

    void init();
    void loadFileToBuffer(const char *filepath);
    Token nextToken();
    
    void rewindTo(Token checkpoint);
};






