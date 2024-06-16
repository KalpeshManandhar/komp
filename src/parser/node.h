#pragma once

#include <tokenizer/token.h>

/*
statement: expr;
expr : assignment 
assignment : lvalue = rvalue

lvalue : identifier
rvalue : subexpression | primary
primary : identifier | literal

*/


struct Node{
    int tag;
};

struct Lvalue: public Node{
    Token leaf;
};

struct Rvalue: public Node{
    Node *subexpr;
};


struct Assignment: public Node{
    Lvalue *left;
    Rvalue *right;
};

struct Primary: public Node{
    Token leaf;
};

struct Declaration: public Node{
    
};

