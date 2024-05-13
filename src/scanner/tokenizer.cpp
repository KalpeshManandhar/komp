#include "keywords.h"
#include "tokenizer.h"

#include <ctype.h>
#include <fstream>


/*
    Numeric: [0-9]
    Non-numeric: [A-Z],[a-z],_

    Identifier char (characters in an identifier): [A-Z],[a-z],_,[0-9] 
    start with [a-z _][a-z0-9_]* 

    Number char (characters in an numeric constant): [a-f],[A-F],[0-9],. 
    - Decimal int: [1-9] [0-9]* 
    - Hex int: 0x[0-9 a-f]*
    - Octal int: 0[0-7]*
    - Binary int: 0b[0|1]*
    - Real: [0-9]*.[0-9]*

    Whitespaces: Spaces, tabs, newlines, carriage returns
    Punctuations: 
    [ ] ( ) { } . ->
    ++   -- & * + - ~ !
    / % << >> < > <= >= ==
    != ^ | && || ? : ; ...
    = *= /= %= += -= <<= >>=
    &= ^= |= , # ##
    <: :> <% %> %: %:%: 
    


    @Referenced from https://learn.microsoft.com/en-us/cpp/c-language/lexical-grammar?view=msvc-170 

*/

static bool isPunctuatorChar(char c){
    static const char* punctuators = "[]{}()<>+-*/~!#%^&;:=|?.,";

    for (int i=0; i<strlen(punctuators); i++){
        if (c == punctuators[i]){
            return true;
        }
    }

    return false;
}

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



void Tokenizer::init(){
    // initialize the dfa for scanning numeric constants
    this->numDFA.init();


}






Token Tokenizer::getIdentifierToken(){
    size_t tokenStart = this->cursor;
    while (!this->isEOF() && isIdentifierChar(this->buffer[this->cursor])){
        this->cursor++;
    }

    Splice s;
    s.data = &this->buffer[tokenStart];
    s.len  = this->cursor - tokenStart;
    
    Token t;
    t.type = TokenPrimaryType::TOKEN_IDENTIFIER;
    t.type2 = TokenSecondaryType::TOKEN_NONE;
    t.string = s;
    return t;
}

Token Tokenizer::getNumberToken(){
    size_t tokenStart = this->cursor;
    
    this->numDFA.restart();
    // 0x12s
    while (!this->isEOF() && !isWhitespace(this->buffer[this->cursor])
            && (!isPunctuatorChar(this->buffer[this->cursor]) || isNumberChar(this->buffer[this->cursor]))){
        this->numDFA.transition(this->buffer[this->cursor]);
        this->cursor++;
    }

    
    Splice s;
    s.data = &this->buffer[tokenStart];
    s.len  = this->cursor - tokenStart;
    
    Token t = this->numDFA.getToken();
    t.string = s;
    return t;
}



Token Tokenizer::getPunctuatorToken(){
    size_t tokenStart = this->cursor;

    while (!this->isEOF() && isPunctuatorChar(this->buffer[this->cursor])){
        this->cursor++;
    }
    
    Splice s;
    s.data = &this->buffer[tokenStart];
    s.len  = this->cursor - tokenStart;
    
    Token t;
    t.type = TokenPrimaryType::TOKEN_PUNCTUATOR;
    t.type2 = TokenSecondaryType::TOKEN_NONE;
    t.string = s;
    return t;
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

    Token t = {0};
    
    // starts with an alphabet or _
    if (isNonNumeric(this->buffer[this->cursor])){
        t = this->getIdentifierToken();
    }
    else if (isNumeric(this->buffer[this->cursor])){
        t = this->getNumberToken();
    }
    else if (isPunctuatorChar(this->buffer[this->cursor])){
        t = this->getPunctuatorToken();
    }
    else{
        this->skipNonWhitespaces();
    }

    if (t.type == TOKEN_ERROR){
        this->skipNonWhitespaces();
    }

    return t;

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
