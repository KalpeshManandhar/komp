#pragma once

#include <tokenizer/token.h>
#include <vector>

/*
statement: expr;
statement: (assignment | declaration);

assignment : lvalue = rvalue
lvalue : identifier
rvalue : subexpr
subexpr : primary operator subexpr | primary
primary : (subexpr) | unary | identifier | literal 
unary   : (-|+|*|!|~)? primary
operator : + | - | / | * 


declaration: type identifier

if_     : if (condition) {(statement)*}


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
    Token identifier;
};

struct IfNode: public Node{
    Subexpr *condition;
    IfNode *nextIf;

    enum IfNodeType{
        IF_NODE = 0, // 'if' and 'else if' blocks with conditions
        ELSE_NODE,   // 'else' block without condition
    }subtag;
    std::vector<Node *> statements;

};



struct WhileNode: public Node{
    Subexpr *condition;
    std::vector<Node *> statements;
};

struct ForNode: public Node{
    Subexpr *exitCondition;
    Declaration *init;
    Assignment  *update;
    std::vector<Node *> statements;
};


