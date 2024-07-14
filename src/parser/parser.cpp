#include <stdio.h>
#include <assert.h>

#include "parser.h"

#define ARRAY_COUNT(x) sizeof((x))/sizeof(*(x))

static TokenType PRIMARY_TOKEN_TYPES[] = {
    TOKEN_IDENTIFIER, 
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
    TOKEN_AMPERSAND, 
    TOKEN_BITWISE_OR, 
    TOKEN_BITWISE_XOR, 
    TOKEN_SHIFT_LEFT, 
    TOKEN_SHIFT_RIGHT,
};


// referenced from https://en.cppreference.com/w/c/language/operator_precedence
int getPrecedence(Token opToken){
    switch (opToken.type){
    case TOKEN_STAR:
    case TOKEN_SLASH:
        return 3;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return 4;
    case TOKEN_SHIFT_LEFT:
    case TOKEN_SHIFT_RIGHT:
        return 5;
    case TOKEN_AMPERSAND:
        return 8;
    case TOKEN_BITWISE_XOR:
        return 9;
    case TOKEN_BITWISE_OR:
        return 10;
    
    default:
        return INT32_MAX;
    }
}





bool Parser::match(TokenType type){
    return currentToken.type == type;
}

bool Parser::matchv(TokenType type[], int n){
    for (int i=0; i<n; i++){
        if (currentToken.type == type[i]){
            return true;
        }
    }
    return false;
}

Token Parser::peekToken(){
    return currentToken;
}


Token Parser::consumeToken(){
    Token current = this->currentToken;
    this->currentToken = this->tokenizer->nextToken();
    return current;
}


Node* Parser::parseLVal(){
    assert(this->match(TOKEN_IDENTIFIER));
    
    Lvalue *l = new Lvalue;
    l->tag = Node::NODE_LVALUE;
    l->leaf = this->consumeToken();
    return l; 
}

Node* Parser::parseRVal(){
    Rvalue *r = new Rvalue;
    r->tag = Node::NODE_RVALUE;
    r->subexpr = (Subexpr *)parseSubexpr(INT32_MAX);
    return r;
}

Node* Parser::parseSubexpr(int precedence){
    
    Subexpr *left = (Subexpr*)this->parsePrimary();

    Subexpr *s = left;
    
    // while next token is an operator and its precedence is higher (value is lower) than current one, add to the tree 
    while (matchv(BINARY_OP_TOKENS, ARRAY_COUNT(BINARY_OP_TOKENS))){
        // for left to right associativity, break out when next op has a lower or equal precedence than current one
        if (getPrecedence(currentToken) >= precedence){
            break;
        }
        
        s = new Subexpr;
        s->tag = Node::NODE_SUBEXPR;
        
        s->left = left;
        s->op = this->consumeToken();
        

        Subexpr *next = (Subexpr*)this->parseSubexpr(getPrecedence(s->op));
        s->right  = next;
        s->subtag = Subexpr::SUBEXPR_RECURSE_OP;
        
        left = s;
    }     

    return s;
}

Node* Parser::parsePrimary(){
    Subexpr *s = new Subexpr;
    s->tag = Node::NODE_SUBEXPR;
    if (match(TOKEN_PARENTHESIS_OPEN)){
        this->consumeToken();
        s->inside = (Subexpr*)this->parseSubexpr(INT32_MAX);
        
        assert(match(TOKEN_PARENTHESIS_CLOSE));
        this->consumeToken();

        s->subtag = Subexpr::SUBEXPR_RECURSE_PARENTHESIS;
    }
    else if (matchv(PRIMARY_TOKEN_TYPES, ARRAY_COUNT(PRIMARY_TOKEN_TYPES))){
        s->leaf = this->consumeToken();
        s->subtag = Subexpr::SUBEXPR_LEAF;
    }
    return s;
}



Node* Parser::parseAssignment(){
    Lvalue *left = (Lvalue*)parseLVal();

    assert(this->match(TOKEN_ASSIGNMENT));
    this->consumeToken();
    
    Rvalue *right = (Rvalue*)parseRVal();


    assert(this->match(TOKEN_SEMI_COLON));
    this->consumeToken();

    Assignment *a = new Assignment;
    a->tag = Node::NODE_ASSIGNMENT;    
    a->left = left; 
    a->right = right;
    return a;
}


void printParseTree(Node *const current, int depth){
    auto printTabs = [&](int n){
        const int tabSize = 4;
        printf("%*s ", n * tabSize, "");
    };

    printTabs(depth);
    printf("%s : {\n", NODE_TAG_STRINGS[current->tag]);

    switch (current->tag){
    case Node::NODE_SUBEXPR:{
        Subexpr * s = (Subexpr *)current;

        switch (s->subtag){
        case Subexpr::SUBEXPR_RECURSE_OP :{
            printParseTree(s->left, depth + 1);
            
            printTabs(depth + 1);
            std::cout<<"OP: " << s->op.string<< "\n";
            
            printParseTree(s->right, depth + 1);   
            break;
        }

        case Subexpr::SUBEXPR_LEAF :{
            printTabs(depth + 1);
            std::cout<<"LEAF: " << s->leaf.string<< "\n";
            break;
        }

        case Subexpr::SUBEXPR_RECURSE_PARENTHESIS :{
            printTabs(depth + 1);
            std::cout<<"(\n";
            printParseTree(s->inside, depth + 1);
            printTabs(depth + 1);
            std::cout<<")\n";
            break;
        }

        default:
            break;
        }
        break;
    }

    case Node::NODE_LVALUE:{
        printTabs(depth + 1);
        std::cout<< ((Lvalue*)current)->leaf.string<< "\n";
        break;
    }
    
    case Node::NODE_RVALUE:{
        printParseTree(((Rvalue*)current)->subexpr, depth + 1);
        break;
    }

    case Node::NODE_ASSIGNMENT: {   
        Assignment *a = (Assignment*)current;
        printTabs(depth + 1);
        std::cout<<"Left: ";
        printParseTree(a->left, depth + 1);

        printTabs(depth + 1);
        std::cout<<"Right: ";
        printParseTree(a->right, depth + 1);
        break;
    }
    default:
        break;
    }
    printTabs(depth);
    printf("}\n");


} 