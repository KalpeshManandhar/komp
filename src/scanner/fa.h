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

    void transition(char a){
        currentState = transitionTable[currentState].transitions[a];
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
        
        // accepting states
        STATE_ZERO,
        STATE_OCTAL,
        STATE_HEX,
        STATE_BINARY,
        STATE_DECIMAL,
        STATE_DOUBLE,

        STATE_COUNT,
    };

    void init(){        

        for (int i=0; i<STATE_COUNT; i++){
            this->addState(i);
        }


        this->addTransition(STATE_START, "0", STATE_ZERO);
        this->addTransition(STATE_START, "123456789", STATE_DECIMAL);

        this->addTransition(STATE_ZERO, "xX", STATE_X);
        this->addTransition(STATE_ZERO, "bB", STATE_B);
        this->addTransition(STATE_ZERO, "01234567", STATE_OCTAL);
        this->addTransition(STATE_ZERO, "89", STATE_INVALID_OCTAL);
        this->addTransition(STATE_ZERO, ".", STATE_POINT);

        this->addTransition(STATE_X, "0123456789abcdefABCDEF", STATE_HEX);
        this->addTransition(STATE_HEX, "0123456789abcdefABCDEF", STATE_HEX);

        this->addTransition(STATE_B, "01", STATE_BINARY);
        this->addTransition(STATE_BINARY, "01", STATE_BINARY);

        this->addTransition(STATE_INVALID_OCTAL, "0123456789", STATE_INVALID_OCTAL);
        this->addTransition(STATE_INVALID_OCTAL, ".", STATE_POINT);
        
        this->addTransition(STATE_OCTAL, "01234567", STATE_OCTAL);
        this->addTransition(STATE_OCTAL, "89", STATE_INVALID_OCTAL);
        this->addTransition(STATE_OCTAL, ".", STATE_POINT);

        this->addTransition(STATE_DECIMAL, "0123456789", STATE_DECIMAL);
        this->addTransition(STATE_DECIMAL, ".", STATE_POINT);

        this->addTransition(STATE_POINT, "0123456789", STATE_DOUBLE);
        this->addTransition(STATE_DOUBLE, "0123456789", STATE_DOUBLE);

        
        this->setStartState(STATE_START);
        this->restart();

    
    }

    Token getToken(){
        Token t;
        t.type = TokenPrimaryType::TOKEN_NUMBER;
        switch (this->currentState){
        case STATE_BINARY:  
            t.type2 = TokenSecondaryType::TOKEN_NUMERIC_BIN; break;
        
        case STATE_DECIMAL:  
            t.type2 = TokenSecondaryType::TOKEN_NUMERIC_DEC; break;
        
        case STATE_OCTAL:  
            t.type2 = TokenSecondaryType::TOKEN_NUMERIC_OCT; break;
        
        case STATE_ZERO:  
        case STATE_DOUBLE:  
            t.type2 = TokenSecondaryType::TOKEN_NUMERIC_DOUBLE; break;
        
        case STATE_HEX:  
            t.type2 = TokenSecondaryType::TOKEN_NUMERIC_HEX; break;
        
        default:
            t.type = TokenPrimaryType::TOKEN_ERROR;
            t.type2 = TokenSecondaryType::TOKEN_NONE;
            break;
        }

        return t;
    }

};


