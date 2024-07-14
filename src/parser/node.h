#pragma once

#include <tokenizer/token.h>

/*
statement: expr;
statement: (assignment | declaration);

assignment : lvalue = rvalue
lvalue : identifier
rvalue : subexpr
subexpr : primary operator subexpr | primary
primary : (subexpr) | identifier | literal
operator : + | - | / | * 


declaration: type identifier (= rvalue)*


*/


struct Node{
    enum Tag{
        NODE_SUBEXPR,
        NODE_LVALUE,
        NODE_RVALUE,
        NODE_ASSIGNMENT,
    };
    int tag;
};

static const char* NODE_TAG_STRINGS[] = {
    "SUBEXPR",
    "LVALUE",
    "RVALUE",
    "ASSIGNMENT",
};

struct Subexpr: public Node{
    enum SubTag{
        SUBEXPR_RECURSE_PARENTHESIS = 1,
        SUBEXPR_RECURSE_OP,
        SUBEXPR_LEAF,
    }subtag;
    
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
    Token type;
};

