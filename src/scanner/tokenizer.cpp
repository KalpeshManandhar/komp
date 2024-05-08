#include "keywords.h"
#include "tokenizer.h"

#include <ctype.h>
#include <fstream>


/*
    Numeric: [0-9]
    Non-numeric: [A-Z],[a-z],_

    Identifier char (characters in an identifier): [A-Z],[a-z],_,[0-9] 
    Number char (characters in an numeric constant): [a-f],[A-F],[0-9],. 
    - Decimal int: [1-9] [0-9]* 
    - Hex int: 0x[0-9 a-f]*
    - Octal int: 0[0-7]*
    - Binary int: 0b[0|1]*
    - Real: [0-9]*.[0-9]*

    Whitespaces: Spaces, tabs, newlines, carriage returns




*/

static bool isBetween(char c, char start, char end){
    return c >= start && c <= end;
}


static bool isNumeric(char c){
    return isdigit(c);
}

static bool isNonNumeric(char c){
    return isalpha(c) || c == '_';
}


static bool isWhitespace(char c){
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static bool isIdentifierChar(char c){
    return isNonNumeric(c) || isdigit(c);
}

static bool isNumberChar(char c){
    return isNumeric(c) || c == '.' || c == 'x' || isBetween(c, 'a', 'f') || isBetween(c, 'A', 'F');
}






void Tokenizer::skipNonWhitespaces(){
    while (!isWhitespace(this->buffer[this->cursor])){
        this->cursor++;
    }
}

void Tokenizer::skipWhitespaces(){
    while (isWhitespace(this->buffer[this->cursor])){
        this->cursor++;
    }
}


Token Tokenizer::getIdentifierToken(){
    while (!this->isEOF() && isIdentifierChar(this->buffer[this->cursor])){
        
        
        this->cursor++;
    }
    
    return Token{
        TokenType::TOKEN_IDENTIFIER
    };
}

Token Tokenizer::getNumberToken(){
    while (!this->isEOF() && isNumberChar(this->buffer[this->cursor])){
        
        
        this->cursor++;
    }
    
    return Token{
        TokenType::TOKEN_NUMBER
    };
}


bool Tokenizer::isEOF(){
    return this->cursor >= this->bufferSize;
}


Token Tokenizer::nextToken(){
    if (this->isEOF()){
        return Token{
            TOKEN_EOF
        };
    }

    // get to next token
    this->skipWhitespaces();
    
    // starts with an alphabet or _
    if (isNonNumeric(this->buffer[this->cursor])){
        return this->getIdentifierToken();
    }

    if (isNumeric(this->buffer[this->cursor])){
        return this->getNumberToken();
    }
    
    this->skipNonWhitespaces();

    return Token{
        TOKEN_ERROR
    };

}



void Tokenizer::loadFileToBuffer(const char *filepath){
    std::ifstream f(filepath);
    
    f.seekg(0, std::ios::end);
    this->bufferSize = f.tellg();
    f.seekg(0, std::ios::beg);

    this->buffer = new char[this->bufferSize + 1];
    
    f.read(this->buffer, this->bufferSize); 
    this->buffer[this->bufferSize] = 0;

    this->cursor = 0;
}