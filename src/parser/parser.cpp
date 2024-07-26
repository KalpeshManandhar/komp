#include <stdio.h>
#include <assert.h>

#include <logger/logger.h>

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
};


static TokenType UNARY_OP_TOKENS[] = {
    TOKEN_PLUS, 
    TOKEN_MINUS, 
    TOKEN_STAR, 
    TOKEN_LOGICAL_NOT, 
    TOKEN_BITWISE_NOT,
    TOKEN_AMPERSAND,
};

static TokenType DATA_TYPE_TOKENS[] = {
    TOKEN_INT,
    TOKEN_FLOAT, 
    TOKEN_LONG, 
    TOKEN_CHAR,
    TOKEN_DOUBLE,
    TOKEN_SHORT
};


// referenced from https://en.cppreference.com/w/c/language/operator_precedence
int getPrecedence(Token opToken){
    switch (opToken.type){
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_MODULO:
        return 3;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return 4;
    case TOKEN_SHIFT_LEFT:
    case TOKEN_SHIFT_RIGHT:
        return 5;
    case TOKEN_GREATER_EQUALS: 
    case TOKEN_GREATER_THAN: 
    case TOKEN_LESS_EQUALS: 
    case TOKEN_LESS_THAN:
        return 6;
    case TOKEN_EQUALITY_CHECK: 
    case TOKEN_NOT_EQUALS:
        return 7;
    case TOKEN_AMPERSAND:
        return 8;
    case TOKEN_BITWISE_XOR:
        return 9;
    case TOKEN_BITWISE_OR:
        return 10;
    case TOKEN_LOGICAL_AND: 
        return 11;
    case TOKEN_LOGICAL_OR: 
        return 12;
    default:
        return INT32_MAX;
    }
}





bool Parser::expect(TokenType type){
    if (!match(type)) {
        fprintf(stderr, "Expected token %s but found %.*s\n", TOKEN_TYPE_STRING[type], (int)this->currentToken.string.len, this->currentToken.string.data);

        assert(false && "Implement error recovery scrub");
        return false;
    }
    consumeToken();
    return true;
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
    assert(match(TOKEN_IDENTIFIER));
    
    Lvalue *l = new Lvalue;
    l->tag = Node::NODE_LVALUE;
    l->leaf = consumeToken();
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
        s->op = consumeToken();
        

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
    // for (subexpr)
    if (match(TOKEN_PARENTHESIS_OPEN)){
        consumeToken();
        s->inside = (Subexpr*)this->parseSubexpr(INT32_MAX);
        
        expect(TOKEN_PARENTHESIS_CLOSE);
        consumeToken();

        s->subtag = Subexpr::SUBEXPR_RECURSE_PARENTHESIS;
    }
    // for unary 
    else if (matchv(UNARY_OP_TOKENS, ARRAY_COUNT(UNARY_OP_TOKENS))){
        s->unaryOp = consumeToken();

        s->unarySubexpr = (Subexpr *)this->parsePrimary();
        s->subtag = Subexpr::SUBEXPR_UNARY;
    }
    // for terminal
    else if (matchv(PRIMARY_TOKEN_TYPES, ARRAY_COUNT(PRIMARY_TOKEN_TYPES))){
        s->leaf = consumeToken();
        s->subtag = Subexpr::SUBEXPR_LEAF;
    }
    return s;
}


Node* Parser::parseAssignment(){
    Lvalue *left = (Lvalue*)parseLVal();

    assert(match(TOKEN_ASSIGNMENT));
    consumeToken();
    
    Rvalue *right = (Rvalue*)parseRVal();

    Assignment *a = new Assignment;
    a->tag = Node::NODE_ASSIGNMENT;    
    a->left = left; 
    a->right = right;
    return a;
}


Node* Parser::parseDeclaration(){
    assert(matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS)));
    Token type = consumeToken();

    Declaration *d = new Declaration;
    d->type = type;
    d->tag = Node::NODE_DECLARATION;

    do {
        assert(match(TOKEN_IDENTIFIER));
        Token id = consumeToken();

        Declaration::DeclInfo var;
        var.identifier = id;
        var.initValue = 0;
        
        // if there is an initializer value
        if (match(TOKEN_ASSIGNMENT)){
            consumeToken();
            var.initValue = (Subexpr *)parseSubexpr(INT32_MAX);
        }
        d->decln.push_back(var);
    }while (match(TOKEN_COMMA) && expect(TOKEN_COMMA));

    return d; 
}


Node* Parser::parseStatement(){
    Node *statement;
    if (matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))){
        statement = parseDeclaration();
        assert(match(TOKEN_SEMI_COLON));
        consumeToken();
    }
    else if (match(TOKEN_IF)){
        statement = parseIf();
    }
    else if (match(TOKEN_WHILE)){
        statement = parseWhile();
    }
    else if (match(TOKEN_FOR)){
        statement = parseFor();
    }
    else{
        statement = parseAssignment();
        expect(TOKEN_SEMI_COLON);
    }



    return statement;
}


Node* Parser::parseStatementBlock(){
    StatementBlock *block = new StatementBlock;
    block->tag = Node::NODE_STMT_BLOCK;
    
    expect(TOKEN_CURLY_OPEN);
    while (!match(TOKEN_CURLY_CLOSE)){
        block->statements.push_back(parseStatement());
    }
    expect(TOKEN_CURLY_CLOSE);

    return block;
}


Node* Parser::parseIf(){
    IfNode *ifNode = new IfNode;

    ifNode->nextIf = NULL;
    ifNode->tag = Node::NODE_IF_BLOCK; 


    // parses 'if' with condition     
    if (match(TOKEN_IF)){
        consumeToken();

        // parse condition
        expect(TOKEN_PARENTHESIS_OPEN);
        ifNode->condition = (Subexpr *)parseSubexpr(INT32_MAX);
        expect(TOKEN_PARENTHESIS_CLOSE);
    
        ifNode->subtag = IfNode::IfNodeType::IF_NODE;
    }
    // else it is statement block of 'else'
    else{
        ifNode->subtag = IfNode::IfNodeType::ELSE_NODE;
    }

    ifNode->block = (StatementBlock *)parseStatementBlock();


    // if there is an 'else' or 'else if', then consumes the 'else' token and recursively parse new 'if' or statement block
    if (match(TOKEN_ELSE)){
        consumeToken();
        
        ifNode->nextIf = (IfNode *)parseIf();
    }
    return ifNode;
}


Node* Parser::parseWhile(){
    WhileNode *whileNode = new WhileNode;

    whileNode->tag = Node::NODE_WHILE; 

    expect(TOKEN_WHILE);
    // parse condition
    expect(TOKEN_PARENTHESIS_OPEN);
    whileNode->condition = (Subexpr *)parseSubexpr(INT32_MAX);
    expect(TOKEN_PARENTHESIS_CLOSE);
    
    whileNode->block = (StatementBlock *)parseStatementBlock();

    return whileNode;
}


Node* Parser::parseFor(){
    ForNode *forNode = new ForNode;

    forNode->tag = Node::NODE_FOR; 

    expect(TOKEN_FOR);
    // parse condition
    expect(TOKEN_PARENTHESIS_OPEN);
    forNode->init = (Declaration *)parseDeclaration();
    expect(TOKEN_SEMI_COLON);
    
    forNode->exitCondition = (Subexpr *)parseSubexpr(INT32_MAX);
    expect(TOKEN_SEMI_COLON);

    forNode->update = (Assignment *)parseAssignment();
    expect(TOKEN_PARENTHESIS_CLOSE);
    
    forNode->block = (StatementBlock *)parseStatementBlock();

    return forNode;
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

        case Subexpr::SUBEXPR_UNARY : {
            printTabs(depth + 1);
            std::cout<<"UNARY OP: " <<s->unaryOp.string << "\n";
            printParseTree(s->unarySubexpr, depth + 1);
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
        printTabs(depth);
        printParseTree(a->left, depth + 1);

        printTabs(depth);
        printParseTree(a->right, depth + 1);
        break;
    }
    case Node::NODE_DECLARATION: {
        Declaration *d = (Declaration*) current;
        printTabs(depth + 1);
        std::cout<< "type: " << d->type.string << "\n";
        
        for (auto &decl: d->decln){
            printTabs(depth + 1);
            std::cout<< "id:   " << decl.identifier.string << "\n";
            
            if (decl.initValue){
                printTabs(depth + 1);
                std::cout<< "initializer value: ";
                printParseTree(decl.initValue, depth + 1);
            }
        }
        break;
    }
    
    case Node::NODE_STMT_BLOCK: {
        StatementBlock *b = (StatementBlock*) current;

        for (auto &stmt: b->statements){
            printParseTree(stmt, depth + 1);
        }

        break;
    }


    case Node::NODE_IF_BLOCK: {
        IfNode *i = (IfNode*) current;
        
        printTabs(depth + 1);
        std::cout<<"{\n";
        

        while (i){
            printTabs(depth + 1);

            switch (i->subtag){
                case IfNode::IF_NODE:{
                    std::cout<< "IF: \n";

                    printTabs(depth + 1);
                    std::cout<< "condition: \n";
                    printParseTree(i->condition, depth + 1);
                    printTabs(depth + 1);
                    std::cout<< "statements: \n";
                    
                    printParseTree(i->block, depth + 1);

                    if (i->nextIf){
                        printTabs(depth + 1);
                        std::cout<< "ELSE ";
                    }
                    break;
                }
                case IfNode::ELSE_NODE:{
                    std::cout<< ": \n";
                    printTabs(depth + 1);
                    std::cout<< "statements: \n";
                    printParseTree(i->block, depth + 1);
                    
                    break;
                }
            }
            i = i->nextIf;
        }
        
        std::cout<<"}\n";
        break;
    }

    case Node::NODE_WHILE: {
        WhileNode *w = (WhileNode*) current;
        
        printTabs(depth + 1);
        std::cout<< "condition: \n";
        printParseTree(w->condition, depth + 1);
        printTabs(depth + 1);
        std::cout<< "statements: \n";
                    
        printParseTree(w->block, depth + 1);

        break;
    }
    
    case Node::NODE_FOR: {
        ForNode *f = (ForNode*) current;
        
        printTabs(depth + 1);
        std::cout<< "init: \n";
        printParseTree(f->init, depth+1);

        printTabs(depth + 1);
        std::cout<< "exit condition: \n";
        printParseTree(f->exitCondition, depth+1);

        printTabs(depth + 1);
        std::cout<< "update: \n";
        printParseTree(f->update, depth+1);
        
        printTabs(depth + 1);
        std::cout<< "statements: \n";
        printParseTree(f->block, depth + 1);

        break;
    }

    default:
        break;
    }
    printTabs(depth);
    printf("}\n");


} 