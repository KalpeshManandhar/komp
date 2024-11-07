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
        EXPR_LOAD_ADDRESS,
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

    
    // the type of the node output
    DataType type;
    
    union{
        /*
            Store value at a given address.
            Used for assignments: lvalue = rvalue
            left   : address of the lvalue; aka where to store the value
            right  : the value to be put
            offset : a compile time constant offset from the given address, used for struct member accesses 
            size   : how many bytes to store at given address
        */
        struct {
            Exp_Expr *left;
            Exp_Expr *right;
            int64_t offset;
            size_t size;
        }store;

        /*
            Cast from one type to another.
            Used for implicit as well as explicit type conversions.
            from   : type of the expression to cast 
            to     : type to cast to
            expr   : the actual expression 
        */
        struct {
            DataType from;
            DataType to;
            Exp_Expr *expr;
        }cast;
        
        

        /*
            To resolve a variable name into its address.
            Only used for leaf nodes, ie variable name tokens
            of     : the leaf node containing the variable name token
            offset : the offset from the frame pointer filled in when the node is resolved during code generation
        */
        struct {
            Exp_Expr *of;
            // filled in on generating node
            int64_t offset;
        }addressOf;


        /*
            A leaf node, containing a variable name token.
            val : the variable name token
        */
        struct {
            Token val;
        }leaf;


        /*
            Dereference a value at a given address.
            Used for variable accesses, pointer dereferences. 
            base   : The base address given. Can either be an address node or an address value in a register.
            offset : A compile time constant offset from the base address. Used for struct member accesses.
            size   : Number of bytes from the given address to load.
        */
        struct {
            Exp_Expr *base;
            // offset used for structs, for indexing use index
            int64_t offset;
            size_t size;
        }deref;


        /*
            To add a given offset to a given address. 
            Used for runtime indexing/variable offsets from given base address. Eg. a[i] 
            base   : The base address given, loaded in a register.
            index  : The variable index/offset which is calculated at runtime.
            size   : Size of the type to be indexed, to be multiplied to get the correct offset. 
        */
        struct {
            Exp_Expr *base;
            Exp_Expr *index;
            size_t size;
        }index;


        /*
            Binary expressions.
            left   : The left operand, usually loaded in register. 
            right  : The right operand, usually loaded in register. 
            op     : The operator.
        */
        struct {        
            Exp_Expr *left; 
            Exp_Expr *right; 
            BinaryOp op;

        }binary;
        

        /*
            A leaf node with the immediate value token.
            val    : The immediate value. 
        */
        struct {
            Token val;
        }immediate;
        

        /*
            To load an address into a register. 
            (AddressOf node only resolves the address of the variable.)
            base    : The base address to load.
            offset  : A compile time constant offset used for struct members.
        */
        struct {
            Exp_Expr *base;
            int64_t offset;
        }loadAddress;

        /*
            Unary expressions.
            op   : The operator.
            unarySubexpr : The operand.
        */
        struct {
            UnaryOp op;
            Exp_Expr *unarySubexpr;
        }unary;
        
        /*
            Function call info: function name, arguments.
        */
        Exp_FunctionCall *functionCall;        
        
    };

};


struct Exp_FunctionCall{
    Token funcName; 
    std::vector<Exp_Expr*> arguments; 
};