#include <stdio.h>
#include <assert.h>
#include <cstdarg>

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
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
};

static TokenType DATA_TYPE_TOKENS[] = {
    TOKEN_INT,
    TOKEN_FLOAT, 
    TOKEN_LONG, 
    TOKEN_CHAR,
    TOKEN_DOUBLE,
    TOKEN_SHORT,
    TOKEN_STRUCT
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


// error recovery: skip until the next semi colon, end of scope or until EOF
bool Parser::tryRecover(){
    TokenType recoveryDelimiters[] = {
        TOKEN_SEMI_COLON, 
        TOKEN_COMMA, 
        TOKEN_PARENTHESIS_CLOSE,
        TOKEN_CURLY_CLOSE,
        TOKEN_SQUARE_CLOSE,
        TOKEN_EOF,
    };

    while (!matchv(recoveryDelimiters, ARRAY_COUNT(recoveryDelimiters))){
        consumeToken();
    }
    return true;
}



// consumes expected token
bool Parser::expect(TokenType type){

    // unexpected token
    if (!match(type)) {
        logErrorMessage(peekToken(), "Expected token %s but found \"%.*s\"\n", TOKEN_TYPE_STRING[type], (int)currentToken.string.len, currentToken.string.data);
        errors++;
        
        tryRecover();
        
        if (match(type)){
            consumeToken();
        }
    }
    else{
        consumeToken();
    }
    return true;
}

// checks if current token matches given type
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


// advance to next token. will not advance past an EOF token
Token Parser::consumeToken(){
    Token current = currentToken;

    if (current.type != TOKEN_EOF){
        currentToken = tokenizer->nextToken();
    }
    return current;
}


// rewind the current token to given checkpoint
void Parser::rewindTo(Token checkpoint){
    currentToken = checkpoint;

    // also rewind the tokenizer to a point after the tokenization of given checkpoint token
    Token tokenizerCheckpoint;
    tokenizerCheckpoint.charNo = checkpoint.charNo + checkpoint.string.len;
    tokenizerCheckpoint.lineNo = checkpoint.lineNo;
    tokenizerCheckpoint.string.data = checkpoint.string.data + checkpoint.string.len;
    tokenizer->rewindTo(tokenizerCheckpoint);
}





DataType Parser::parseDataType(){
    assert(matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS)));


    DataType d;
    d.type = consumeToken();
    d.tag = DataType::TYPE_PRIMARY;

    // if a pointer
    while (match(TOKEN_STAR)){
        DataType *ptrTo = new DataType;
        *ptrTo = d;
        
        d.ptrTo = ptrTo;
        d.tag = DataType::TYPE_PTR;
        consumeToken(); 
    }
    
    return d;
}





/* TODO: fix this? it returns a subexpr instead of a node* as the parsePrimary also allocates memory 
which could lead to a memory leak
*/
Subexpr Parser::parseIdentifier(StatementBlock *scope){
    assert(match(TOKEN_IDENTIFIER));


    Subexpr identifier;
    identifier.tag = Node::NODE_SUBEXPR;

    // check if identifier has been declared
    auto checkDeclaration = [&](Splice name) -> bool{
        StatementBlock *currentScope = scope;
        while(currentScope){
            if (currentScope->symbols.existKey(name)){
                return true;
            }
            currentScope = currentScope->parent;
        }
        return false;
    };

    identifier.leaf = consumeToken();
    identifier.subtag = Subexpr::SUBEXPR_LEAF;

    if (!checkDeclaration(identifier.leaf.string)){
        logErrorMessage(identifier.leaf, "Undeclared identifier \"%.*s\"\n", (int)identifier.leaf.string.len, identifier.leaf.string.data);
        errors++;
    }

    return identifier;

}


// TODO: change this to parse an identifier instead of an lvalue
Node* Parser::parseLVal(StatementBlock *scope){
    assert(match(TOKEN_IDENTIFIER));
    
    Lvalue *l = new Lvalue;
    l->tag = Node::NODE_LVALUE;
    l->leaf = consumeToken();
    return l; 
}

Node* Parser::parseRVal(StatementBlock *scope){
    Rvalue *r = new Rvalue;
    r->tag = Node::NODE_RVALUE;
    r->subexpr = (Subexpr *)parseSubexpr(INT32_MAX, scope);
    return r;
}

Subexpr* Parser::parseSubexpr(int precedence, StatementBlock *scope){
    
    Subexpr *left = (Subexpr*)parsePrimary(scope);

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
        

        Subexpr *next = (Subexpr*)parseSubexpr(getPrecedence(s->op), scope);
        s->right  = next;
        s->subtag = Subexpr::SUBEXPR_RECURSE_OP;
        
        left = s;
    }     

    return s;
}

Subexpr* Parser::parsePrimary(StatementBlock *scope){
    Subexpr *s = new Subexpr;
    s->tag = Node::NODE_SUBEXPR;
    // (subexpr)
    if (match(TOKEN_PARENTHESIS_OPEN)){
        consumeToken();
        s->inside = (Subexpr*)parseSubexpr(INT32_MAX, scope);
        
        expect(TOKEN_PARENTHESIS_CLOSE);

        s->subtag = Subexpr::SUBEXPR_RECURSE_PARENTHESIS;
    }
    // unary 
    else if (matchv(UNARY_OP_TOKENS, ARRAY_COUNT(UNARY_OP_TOKENS))){
        s->unaryOp = consumeToken();

        s->unarySubexpr = (Subexpr *)parsePrimary(scope);
        s->subtag = Subexpr::SUBEXPR_UNARY;
    }
    // identifiers
    else if(match(TOKEN_IDENTIFIER)){
        Token identifier = peekToken();
        // parse function call
        if (functions.existKey(identifier.string)){
            Function foo = functions.getInfo(identifier.string).info;
            
            FunctionCall *fooCall = new FunctionCall;
            fooCall->funcName = consumeToken();
            size_t nArgs = 0;

            expect(TOKEN_PARENTHESIS_OPEN);
            
            if (!match(TOKEN_PARENTHESIS_CLOSE)){
                while (true){
                    Subexpr *arg = parseSubexpr(INT32_MAX, scope);
                    fooCall->arguments.push_back(arg);
                    nArgs++;

                    if (match(TOKEN_COMMA)){
                        consumeToken();
                    }
                    else{
                        break;
                    }
                }
            }

            expect(TOKEN_PARENTHESIS_CLOSE);    
            
            s->functionCall = fooCall;
            s->subtag = Subexpr::SUBEXPR_FUNCTION_CALL;

            if (foo.parameters.size() != nArgs){
                errors++;
                logErrorMessage(identifier, "In function \"%.*s\", required %llu but found %llu arguments.\n", 
                            (int)fooCall->funcName.string.len, fooCall->funcName.string.data,
                            foo.parameters.size(), nArgs);
            }
            
        }
        // parse identifier
        else{
            *s = parseIdentifier(scope);
        }

    }
    // terminal
    else if (matchv(PRIMARY_TOKEN_TYPES, ARRAY_COUNT(PRIMARY_TOKEN_TYPES))){
        s->leaf = consumeToken();
        s->subtag = Subexpr::SUBEXPR_LEAF;   
    }
    else{
        s->tag = Node::NODE_ERROR;
        errors++;
        // TODO: more descriptive errors pls
        logErrorMessage(peekToken(), "Unexpected token \"%.*s\". Should be a subexpression.\n", (int)peekToken().string.len, peekToken().string.data);
        // skip until a semicolon/end of scope
        tryRecover();
    }
    return s;
}


Node* Parser::parseAssignment(StatementBlock *scope){
    Lvalue *left = (Lvalue*)parseLVal(scope);

    expect(TOKEN_ASSIGNMENT);
    
    Rvalue *right = (Rvalue*)parseRVal(scope);
    
    expect(TOKEN_SEMI_COLON);


    Assignment *a = new Assignment;
    a->tag = Node::NODE_ASSIGNMENT;    
    a->left = left; 
    a->right = right;
    return a;
}


Subexpr Parser::parseFunctionCall(StatementBlock *scope){
    return Subexpr{0};
}




Node* Parser::parseDeclaration(StatementBlock *scope){
    assert(matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS)));
    DataType type = parseDataType();

    assert(match(TOKEN_IDENTIFIER));
    Token identifier = consumeToken();

    // if ( is present, then it is func declaration
    if (match(TOKEN_PARENTHESIS_OPEN)){
        expect(TOKEN_PARENTHESIS_OPEN);
        
        Function foo;
        foo.returnType = type;
        foo.funcName = identifier;
        foo.block = new StatementBlock;
        
        // parse parameters
        while (matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))){
            Function::Parameter p;
            p.type = parseDataType();
            
            assert(match(TOKEN_IDENTIFIER));
            p.identifier = consumeToken();

            foo.parameters.push_back(p);
            // add to symbol table
            foo.block->symbols.add(p.identifier.string, p.type);
            
            if (!match(TOKEN_COMMA)){
                break;
            }
            else{
                consumeToken();
            }
        };

        expect(TOKEN_PARENTHESIS_CLOSE);
        
        // parse function block
        /* NOTE: i wouldve liked to use parseStatementBlock() but that function is pretty much a standalone function that parses an independent block 
           for function definition, the parameters need to be preadded to the symbol table before calling parseStatementBlock(), 
           which would require modifying the function somehow, hence the repetition
           
           maybe keeping the symboltable as a reference instead of a member in a statement block might work, 
           by ensuring that the table is allocated by the calling function
        */
        foo.block->tag = Node::NODE_STMT_BLOCK;
        foo.block->parent = &global;
        
        expect(TOKEN_CURLY_OPEN);
        while (!match(TOKEN_CURLY_CLOSE)){
            Node *stmt = parseStatement(foo.block);
            if (stmt){
                foo.block->statements.push_back(stmt);
            }
        }
        expect(TOKEN_CURLY_CLOSE);

        functions.add(foo.funcName.string, foo);
        // TODO: maybe refactor so that no need to return NULL?
        return NULL;
    }
    // else it is var declaration
    else{        
        Declaration *d = new Declaration;
        d->type = type;
        d->tag = Node::NODE_DECLARATION;

        rewindTo(identifier);
        
        do {
            assert(match(TOKEN_IDENTIFIER));
            
            Declaration::DeclInfo var;
            var.identifier = consumeToken();
            var.initValue = 0;
            
            // if there is an initializer value
            if (match(TOKEN_ASSIGNMENT)){
                consumeToken();
                var.initValue = (Subexpr *)parseSubexpr(INT32_MAX, scope);
            }
            d->decln.push_back(var);

            scope->symbols.add(var.identifier.string, d->type);
                
            
        }while (match(TOKEN_COMMA) && expect(TOKEN_COMMA));
        expect(TOKEN_SEMI_COLON);

        return d; 
    }
}


Node* Parser::parseStatement(StatementBlock *scope){
    Node *statement;
    if (matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))){
        statement = parseDeclaration(scope);
    }
    else if (match(TOKEN_IF)){
        statement = parseIf(scope);
    }
    else if (match(TOKEN_WHILE)){
        statement = parseWhile(scope);
    }
    else if (match(TOKEN_FOR)){
        statement = parseFor(scope);
    }
    else if (match(TOKEN_RETURN)){
        statement = parseReturn(scope);
    }
    else if (match(TOKEN_CURLY_OPEN)){
        statement = parseStatementBlock(scope);
    }
    else if (match(TOKEN_IDENTIFIER)){
        statement = parseAssignment(scope);
    }
    else if (match(TOKEN_SEMI_COLON)){
        statement = NULL;
        consumeToken();
    }
    else {
        errors++;
        // TODO: more descriptive errors pls
        logErrorMessage(peekToken(), "Unexpected token \"%.*s\".\n", (int)peekToken().string.len, peekToken().string.data);
        // skip until a semicolon/end of scope
        tryRecover();
        consumeToken();
    }



    return statement;
}


// TODO: add function return value checks
ReturnNode* Parser::parseReturn(StatementBlock *scope){
    expect(TOKEN_RETURN);
    
    ReturnNode* r = new ReturnNode;
    r->tag = Node::NODE_RETURN;

    
    if (!match(TOKEN_SEMI_COLON)){
        r->returnVal = parseSubexpr(INT32_MAX, scope);
    }
    
    expect(TOKEN_SEMI_COLON);

    return r;
}




StatementBlock* Parser::parseStatementBlock(StatementBlock *scope){
    StatementBlock *block = new StatementBlock;
    block->tag = Node::NODE_STMT_BLOCK;
    block->parent = scope;
    
    expect(TOKEN_CURLY_OPEN);
    while (!match(TOKEN_CURLY_CLOSE)){
        Node *stmt = parseStatement(block);
        if (stmt){
            block->statements.push_back(stmt);
        }
    }
    expect(TOKEN_CURLY_CLOSE);

    return block;
}


Node* Parser::parseIf(StatementBlock *scope){
    IfNode *ifNode = new IfNode;

    ifNode->nextIf = NULL;
    ifNode->tag = Node::NODE_IF_BLOCK; 


    // parses 'if' with condition     
    if (match(TOKEN_IF)){
        consumeToken();

        // parse condition
        expect(TOKEN_PARENTHESIS_OPEN);
        ifNode->condition = (Subexpr *)parseSubexpr(INT32_MAX, scope);
        expect(TOKEN_PARENTHESIS_CLOSE);
    
        ifNode->subtag = IfNode::IfNodeType::IF_NODE;
    }
    // else it is statement block of 'else'
    else{
        ifNode->subtag = IfNode::IfNodeType::ELSE_NODE;
    }

    ifNode->block = (StatementBlock *)parseStatementBlock(scope);


    // if there is an 'else' or 'else if', then consumes the 'else' token and recursively parse new 'if' or statement block
    if (match(TOKEN_ELSE)){
        consumeToken();
        
        ifNode->nextIf = (IfNode *)parseIf(scope);
    }
    return ifNode;
}


Node* Parser::parseWhile(StatementBlock *scope){
    WhileNode *whileNode = new WhileNode;

    whileNode->tag = Node::NODE_WHILE; 

    expect(TOKEN_WHILE);
    // parse condition
    expect(TOKEN_PARENTHESIS_OPEN);
    whileNode->condition = (Subexpr *)parseSubexpr(INT32_MAX, scope);
    expect(TOKEN_PARENTHESIS_CLOSE);
    
    whileNode->block = (StatementBlock *)parseStatementBlock(scope);

    return whileNode;
}


Node* Parser::parseFor(StatementBlock *scope){
    ForNode *forNode = new ForNode;

    forNode->tag = Node::NODE_FOR; 

    expect(TOKEN_FOR);
    // parse condition
    expect(TOKEN_PARENTHESIS_OPEN);
    forNode->init = (Assignment *)parseAssignment(scope);
    expect(TOKEN_SEMI_COLON);
    
    forNode->exitCondition = (Subexpr *)parseSubexpr(INT32_MAX, scope);
    expect(TOKEN_SEMI_COLON);

    forNode->update = (Assignment *)parseAssignment(scope);
    expect(TOKEN_PARENTHESIS_CLOSE);
    
    forNode->block = (StatementBlock *)parseStatementBlock(scope);

    return forNode;
}



void printParseTree(Node *const current, int depth){
    if (!current){
        return;
    }


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
        case Subexpr::SUBEXPR_FUNCTION_CALL : {
            printTabs(depth + 1);
            std::cout<<"Function:  " <<s->functionCall->funcName.string << "\n";
            printTabs(depth + 1);
            std::cout<<"Parameters: \n ";

            for (auto &arg : s->functionCall->arguments){
                printParseTree(arg, depth + 1);
            }

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
        printParseTree(a->left, depth + 1);

        printParseTree(a->right, depth + 1);
        break;
    }
    case Node::NODE_DECLARATION: {
        Declaration *d = (Declaration*) current;
        printTabs(depth + 1);

        std::cout<< "type: ";
        DataType *type;
        for (type = &d->type; type->tag == DataType::TYPE_PTR; type = type->ptrTo){
            std::cout<<"*";
        }
        std::cout<<type->type.string<<"\n";


        for (auto &decl: d->decln){
            printTabs(depth + 1);
            std::cout<< "id:   " << decl.identifier.string << "\n";
            
            if (decl.initValue){
                printTabs(depth + 1);
                std::cout<< "initializer value: \n";
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
        
        printTabs(depth+1);
        std::cout<<"Symbol table:\n";
        for (auto &pair : b->symbols.entries){
            printTabs(depth + 2);
            std::cout<<pair.second.identifier <<": ";
            DataType *type = &pair.second.info;
            for (; type->tag == DataType::TYPE_PTR; type = type->ptrTo){
                std::cout<<"*";
            }
            std::cout<<type->type.string<<"\n";
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
    
    case Node::NODE_RETURN: {
        ReturnNode *r = (ReturnNode*) current;
        printTabs(depth + 1);        
        std::cout<<"Value: \n";
        printParseTree(r->returnVal, depth + 1);

        break;
    }

    default:
        break;
    }
    printTabs(depth);
    printf("}\n");


} 