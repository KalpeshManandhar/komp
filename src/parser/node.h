#pragma once

#include <tokenizer/token.h>
#include "symbol-table.h"
#include <vector>



/*
statement: expr;
statement: (assignment | declaration);

assignment : lvalue = rvalue
lvalue : identifier
rvalue : subexpr
subexpr : primary operator subexpr | primary
primary : '('subexpr')' | unary | identifier | literal 
unary   : (-|+|*|!|~)? primary
operator : + | - | / | * 


declaration: type identifier (= rvalue) (, identifier (= rvalue))*  

if_     : if (condition) {
            (statement)*
          } 
          (else if_|{
            (statement)*
          })?

while_  : while (condition) {
            (statement | break; | continue;)*
          }

for_    : for (declaration; subexpr; assignment){
            (statement | break; | continue;)*
          }



*/


struct Node{
    enum Tag{
        NODE_SUBEXPR,
        NODE_LVALUE,
        NODE_RVALUE,
        NODE_ASSIGNMENT,
        NODE_DECLARATION,
        NODE_IF_BLOCK,
        NODE_WHILE,
        NODE_FOR,
        NODE_STMT_BLOCK,
    };
    int tag;
};

static const char* NODE_TAG_STRINGS[] = {
    "SUBEXPR",
    "LVALUE",
    "RVALUE",
    "ASSIGNMENT",
    "DECLARATION",
    "IF_BLOCK",
    "WHILE",
    "FOR",
    "STMT_BLOCK",
};

struct Subexpr: public Node{
    enum SubTag{
        SUBEXPR_RECURSE_PARENTHESIS = 1,
        SUBEXPR_RECURSE_OP,
        SUBEXPR_LEAF,
        SUBEXPR_UNARY,
    }subtag;
    
    union{
        struct{        
            Subexpr *left; 
            Token op; 
            Subexpr *right; 
        };

        Token leaf;
        Subexpr *inside;
        
        struct {
            Token unaryOp;
            Subexpr *unarySubexpr;
        };
    };
};

struct Lvalue: public Node{
    Token leaf;
};

struct Rvalue: public Node{
    Subexpr *subexpr;
};


struct Assignment: public Node{
    Lvalue *left;
    Rvalue *right;
};


struct Declaration: public Node{
    Token type;
    // info for a single variable
    struct DeclInfo{
        Token identifier;
        Subexpr *initValue;
    };
    std::vector<DeclInfo> decln;
};

struct StatementBlock: public Node{
    std::vector<Node *> statements;
    SymbolTable symbols;
    StatementBlock *parent;
};


struct IfNode: public Node{
    Subexpr *condition;
    IfNode *nextIf;

    enum IfNodeType{
        IF_NODE = 0, // 'if' and 'else if' blocks with conditions
        ELSE_NODE,   // 'else' block without condition
    }subtag;
    StatementBlock *block;
};



struct WhileNode: public Node{
    Subexpr *condition;
    StatementBlock *block;
};

struct ForNode: public Node{
    Subexpr *exitCondition;
    Assignment *init;
    Assignment *update;
    StatementBlock *block;
};



