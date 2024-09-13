#pragma once

#include <tokenizer/token.h>
#include "symbol-table.h"
#include <vector>


/*

TODO:
    function params into symbols table - done
    break/continue/return
    character literal tokenization
    += -= operators in assignment
    structs
    arrays
    function parsing only in global


*/



/*
The grammar *currently* used in the parser. 
NOTE: 
    - Literals are surrounded with double quotes "" 
    - (x)? denotes 0 or 1 occurence of x
    - (x|y) denotes either x or y
    - (x)* denotes 0 or more occurences of x


program: 
        declaration
        | function_definition
        


statement: 
        assignment ";" 
        | declaration ";" 
        | WHILE 
        | IF 
        | FOR

assignment : 
        lvalue "=" rvalue

lvalue : 
        identifier

rvalue : 
        subexpr

subexpr : 
        primary binary_op subexpr 
        | primary

primary : 
        "("subexpr")" 
        | unary_op primary
        | identifier 
        | literal 
        | func_call

func_call:
        func_identifier "(" subexpr ( "," subexpr )* ")"

unary_op :  
        "-" | "+" | "*" | "!" | "~" | "++" | "--" | "&"

binary_op : 
        "+" | "-" | "/" | "*" | "%" | "&" | "|" | "<<" 
        | ">>" | "&&" | "||" | "==" | "!=" | ">" | "<" | ">=" | "<="

declaration: 
        data_type identifier ( "=" subexpr )? ("," identifier ( "=" subexpr )? )*  

statement_block:
        "{"
            ( statement )*
        "}"

IF : 
        "if" "("condition")" statement_block
        ( "else" ( IF | statement_block ) )?

WHILE : 
        "while" "("subexpr")" statement_block

FOR : 
        "for" "("assignment";" subexpr";" assignment")" statement_block

function_definition:
        data_type identifier "("data_type identifier ( "," data_type identifier )* ) statement_block

data_type:
        ("int" | "float" | "double" | "short" | "long" | "char" | "struct") ("*")* 


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
        NODE_RETURN,
        NODE_ERROR,
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
    "RETURN",
    "ERROR",

};

struct FunctionCall;


struct Subexpr: public Node{
    enum SubTag{
        SUBEXPR_RECURSE_PARENTHESIS = 1,
        SUBEXPR_RECURSE_OP,
        SUBEXPR_LEAF,
        SUBEXPR_UNARY,
        SUBEXPR_FUNCTION_CALL,
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


        FunctionCall *functionCall;        
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


struct ReturnNode: public Node{
    Subexpr *returnVal;
};


struct Declaration: public Node{
    DataType type;
    // info for a single variable
    struct DeclInfo{
        Token identifier;
        Subexpr *initValue;
    };
    std::vector<DeclInfo> decln;
};


struct StatementBlock: public Node{
    std::vector<Node *> statements;
    SymbolTable<DataType> symbols;
    StatementBlock *parent;
};

struct Function: public Node{
    DataType returnType;
    Token funcName;

    struct Parameter{
        DataType type;
        Token identifier;
    };
    std::vector<Parameter> parameters;

    StatementBlock *block;
};


struct FunctionCall{
    Token funcName; 
    std::vector<Subexpr*> arguments; 
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
    Subexpr *init;
    Subexpr *update;
    StatementBlock *block;
};



