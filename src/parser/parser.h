#pragma once

#include "node.h"
#include <tokenizer/tokenizer.h>
#include <vector>


struct Parser{
    std::vector<Node *> statements;

    Tokenizer * tokenizer;
    Token currentToken;

    StatementBlock global;


    bool expect(TokenType type);
    bool match(TokenType type);
    bool matchv(TokenType type[], int n);
    bool tryRecover();
    Token peekToken();
    Token consumeToken();  




    Subexpr parseIdentifier(StatementBlock *scope);
    Node* parseLVal(StatementBlock *scope);
    Node* parseRVal(StatementBlock *scope);
    Node* parsePrimary(StatementBlock *scope);
    Node* parseAssignment(StatementBlock *scope);
    Node* parseSubexpr(int precedence, StatementBlock *scope);
    Node* parseDeclaration(StatementBlock *scope);
    Node* parseStatement(StatementBlock *scope);
    Node* parseStatementBlock(StatementBlock *scope);
    Node* parseIf(StatementBlock *scope);
    Node* parseWhile(StatementBlock *scope);
    Node* parseFor(StatementBlock *scope);


public:
        
    size_t errors;


    void init(Tokenizer *t){
        this->tokenizer = t;
        this->currentToken = t->nextToken();
        this->errors = 0;
        this->global.parent = 0;

    }
    
    bool parse(){
        while (currentToken.type != TOKEN_EOF){
            this->statements.push_back(this->parseStatement(&global));
        }
        
        fprintf(stdout, "[Parser] %llu errors generated.\n", errors);
        return errors == 0;
    }
};


void printParseTree(Node *const root, int depth = 0);
