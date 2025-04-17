#include "keywords.h"
#include "tokenizer.h"
#include <logger/logger.h>

#include <utils/utils.h>

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
    return isNumeric(c) || c == '.' || c == 'x' || isBetween(c, 'a', 'f') || isBetween(c, 'A', 'F') || c == 'U' || c == 'L' || c == '+' || c == '-';
}

static bool isStringLiteralChar(char c){
    return c >= ' ' && c <= '~';
}




void Tokenizer::skipNonWhitespaces(){
    while (!this->isEOF() && !isWhitespace(this->buffer[this->cursor])){
        this->consumeChar();
    }
}

void Tokenizer::skipWhitespaces(){
    while (!this->isEOF() && isWhitespace(this->buffer[this->cursor])){
        this->consumeChar();
    }
}

void Tokenizer::skipUntil(char c){
    while (!this->isEOF() && this->buffer[this->cursor] != c){
        this->consumeChar();
    }
}

void Tokenizer::skipComments(){
    if (!this->isEOF() && this->buffer[this->cursor] == '/'){
        // if single line comment, skip until a newline
        if ((this->cursor + 1) < this->bufferSize && this->buffer[this->cursor + 1] == '/'){
            this->skipUntil('\n');
            this->consumeChar();
        }
        // if multi line comment, skip until */ is found
        else if ((this->cursor + 1) < this->bufferSize && this->buffer[this->cursor + 1] == '*'){
            // skip until a *, then check if next char is a /
            while (true){
                this->skipUntil('*');
                this->consumeChar();
                
                // if eof is encountered before finding a */, return error
                if (this->isEOF()){
                    std::cout<<"ERROR: Multiline comment no end";
                    return;
                } 
                if (this->buffer[this->cursor] == '/'){
                    this->consumeChar();
                    break;
                }
            }

        }
    }   
}



void Tokenizer::init(){
    // initialize the dfas 
    this->numDFA.init();
    this->puncDFA.init();
    this->strDFA.init();

    lineNo = 1;
    charNo = 1;
    errors = 0;


}






Token Tokenizer::getIdentifierToken(){
    size_t tokenStart = this->cursor;
    while (!this->isEOF() && isIdentifierChar(this->peekChar())){
        this->consumeChar();
    }

    Splice s;
    s.data = &this->buffer[tokenStart];
    s.len  = this->cursor - tokenStart;

    Token t;
    t.type = TokenType::TOKEN_IDENTIFIER;
    t.string = s;
    
    // compare with keywords
    for (int i=0; i<N_KEYWORDS; i++){
        if (compare(s, KEYWORDS[i])){
            t.type = TokenType::TOKEN_KEYWORDS_START + i + 1;
            break;
        }
    }
    
    return t;
}


Token Tokenizer::getStringLiteralToken(){
    size_t tokenStart = this->cursor;

    this->strDFA.restart();
    while (!this->isEOF() && isStringLiteralChar(this->buffer[this->cursor])){
        if (this->strDFA.willErrorTransition(this->buffer[this->cursor])){
            break;
        }
        this->strDFA.transition(this->buffer[this->cursor]);
        this->consumeChar();
    }

    Splice s;
    s.data = &this->buffer[tokenStart];
    s.len  = this->cursor - tokenStart;

    Token t;
    t.type = TokenType::TOKEN_STRING_LITERAL;
    t.string = s;
    
    return t;
}

Token Tokenizer::getNumberToken(){
    size_t tokenStart = this->cursor;
    
    this->numDFA.restart();
    // isPunctuatorChar is used to consume chars even if it is error
    // eg: 0x123s is a whole error token instead of 0x123 and s as two separate tokens
    while (!this->isEOF() && !isWhitespace(this->buffer[this->cursor])
            && (!isPunctuatorChar(this->buffer[this->cursor]) || isNumberChar(this->buffer[this->cursor]))){
        this->numDFA.transition(this->buffer[this->cursor]);
        this->consumeChar();
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
    
    this->puncDFA.restart();
    while (!this->isEOF() && isPunctuatorChar(this->buffer[this->cursor])){
        if (this->puncDFA.willErrorTransition(this->buffer[this->cursor])){
            break;
        }
        this->puncDFA.transition(this->buffer[this->cursor]);
        this->consumeChar();
    }
    
    Splice s;
    s.data = &this->buffer[tokenStart];
    s.len  = this->cursor - tokenStart;
    
    Token t = this->puncDFA.getToken();
    t.string = s;
    return t;
}


Token Tokenizer::getCharLiteralToken() {
    size_t tokenStart = this->cursor;
    Token t;
    t.type = TOKEN_CHARACTER_LITERAL;
    // consume start single quote
    this->consumeChar();
    
    if (this->peekChar() == '\\'){
        this->consumeChar();
        auto isAllowed = [&](char c){
            char allowed[] = {
                'r',
                'n',
                't',
                '\\',
                '\'',
                '0',
            };
            
            for (int i=0; i<ARRAY_COUNT(allowed); i++){
                if (c == allowed[i]){
                    return true;
                }
            }
            return false;
        };

        if (!isAllowed(this->peekChar())){
            t.type = TOKEN_ERROR;
        }
        this->consumeChar();
    }
    else {
        this->consumeChar();
    }


    if (this->peekChar() != '\''){
        this->skipNonWhitespaces();
        t.type = TOKEN_ERROR;
    }
    else {
        this->consumeChar();
    }

    Splice s;
    s.data = &this->buffer[tokenStart];
    s.len  = this->cursor - tokenStart;
    
    t.string = s;

    return t;
    
}



bool Tokenizer::isEOF(){
    return this->cursor >= this->bufferSize;
}




Token Tokenizer::nextToken(){
    // loop until a valid token is reached
    while(true){
        // get to next token
        this->skipWhitespaces();
        
        // check if it is start of a comment and skip
        if (!this->checkForComments())
            break;

        this->skipComments();
    }

    if (this->isEOF()){
        return Token{
            .type = TOKEN_EOF,
            .charNo = this->charNo,
            .lineNo = this->lineNo,
        };
    }

    Token t = {0};
    int tokenCharNo = this->charNo;
    int tokenLineNo = this->lineNo;

    
    // starts with an alphabet or _
    if (isNonNumeric(this->buffer[this->cursor])){
        t = this->getIdentifierToken();
    }
    // is a numeric digit: numeric constant
    else if (isNumeric(this->buffer[this->cursor])){
        t = this->getNumberToken();
    }
    // is a punctuator character: punctuator
    else if (isPunctuatorChar(this->buffer[this->cursor])){
        t = this->getPunctuatorToken();
    }
    // starts with ": string
    else if (this->buffer[this->cursor] == '"'){
        t = this->getStringLiteralToken();
    }
    // starts with ": string
    else if (this->buffer[this->cursor] == '\''){
        t = this->getCharLiteralToken();
    }
    else{
        t.type = TOKEN_ERROR;
    }
    
    

    if (t.type == TOKEN_ERROR){
        this->errors++;
        
        logErrorCode(this->fileName, this->lineNo, this->charNo, ERROR_UNKNOWN);
        this->skipNonWhitespaces();
    }
    t.lineNo = tokenLineNo;
    t.charNo = tokenCharNo;
    return t;

}



void Tokenizer::loadFileToBuffer(const char *filepath){
    std::ifstream f(filepath, std::ios::binary);
    
    f.seekg(0, std::ios::end);
    this->bufferSize = f.tellg();
    f.seekg(0, std::ios::beg);

    this->buffer = new char[this->bufferSize + 1];
    
    f.read(this->buffer, this->bufferSize); 
    this->buffer[this->bufferSize] = 0;

    this->cursor = 0;
    f.close();

    strncpy(this->fileName, filepath, min(sizeof(this->fileName), strlen(filepath)));
}


bool Tokenizer::checkForComments(){
    if (!this->isEOF() && this->buffer[this->cursor] == '/'){
        // check for "//"
        if ((this->cursor + 1) < this->bufferSize && this->buffer[this->cursor + 1] == '/'){
            return true;
        }
        // check for "/*"
        if ((this->cursor + 1) < this->bufferSize && this->buffer[this->cursor + 1] == '*'){
            return true;
        }
    }
    return false;
}


char Tokenizer::peekChar(){
    return this->buffer[this->cursor];
}

char Tokenizer::consumeChar(){
    if (isEOF()){
        return 0;
    }
    char c = this->buffer[this->cursor];
    if (c == '\n'){
        this->lineNo++;
        this->charNo = 1;
    }
    else{
        this->charNo++;
    }
    this->cursor++;
    return c;
}

void Tokenizer::rewindTo(Token checkpoint){
    this->charNo = checkpoint.charNo;
    this->lineNo = checkpoint.lineNo;
    this->cursor = checkpoint.string.data - this->buffer;
}