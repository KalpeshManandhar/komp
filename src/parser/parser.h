#pragma once

#include <IR/ir.h>
#include <tokenizer/tokenizer.h>


struct Parser{
    Tokenizer *tokenizer;
    Token currentToken;
    

    // to check if any parsing went into error: set to true whenever the parser tries to recover from an error
    bool didError;

    IR *ir;
    

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
    bool isExprStart();
    StatementBlock* findStructDeclaration(Token structName, StatementBlock *scope);
    
    // type checking
    bool canBeConverted(Subexpr *from, DataType fromType, DataType toType);
    DataType checkSubexprType(Subexpr *operation, StatementBlock *scope);
    bool checkContext(Node *n, StatementBlock *scope);

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
    ContinueNode* parseContinue(StatementBlock *scope);
    BreakNode* parseBreak(StatementBlock *scope);
    StatementBlock* parseStatementBlock(StatementBlock *scope);


public:
        
    size_t errors;

    IR * parseProgram();
    

    void init(Tokenizer *t){
        this->tokenizer = t;
        this->currentToken = t->nextToken();
        this->errors = 0;

        this->ir = new IR;

        this->ir->global.parent = 0;
        this->ir->global.tag = Node::NODE_STMT_BLOCK;
        this->ir->global.subtag = StatementBlock::BLOCK_UNNAMED;
    }
    
    IR *parse(){
        while (currentToken.type != TOKEN_EOF){
            Node *stmt = this->parseStatement(&ir->global);
            if (stmt){
                this->ir->global.statements.push_back(stmt);
                checkContext(stmt, &ir->global);
            }
        }
        
        for (auto &foo: ir->functions.entries){
            checkContext(foo.second.info.block, &ir->global);
        }

        
        fprintf(stdout, "[Parser] %llu errors generated.\n", errors);
        return (errors == 0)? ir : NULL;
    }
};


void printParseTree(Node *const root, int depth = 0);
