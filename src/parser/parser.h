#pragma once

#include "node.h"
#include <tokenizer/tokenizer.h>
#include <vector>


struct Parser{
    std::vector<Node *> statements;

    Tokenizer * tokenizer;
    Token currentToken;

    bool expect(TokenType type);
    bool match(TokenType type);
    bool matchv(TokenType type[], int n);
    Token peekToken();
    Token consumeToken();  


    Node* parseLVal();
    Node* parseRVal();
    Node* parsePrimary();
    Node* parseAssignment();
    Node* parseSubexpr(int precedence);
    Node* parseDeclaration();
    Node* parseStatement();
    Node* parseStatementBlock();
    Node* parseIf();
    Node* parseWhile();
    Node* parseFor();


public:

    void init(Tokenizer *t){
        this->tokenizer = t;
        this->currentToken = t->nextToken();
    }
    
    bool parse(){
        while (currentToken.type != TOKEN_EOF){
            this->statements.push_back(this->parseStatement());
        }
        return true;
    }
};


void printParseTree(Node *const root, int depth = 0);
