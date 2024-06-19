#include <stdio.h>
#include <assert.h>

#include "parser.h"

#define ARRAY_COUNT(x) sizeof((x))/sizeof(*(x))

static TokenType primaryTokenTypes[] = {
    TOKEN_IDENTIFIER, 
    TOKEN_CHARACTER_LITERAL, 
    TOKEN_STRING_LITERAL, 
    TOKEN_NUMERIC_BIN, 
    TOKEN_NUMERIC_DEC, 
    TOKEN_NUMERIC_DOUBLE, 
    TOKEN_NUMERIC_FLOAT, 
    TOKEN_NUMERIC_HEX, 
    TOKEN_NUMERIC_OCT
};

static TokenType binaryOperators[] = {
    TOKEN_PLUS, 
    TOKEN_MINUS, 
    TOKEN_STAR, 
    TOKEN_SLASH, 
    TOKEN_AMPERSAND, 
    TOKEN_BITWISE_OR, 
    TOKEN_BITWISE_XOR, 
    TOKEN_SHIFT_LEFT, 
    TOKEN_SHIFT_RIGHT,
};


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
    return new Rvalue{.subexpr = (Subexpr *)parseSubexpr()};
}

Node* Parser::parseSubexpr(){
    Subexpr *s = new Subexpr;
    s->left = (Subexpr*)this->parsePrimary();

    if (expectv(binaryOperators, ARRAY_COUNT(binaryOperators))){
        s->op = this->consumeToken();

        s->right = (Subexpr*)this->parseSubexpr();
        s->tag = Subexpr::SUBEXPR_RECURSE_OP;
    }        
    return s;
}

Node* Parser::parsePrimary(){
    Subexpr *s = new Subexpr;
    if (expect(TOKEN_PARENTHESIS_OPEN)){
        this->consumeToken();
        s->inside = (Subexpr*)this->parseSubexpr();
        
        assert(expect(TOKEN_PARENTHESIS_CLOSE));
        this->consumeToken();
    }
    else if (expectv(primaryTokenTypes, ARRAY_COUNT(primaryTokenTypes))){
        s->leaf = this->consumeToken();
        s->tag = Subexpr::SUBEXPR_LEAF;
    }
    return s;
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


void printParseTree(Parser *const p){
    
}
 