#pragma once

#include <tokenizer/token.h>
#include "symbol-table.h"
#include "datatype.h"
#include <vector>


/*

TODO:
    function params into symbols table - done
    function parsing only in global - done
    += -= operators in assignment + assignments are binary expressions - done
    break/continue/return - done
    
    arrays - indexing works, declarations remain, initialization remain
    type checking - somewhat done
    structs - declaration done
    
    character literal tokenization
    

    Specific refactors:
        - implement an arena to reduce chances of memory leaks and easy allocs/frees - done
        - find a better way to call the type checking function. - done
        
        - probably change declaration initializers to be an assignment 






*/




/*
The grammar *currently* used in the parser. 
NOTE: 
    - Literals are surrounded with double quotes "" 
    - (x)? denotes 0 or 1 occurence of x
    - (x|y) denotes either x or y
    - (x)* denotes 0 or more occurences of x
    - (x)+ denotes 1 or more occurences of x


program: 
        (declaration
        | function_definition)+
        


statement: 
        subexpr ";" 
        | declaration ";" 
        | WHILE 
        | IF 
        | FOR


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
        "+" | "-" | "/" | "*" | "%" | "&" | "|" | "<<" | "=" | "+=" 
        | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "^=" | "!=" 
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
        type_modifiers ("int" | "float" | "double" | "char" | "struct") ("*")* 


type_modifiers:
        ("long" | "short" | "signed" | "unsigned")*


*/


struct Node{
    enum Tag{
        NODE_SUBEXPR,
        NODE_DECLARATION,
        NODE_IF_BLOCK,
        NODE_WHILE,
        NODE_FOR,
        NODE_STMT_BLOCK,
        NODE_RETURN,
        NODE_BREAK,
        NODE_CONTINUE,
        NODE_ERROR,
    };
    int tag;
};

static const char* NODE_TAG_STRINGS[] = {
    "SUBEXPR",
    "DECLARATION",
    "IF_BLOCK",
    "WHILE",
    "FOR",
    "STMT_BLOCK",
    "RETURN",
    "BREAK",
    "CONTINUE",
    "ERROR",

};

struct FunctionCall;


struct Subexpr: public Node{
    enum SubTag{
        SUBEXPR_RECURSE_PARENTHESIS = 1,
        SUBEXPR_BINARY_OP,
        SUBEXPR_LEAF,
        SUBEXPR_UNARY,
        SUBEXPR_FUNCTION_CALL,
    }subtag;
    
    union{
        // binary: left op right
        struct {        
            Subexpr *left; 
            Token op; 
            Subexpr *right; 
        };
        
        // leaf 
        Token leaf;

        // parenthesis: (inside)
        Subexpr *inside;
        
        // unary: unaryOp unarySubexpr
        struct {
            Token unaryOp;
            Subexpr *unarySubexpr;
        };
        
        FunctionCall *functionCall;        
        
    };
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

struct Struct: public Node{
    Token structName;
    bool defined;

    struct MemberInfo{
        DataType type;
        Token memberName;
    };

    SymbolTable<MemberInfo> members;
};


struct StatementBlock: public Node{
    enum BlockType{
        BLOCK_FUNCTION_BODY,
        BLOCK_WHILE,
        BLOCK_IF,
        BLOCK_FOR,
        BLOCK_UNNAMED,
    }subtag;

    union {
        Token funcName;
        Node *scope;
    };


    std::vector<Node *> statements;
    SymbolTable<DataType> symbols;
    SymbolTable<Struct> structs;
    StatementBlock *parent;


    StatementBlock* getParentFunction(){
        StatementBlock *currentScope = this;
        while (currentScope){
            if (currentScope->subtag == StatementBlock::BLOCK_FUNCTION_BODY){
                return currentScope;
            }
            currentScope = currentScope->parent;
        }
        return NULL;
    };
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


struct ReturnNode: public Node{
    Token returnToken;
    Subexpr *returnVal;
};

struct BreakNode: public Node{
    Token breakToken;
};

struct ContinueNode: public Node{
    Token continueToken;
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



