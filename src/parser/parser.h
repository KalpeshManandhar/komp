#pragma once

#include "node.h"
#include <tokenizer/tokenizer.h>
#include <vector>


struct Parser{
    Tokenizer *tokenizer;
    Token currentToken;
    

    // to check if any parsing went into error: set to true whenever the parser tries to recover from an error
    bool didError;


    StatementBlock global;
    SymbolTable<Function> functions;
    std::vector<Node *> statements;

    // token/state management
    bool expect(TokenType type);
    bool match(TokenType type);
    bool matchv(TokenType type[], int n);
    bool match(Token token, TokenType type);
    bool matchv(Token token, TokenType type[], int n);
    bool tryRecover(TokenType extraDelimiter = TOKEN_EOF);
    Token peekToken();
    Token consumeToken(); 
    void rewindTo(Token checkpoint);  
    

    // checks
    Token getSubexprToken(Subexpr *expr);
    bool isValidLvalue(Subexpr *expr);
    StatementBlock* findStructDeclaration(Token structName, StatementBlock *scope);
    
    // type checking
    bool canBeConverted(Subexpr *from, DataType fromType, DataType toType, StatementBlock *scope);
    DataType checkContextAndType(Subexpr *operation, StatementBlock *scope);


    // parsing
    Node* parseDeclaration(StatementBlock *scope);
    Node* parseStatement(StatementBlock *scope);
    Node* parseIf(StatementBlock *scope);
    Node* parseWhile(StatementBlock *scope);
    Node* parseFor(StatementBlock *scope);
    DataType parseDataType(StatementBlock *scope);
    Token parseStructDefinition(StatementBlock *scope);
    Subexpr* parsePrimary(StatementBlock *scope);
    Subexpr* parseSubexpr(int precedence, StatementBlock *scope); 
    ReturnNode* parseReturn(StatementBlock *scope);
    StatementBlock* parseStatementBlock(StatementBlock *scope);


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
