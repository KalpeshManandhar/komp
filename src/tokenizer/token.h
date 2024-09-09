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


    // punctuators?
    TOKEN_QUESTION_MARK,
    TOKEN_COLON,
    TOKEN_SEMI_COLON,
    TOKEN_COMMA,
    TOKEN_HASH,
    
    
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


    // punctuators?
    "TOKEN_QUESTION_MARK",
    "TOKEN_COLON",
    "TOKEN_SEMI_COLON",
    "TOKEN_COMMA",
    "TOKEN_HASH",
    
    
    "TOKEN_EOF",
    
    

    "TOKEN_COUNT"
};

