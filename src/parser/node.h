#pragma once

#include <tokenizer/token.h>

/*
statement: expr;
expr : assignment 
assignment : lvalue = rvalue

lvalue : identifier
rvalue : primary | subexpr
subexpr : primary operator subexpr | primary
primary : (subexpr) | identifier | literal

operator : + | - | / | * 
*/


struct Node{
    int tag;
};

struct Subexpr: public Node{
    enum Tag{
        SUBEXPR_RECURSE_PARENTHESIS,
        SUBEXPR_RECURSE_OP,
        SUBEXPR_LEAF,
    }tag;
    
    union{
        struct{        
            Subexpr *left; 
            Token op; 
            Subexpr *right; 
        };
        Token leaf;
        Subexpr *inside;
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
    
};

