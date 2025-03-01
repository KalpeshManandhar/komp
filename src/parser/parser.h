#pragma once

#include <IR/ir.h>
#include <tokenizer/tokenizer.h>
#include <arena/arena.h>


struct Parser{
    Tokenizer *tokenizer;
    Token currentToken;
    
    Arena *arena;

    // to check if any parsing went into error: set to true whenever the parser tries to recover from an error
    bool didError;

    AST *ir;
    

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
    bool isExprStart();
    bool isStartOfType(StatementBlock* scope);
    
    // type/context checking
    bool canBeConverted(Subexpr *from, DataType fromType, DataType toType, StatementBlock* scope);
    DataType checkSubexprType(Subexpr *operation, StatementBlock *scope);
    bool checkContext(Node *n, StatementBlock *scope);
    bool canResolveToConstant(Subexpr *s, StatementBlock *scope);
    bool isTypeDefined(DataType d,  StatementBlock* scope);
    bool isValidLvalue(DataType leftType, Subexpr* leftOperand);


    // parsing
    Node* parseDeclaration(StatementBlock *scope);
    Node* parseStatement(StatementBlock *scope);
    Node* parseIf(StatementBlock *scope);
    Node* parseWhile(StatementBlock *scope);
    Node* parseFor(StatementBlock *scope);
    DataType parseBaseDataType(StatementBlock *scope);
    DataType parsePointerType(StatementBlock *scope, DataType baseType);
    DataType parseArrayType(StatementBlock *scope, DataType baseType);
    DataType parseDataType(StatementBlock *scope);

    void parseTypedef(StatementBlock *scope);
    Token parseStructDefinition(StatementBlock *scope);
    Subexpr* parsePrimary(StatementBlock *scope);
    Subexpr* parseSubexpr(int precedence, StatementBlock *scope); 
    ReturnNode* parseReturn(StatementBlock *scope);
    ContinueNode* parseContinue(StatementBlock *scope);
    BreakNode* parseBreak(StatementBlock *scope);
    StatementBlock* parseStatementBlock(StatementBlock *scope);

public:
        
    size_t errors;

    AST * parseProgram();
    

    void init(Tokenizer *t, Arena *arena){
        this->tokenizer = t;
        this->currentToken = t->nextToken();
        this->errors = 0;

        this->ir = new AST;

        this->ir->global.parent = 0;
        this->ir->global.tag = Node::NODE_STMT_BLOCK;
        this->ir->global.subtag = StatementBlock::BLOCK_UNNAMED;

        this->arena = arena;
    }
    
    AST *parse(){

        while (currentToken.type != TOKEN_EOF){
            Node *stmt = this->parseStatement(&ir->global);
            if (stmt){
                this->ir->global.statements.push_back(stmt);
                arena->createFrame();
                checkContext(stmt, &ir->global);
                arena->destroyFrame();
            }
        }
        
        for (auto &foo: ir->functions.entries){
            arena->createFrame();
            checkContext(foo.second.info.block, &ir->global);
            arena->destroyFrame();
        }

        
        fprintf(stdout, "[Parser] %" PRIu64 " errors generated.\n", errors);
        return (errors == 0)? ir : NULL;
    }
};
