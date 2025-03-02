#pragma once

#include "node.h"
#include "datatype.h"
#include "mir-datatype.h"
#include <arena/arena.h>

#include "label.h"

struct MIR_Primitive {
    enum PrimitiveType{
        PRIM_IF,
        PRIM_LOOP,
        
        PRIM_STACK_ALLOC,
        PRIM_STACK_FREE,
        
        PRIM_RETURN,
        PRIM_JUMP,
        PRIM_EXPR,
        PRIM_SCOPE,

        PRIM_LABEL,
        
        // only for intermediate
        PRIM_UNSPECIFIED,
        PRIM_COUNT,
    }ptag;
};




struct MIR_FunctionCall;



struct MIR_Expr : public MIR_Primitive{
    enum ExprType{
        EXPR_ADDRESSOF,
        EXPR_LOAD,
        EXPR_INDEX,
        EXPR_LEAF,

        EXPR_LOAD_ADDRESS,
        EXPR_LOAD_IMMEDIATE,
        
        EXPR_STORE,

        EXPR_CALL,
        
        EXPR_CAST,
        EXPR_BINARY,
        EXPR_UNARY,

    }tag;
    
    
    enum class BinaryOp{
        EXPR_IADD,
        EXPR_ISUB,
        EXPR_IMUL,
        EXPR_IDIV,
        EXPR_IMOD,
        EXPR_UADD,
        EXPR_USUB,
        EXPR_UMUL,
        EXPR_UDIV,
        EXPR_UMOD,
        
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
        EXPR_FNEGATE,
        EXPR_IBITWISE_NOT,
        EXPR_LOGICAL_NOT,
    };
    
    enum class LoadType{
        EXPR_ILOAD, // integer load
        EXPR_FLOAD, // float load
        EXPR_MEMLOAD, // general load
    };
    
    // the type of the node output
    DataType type;
    MIR_Datatype _type;
    
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
            MIR_Expr *left;
            MIR_Expr *right;
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
            MIR_Datatype _from;
            MIR_Datatype _to;
            MIR_Expr *expr;
        }cast;
        
        

        /*
            To resolve a variable name into its address.
            Only used for leaf nodes, ie variable name tokens
            of     : the leaf node containing the variable name token
            offset : the offset from the frame pointer filled in when the node is resolved during code generation
        */
        struct {
            Splice symbol;
            // filled in on generating node
            // int64_t offset;
        }addressOf;


        /*
            A leaf node, containing a variable name token.
            val : the variable name token
        */
        struct {
            Splice val;
        }leaf;

        /*
            Load a value at a given address.
            Used for variable accesses, pointer dereferences. 
            base   : The base address given. Can either be an address node or an address value in a register.
            offset : A compile time constant offset from the base address. Used for struct member accesses.
            size   : Number of bytes from the given address to load.
        */
        struct {
            MIR_Expr *base;
            // offset used for structs, for indexing use index
            int64_t offset;
            size_t size;
            LoadType type;
        }load;

        /*
            To add a given offset to a given address. 
            Used for runtime indexing/variable offsets from given base address. Eg. a[i] 
            base   : The base address given, loaded in a register.
            index  : The variable index/offset which is calculated at runtime.
            size   : Size of the type to be indexed, to be multiplied to get the correct offset. 
        */
        struct {
            MIR_Expr *base;
            MIR_Expr *index;
            size_t size;
        }index;


        /*
            Binary expressions.
            left   : The left operand, usually loaded in register. 
            right  : The right operand, usually loaded in register. 
            op     : The operator.
        */
        struct {        
            MIR_Expr *left; 
            MIR_Expr *right; 
            BinaryOp op;
            size_t size;
        }binary;
        

        /*
            A leaf node with the immediate value token.
            val    : The immediate value. 
        */
        struct {
            Splice val;
        }immediate;
        

        /*
            To load an address into a register. 
            (AddressOf node only resolves the address of the variable.)
            base    : The base address to load.
            offset  : A compile time constant offset used for struct members.
        */
        struct {
            MIR_Expr *base;
            int64_t offset;
        }loadAddress;

        /*
            Unary expressions.
            op   : The operator.
            unarySubexpr : The operand.
        */
        struct {
            UnaryOp op;
            MIR_Expr *expr;
        }unary;
        
        /*
            Function call info: function name, arguments.
        */
        MIR_FunctionCall *functionCall;        
        
    };

};


struct MIR_FunctionCall {
    Splice funcName;
    std::vector <MIR_Expr*> arguments;
};



struct MIR_Scope : public MIR_Primitive{
    std::vector<MIR_Primitive*> statements;
    SymbolTableOrdered<MIR_Datatype> symbols;

    MIR_Scope* parent;  
    MIR_Primitive* extraInfo;
};


struct MIR_If : public MIR_Primitive{
    MIR_Expr* condition;
    MIR_If* next;
    MIR_Scope* scope;

    Label falseLabel;
    Label endLabel;
};

struct MIR_Loop : public MIR_Primitive{
    MIR_Expr* condition;
    MIR_Expr* update;
    MIR_Scope* scope;

    Label startLabel;
    Label updateLabel;
    Label endLabel;
};

struct MIR_Return : public MIR_Primitive{
    MIR_Expr* returnValue;
    Splice funcName;
};

struct MIR_Jump : public MIR_Primitive {
    Label jumpLabel;
};

struct MIR_Label: public MIR_Primitive {
    Label labelName;
};


struct MIR_Function : public MIR_Scope{
    MIR_Datatype returnType;
    Splice funcName; 
    bool isExtern;

    struct Parameter{
        MIR_Datatype type;
        Splice identifier;
    };
    std::vector<Parameter> parameters;

};






