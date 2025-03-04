#pragma once

#include "str.h"

struct Token{
    int type;
    Splice string;
    int charNo;
    int lineNo;
};

enum TokenType{
    TOKEN_ERROR,
    TOKEN_IDENTIFIER,
    
    // keywords
    TOKEN_KEYWORDS_START,
    
    TOKEN_AUTO,
    TOKEN_BREAK,
    TOKEN_CASE,
    TOKEN_CHAR,
    TOKEN_CONST,
    TOKEN_CONTINUE,
    TOKEN_DEFAULT,
    TOKEN_DO,
    TOKEN_DOUBLE,
    TOKEN_ELSE,
    TOKEN_ENUM,
    TOKEN_EXTERN,
    TOKEN_FLOAT,
    TOKEN_FOR,
    TOKEN_GOTO,
    TOKEN_IF,
    TOKEN_INLINE,
    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_REGISTER,
    TOKEN_RESTRICT,
    TOKEN_RETURN,
    TOKEN_SHORT,
    TOKEN_SIGNED,
    TOKEN_SIZEOF,
    TOKEN_STATIC,
    TOKEN_STRUCT,
    TOKEN_SWITCH,
    TOKEN_TYPEDEF,
    TOKEN_TYPEOF,
    TOKEN_TYPEOF_UNQUAL,
    TOKEN_UNION,
    TOKEN_UNSIGNED,
    TOKEN_VOID,
    TOKEN_VOLATILE,
    TOKEN_WHILE,

    // string literals
    TOKEN_STRING_LITERAL,
    TOKEN_CHARACTER_LITERAL,

    TOKEN_NUMERICS_START,

    // Numeric tokens
    TOKEN_NUMERIC_HEX,
    TOKEN_NUMERIC_BIN,
    TOKEN_NUMERIC_DEC,
    TOKEN_NUMERIC_OCT,
    TOKEN_NUMERIC_DOUBLE,
    TOKEN_NUMERIC_FLOAT,
    

    // to be used to return tokens based on states of the dfa
    TOKEN_PUNCTUATORS_START,

    TOKEN_SQUARE_OPEN,
    TOKEN_SQUARE_CLOSE,
    TOKEN_CURLY_OPEN,
    TOKEN_CURLY_CLOSE,
    TOKEN_PARENTHESIS_OPEN,
    TOKEN_PARENTHESIS_CLOSE,

    TOKEN_DOT, // .
    TOKEN_ARROW, // ->
    
    TOKEN_PLUS_PLUS, // ++
    TOKEN_MINUS_MINUS, // --
    
    
    // bitwise operators 
    TOKEN_AMPERSAND, // represents both AND and ADDRESS
    TOKEN_BITWISE_OR, // |
    TOKEN_BITWISE_NOT, // ~
    TOKEN_BITWISE_XOR,
    TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT,

    // arithmetic operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR, // represents deferencing and multiply
    TOKEN_SLASH,
    TOKEN_MODULO,

    // logical operators
    TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN,
    
    TOKEN_LESS_EQUALS,
    TOKEN_GREATER_EQUALS,
    TOKEN_EQUALITY_CHECK, // ==
    TOKEN_NOT_EQUALS,

    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_LOGICAL_NOT,
    
    // assignment 
    TOKEN_ASSIGNMENT, // =
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN,
    TOKEN_BITWISE_AND_ASSIGN,
    TOKEN_BITWISE_OR_ASSIGN,
    TOKEN_BITWISE_XOR_ASSIGN,
    TOKEN_MODULO_ASSIGN,


    // punctuators?
    TOKEN_QUESTION_MARK,
    TOKEN_COLON,
    TOKEN_SEMI_COLON,
    TOKEN_COMMA,
    TOKEN_HASH,
    
    TOKEN_DOT_DOT_DOT, // ...
    
    // postfix
    TOKEN_PLUS_PLUS_POSTFIX, // ++
    TOKEN_MINUS_MINUS_POSTFIX, // --
    
    TOKEN_EOF,



    
    TOKEN_COUNT
};

static const char *TOKEN_TYPE_STRING[] = {
    "TOKEN_ERROR",
    "TOKEN_IDENTIFIER",



    "TOKEN_KEYWORDS_START",
    
    "TOKEN_AUTO",
    "TOKEN_BREAK",
    "TOKEN_CASE",
    "TOKEN_CHAR",
    "TOKEN_CONST",
    "TOKEN_CONTINUE",
    "TOKEN_DEFAULT",
    "TOKEN_DO",
    "TOKEN_DOUBLE",
    "TOKEN_ELSE",
    "TOKEN_ENUM",
    "TOKEN_EXTERN",
    "TOKEN_FLOAT",
    "TOKEN_FOR",
    "TOKEN_GOTO",
    "TOKEN_IF",
    "TOKEN_INLINE",
    "TOKEN_INT",
    "TOKEN_LONG",
    "TOKEN_REGISTER",
    "TOKEN_RESTRICT",
    "TOKEN_RETURN",
    "TOKEN_SHORT",
    "TOKEN_SIGNED",
    "TOKEN_SIZEOF",
    "TOKEN_STATIC",
    "TOKEN_STRUCT",
    "TOKEN_SWITCH",
    "TOKEN_TYPEDEF",
    "TOKEN_TYPEOF",
    "TOKEN_TYPEOF_UNQUAL",
    "TOKEN_UNION",
    "TOKEN_UNSIGNED",
    "TOKEN_VOID",
    "TOKEN_VOLATILE",
    "TOKEN_WHILE",

    // string literals
    "TOKEN_STRING_LITERAL",
    "TOKEN_CHARACTER_LITERAL",

    "TOKEN_NUMERICS_START",

    // Numeric tokens
    "TOKEN_NUMERIC_HEX",
    "TOKEN_NUMERIC_BIN",
    "TOKEN_NUMERIC_DEC",
    "TOKEN_NUMERIC_OCT",
    "TOKEN_NUMERIC_DOUBLE",
    "TOKEN_NUMERIC_FLOAT",
    

    // to be used to return tokens based on states of the dfa
    "TOKEN_PUNCTUATORS_START",

    "TOKEN_SQUARE_OPEN",
    "TOKEN_SQUARE_CLOSE",
    "TOKEN_CURLY_OPEN",
    "TOKEN_CURLY_CLOSE",
    "TOKEN_PARENTHESIS_OPEN",
    "TOKEN_PARENTHESIS_CLOSE",

    "TOKEN_DOT", // .
    "TOKEN_ARROW", // ->
    
    "TOKEN_PLUS_PLUS", // ++
    "TOKEN_MINUS_MINUS", // --
    
    
    // bitwise operators 
    "TOKEN_AMPERSAND", // represents both AND and ADDRESS
    "TOKEN_BITWISE_OR", // |
    "TOKEN_BITWISE_NOT", // ~
    "TOKEN_BITWISE_XOR",
    "TOKEN_SHIFT_LEFT",
    "TOKEN_SHIFT_RIGHT",

    // arithmetic operators
    "TOKEN_PLUS",
    "TOKEN_MINUS",
    "TOKEN_STAR", // represents deferencing and multiply
    "TOKEN_SLASH",
    "TOKEN_MODULO",

    // logical operators
    "TOKEN_LESS_THAN",
    "TOKEN_GREATER_THAN",
    
    "TOKEN_LESS_EQUALS",
    "TOKEN_GREATER_EQUALS",
    "TOKEN_EQUALITY_CHECK", // ==
    "TOKEN_NOT_EQUALS",

    "TOKEN_LOGICAL_AND",
    "TOKEN_LOGICAL_OR",
    "TOKEN_LOGICAL_NOT",
    
    // assignment 
    "TOKEN_ASSIGNMENT", // =
    "TOKEN_PLUS_ASSIGN",
    "TOKEN_MINUS_ASSIGN",
    "TOKEN_MUL_ASSIGN",
    "TOKEN_DIV_ASSIGN",
    "TOKEN_LSHIFT_ASSIGN",
    "TOKEN_RSHIFT_ASSIGN",
    "TOKEN_BITWISE_AND_ASSIGN",
    "TOKEN_BITWISE_OR_ASSIGN",
    "TOKEN_BITWISE_XOR_ASSIGN",
    "TOKEN_MODULO_ASSIGN",


    // punctuators?
    "TOKEN_QUESTION_MARK",
    "TOKEN_COLON",
    "TOKEN_SEMI_COLON",
    "TOKEN_COMMA",
    "TOKEN_HASH",
    
    "TOKEN_DOT_DOT_DOT",
    
    // postfix
    "TOKEN_PLUS_PLUS_POSTFIX", // ++
    "TOKEN_MINUS_MINUS_POSTFIX", // --
    
    "TOKEN_EOF",

    

    "TOKEN_COUNT"
};

#define ARRAY_COUNT(x) sizeof((x))/sizeof(*(x))

static TokenType LITERAL_TOKEN_TYPES[] = {
    TOKEN_CHARACTER_LITERAL, 
    TOKEN_STRING_LITERAL, 
    TOKEN_NUMERIC_BIN, 
    TOKEN_NUMERIC_DEC, 
    TOKEN_NUMERIC_DOUBLE, 
    TOKEN_NUMERIC_FLOAT, 
    TOKEN_NUMERIC_HEX, 
    TOKEN_NUMERIC_OCT
};

static TokenType BINARY_OP_TOKENS[] = {
    TOKEN_PLUS, 
    TOKEN_MINUS, 
    TOKEN_STAR, 
    TOKEN_SLASH, 
    TOKEN_MODULO, 
    TOKEN_AMPERSAND, 
    TOKEN_BITWISE_OR, 
    TOKEN_BITWISE_XOR, 
    TOKEN_SHIFT_LEFT, 
    TOKEN_SHIFT_RIGHT,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_EQUALITY_CHECK,
    TOKEN_NOT_EQUALS,
    TOKEN_GREATER_EQUALS,
    TOKEN_GREATER_THAN,
    TOKEN_LESS_EQUALS,
    TOKEN_LESS_THAN,
    
    // require checks for left operands: copied to another array
    TOKEN_ASSIGNMENT,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_SQUARE_OPEN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN,
    TOKEN_BITWISE_AND_ASSIGN,
    TOKEN_BITWISE_OR_ASSIGN,
    TOKEN_BITWISE_XOR_ASSIGN,

    // require checks for both left and right operands: copied to another array
    TOKEN_ARROW,
    TOKEN_DOT,
};

static TokenType ASSIGNMENT_OP[] = {
    // require checks for left operands
    TOKEN_ASSIGNMENT,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN,
    TOKEN_BITWISE_AND_ASSIGN,
    TOKEN_BITWISE_OR_ASSIGN,
    TOKEN_BITWISE_XOR_ASSIGN,
    TOKEN_MODULO_ASSIGN,
};

static TokenType STRUCT_ACCESS_OP[] = {
    // require checks for both left and right operands
    TOKEN_ARROW,
    TOKEN_DOT,
};


static TokenType UNARY_OP_TOKENS[] = {
    TOKEN_PLUS, 
    TOKEN_MINUS, 
    TOKEN_STAR, 
    TOKEN_LOGICAL_NOT, 
    TOKEN_BITWISE_NOT,
    TOKEN_AMPERSAND,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
};

static TokenType DATA_TYPE_TOKENS[] = {
    TOKEN_INT,
    TOKEN_FLOAT, 
    TOKEN_CHAR,
    TOKEN_DOUBLE,
    TOKEN_STRUCT,
    TOKEN_VOID,
};

static TokenType STORAGE_CLASS_SPECIFIER_TOKENS[] = {
    TOKEN_EXTERN,
    TOKEN_STATIC,
};

static TokenType INLINE_SPECIFIER_TOKENS[] = {TOKEN_INLINE};

static TokenType TYPE_MODIFIER_TOKENS[] = {
    TOKEN_UNSIGNED,
    TOKEN_SIGNED,
    TOKEN_LONG,
    TOKEN_SHORT,
};

static TokenType TYPE_QUALIFIER_TOKENS[] = {
    TOKEN_VOLATILE,
    TOKEN_CONST,
};

static TokenType TYPE_PREFIX_OPERATORS [] = {
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
};

static TokenType TYPE_POSTFIX_OPERATORS [] = {
    TOKEN_PLUS_PLUS_POSTFIX,
    TOKEN_MINUS_MINUS_POSTFIX,
};


static bool _match(Token token, TokenType type){
    return token.type == type;
}

static bool _matchv(Token token, TokenType type[], int n){
    for (int i=0; i<n; i++){
        if (token.type == type[i]){
            return true;
        }
    }
    return false;
}

