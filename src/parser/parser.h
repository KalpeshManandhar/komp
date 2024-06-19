#pragma once

#include "node.h"
#include <tokenizer/tokenizer.h>


struct Parser{
    Node * root;

    Tokenizer * tokenizer;
    Token currentToken;

    bool expect(TokenType type);
    bool expectv(TokenType type[], int n);
    Token peekToken();
    Token consumeToken();  


    Node* parseLVal();
    Node* parseRVal();
    Node* parsePrimary();
    Node* parseAssignment();
    Node* parseSubexpr();


public:

    void init(Tokenizer *t){
        this->tokenizer = t;
        this->currentToken = t->nextToken();
    }
    
    bool parse(){
        this->root = parseAssignment();
        return true;
    }
};


void printParseTree(Parser *const p);
