#pragma once

#include "node.h"
#include <tokenizer/tokenizer.h>
#include <vector>


struct Parser{
    std::vector<Node *> statements;

    Tokenizer * tokenizer;
    Token currentToken;

    StatementBlock global;
    SymbolTable<Function> functions;


    bool expect(TokenType type);
    bool match(TokenType type);
    bool matchv(TokenType type[], int n);
    bool match(Token token, TokenType type);
    bool matchv(Token token, TokenType type[], int n);
    bool tryRecover();
    Token peekToken();
    Token consumeToken(); 
    void rewindTo(Token checkpoint);  

    bool isValidLvalue(Subexpr *expr);
    DataType getDataType(Subexpr *operation, StatementBlock *scope);
    DataType parseDataType();

    Subexpr parseFunctionCall(StatementBlock *scope);
    Subexpr parseIdentifier(StatementBlock *scope);
    Subexpr* parsePrimary(StatementBlock *scope);
    Subexpr* parseSubexpr(int precedence, StatementBlock *scope); 
    ReturnNode* parseReturn(StatementBlock *scope);
    Node* parseDeclaration(StatementBlock *scope);
    Node* parseStatement(StatementBlock *scope);
    StatementBlock* parseStatementBlock(StatementBlock *scope);
    Node* parseIf(StatementBlock *scope);
    Node* parseWhile(StatementBlock *scope);
    Node* parseFor(StatementBlock *scope);


public:
        
    size_t errors;

    bool parseProgram();
    

    void init(Tokenizer *t){
        this->tokenizer = t;
        this->currentToken = t->nextToken();
        this->errors = 0;
        this->global.parent = 0;

    }
    
    bool parse(){
        while (currentToken.type != TOKEN_EOF){
            Node *stmt = this->parseStatement(&global);
            if (stmt){
                this->statements.push_back(stmt);
            }
        }
        
        fprintf(stdout, "[Parser] %llu errors generated.\n", errors);
        return errors == 0;
    }
};


void printParseTree(Node *const root, int depth = 0);
