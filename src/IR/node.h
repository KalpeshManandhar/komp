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
    
    DataType type;

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
    // info for a single variable
    struct DeclInfo{
        DataType type;
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

    StatementBlock* findStructDeclaration(Token structName){
        StatementBlock *currentScope = this;
        while (currentScope){
            if (currentScope->structs.existKey(structName.string)){
                if (currentScope->structs.getInfo(structName.string).info.defined){
                    return currentScope;
                }
            }

            currentScope = currentScope->parent;
        }
        return NULL;
    }


    StatementBlock * findVarDeclaration(Splice name) {
        StatementBlock *currentScope = this;
        while(currentScope){
            if (currentScope->symbols.existKey(name)){
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




static DataType getResultantType(DataType left, DataType right, const Subexpr *expr){
    
    // diff level of indirection
    if (left.indirectionLevel() != right.indirectionLevel()){
        // pointer arithmetic 
        // (ptr + int)/(ptr - int)/(ptr += int)/(ptr -= int)
        if (left.indirectionLevel() > 0 && (_match(right.type, TOKEN_INT) || _match(right.type, TOKEN_CHAR))){ 
            if (_match(expr->op, TOKEN_PLUS) || _match(expr->op, TOKEN_MINUS) 
            || _match(expr->op, TOKEN_PLUS_ASSIGN) || _match(expr->op, TOKEN_MINUS_ASSIGN) ){
                return left;
            }
            else if (_match(expr->op, TOKEN_ASSIGNMENT)){
                return left;
            }
        }
        // (int + ptr) 
        else if (right.indirectionLevel() > 0 && (_match(left.type, TOKEN_INT) || _match(left.type, TOKEN_CHAR))){
            if (_match(expr->op, TOKEN_PLUS)){
                return right;
            }
            if (_match(expr->op, TOKEN_ASSIGNMENT)){
                return right;
            }
        }
        // both pointers of different level of indirection
        // can only assign but log a warning
        else if (left.indirectionLevel() > 0 && right.indirectionLevel() > 0){
            if (_match(expr->op, TOKEN_ASSIGNMENT)){
                return left;
            }
        }

    }
    // same level of indirection
    else{
        // both are pointers
        if (left.indirectionLevel() > 0){
            // can assign, but log error if of different types
            if (_match(expr->op, TOKEN_ASSIGNMENT)){
                return left;
            }

            if (left == right){
                // ptr difference: (ptr - ptr)
                if (_match(expr->op, TOKEN_MINUS)){
                    return DataTypes::Long_Long;
                }
            }
        }
        // same type but not pointers
        else if (left == right){
            // assignment is defined for all operands of the same type  
            if (_match(expr->op, TOKEN_ASSIGNMENT)){
                return left;
            }
            
            // all other primary operands work with all other operators 
            // (floating point exceptions have been handled at the start)
            if (left.tag == DataType::TAG_PRIMARY){
                return left;
            }

            // if struct, only assignment between same structs is allowed 
            if (_match(left.type, TOKEN_STRUCT)){
                if (_match(expr->op, TOKEN_ASSIGNMENT) && compare(left.structName.string, right.structName.string)){
                    return left;
                }
            }
        }
        
        // different types of primary types
        else if (left.tag == DataType::TAG_PRIMARY && right.tag == DataType::TAG_PRIMARY){
            
            // for valid assignment operations, the resultant type is the type of the left operand
            if (_matchv(expr->op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP))){
                return left;
            }

            // if any is double or float, convert to that
            if (_match(left.type, TOKEN_DOUBLE) || _match(right.type, TOKEN_DOUBLE)){
                return DataTypes::Double;
            }   
            else if (_match(left.type, TOKEN_FLOAT) || _match(right.type, TOKEN_FLOAT)){
                return DataTypes::Float;
            }   

            // same signedness, conversion to greater conversion rank
            else if ((left.isSet(DataType::Specifiers::SIGNED) && right.isSet(DataType::Specifiers::SIGNED))
                    || (left.isSet(DataType::Specifiers::UNSIGNED) && right.isSet(DataType::Specifiers::UNSIGNED))){
                
                if (getIntegerConversionRank(left) > getIntegerConversionRank(right)){
                    return left;
                }
                
                return right;
                
            }   
            
            // different signedness
            else {
                // if unsigned has higher or equal rank, then unsigned
                // else, if signed can accomodate full range of unsigned then convert to signed, 
                // else convert to unsigned counterpart of the signed types
                
                auto signedUnsignedConversion = [&](DataType unsignedType, DataType signedType){
                    if (getIntegerConversionRank(unsignedType) >= getIntegerConversionRank(signedType)){
                        return unsignedType;
                    }
                    else if (signedType.isSet(DataType::Specifiers::LONG_LONG)){
                        return signedType;
                    }
                    else if (signedType.isSet(DataType::Specifiers::LONG)){
                        // signed long cannot accomodate unsigned int
                        if (!unsignedType.isSet(DataType::Specifiers::LONG)){
                            return signedType;
                        }
                    }
                    else if (!signedType.isSet(DataType::Specifiers::SHORT)){
                        // signed int can accomodate unsigned short and char
                        if (unsignedType.isSet(DataType::Specifiers::SHORT) || _match(unsignedType.type, TOKEN_CHAR)){
                            return signedType;
                        }
                    }
                    else if (signedType.isSet(DataType::Specifiers::SHORT)){
                        // signed short can accomodate unsigneds char
                        if (_match(unsignedType.type, TOKEN_CHAR)){
                            return signedType;
                        }
                    }
                    // unsigned counterpart of the signed types
                    signedType.flags |= DataType::Specifiers::UNSIGNED;
                    signedType.flags ^= DataType::Specifiers::SIGNED;
                    return signedType;
                };
                
                
                if (left.isSet(DataType::Specifiers::UNSIGNED)){
                    return signedUnsignedConversion(left, right);
                }
                else {
                    return signedUnsignedConversion(right, left);
                }
            }
            
        
        }

    }

    return DataTypes::Int;
};
