#pragma once

#include <tokenizer/token.h>

/*
statement: expr;
statement: (assignment | declaration);

assignment : lvalue = rvalue
lvalue : identifier
rvalue : subexpr
subexpr : primary operator subexpr | primary
primary : (subexpr) | unary 
unary   : (-|+|*|!|~| ) (identifier | literal)
operator : + | - | / | * 


declaration: type identifier (= rvalue)


*/


struct Node{
    enum Tag{
        NODE_SUBEXPR,
        NODE_LVALUE,
        NODE_RVALUE,
        NODE_ASSIGNMENT,
        NODE_DECLARATION,
    };
    int tag;
};

static const char* NODE_TAG_STRINGS[] = {
    "SUBEXPR",
    "LVALUE",
    "RVALUE",
    "ASSIGNMENT",
    "DECLARATION",
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

