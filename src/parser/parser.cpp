#include <stdio.h>
#include <assert.h>

#include "parser.h"

#define ARRAY_COUNT(x) sizeof((x))/sizeof(*(x))




bool Parser::expect(TokenType type){
    return currentToken.type == type;
}

bool Parser::expectv(TokenType type[], int n){
    for (int i=0; i<n; i++){
        if (currentToken.type == type[i]){
            return true;
        }
    }
    return false;
}

Token Parser::peekToken(){
    return currentToken;
}


Token Parser::consumeToken(){
    Token current = this->currentToken;
    this->currentToken = this->tokenizer->nextToken();
    return current;
}


Node* Parser::parseLVal(){
    assert(this->expect(TOKEN_IDENTIFIER));
    return new Lvalue{.leaf = this->consumeToken()};
}

Node* Parser::parseRVal(){
    TokenType primaryTokenTypes[] = {
        TOKEN_IDENTIFIER, TOKEN_CHARACTER_LITERAL, TOKEN_STRING_LITERAL, 
        TOKEN_NUMERIC_BIN, TOKEN_NUMERIC_DEC, TOKEN_NUMERIC_DOUBLE, TOKEN_NUMERIC_FLOAT, TOKEN_NUMERIC_HEX, TOKEN_NUMERIC_OCT
    };
    
    if (this->expectv(primaryTokenTypes, ARRAY_COUNT(primaryTokenTypes))){
        return new Rvalue{.subexpr = parsePrimary()};
    }

    return 0;
}

Node* Parser::parsePrimary(){
    return new Primary{.leaf = this->consumeToken()};
}

Node* Parser::parseAssignment(){
    Lvalue *left = (Lvalue*)parseLVal();

    assert(this->expect(TOKEN_ASSIGNMENT));
    this->consumeToken();
    
    Rvalue *right = (Rvalue*)parseRVal();


    assert(this->expect(TOKEN_SEMI_COLON));
    this->consumeToken();


    return new Assignment{.left = left, .right = right};
}