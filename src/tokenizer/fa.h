#pragma once

#include <unordered_map>
#include <assert.h>

#include "token.h"


struct TransitionTableEntry{
    int transitions[256] = {0};
};

struct DFA{
    std::unordered_map<int, TransitionTableEntry> transitionTable;
    int currentState;
    int startState;
    
    void setStartState(int s){
        assert(transitionTable.contains(s));
        this->startState = s;
        this->currentState = s;
    }
    
    void restart(){
        this->currentState = this->startState;
    }

    void addState(int s){
        transitionTable.insert({s, TransitionTableEntry{0}});
    }

    void addTransition(int s, const char *a, int sNext){
        assert(transitionTable.contains(s) && transitionTable.contains(sNext));
        
        for (int i=0; i<strlen(a); i++){
            transitionTable[s].transitions[a[i]] = sNext;
        }
    }
    
    void addTransitionRange(int s, char start, char end, int sNext){
        assert(transitionTable.contains(s) && transitionTable.contains(sNext));
        
        for (int i=start; i<=end; i++){
            transitionTable[s].transitions[i] = sNext;
        }
    }

    void transition(char a){
        currentState = transitionTable[currentState].transitions[a];
    }
    
    // assumes 0 as error state, ie, if input will cause transition to an error state '0'
    bool willErrorTransition(char a){
        return transitionTable[currentState].transitions[a] == 0; 
    }
};



struct NumConstDFA: public DFA{
    enum NumConstDFA_States{
        // start and error states (non accepting) 
        STATE_ERROR = 0,
        STATE_INVALID_OCTAL,
        STATE_START,

        // intermediate states (non accepting) 
        STATE_X,
        STATE_B,
        STATE_POINT,

        STATE_HEXDOUBLE_DEC,
        STATE_HEXDOUBLE_AFTER_DEC,
        STATE_HEXDOUBLE_P,
        STATE_HEXDOUBLE_SIGN,
        
        ACCEPTING_STATES_START,

        // accepting states
        STATE_ZERO,
        STATE_HEX,
        STATE_BINARY,
        STATE_DECIMAL,
        STATE_OCTAL,
        STATE_DOUBLE,
        STATE_FLOAT,

        STATE_L,
        STATE_LL,
        STATE_U,
        STATE_UL,
        STATE_ULL,


        STATE_HEXDOUBLE,
        STATE_HEXFLOAT,

        STATE_COUNT,
    };

    void init(){        

        for (int i=0; i<STATE_COUNT; i++){
            this->addState(i);
        }

        // NOTE: STATE_ERROR is set to 0, which is the default transition value on adding a new state
        this->addTransition(STATE_START, "0", STATE_ZERO);
        this->addTransition(STATE_START, "123456789", STATE_DECIMAL);

        this->addTransition(STATE_ZERO, "xX", STATE_X);
        this->addTransition(STATE_ZERO, "bB", STATE_B);
        this->addTransition(STATE_ZERO, "01234567", STATE_OCTAL);
        this->addTransition(STATE_ZERO, "89", STATE_INVALID_OCTAL);
        this->addTransition(STATE_ZERO, ".", STATE_DOUBLE);
        this->addTransition(STATE_ZERO, "U", STATE_U);
        this->addTransition(STATE_ZERO, "L", STATE_L);

        
        this->addTransition(STATE_X, "0123456789abcdefABCDEF", STATE_HEX);
        this->addTransition(STATE_HEX, "0123456789abcdefABCDEF", STATE_HEX);
        this->addTransition(STATE_HEX, "U", STATE_U);
        this->addTransition(STATE_HEX, "L", STATE_L);
        this->addTransition(STATE_HEX, ".", STATE_HEXDOUBLE_DEC);
        
        this->addTransition(STATE_HEXDOUBLE_DEC, "0123456789abcdefABCDEF", STATE_HEXDOUBLE_AFTER_DEC);
        this->addTransition(STATE_HEXDOUBLE_AFTER_DEC, "0123456789abcdefABCDEF", STATE_HEXDOUBLE_AFTER_DEC);
        
        this->addTransition(STATE_HEXDOUBLE_AFTER_DEC, "p", STATE_HEXDOUBLE_P);
        
        this->addTransition(STATE_HEXDOUBLE_P, "0123456789", STATE_HEXFLOAT);
        this->addTransition(STATE_HEXDOUBLE_P, "+-", STATE_HEXDOUBLE_SIGN);

        this->addTransition(STATE_HEXDOUBLE_SIGN, "0123456789", STATE_HEXDOUBLE);

        this->addTransition(STATE_HEXDOUBLE, "0123456789", STATE_HEXDOUBLE);
        this->addTransition(STATE_HEXDOUBLE, "f", STATE_HEXFLOAT);
        
        this->addTransition(STATE_B, "01", STATE_BINARY);
        this->addTransition(STATE_BINARY, "01", STATE_BINARY);
        
        this->addTransition(STATE_INVALID_OCTAL, "0123456789", STATE_INVALID_OCTAL);
        this->addTransition(STATE_INVALID_OCTAL, ".", STATE_DOUBLE);
        
        
        this->addTransition(STATE_OCTAL, "01234567", STATE_OCTAL);
        this->addTransition(STATE_OCTAL, "89", STATE_INVALID_OCTAL);
        this->addTransition(STATE_OCTAL, ".", STATE_DOUBLE);
        this->addTransition(STATE_OCTAL, "U", STATE_U);
        this->addTransition(STATE_OCTAL, "L", STATE_L);
        
        this->addTransition(STATE_DECIMAL, "0123456789", STATE_DECIMAL);
        this->addTransition(STATE_DECIMAL, ".", STATE_DOUBLE);
        this->addTransition(STATE_DECIMAL, "U", STATE_U);
        this->addTransition(STATE_DECIMAL, "L", STATE_L);
        
        
        this->addTransition(STATE_DOUBLE, "0123456789", STATE_DOUBLE);
        this->addTransition(STATE_DOUBLE, "f", STATE_FLOAT);
        
        this->addTransition(STATE_U, "L", STATE_UL);
        this->addTransition(STATE_L, "L", STATE_LL);
        
        this->addTransition(STATE_UL, "L", STATE_ULL);
        
        this->setStartState(STATE_START);
        this->restart();

    }

    Token getToken(){
        Token t;
        switch (this->currentState){
        case STATE_BINARY:  
            t.type = TokenType::TOKEN_NUMERIC_BIN; break;
        
        case STATE_ZERO:  
        case STATE_DECIMAL:  
            t.type = TokenType::TOKEN_NUMERIC_DEC; break;
            
        case STATE_OCTAL:  
            t.type = TokenType::TOKEN_NUMERIC_OCT; break;
        
        case STATE_DOUBLE:  
        case STATE_HEXDOUBLE:  
            t.type = TokenType::TOKEN_NUMERIC_DOUBLE; break;
        case STATE_HEXFLOAT:  
        case STATE_FLOAT:  
            t.type = TokenType::TOKEN_NUMERIC_FLOAT; break;
        
        case STATE_HEX:  
            t.type = TokenType::TOKEN_NUMERIC_HEX; break;
        case STATE_U:
        case STATE_UL:
        case STATE_ULL:
        case STATE_L:
        case STATE_LL:
            t.type = TokenType::TOKEN_NUMERIC_DEC; break;
        default:
            t.type = TokenType::TOKEN_ERROR;
            break;
        }

        return t;
    }

};



struct PunctuatorDFA: public DFA{
    enum PunctuatorDFA_States{
        // start and error states (non accepting) 
        STATE_ERROR = 0,
        STATE_START,
        STATE_DOT_DOT,
        
        // start of accepting states, 
        // all following states are in same sequence as in their counterparts in 
        // TokenSecondaryType
        ACCEPTING_STATES_START,

        // brackets
        STATE_SQUARE_OPEN,
        STATE_SQUARE_CLOSE,
        STATE_CURLY_OPEN,
        STATE_CURLY_CLOSE,
        STATE_PARENTHESIS_OPEN,
        STATE_PARENTHESIS_CLOSE,

        STATE_DOT, // .
        STATE_ARROW, // ->
        
        STATE_INC, // ++
        STATE_DEC, // --
        

        // bitwise operators 
        STATE_AMPERSAND, // represents both AND and ADDRESS
        STATE_BITWISE_OR, // |
        STATE_BITWISE_NOT, // ~
        STATE_BITWISE_XOR,
        STATE_SHIFT_LEFT,
        STATE_SHIFT_RIGHT,

        // arithmetic operators
        STATE_PLUS,
        STATE_MINUS,
        STATE_STAR, // represents deferencing and multiply
        STATE_SLASH,
        STATE_MODULO,

        // logical operators
        STATE_LESS_THAN,
        STATE_GREATER_THAN,
        
        STATE_LESS_EQUALS,
        STATE_GREATER_EQUALS,
        STATE_EQUALITY_CHECK, // ==
        STATE_NOT_EQUALS,

        STATE_LOGICAL_AND,
        STATE_LOGICAL_OR,
        STATE_LOGICAL_NOT,
        
        // assignment 
        STATE_ASSIGNMENT, // =
        STATE_PLUS_ASSIGN,
        STATE_MINUS_ASSIGN,
        STATE_MUL_ASSIGN,
        STATE_DIV_ASSIGN,
        STATE_LSHIFT_ASSIGN,
        STATE_RSHIFT_ASSIGN,
        STATE_BITWISE_AND_ASSIGN,
        STATE_BITWISE_OR_ASSIGN,
        STATE_BITWISE_XOR_ASSIGN,
        STATE_MODULO_ASSIGN,


        // punctuators?
        STATE_QUESTION_MARK,
        STATE_COLON,
        STATE_SEMI_COLON,
        STATE_COMMA,
        STATE_HASH,
        

        // currently havent supported these
        // ... # ##
        // <: :> <% %> %: %:%: digraphs and trigraphs
        STATE_DOT_DOT_DOT,

        STATE_COUNT,
    };

    void init(){        

        for (int i=0; i<STATE_COUNT; i++){
            this->addState(i);
        }

        // NOTE: STATE_ERROR is set to 0, which is the default transition value on adding a new state
        this->addTransition(STATE_START, "[", STATE_SQUARE_OPEN);
        this->addTransition(STATE_START, "]", STATE_SQUARE_CLOSE);
        this->addTransition(STATE_START, "{", STATE_CURLY_OPEN);
        this->addTransition(STATE_START, "}", STATE_CURLY_CLOSE);
        this->addTransition(STATE_START, "(", STATE_PARENTHESIS_OPEN);
        this->addTransition(STATE_START, ")", STATE_PARENTHESIS_CLOSE);
        this->addTransition(STATE_START, ".", STATE_DOT);
        
        this->addTransition(STATE_DOT, ".", STATE_DOT_DOT);
        this->addTransition(STATE_DOT_DOT, ".", STATE_DOT_DOT_DOT);

        this->addTransition(STATE_START, "&", STATE_AMPERSAND);
        this->addTransition(STATE_START, "|", STATE_BITWISE_OR);
        this->addTransition(STATE_START, "~", STATE_BITWISE_NOT);
        this->addTransition(STATE_START, "^", STATE_BITWISE_XOR);
        
        this->addTransition(STATE_START, "+", STATE_PLUS);
        this->addTransition(STATE_START, "-", STATE_MINUS);
        this->addTransition(STATE_START, "*", STATE_STAR);
        this->addTransition(STATE_START, "/", STATE_SLASH);
        this->addTransition(STATE_START, "%", STATE_MODULO);
        

        this->addTransition(STATE_START, ">", STATE_GREATER_THAN);
        this->addTransition(STATE_START, "<", STATE_LESS_THAN);
        this->addTransition(STATE_START, "=", STATE_ASSIGNMENT);
        this->addTransition(STATE_START, "?", STATE_QUESTION_MARK);
        this->addTransition(STATE_START, ":", STATE_COLON);
        this->addTransition(STATE_START, ";", STATE_SEMI_COLON);
        this->addTransition(STATE_START, ",", STATE_COMMA);
        this->addTransition(STATE_START, "!", STATE_LOGICAL_NOT);
        this->addTransition(STATE_START, "#", STATE_HASH);
        
        this->addTransition(STATE_MINUS, ">", STATE_ARROW);
        this->addTransition(STATE_MINUS, "-", STATE_DEC);
        this->addTransition(STATE_MINUS, "=", STATE_MINUS_ASSIGN);

        this->addTransition(STATE_PLUS, "+", STATE_INC);
        this->addTransition(STATE_PLUS, "=", STATE_PLUS_ASSIGN);

        
        this->addTransition(STATE_MODULO, "=", STATE_MODULO_ASSIGN);
        this->addTransition(STATE_STAR, "=", STATE_MUL_ASSIGN);
        this->addTransition(STATE_SLASH, "=", STATE_DIV_ASSIGN);
        this->addTransition(STATE_SHIFT_LEFT, "=", STATE_LSHIFT_ASSIGN);
        this->addTransition(STATE_SHIFT_RIGHT, "=", STATE_RSHIFT_ASSIGN);
        
        this->addTransition(STATE_LESS_THAN, "<", STATE_SHIFT_LEFT);
        this->addTransition(STATE_LESS_THAN, "=", STATE_LESS_EQUALS);

        this->addTransition(STATE_GREATER_THAN, ">", STATE_SHIFT_RIGHT);
        this->addTransition(STATE_GREATER_THAN, "=", STATE_GREATER_EQUALS);
        
        this->addTransition(STATE_ASSIGNMENT, "=", STATE_EQUALITY_CHECK);

        this->addTransition(STATE_LOGICAL_NOT, "=", STATE_NOT_EQUALS);
        
        this->addTransition(STATE_AMPERSAND, "&", STATE_LOGICAL_AND);
        this->addTransition(STATE_AMPERSAND, "=", STATE_BITWISE_AND_ASSIGN);

        this->addTransition(STATE_BITWISE_OR, "|", STATE_LOGICAL_OR);
        this->addTransition(STATE_BITWISE_OR, "=", STATE_BITWISE_OR_ASSIGN);
        
        this->addTransition(STATE_BITWISE_XOR, "=", STATE_BITWISE_XOR_ASSIGN);

        
        this->setStartState(STATE_START);
        this->restart();

    }

    Token getToken(){
        Token t;
        
        if (this->currentState > ACCEPTING_STATES_START){
            t.type = TokenType::TOKEN_PUNCTUATORS_START + (this->currentState - ACCEPTING_STATES_START); 
        }
        else{
            t.type = TokenType::TOKEN_ERROR;
        }
        return t;
    }

};




struct StringLitDFA: public DFA{
    enum StringLitDFA_States{
        STATE_ERROR = 0,
        STATE_START,

        STATE_START_QUOTE,
        STATE_BACKSLASH,
        
        STATE_END_QUOTE,

        STATE_COUNT

    };

    void init(){        

        for (int i=0; i<STATE_COUNT; i++){
            this->addState(i);

        }

        // NOTE: STATE_ERROR is set to 0, which is the default transition value on adding a new state
        this->addTransition(STATE_START, "\"", STATE_START_QUOTE);

        this->addTransitionRange(STATE_START_QUOTE, ' ', '~', STATE_START_QUOTE);
        this->addTransition(STATE_START_QUOTE, "\\", STATE_BACKSLASH);
        this->addTransition(STATE_START_QUOTE, "\"", STATE_END_QUOTE);

        this->addTransition(STATE_BACKSLASH, "\\nrabftv0\"?", STATE_START_QUOTE);

        
        this->setStartState(STATE_START);
        this->restart();

    }


    Token getToken(){
        Token t;
        
        if (this->currentState == STATE_END_QUOTE){
            t.type = TokenType::TOKEN_STRING_LITERAL;
        }
        else{
            t.type = TokenType::TOKEN_ERROR;
        }
        return t;

    }


};


struct CommentDFA{
    enum CommentDFA_States{
        STATE_ERROR,
        STATE_START,

        STATE_SLASH,

        STATE_COUNT,
    };
};



