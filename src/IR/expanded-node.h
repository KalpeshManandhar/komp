#pragma once

#include "node.h"
#include <arena/arena.h>


struct Exp_FunctionCall;

struct Exp_Expr{
    enum ExprType{
        EXPR_ADDRESSOF,
        EXPR_DEREF,
        EXPR_INDEX,
        EXPR_LEAF,

        EXPR_LOAD,
        EXPR_LOAD_IMMEDIATE,
        
        EXPR_STORE,
        
        EXPR_CAST,
        EXPR_BINARY,
        EXPR_UNARY,

        EXPR_FUNCTION_CALL,
    }tag;
    
    
    enum class BinaryOp{
        EXPR_IADD,
        EXPR_ISUB,
        EXPR_IMUL,
        EXPR_IDIV,
        EXPR_UADD,
        EXPR_USUB,
        EXPR_UMUL,
        EXPR_UDIV,

        EXPR_FADD,
        EXPR_FSUB,
        EXPR_FMUL,
        EXPR_FDIV,

        
        EXPR_IBITWISE_AND,
        EXPR_IBITWISE_OR,
        EXPR_IBITWISE_XOR,
        
        EXPR_LOGICAL_AND,
        EXPR_LOGICAL_OR,
        
        EXPR_IBITWISE_LSHIFT,
        EXPR_IBITWISE_RSHIFT,

        EXPR_ICOMPARE_LT,
        EXPR_ICOMPARE_GT,
        EXPR_ICOMPARE_LE,
        EXPR_ICOMPARE_GE,
        EXPR_ICOMPARE_EQ,
        EXPR_ICOMPARE_NEQ,

        EXPR_FCOMPARE_LT,
        EXPR_FCOMPARE_GT,
        EXPR_FCOMPARE_LE,
        EXPR_FCOMPARE_GE,
        EXPR_FCOMPARE_EQ,
        EXPR_FCOMPARE_NEQ,
    };

    enum class UnaryOp{
        EXPR_INEGATE,
        EXPR_IBITWISE_NOT,
        EXPR_LOGICAL_NOT,
    };

    

    DataType type;
    
    union{
        struct StoreInfo{
            Exp_Expr *left;
            Exp_Expr *right;
            int64_t offset;
            size_t size;
        }store;

        struct Cast{
            DataType from;
            DataType to;
            Exp_Expr *expr;
        }cast;
        

        struct AddressOfInfo{
            Exp_Expr *of;

            int64_t offset;
        }addressOf;

        struct Leaf{
            Token val;
        }leaf;

        struct Deref{
            Exp_Expr *base;
            // offset used for structs, for indexing use offset
            int64_t offset;
            size_t size;
        }deref;

        struct IndexInfo{
            Exp_Expr *base;
            Exp_Expr *index;
            size_t size;
        }index;

        // binary: left op right
        struct BinaryExpr{        
            Exp_Expr *left; 
            Exp_Expr *right; 
            BinaryOp op;

        }binary;
        
        // leaf 
        struct Immediate{
            Token val;
        }immediate;

        // unary: unaryOp unarySubexpr
        struct UnaryExpr{
            UnaryOp op;
            Exp_Expr *unarySubexpr;
        }unary;
        
        Exp_FunctionCall *functionCall;        
        
    };

};


struct Exp_FunctionCall{
    Token funcName; 
    std::vector<Exp_Expr*> arguments; 
};