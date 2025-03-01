#pragma once

#include <tokenizer/token.h>
#include "symbol-table.h"
#include "datatype.h"
#include <vector>




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
struct InitializerList;


struct Subexpr: public Node{
    enum SubTag{
        SUBEXPR_RECURSE_PARENTHESIS = 1,
        SUBEXPR_BINARY_OP,
        SUBEXPR_LEAF,
        SUBEXPR_UNARY,
        SUBEXPR_FUNCTION_CALL,
        SUBEXPR_CAST,
        SUBEXPR_INITIALIZER_LIST,
    }subtag;
    
    DataType type;

    union{
        // binary: left op right
        struct {        
            Subexpr *left; 
            Token op; 
            Subexpr *right; 
        }binary;
        
        // leaf 
        Token leaf;

        // parenthesis: (inside)
        Subexpr *inside;
        
        // unary: unaryOp unarySubexpr
        struct {
            Token op;
            Subexpr *expr;
        }unary;
        
        // cast 
        struct {
            DataType to;
            Subexpr* expr;
        }cast;
        

        InitializerList *initList;        
        FunctionCall *functionCall;        
        
    };
};


struct InitializerList {
    std::vector <Subexpr*> values;
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
    size_t size;
    size_t alignment;

    struct MemberInfo{
        DataType type;
        Token memberName;
        size_t offset;
    };

    SymbolTableOrdered<MemberInfo> members;
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
    SymbolTableOrdered<DataType> symbols;
    SymbolTableOrdered<Struct> structs;
    SymbolTable<TypedefInfo> typedefs;
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

    StatementBlock * findTypedef(Splice name) {
        StatementBlock *currentScope = this;
        while(currentScope){
            if (currentScope->typedefs.existKey(name)){
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



