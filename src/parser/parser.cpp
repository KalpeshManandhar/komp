#include <stdio.h>
#include <assert.h>
#include <cstdarg>

#include <logger/logger.h>

#include "parser.h"



/*
    Get relative precedence of an operator.
    Referenced from https://en.cppreference.com/w/c/language/operator_precedence
*/
int getPrecedence(Token opToken, bool isUnary = false){
    if (isUnary){
        switch (opToken.type)
        {        
        case TOKEN_PLUS_PLUS_POSTFIX:
        case TOKEN_MINUS_MINUS_POSTFIX:
            return 1;
        case TOKEN_PLUS_PLUS:
        case TOKEN_MINUS_MINUS:
            return 2;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_LOGICAL_NOT:
        case TOKEN_BITWISE_NOT:
        case TOKEN_STAR:
        case TOKEN_AMPERSAND:
            return 2;

        default:
            return INT32_MAX;
        }
    }
    
    
    switch (opToken.type){
    
    case TOKEN_SQUARE_OPEN:
    case TOKEN_DOT:
    case TOKEN_ARROW:
        return 1;
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
    case TOKEN_ASSIGNMENT: 
    case TOKEN_PLUS_ASSIGN: 
    case TOKEN_MINUS_ASSIGN: 
    case TOKEN_MUL_ASSIGN: 
    case TOKEN_DIV_ASSIGN: 
        return 14;
    default:
        return INT32_MAX;
    }
}


/*
    Recover from error: skip until the next semi colon, start/end of scope or EOF.
*/
bool Parser::tryRecover(TokenType extraDelimiter){
    TokenType recoveryDelimiters[] = {
        TOKEN_SEMI_COLON, 
        TOKEN_CURLY_OPEN,
        TOKEN_CURLY_CLOSE,
        TOKEN_EOF,
    };

    while (!matchv(recoveryDelimiters, ARRAY_COUNT(recoveryDelimiters)) && !match(extraDelimiter)){
        consumeToken();
    }

    didError = true;

    return true;
}


/*
    Expect that the current token is of given type. If not, try and recover.
    Consumes the current token if the type matches.
*/
bool Parser::expect(TokenType type){
    // unexpected token
    if (!match(type)) {
        if (didError){
            return false;
        }

        logErrorMessage(peekToken(), "Expected token %s but found \"%.*s\"", TOKEN_TYPE_STRING[type], (int)currentToken.string.len, currentToken.string.data);
        errors++;
        
        // try and recover from syntax error
        tryRecover(type);
        
        if (match(type)){
            consumeToken();
        }
    }
    else{
        consumeToken();
    }
    return true;
}


/*
    Checks if current token matches given type.
*/
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


/*
    Checks if a given token matches given type.
*/
bool Parser::match(Token token, TokenType type){
    return token.type == type;
}

bool Parser::matchv(Token token, TokenType type[], int n){
    for (int i=0; i<n; i++){
        if (token.type == type[i]){
            return true;
        }
    }
    return false;
}


/*
    Return the current token without consuming it.
*/
Token Parser::peekToken(){
    return currentToken;
}


/*
    Consume the current token and advance to next token. 
    Will not advance past an EOF token.
*/
Token Parser::consumeToken(){
    Token current = currentToken;

    if (current.type != TOKEN_EOF){
        currentToken = tokenizer->nextToken();
    }
    return current;
}


/*
    Rewind the parser state (current token) and tokenizer state (cursor/lineNo/charNo) to given checkpoint.
*/
void Parser::rewindTo(Token checkpoint){
    currentToken = checkpoint;

    // also rewind the tokenizer to a point after the tokenization of given checkpoint token
    Token tokenizerCheckpoint;
    tokenizerCheckpoint.charNo = checkpoint.charNo + checkpoint.string.len;
    tokenizerCheckpoint.lineNo = checkpoint.lineNo;
    tokenizerCheckpoint.string.data = checkpoint.string.data + checkpoint.string.len;
    tokenizer->rewindTo(tokenizerCheckpoint);
}


/*
    Check if the current token can be the start of an expression.
*/
bool Parser::isExprStart(){
    return match(TOKEN_IDENTIFIER) || matchv(UNARY_OP_TOKENS, ARRAY_COUNT(UNARY_OP_TOKENS))
            || matchv(LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES))
            || match(TOKEN_PARENTHESIS_OPEN);
}

/*
    Parses the array info for a variable declaration
*/
DataType Parser::parseArrayType(StatementBlock *scope, DataType memberType){
    DataType varType = memberType;
    DataType *arrayChainEnd = &varType;


    while (match(TOKEN_SQUARE_OPEN)){
        consumeToken();

        size_t elementCount = 0;

        
        if (match(TOKEN_SQUARE_CLOSE)){
            logErrorMessage(peekToken(), "Incomplete type: Missing array size.");
            errors++;
        }
        else{
            Subexpr * count = parseSubexpr(INT32_MAX, scope);
            

            // TODO: implement this to evaluate constant operations at compile time
            assert(count->subtag == Subexpr::SUBEXPR_LEAF);

            DataType d = checkSubexprType(count, scope);
            assert(match(d.type, TOKEN_INT));

            elementCount = strtoll(count->leaf.string.data, 0, 10);
        }
        

        // add array to type
        DataType array = {};
        array.tag = DataType::TAG_ARRAY;
        array.ptrTo = (DataType *)arena->alloc(sizeof(DataType));
        array.arrayCount = elementCount;
        array.flags |= DataType::Specifiers::CONST;
        
        (*arrayChainEnd) = array;
        arrayChainEnd = arrayChainEnd->ptrTo;

        expect(TOKEN_SQUARE_CLOSE);
    }

    (*arrayChainEnd) = memberType;

    return varType;
}


bool Parser::isTypeDefined(DataType d,  StatementBlock* scope){
    switch (d.tag) {
    case DataType::TAG_ADDRESS :
        return true;
    case DataType::TAG_ARRAY :
        return isTypeDefined(*d.ptrTo, scope);
    case DataType::TAG_PRIMARY :
        return true;
    case DataType::TAG_PTR :
        return true;
    case DataType::TAG_UNION :
    case DataType::TAG_STRUCT :{
        StatementBlock* structDefScope = scope->findCompositeDeclaration(d.compositeName.string);
        if (!structDefScope)
            return false;
        
        if (!structDefScope->composites.getInfo(d.compositeName.string).info.defined)
            return false;
        
        return true;
    }
    case DataType::TAG_VOID :
        return true;
    case DataType::TAG_ERROR :
        return true;
    
    default:
        break;
    }
    assert(false && "Some type unaccounted for.");
    return true;
}






/*
    Parse struct definition. 
    Supports struct declaration and definition.
*/
Token Parser::parseStructDefinition(StatementBlock *scope){
    assert(match(TOKEN_STRUCT) || match(TOKEN_UNION));
     
    Token unionOrStruct = consumeToken();

    Composite s;
    s.defined = false;
    s.isUnion = match(unionOrStruct, TOKEN_UNION);
    
    // unnamed composites arent supported.
    if (match(TOKEN_IDENTIFIER)){
        s.compositeName = consumeToken();
    }
    else {
        logErrorMessage(peekToken(), "Missing struct identifier.");
        errors++;
        s.compositeName = peekToken();
        s.compositeName.string.data = "unnamed-struct";
        s.compositeName.string.len = strlen("unnamed-struct");
    }
    

    if (match(TOKEN_CURLY_OPEN)){
        expect(TOKEN_CURLY_OPEN);
        

        // TODO: maybe change this to a declaration?
        // porse the struct members
        while (isStartOfType(scope)){
            Composite::MemberInfo member;
            DataType base = parseBaseDataType(scope);
            

            if (isCompositeType(base)){
                // struct declarations can also occur without identifiers/ variable declarations
                if (match(TOKEN_SEMI_COLON)){
                    consumeToken();
                    continue;
                }
            }


            do{
                // parse pointer info
                DataType withPtr = parsePointerType(scope, base);
                if (match(TOKEN_IDENTIFIER)){
                    member.memberName = consumeToken();
                    // parse array info
                    DataType withArray = parseArrayType(scope, withPtr);
                    member.type = withArray;
                    s.members.add(member.memberName.string, member);   
                    
                    // if the struct type is incomplete
                    if (!isTypeDefined(member.type, scope)){
                        logErrorMessage(member.type.compositeName, "Composite \"%.*s\" incomplete.", splicePrintf(member.type.compositeName.string));
                        errors++;
                    }
                }
                else{
                    logErrorMessage(peekToken(), "Expected an identifier for member name.");
                    errors++;
                }

                
                
                
                if (!match(TOKEN_COMMA)){
                    break;
                }

            }while (match(TOKEN_IDENTIFIER));
            
            expect(TOKEN_SEMI_COLON);

        }
    
        expect(TOKEN_CURLY_CLOSE);
        
        s.defined = true;
    }
    
    // if struct exists and is already defined
    if (s.defined){
        if (scope->composites.existKey(s.compositeName.string)){
            if (scope->composites.getInfo(s.compositeName.string).info.defined){
                logErrorMessage(s.compositeName, "Redefinition of struct \"%.*s\".", splicePrintf(s.compositeName.string));
                errors++; 
            }
            else{
                scope->composites.update(s.compositeName.string, s);
            }
        }
        else{
            // unnamed composites aren't supported
            if (!compare(s.compositeName.string, "unnamed-struct"))
                scope->composites.add(s.compositeName.string, s);
        }
    }

    return s.compositeName;
}



/*
    Parses the pointer part of a declaration. 
    Eg: int (* const) 
*/
DataType Parser::parsePointerType(StatementBlock *scope, DataType baseType){
    DataType ptr = baseType;
    if (match(TOKEN_STAR)){
        consumeToken();
        ptr.tag = DataType::TAG_PTR;
        ptr.flags = DataType::Specifiers::NONE;
        ptr.ptrTo = (DataType *)arena->alloc(sizeof(DataType));
        *(ptr.ptrTo) = baseType;
    

        while (matchv(TYPE_QUALIFIER_TOKENS, ARRAY_COUNT(TYPE_QUALIFIER_TOKENS))){
            if (match(TOKEN_CONST)){
                ptr.flags |= DataType::Specifiers::CONST;
            }
            else if (match(TOKEN_VOLATILE)){
                ptr.flags |= DataType::Specifiers::VOLATILE;
            }
            consumeToken();
        }
    }

    return ptr;
}



/*
    Parses the base data type without pointers. 
    (const int) * a;
*/
DataType Parser::parseBaseDataType(StatementBlock *scope){
    assert(isStartOfType(scope));

    bool didError = false;

    DataType d;
    d.flags = DataType::Specifiers::NONE;
    
    int flags = DataType::Specifiers::NONE;

    
    // type modifiers: signed unsigned long and short
    while (matchv(TYPE_MODIFIER_TOKENS, ARRAY_COUNT(TYPE_MODIFIER_TOKENS))
        || matchv(TYPE_QUALIFIER_TOKENS, ARRAY_COUNT(TYPE_QUALIFIER_TOKENS))
    ){
        if (match(TOKEN_CONST)){
            flags |= DataType::Specifiers::CONST;
        }
        
        if (match(TOKEN_UNSIGNED)){
            didError = didError || (flags & DataType::Specifiers::UNSIGNED);
            flags |= DataType::Specifiers::UNSIGNED;
        }
        else if (match(TOKEN_SIGNED)){
            didError = didError || (flags & DataType::Specifiers::SIGNED);
            flags |= DataType::Specifiers::SIGNED;
        }
        else if (match(TOKEN_LONG)){
            didError = didError || (flags & DataType::Specifiers::LONG_LONG);
            if (flags & DataType::Specifiers::LONG){
                flags |= DataType::Specifiers::LONG_LONG;
                flags ^= DataType::Specifiers::LONG;
            }
            else{
                flags |= DataType::Specifiers::LONG;
            }
        }
        else if (match(TOKEN_SHORT)){
            didError = didError || (flags & DataType::Specifiers::SHORT);
            flags |= DataType::Specifiers::SHORT;
        }
        consumeToken();
    }
    d.flags = flags;

    // long and short conflict
    if ((d.flags & (DataType::Specifiers::LONG | DataType::Specifiers::LONG_LONG) )
        && (d.flags & DataType::Specifiers::SHORT)){
        didError = true;
    }
    // signed and unsigned conflict
    if ((d.flags & (DataType::Specifiers::SIGNED))
        && (d.flags & DataType::Specifiers::UNSIGNED)){
        didError = true;
    }

    
    // if struct then parse the struct type or struct definition as well
    if (match(TOKEN_STRUCT)){
        d.compositeName = parseStructDefinition(scope);
        d.tag = DataType::TAG_STRUCT;
    }
    else if (match(TOKEN_UNION)){
        d.compositeName = parseStructDefinition(scope);
        d.tag = DataType::TAG_UNION;
    }
    // enum is essentially an int
    else if (match(TOKEN_ENUM)){
        parseEnum(scope);
        return DataTypes::Int;
    }
    // if is a typedef alias
    else if (scope->findTypedef(peekToken().string)){
        StatementBlock* typedefScope = scope->findTypedef(peekToken().string);
        d = typedefScope->typedefs.getInfo(peekToken().string).info.aliasFor;
        consumeToken();
    }
    // else other primary data type
    else if (matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))){
        d.type = consumeToken();
        d.tag = DataType::TAG_PRIMARY;
    }
    // default type is int if there are specifiers but no type
    else{
        d.type = DataTypes::Int.type;
        d.tag = DataType::TAG_PRIMARY;
    }

    // cannot use type modifiers with floats and doubles 
    if ((d.isSet(DataType::Specifiers::LONG_LONG) || d.isSet(DataType::Specifiers::LONG)
        || d.isSet(DataType::Specifiers::SHORT) || d.isSet(DataType::Specifiers::SIGNED)
        || d.isSet(DataType::Specifiers::UNSIGNED))  
        && (match(d.type, TOKEN_FLOAT) || match(d.type, TOKEN_DOUBLE) 
            || match(d.type, TOKEN_VOID) || match(d.type, TOKEN_STRUCT))){
        
        logErrorMessage(d.type, "Invalid use of modifiers with type \"%.*s\".", splicePrintf(d.type.string));
        errors++;
        return DataTypes::Error;
    }
    // cannot use long, short with char 
    if ((d.isSet(DataType::Specifiers::LONG_LONG) || d.isSet(DataType::Specifiers::LONG)
        || d.isSet(DataType::Specifiers::SHORT)) 
        && match(d.type, TOKEN_CHAR) ){
        logErrorMessage(d.type, "Invalid use of modifiers with type \"%.*s\".", splicePrintf(d.type.string));
        errors++;
        return DataTypes::Error;
    }



    if (didError){
        logErrorMessage(peekToken(), "Invalid combination of type modifiers.");
        errors++;
        return DataTypes::Error;
    }

    // if not unsigned, then signed by default
    if (!d.isSet(DataType::Specifiers::UNSIGNED) && match(d.type, TOKEN_INT)){
        d.flags |= DataType::Specifiers::SIGNED;
    }

    if (match(d.type, TOKEN_VOID)){
        d.tag = DataType::TAG_VOID;
    }

    return d;
}


/*
    Parse a full data type, without array.
    Eg: (const int * const) a;
*/
DataType Parser::parseDataType(StatementBlock *scope){
    DataType base = parseBaseDataType(scope);
    
    DataType d = base;
    while (match(TOKEN_STAR)){
        DataType ptr = parsePointerType(scope, d);
        d = ptr; 
    }

    return d;
}


/*
    Get the token of a subexpr for error logging.
*/
Token Parser::getSubexprToken(Subexpr *expr) {
    switch (expr->subtag){
        case Subexpr::SUBEXPR_LEAF:
            return expr->leaf; 
        
        case Subexpr::SUBEXPR_BINARY_OP:
            return expr->binary.op;

        case Subexpr::SUBEXPR_UNARY:
            return expr->unary.op;

        case Subexpr::SUBEXPR_CAST:
            return getSubexprToken(expr->cast.expr);

        case Subexpr::SUBEXPR_FUNCTION_CALL:
            return expr->functionCall->funcName;

        case Subexpr::SUBEXPR_RECURSE_PARENTHESIS:
            return getSubexprToken(expr->inside);

        case Subexpr::SUBEXPR_INITIALIZER_LIST:
            return getSubexprToken(expr->initList->values[0]);

        default:
            return expr->leaf;
    }
};



/*
    Check if a datatype can be converted to another.
*/
bool Parser::canBeConverted(Subexpr *from, DataType fromType, DataType toType, StatementBlock* scope){
    if (fromType.tag == DataType::TAG_ERROR || toType.tag == DataType::TAG_ERROR){
        return true;
    }
    if (fromType.tag == DataType::TAG_VOID && toType.tag == DataType::TAG_VOID){
        return true;
    }

    // void cannot be converted to or from anything
    if ((fromType.tag == DataType::TAG_VOID) || (toType.tag == DataType::TAG_VOID)){
        return false;
    }
    
    if (fromType.tag == DataType::TAG_COMPOSITE_UNSPECIFIED){
        if (toType.tag == DataType::TAG_PRIMARY || toType.tag == DataType::TAG_PTR){
            return false;
        }

        if (toType.tag == DataType::TAG_ARRAY){
            if (from->initList->values.size() != toType.arrayCount){
                Token subexprToken = getSubexprToken(from);
                errors++;
                logErrorMessage(subexprToken, "Number of provided initializer values doesn't match that of required type.");
                return false;
            }

            
            bool willWork = true;
            
            for (auto &value : from->initList->values){
                if (!canBeConverted(value, value->type, *(toType.ptrTo), scope)){
                    Token subexprToken = getSubexprToken(value);
                    
                    errors++;
                    logErrorMessage(subexprToken, "Cannot convert a expression of type \"%s\" to initialize type \"%s\"",
                                    dataTypePrintf(value->type), dataTypePrintf(*(toType.ptrTo))
                    );
                    willWork = false;
                }
            }
            

            return willWork;
        }
        
        if (toType.tag == DataType::TAG_STRUCT){
            StatementBlock* structDeclnScope = scope->findCompositeDeclaration(toType.compositeName.string);
            assert(structDeclnScope != NULL);

            Composite &structInfo = structDeclnScope->composites.getInfo(toType.compositeName.string).info;
            
            if (from->initList->values.size() != structInfo.members.entries.size()){
                Token subexprToken = getSubexprToken(from);
                errors++;
                logErrorMessage(subexprToken, "Number of provided initializer values doesn't match that of required type.");
                return false;
            }


            bool canConvert = true;
            for (int i=0; i < from->initList->values.size(); i++){
                Subexpr* value = from->initList->values[i];
                Composite::MemberInfo member = structInfo.members.getInfo(structInfo.members.order[i]).info;
                

                if (!canBeConverted(value, value->type, member.type, scope)){
                    Token subexprToken = getSubexprToken(value);
                    
                    errors++;
                    logErrorMessage(subexprToken, "Cannot convert a expression of type \"%s\" to initialize type \"%s\"",
                                    dataTypePrintf(value->type), dataTypePrintf(member.type)
                    );
                    canConvert = false;
                }
            }

            return canConvert;
        }
    }



    Token subexprToken = getSubexprToken(from);

    // pointers can be converted to other pointers and to integers
    if (fromType.indirectionLevel() > 0){
        if (toType.indirectionLevel() > 0){
            // dont log an error with void pointers?
            if (fromType.getBaseType().tag == DataType::TAG_VOID || 
                toType.getBaseType().tag == DataType::TAG_VOID){
                return true;
            }
            else if (fromType.indirectionLevel() != toType.indirectionLevel() || !(*(fromType.ptrTo) == *(toType.ptrTo))){
                logWarningMessage(subexprToken, "Conversion from pointer of type \"%s\" to \"%s\".",
                    dataTypePrintf(fromType), dataTypePrintf(toType)
                );
            }
            else {
                auto willDiscardConst = [&](DataType from, DataType to) -> bool{
                    while (from.indirectionLevel() > 0 && to.indirectionLevel() > 0){
                        if(from.isSet(DataType::Specifiers::CONST) && !to.isSet(DataType::Specifiers::CONST)){
                            return true;
                        }
                        from = *from.ptrTo;
                        to = *to.ptrTo;
                    }

                    return from.isSet(DataType::Specifiers::CONST) && !to.isSet(DataType::Specifiers::CONST);
                };


                if (willDiscardConst(*fromType.ptrTo, *toType.ptrTo)){
                    logWarningMessage(subexprToken, "Conversion of a const pointer type \"%s\" to non-const pointer type \"%s\". May discard const qualifier.", 
                        dataTypePrintf(fromType), dataTypePrintf(toType)
                    );
                }
            }
            return true;
        }

        if (toType.tag == DataType::TAG_PRIMARY){
            if (match(toType.type, TOKEN_INT) || match(toType.type, TOKEN_CHAR)){
                logWarningMessage(subexprToken, "Conversion from pointer type \"%s\" to integer type \"%s\".",
                                dataTypePrintf(fromType), dataTypePrintf(toType));
                return true;
            }
        }
        
        if (fromType.tag == DataType::TAG_PRIMARY){
            if (match(fromType.type, TOKEN_INT) || match(fromType.type, TOKEN_CHAR)){
                logWarningMessage(subexprToken, "Conversion from pointer type \"%s\" to integer type \"%s\".",
                                dataTypePrintf(fromType), dataTypePrintf(toType));
                return true;
            }
        }
        
        return false;
    }

    // composites can only be converted to the same struct
    if (fromType.tag == DataType::TAG_STRUCT){
        if (toType.tag == DataType::TAG_STRUCT && compare(fromType.compositeName.string, toType.compositeName.string)){
            return true;
        }
        return false;
    }
    // union can only be converted to the same struct
    if (fromType.tag == DataType::TAG_UNION){
        if (toType.tag == DataType::TAG_UNION && compare(fromType.compositeName.string, toType.compositeName.string)){
            return true;
        }
        return false;
    }
    
    if (fromType.tag == DataType::TAG_PRIMARY){
        
        // floating point numbers cannot be converted to pointers
        if ((match(fromType.type, TOKEN_FLOAT) || match(fromType.type, TOKEN_DOUBLE))
            && toType.indirectionLevel() > 0){
            return false;
        }
        // primary data types can be converted between each other
        if (fromType.tag == DataType::TAG_PRIMARY && toType.tag == DataType::TAG_PRIMARY){
            return true;
        }
        
        // integers can be converted to pointers but with warning
        if ((match(fromType.type, TOKEN_INT) || match(fromType.type, TOKEN_CHAR))
            && toType.indirectionLevel() > 0){
            if (toType.getBaseType().tag != DataType::TAG_VOID){
                logWarningMessage(subexprToken, "Conversion from integer type \"%s\" to pointer type \"%s\".",
                    dataTypePrintf(fromType), dataTypePrintf(toType));

            }
            return true;
        }
        
        // primary cannot be converted to composites
        return false;
    }

    return false;


}



bool Parser::isValidLvalue(DataType leftOperandType, Subexpr *leftOperand){
    // const type
    if (leftOperandType.isSet(DataType::Specifiers::CONST)){
        return false;
    }
    // const type
    if (leftOperandType.tag == DataType::TAG_ARRAY){
        return false;
    }
    
    TokenType lvalueOp[] = {
        TOKEN_ARROW,
        TOKEN_DOT,
        TOKEN_SQUARE_OPEN,
    };

    if (leftOperand->subtag == Subexpr::SUBEXPR_BINARY_OP){
        return matchv(leftOperand->binary.op, lvalueOp, ARRAY_COUNT(lvalueOp));
    }
    
    // single variable identifier
    if (leftOperand->subtag == Subexpr::SUBEXPR_LEAF 
        && matchv(leftOperand->leaf, LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES))){
        return false;
    }

    return true;

};


void Parser :: parseTypedef(StatementBlock *scope){
    assert(match(TOKEN_TYPEDEF));
    
    consumeToken();
    
    TypedefInfo alias;
    // the datatype being aliased
    alias.aliasFor = parseDataType(scope);
    
    if (!match(TOKEN_IDENTIFIER)){
        logErrorMessage(peekToken(), "Typedef requires a name for the alias.");
        errors++;
        return;
    }
    
    alias.identifier = consumeToken().string;

    scope->typedefs.add(alias.identifier, alias);
    expect(TOKEN_SEMI_COLON);
}







/*
    Get the expected type of a subexpr while checking for errors.
    Type conversion rules referenced from https://en.cppreference.com/w/c/language/conversion
*/
DataType Parser::checkSubexprType(Subexpr *expr, StatementBlock *scope){
    if (!expr){
        return DataTypes::Void;
    }
        

    auto checkType = [&]() -> DataType{
        
        switch (expr->subtag){
        
        case Subexpr::SUBEXPR_CAST: {
            DataType from = checkSubexprType(expr->cast.expr, scope);
            
            if (!canBeConverted(expr->cast.expr, from, expr->cast.to, scope)){
                logErrorMessage(getSubexprToken(expr->cast.expr), "Cannot convert expression of type \"%s\" to type \"%s\".", 
                    dataTypePrintf(from), dataTypePrintf(expr->cast.to)
                );
                errors++;
                return DataTypes::Error;
            }

            return expr->cast.to;
        }

        case Subexpr::SUBEXPR_INITIALIZER_LIST: {
            DataType d = DataTypes::MemBlock;
            
            void* mem = arena->alloc(sizeof(DataType::CompositeType));
            d.composite = new (mem) DataType::CompositeType;
            for (auto &val : expr->initList->values){
                DataType dt = checkSubexprType(val, scope);
                d.composite->types.push_back(dt);
            }
            return d;
        } 


        case Subexpr::SUBEXPR_BINARY_OP:{
            
            DataType left = checkSubexprType(expr->binary.left, scope);

            // okay this is a pain in the ass cause the right operand (member name) will not have a type from the identifier itself
            // that member name should not be checked for declaration by itself. 
            // so a valid memberName right operand will require to not be checked and hence, only left operand is checked at first for struct accesses 
            
            // struct accesses: struct.member and struct_ptr->member
            if (matchv(expr->binary.op, STRUCT_ACCESS_OP, ARRAY_COUNT(STRUCT_ACCESS_OP))){
                if (left.tag == DataType::TAG_ERROR){
                    return DataTypes::Error;
                }
                
                // left must be of type struct or union
                if ((left.getBaseType().tag != DataType::TAG_STRUCT) && (left.getBaseType().tag != DataType::TAG_UNION)){
                    logErrorMessage(expr->binary.op, "Not a valid composite type.");
                    errors++;
                    return DataTypes::Error;
                }
                
                // left must be struct if .
                if (match(expr->binary.op, TOKEN_DOT)){
                    if (left.indirectionLevel() != 0){
                        logErrorMessage(expr->binary.op, "Not a valid composite type.");
                        errors++;
                        return DataTypes::Error;
                    }
                }
                // left must be struct * if ->
                if (match(expr->binary.op, TOKEN_ARROW)){
                    if (left.indirectionLevel() != 1){
                        logErrorMessage(expr->binary.op, "Not a valid composite type pointer.");
                        errors++;
                        return DataTypes::Error;
                    }
                }
                
                DataType baseStructType = left.getBaseType();
                StatementBlock *structDeclScope = scope->findCompositeDeclaration(baseStructType.compositeName.string);

                if (!structDeclScope){
                    logErrorMessage(expr->binary.op, "Not a valid composite type.");
                    errors++;
                    return DataTypes::Error;
                }
                
                // the right operand must be valid member name, ie, identifier
                if (!(expr->binary.right->subtag == Subexpr::SUBEXPR_LEAF && expr->binary.right->leaf.type == TOKEN_IDENTIFIER)){
                    logErrorMessage(expr->binary.op, "Not a valid member identifier.");
                    errors++;
                    return DataTypes::Error;
                }
                
                Splice memberName = expr->binary.right->leaf.string;
                Composite st = structDeclScope->composites.getInfo(baseStructType.compositeName.string).info;
                
                // the right identifier must be a valid member name in the struct
                if (!st.members.existKey(memberName)){
                    logErrorMessage(expr->binary.right->leaf, "No \"%.*s\" member exists in struct \"%.*s\".",
                                    splicePrintf(memberName), splicePrintf(st.compositeName.string));
                    errors++;
                    return DataTypes::Error;
                }

                DataType memberType = st.members.getInfo(memberName).info.type;

                return memberType;
                
            }



            DataType right = checkSubexprType(expr->binary.right, scope);
            
            
            // if error, just return; dont log any errors
            if (left.tag == DataType::TAG_ERROR || right.tag == DataType::TAG_ERROR){
                return DataTypes::Error;
            }
            // void types cannot be used in operations
            if ((left.tag == DataType::TAG_VOID && left.indirectionLevel() == 0)
                || right.tag == DataType::TAG_VOID && right.indirectionLevel() == 0){
                logErrorMessage(expr->binary.op, "Cannot perform operation \"%.*s\" with void type.", 
                                    splicePrintf(expr->binary.op.string));
                errors++;
                return DataTypes::Error;
            }


            // indexing only works with integers
            if (match(expr->binary.op, TOKEN_SQUARE_OPEN)){
                if (!match(right.type,TOKEN_INT) && !match(right.type,TOKEN_CHAR)){
                    logErrorMessage(expr->binary.op, "Indexing only works with integer type.");
                    errors++;
                    return DataTypes::Error;
                }
                
                return *(left.ptrTo);
            }


            // check for lvalue validity
            else if (matchv(expr->binary.op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP))){
                

                if (!isValidLvalue(left, expr->binary.left)){
                    logErrorMessage(expr->binary.op, "Not a valid lvalue.");
                    errors++;
                }
                
            }

            


            // bitwise and modulo operations dont work with floating point types
            if (match(left.type, TOKEN_FLOAT) || match(left.type, TOKEN_DOUBLE) ||
                match(right.type, TOKEN_FLOAT) || match(right.type, TOKEN_DOUBLE)){
                
                TokenType invalidOpForFloatingTypes[] = {
                    TOKEN_MODULO,
                    TOKEN_AMPERSAND,
                    TOKEN_BITWISE_AND_ASSIGN,
                    TOKEN_BITWISE_OR_ASSIGN,
                    TOKEN_BITWISE_OR,
                    TOKEN_BITWISE_XOR_ASSIGN,
                    TOKEN_BITWISE_XOR,
                    TOKEN_BITWISE_NOT,
                };

                if (matchv(expr->binary.op, invalidOpForFloatingTypes, ARRAY_COUNT(invalidOpForFloatingTypes))){
                    logErrorMessage(expr->binary.op, "The operation \"%.*s\" cannot be used with floating point types.", splicePrintf(expr->binary.op.string));
                    errors++;
                    return DataTypes::Error;
                }
            }

        

            auto getResultantType = [&](DataType left, DataType right) -> DataType{
                bool didError = false;
                
                // diff level of indirection
                if (left.indirectionLevel() != right.indirectionLevel()){
                    // pointer arithmetic 
                    // (ptr + int)/(ptr - int)/(ptr += int)/(ptr -= int)
                    if (left.indirectionLevel() > 0 && (match(right.type, TOKEN_INT) || match(right.type, TOKEN_CHAR))){ 
                        if (match(expr->binary.op, TOKEN_PLUS) || match(expr->binary.op, TOKEN_MINUS) 
                        || match(expr->binary.op, TOKEN_PLUS_ASSIGN) || match(expr->binary.op, TOKEN_MINUS_ASSIGN) ){
                            return left;
                        }
                        else if (match(expr->binary.op, TOKEN_ASSIGNMENT)){
                            logWarningMessage(expr->binary.op, "Incompatible conversion from integer to pointer.");
                            return left;
                        }
                    }
                    // (int + ptr) 
                    else if (right.indirectionLevel() > 0 && (match(left.type, TOKEN_INT) || match(left.type, TOKEN_CHAR))){
                        if (match(expr->binary.op, TOKEN_PLUS)){
                            return right;
                        }
                        if (match(expr->binary.op, TOKEN_ASSIGNMENT)){
                            logWarningMessage(expr->binary.op, "Incompatible conversion from pointer to integer.");
                            return right;
                        }
                    }
                    // both pointers of different level of indirection
                    // can only assign but log a warning
                    else if (left.indirectionLevel() > 0 && right.indirectionLevel() > 0){
                        if (match(expr->binary.op, TOKEN_ASSIGNMENT)){
                            if (canBeConverted(expr->binary.right, right, left, scope)){}

                            return left;
                        }
                        if (match(expr->binary.op, TOKEN_EQUALITY_CHECK) ||
                            match(expr->binary.op, TOKEN_NOT_EQUALS)){
                            return DataTypes::Int;
                        }
                    }

                    didError = true;
                }
                // same level of indirection
                else{
                    // both are pointers
                    if (left.indirectionLevel() > 0){
                        // can assign, but log error if of different types
                        if (match(expr->binary.op, TOKEN_ASSIGNMENT)){
                            if (canBeConverted(expr->binary.right, right, left, scope)){}
                            
                            return left;
                        }

                        if (match(expr->binary.op, TOKEN_EQUALITY_CHECK) || 
                            match(expr->binary.op, TOKEN_NOT_EQUALS)){
                            return DataTypes::Int;
                        }

                        if (left == right){
                            // ptr difference: (ptr - ptr)
                            if (match(expr->binary.op, TOKEN_MINUS)){
                                return DataTypes::Long_Long;
                            }
                        }

                        didError = true;
                    }
                    // same type but not pointers
                    else if (left == right){
                        // assignment is defined for all operands of the same type  
                        if (match(expr->binary.op, TOKEN_ASSIGNMENT)){
                            return left;
                        }
                        
                        // all other primary operands work with all other operators 
                        // (floating point exceptions have been handled at the start)
                        if (left.tag == DataType::TAG_PRIMARY){
                            return left;
                        }

                        // if struct, only assignment between same composites is allowed 
                        if (match(left.type, TOKEN_STRUCT)){
                            if (match(TOKEN_ASSIGNMENT) && compare(left.compositeName.string, right.compositeName.string)){
                                return left;
                            }
                        }
                        didError = true;

                    }
                    
                    // different types of primary types
                    else if (left.tag == DataType::TAG_PRIMARY && right.tag == DataType::TAG_PRIMARY){
                        
                        // for valid assignment operations, the resultant type is the type of the left operand
                        if (matchv(expr->binary.op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP))){
                            return left;
                        }

                        // if any is double or float, convert to that
                        if (match(left.type, TOKEN_DOUBLE) || match(right.type, TOKEN_DOUBLE)){
                            return DataTypes::Double;
                        }   
                        else if (match(left.type, TOKEN_FLOAT) || match(right.type, TOKEN_FLOAT)){
                            return DataTypes::Float;
                        }   

                        // same signedness, conversion to greater conversion rank
                        else if ((left.isSet(DataType::Specifiers::SIGNED) && right.isSet(DataType::Specifiers::SIGNED))
                                || (left.isSet(DataType::Specifiers::UNSIGNED) && right.isSet(DataType::Specifiers::UNSIGNED))){
                            
                            if (getIntegerConversionRank(left) > getIntegerConversionRank(right)){
                                return left;
                            }
                            
                            return right;
                            
                        }   
                        
                        // different signedness
                        else {
                            // if unsigned has higher or equal rank, then unsigned
                            // else, if signed can accomodate full range of unsigned then convert to signed, 
                            // else convert to unsigned counterpart of the signed types
                            
                            auto signedUnsignedConversion = [&](DataType unsignedType, DataType signedType){
                                if (getIntegerConversionRank(unsignedType) >= getIntegerConversionRank(signedType)){
                                    return unsignedType;
                                }
                                else if (signedType.isSet(DataType::Specifiers::LONG_LONG)){
                                    return signedType;
                                }
                                else if (signedType.isSet(DataType::Specifiers::LONG)){
                                    // signed long cannot accomodate unsigned int
                                    if (!unsignedType.isSet(DataType::Specifiers::LONG)){
                                        return signedType;
                                    }
                                }
                                else if (!signedType.isSet(DataType::Specifiers::SHORT)){
                                    // signed int can accomodate unsigned short and char
                                    if (unsignedType.isSet(DataType::Specifiers::SHORT) || match(unsignedType.type, TOKEN_CHAR)){
                                        return signedType;
                                    }
                                }
                                else if (signedType.isSet(DataType::Specifiers::SHORT)){
                                    // signed short can accomodate unsigneds char
                                    if (match(unsignedType.type, TOKEN_CHAR)){
                                        return signedType;
                                    }
                                }
                                // unsigned counterpart of the signed types
                                signedType.flags |= DataType::Specifiers::UNSIGNED;
                                signedType.flags ^= DataType::Specifiers::SIGNED;
                                return signedType;
                            };
                            
                            
                            if (left.isSet(DataType::Specifiers::UNSIGNED)){
                                return signedUnsignedConversion(left, right);
                            }
                            else {
                                return signedUnsignedConversion(right, left);
                            }
                        }
                        
                    
                    }

                    // different types that are not primary: no operators are defined
                    else{
                        didError = true;
                    }
                }

                if (didError){
                    logErrorMessage(expr->binary.op, "No \"%.*s\" operator defined for type \"%s\" and \"%s\".", 
                                    splicePrintf(expr->binary.op.string),
                                    dataTypePrintf(left), dataTypePrintf(right));
                    errors++;
                    return DataTypes::Error;
                }


                return DataTypes::Int;
            };

            return getResultantType(left, right);
            break;
        }
            
        case Subexpr::SUBEXPR_UNARY:{

            DataType operand = checkSubexprType(expr->unary.expr, scope);
            // *ptr
            if (match(expr->unary.op, TOKEN_STAR)){
                if (operand.indirectionLevel() > 0){
                    return *(operand.ptrTo);
                }
                else{
                    logErrorMessage(expr->unary.op, "Cannot be dereferenced. Not a valid pointer.");
                    errors++;
                    return DataTypes::Error;
                }
            }
            // &var
            else if (match(expr->unary.op, TOKEN_AMPERSAND)){
                // cannot get the address of an address
                if (operand.tag == DataType::TAG_ADDRESS){
                    logErrorMessage(expr->unary.op, "Cannot get the address of an address literal.");
                    errors++;
                    return DataTypes::Error;
                }
                
                // cannot get the address of a literal
                if (expr->unary.expr->subtag == Subexpr::SUBEXPR_LEAF && 
                    matchv(expr->unary.expr->leaf, LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES))){
                    
                    logErrorMessage(expr->unary.op, "\"%.*s\" is not a valid identifier. Cannot get the address of a literal.",
                                    splicePrintf(expr->unary.expr->leaf.string));
                    errors++;
                    return DataTypes::Error;
                }
                
                // &val is valid only if &identifier, &struct.member, &array[i]
                bool isValid = false;
                
                if (expr->unary.expr->subtag == Subexpr::SUBEXPR_LEAF && match(expr->unary.expr->leaf, TOKEN_IDENTIFIER)){
                    isValid = true;
                }
                else if (expr->unary.expr->subtag == Subexpr::SUBEXPR_BINARY_OP){
                    TokenType VALID_OP[] = {
                        TOKEN_SQUARE_OPEN,
                        TOKEN_DOT,
                        TOKEN_ARROW,
                    };
                    
                    isValid = isValid || matchv(expr->unary.expr->binary.op, VALID_OP, ARRAY_COUNT(VALID_OP));
                }
                
                if (isValid){
                    DataType d;
                    d.tag = DataType::TAG_ADDRESS;
                    d.ptrTo =  (DataType*) arena->alloc(sizeof(DataType));
                    *(d.ptrTo) = operand;

                    return d;
                }
                else{
                    logErrorMessage(expr->unary.op, "Not a valid identifier.");
                    errors++;
                    return DataTypes::Error;
                }
            }
            else if (matchv(TYPE_PREFIX_OPERATORS, ARRAY_COUNT(TYPE_PREFIX_OPERATORS)) 
                    || matchv(TYPE_POSTFIX_OPERATORS, ARRAY_COUNT(TYPE_POSTFIX_OPERATORS))){
                
                bool isOperandIntegerType = !(operand == DataTypes::Int);
                bool isOperandVariable =  (expr->unary.expr->subtag != Subexpr::SUBEXPR_LEAF) 
                                        && _match(expr->unary.expr->leaf, TOKEN_IDENTIFIER);
                if (!isOperandIntegerType || !isOperandVariable){
                    logErrorMessage(getSubexprToken(expr->unary.expr), "Postfix and prefix operators can only be used with integer variables.");
                    errors++;
                    return DataTypes::Error;
                }
                
                return operand;
            }
            // if struct then, no other unary operator other than & are defined
            else if (operand.tag == DataType::TAG_STRUCT){
                return DataTypes::Error;
            }
            // if union then, no other unary operator other than & are defined
            else if (operand.tag == DataType::TAG_UNION){
                return DataTypes::Error;
            }

            return operand;
            break;
        }

        case Subexpr::SUBEXPR_LEAF:{
            // check if identifier has been declared
            if (match(expr->leaf, TOKEN_IDENTIFIER)){
                StatementBlock *varDeclScope = scope->findVarDeclaration(expr->leaf.string);
                
                if (varDeclScope){
                    DataType type = varDeclScope->symbols.getInfo(expr->leaf.string).info;
                    return type;
                }
                
                StatementBlock *enumDeclScope = scope->findEnumValue(expr->leaf.string);
                if (enumDeclScope){
                    return DataTypes::Int;
                }

                logErrorMessage(expr->leaf, "Undeclared identifier \"%.*s\"", splicePrintf(expr->leaf.string));
                errors++;
                
                return DataTypes::Error;
            }
            
            // e;se is an immediate value
            switch (expr->leaf.type){
                case TOKEN_CHARACTER_LITERAL:
                    return DataTypes::Char;
                case TOKEN_NUMERIC_FLOAT:
                    return DataTypes::Float;
                case TOKEN_NUMERIC_DOUBLE:
                    return DataTypes::Double;
                case TOKEN_NUMERIC_DEC:
                case TOKEN_NUMERIC_BIN:
                case TOKEN_NUMERIC_HEX:
                case TOKEN_NUMERIC_OCT:
                    return DataTypes::Int;
                case TOKEN_STRING_LITERAL:
                    return DataTypes::String;
                default:
                    return DataTypes::Error;
            }

            break;
        }

        
        case Subexpr::SUBEXPR_FUNCTION_CALL:{

            FunctionCall *fooCall = expr->functionCall;

            // check if function has been declared
            if (!ir->functions.existKey(fooCall->funcName.string)){
                logErrorMessage(fooCall->funcName, "Invalid implicit declaration of function \"%.*s\"", 
                                splicePrintf(fooCall->funcName.string));
                errors++;
                return DataTypes::Error;
            }

            Function foo = ir->functions.getInfo(fooCall->funcName.string).info;
            // check for number of arguments
            if (!foo.isVariadic && foo.parameters.size() != fooCall->arguments.size()){
                logErrorMessage(fooCall->funcName, "In function \"%.*s\", required %" PRIu64 " but found %" PRIu64 " arguments.", 
                            splicePrintf(fooCall->funcName.string), foo.parameters.size(), fooCall->arguments.size());
                errors++;
            }
            else if (foo.isVariadic && foo.parameters.size() > fooCall->arguments.size()){
                logErrorMessage(fooCall->funcName, "In function \"%.*s\", required a minimum of %" PRIu64 " arguments but found %" PRIu64 ".", 
                            splicePrintf(fooCall->funcName.string), foo.parameters.size(), fooCall->arguments.size());
                errors++;
            }
            else{
                // check if arguments are of correct type/can be implicitly converted to the correct type
                auto matchArgType = [&](Function *foo, FunctionCall *fooCall){
                    for (int i=0; i<fooCall->arguments.size(); i++){
                        DataType fromType = checkSubexprType(fooCall->arguments[i], scope);
                        
                        if (i < foo->parameters.size()){
                            if (!canBeConverted(fooCall->arguments[i], fromType, foo->parameters[i].type, scope)){
                                logErrorMessage(getSubexprToken(fooCall->arguments[i]), "Cannot convert argument of type \"%s\" to \"%s\"",
                                                dataTypePrintf(fromType), dataTypePrintf(foo->parameters[i].type));
                                errors++;
                            }
                        }
                    }
                };

                matchArgType(&foo, fooCall);

            }


            return foo.returnType;
        }
        
        case Subexpr::SUBEXPR_RECURSE_PARENTHESIS:
            return checkSubexprType(expr->inside, scope);
        
        default:
            return DataTypes::Error;
        }
    };

    
    DataType dt = checkType();
    expr->type = dt;
    
    return dt;
}



/*
    Parses a general expression.
*/
Subexpr* Parser::parseSubexpr(int precedence, StatementBlock *scope){
    Subexpr *left = (Subexpr*)parsePrimary(scope);


    Subexpr *s = left;
    
    // while next token is an operator and its precedence is higher (value is lower) than current one, add to the tree 
    while (matchv(BINARY_OP_TOKENS, ARRAY_COUNT(BINARY_OP_TOKENS)) 
        || matchv(TYPE_PREFIX_OPERATORS, ARRAY_COUNT(TYPE_PREFIX_OPERATORS))){
        bool isRtoLAssociative = matchv(ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP));
        bool isUnary = matchv(TYPE_PREFIX_OPERATORS, ARRAY_COUNT(TYPE_PREFIX_OPERATORS));
        
        // isRtoLAssociative = isRtoLAssociative || match(TOKEN_SQUARE_OPEN);
        int currentOpPrecedence = isUnary? getPrecedence(Token{.type = TOKEN_PLUS_PLUS_POSTFIX}, isUnary) : getPrecedence(peekToken());
        
        // for left to right associativity, break out when next op has a lower or equal precedence than current one
        if (currentOpPrecedence >= precedence){
            // right to left associativity for assignment operators, ie dont break out for same precedence
            if (isRtoLAssociative){
                if (currentOpPrecedence > precedence)
                    break;
            }
            else{
                break;
            }

        }
        
        s = (Subexpr*) arena->alloc(sizeof(Subexpr));
        s->tag = Node::NODE_SUBEXPR;
        
        if (isUnary){
            Subexpr* postfix = (Subexpr*) arena->alloc(sizeof(Subexpr));
            postfix->subtag = Subexpr::SUBEXPR_UNARY;
            postfix->tag = Node::NODE_SUBEXPR;
    
            if (match(TOKEN_PLUS_PLUS)){
                postfix->unary.op = consumeToken();
                postfix->unary.op.type = TOKEN_PLUS_PLUS_POSTFIX;
            }
            else if (match(TOKEN_MINUS_MINUS)){
                postfix->unary.op = consumeToken();
                postfix->unary.op.type = TOKEN_MINUS_MINUS_POSTFIX;
            }
            postfix->unary.expr = left;
            s = postfix;
        }
        else {
            s->binary.left = left;
            s->binary.op = consumeToken();
            
    
            Subexpr *next;
            // for array indexing []
            if (match(s->binary.op,TOKEN_SQUARE_OPEN)){
                next = parseSubexpr(INT32_MAX, scope);
                expect(TOKEN_SQUARE_CLOSE);
            }
            else{
                next = parseSubexpr(getPrecedence(s->binary.op), scope);
            }
            
            s->binary.right  = next;
            s->subtag = Subexpr::SUBEXPR_BINARY_OP;

            if (next->tag == Node::NODE_ERROR){
                s->tag = Node::NODE_ERROR;
            }
        }
        
        
        left = s;
    }     


    return s;
}



/*
    Parses a primary value that has higher precedence.
*/
Subexpr* Parser::parsePrimary(StatementBlock *scope){
    Subexpr *s = (Subexpr*) arena->alloc(sizeof(Subexpr));
    s->tag = Node::NODE_SUBEXPR;
    // (subexpr)
    if (match(TOKEN_PARENTHESIS_OPEN)){
        consumeToken();

        if (isStartOfType(scope)){
            s->cast.to = parseDataType(scope);
            s->subtag = Subexpr::SUBEXPR_CAST;
            expect(TOKEN_PARENTHESIS_CLOSE);
            
            s->cast.expr = parsePrimary(scope);
        }
        else {
            s->inside = parseSubexpr(INT32_MAX, scope);
            s->subtag = Subexpr::SUBEXPR_RECURSE_PARENTHESIS;
            expect(TOKEN_PARENTHESIS_CLOSE);

        }



        

    }
    // initializer list
    else if (match(TOKEN_CURLY_OPEN)){
        expect(TOKEN_CURLY_OPEN);
        
        void *mem = arena->alloc(sizeof(InitializerList));
        s->initList = new (mem) InitializerList;
        s->tag = Node::NODE_SUBEXPR;
        s->subtag = Subexpr::SUBEXPR_INITIALIZER_LIST;
        
        while (isExprStart() || match(TOKEN_CURLY_OPEN)){
            Subexpr* expr = parseSubexpr(INT32_MAX, scope);
            s->initList->values.push_back(expr);

            if (!match(TOKEN_COMMA)){
                break;
            }
            expect(TOKEN_COMMA);
        }
        
        expect(TOKEN_CURLY_CLOSE);
    }
    // unary 
    else if (matchv(UNARY_OP_TOKENS, ARRAY_COUNT(UNARY_OP_TOKENS))){
        s->unary.op = consumeToken();

        s->unary.expr = parseSubexpr(getPrecedence(s->unary.op, true), scope);
        s->subtag = Subexpr::SUBEXPR_UNARY;
    }
    // identifiers
    else if(match(TOKEN_IDENTIFIER)){
        Token identifier = consumeToken();
        // parse function call
        if (match(TOKEN_PARENTHESIS_OPEN)){
            expect(TOKEN_PARENTHESIS_OPEN);

            FunctionCall *fooCall = (FunctionCall*) arena->alloc(sizeof(FunctionCall));
            fooCall->funcName = identifier;
            size_t nArgs = 0;

            
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
            
            
        }
        // parse identifier
        else{
            s->subtag = Subexpr::SUBEXPR_LEAF;
            s->leaf = identifier;
        }

    }
    // literal
    else if (matchv(LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES))){
        s->leaf = consumeToken();
        s->subtag = Subexpr::SUBEXPR_LEAF;   
    }
    else{
        s->tag = Node::NODE_ERROR;
        errors++;
        logErrorMessage(peekToken(), "Invalid subexpression at token \"%.*s\".", (int)peekToken().string.len, peekToken().string.data);
        // skip until a semicolon/end of scope
        tryRecover();
    }
    return s;
}


bool Parser::canResolveToConstant(Subexpr *s, StatementBlock *scope){
    if (!s){
        return false;
    }
    
    switch (s->subtag){
    case Subexpr::SUBEXPR_LEAF:{
        TokenType VALID_LITERALS[] = {
            TOKEN_NUMERIC_BIN,
            TOKEN_NUMERIC_DEC,
            TOKEN_NUMERIC_OCT,
            TOKEN_NUMERIC_HEX,
            TOKEN_NUMERIC_FLOAT,
            TOKEN_NUMERIC_DOUBLE,
            TOKEN_CHAR,
        };

        return matchv(s->leaf, VALID_LITERALS, ARRAY_COUNT(VALID_LITERALS));
    }
    case Subexpr::SUBEXPR_BINARY_OP:{
        if (matchv(s->binary.op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP))){
            return false;
        }

        return canResolveToConstant(s->binary.left, scope) &&  canResolveToConstant(s->binary.right, scope);
    }
    case Subexpr::SUBEXPR_UNARY:{
        TokenType INVALID_OP[] = {
            TOKEN_AMPERSAND,
            TOKEN_STAR,
            TOKEN_PLUS_PLUS,
            TOKEN_MINUS_MINUS,
        };
        
        if (matchv(s->unary.op, INVALID_OP, ARRAY_COUNT(INVALID_OP))){
            return false;
        }

        return canResolveToConstant(s->unary.expr, scope);
    }

    case Subexpr::SUBEXPR_RECURSE_PARENTHESIS:{
        return canResolveToConstant(s->inside, scope);
    }
    case Subexpr::SUBEXPR_CAST:{
        return canResolveToConstant(s->cast.expr, scope);
    }
    case Subexpr::SUBEXPR_INITIALIZER_LIST:{
        for (auto const &val : s->initList->values){
            if (!canResolveToConstant(val, scope)){
                return false;
            }
        } 
        return true;
    }
    default:
        break;
    }

    return false;

    
}


void Parser :: parseEnum(StatementBlock* scope){
    assert(expect(TOKEN_ENUM));
    
    EnumClass e = {0};
    bool isNamed = false; 

    if (match(TOKEN_IDENTIFIER)){
        e.name = consumeToken().string;
        isNamed = true;
    }
    
    if (match(TOKEN_CURLY_OPEN)){
        
        expect(TOKEN_CURLY_OPEN);
        
        int i = 0; 
        do {
            if (!match(TOKEN_IDENTIFIER)){
                logErrorMessage(peekToken(), "Expected an identifier for the enum value.");
                errors++;
                continue;
            }
            
            EnumValue v;
            v.name = consumeToken().string;
            v.value = i++;
            
            if (scope->enumValues.existKey(v.name)){
                logErrorMessage(peekToken(), "Enum value of \"%.*s\" already defined.", splicePrintf(v.name));
                errors++;
            }
            scope->enumValues.update(v.name, v);
            
            if (match(TOKEN_COMMA)){
                consumeToken();
            }

            
        }while(!match(TOKEN_CURLY_CLOSE));
        
        
        expect(TOKEN_CURLY_CLOSE);
        e.isDefined = true;
    }
    

    if (isNamed){
        if (scope->findVarDeclaration(e.name) || (scope->findEnumClass(e.name) && e.isDefined)|| scope->findCompositeDeclaration(e.name) || scope->findTypedef(e.name)){
            logErrorMessage(peekToken(), "Redefinition of type \"%.*s\".", splicePrintf(e.name));
            errors++;
        }
        scope->enumClasses.update(e.name, e);
    }
}
    
    
    
    

/*
    Parses variable declaration, function declaration/definition and struct declaration/definition.
*/
Node* Parser::parseDeclaration(StatementBlock *scope){
    // parse storage class
    {
        int i=0;
        while (matchv(STORAGE_CLASS_SPECIFIER_TOKENS, ARRAY_COUNT(STORAGE_CLASS_SPECIFIER_TOKENS))
            || matchv(INLINE_SPECIFIER_TOKENS, ARRAY_COUNT(INLINE_SPECIFIER_TOKENS))){
            Token t = consumeToken();
            if (i == 1){
                logErrorMessage(t, "Can only have one storage class.");
                errors++;
            }
            if (matchv(STORAGE_CLASS_SPECIFIER_TOKENS, ARRAY_COUNT(STORAGE_CLASS_SPECIFIER_TOKENS))){   
                i++;
            }
        }
    }

    DataType type = parseDataType(scope);
    

    // only struct declaration w/o variable declaration
    if (type.tag == DataType::TAG_STRUCT 
        && match(TOKEN_SEMI_COLON)){
        consumeToken();
        return NULL;
    }
    // only struct declaration w/o variable declaration
    if (type.tag == DataType::TAG_UNION 
        && match(TOKEN_SEMI_COLON)){
        consumeToken();
        return NULL;
    }

    if (match(TOKEN_SEMI_COLON)){
        consumeToken();
        return NULL;
    }

    // check if struct has been defined: only defined composites can be used for declaration
    if (!isTypeDefined(type, scope)){
        logErrorMessage(type.compositeName, "Composite \"%.*s\" incomplete.", splicePrintf(type.compositeName.string));
        errors++;
    }

    
    assert(match(TOKEN_IDENTIFIER));
    Token identifier = consumeToken();
    

    // if ( is present, then it is func declaration/definition
    if (match(TOKEN_PARENTHESIS_OPEN)){
        expect(TOKEN_PARENTHESIS_OPEN);
        
        Function foo;
        foo.returnType = type;
        foo.funcName = identifier;
        foo.block = NULL;
        foo.isVariadic = false;
        
        bool isDeclOnly = false;

        // parse parameters
        while (isStartOfType(scope)){
            Function::Parameter p;
            p.type = parseDataType(scope);
            if (p.type.tag == DataType::TAG_VOID) {
                break;
            }
            
            if (!match(TOKEN_IDENTIFIER)){
                isDeclOnly = true;
                p.identifier = {.string = {.data = "", .len = 0}};
            }
            else {
                p.identifier = consumeToken();
            }
            
            foo.parameters.push_back(p);
            
            
            if (!match(TOKEN_COMMA)){
                break;
            }
            else{
                consumeToken();
            }

            if (match(TOKEN_DOT_DOT_DOT)){
                consumeToken();
                foo.isVariadic = true;
                break;
            }
        };

        expect(TOKEN_PARENTHESIS_CLOSE);
        
        
        // if definition exists
        if (!isDeclOnly && match(TOKEN_CURLY_OPEN)){
            
            // parse function body
            foo.block = parseStatementBlock(scope, true);
            foo.block->subtag = StatementBlock::BLOCK_FUNCTION_BODY;
            foo.block->funcName = foo.funcName;

            // add parameters to symbol table
            for (auto &p : foo.parameters){
                foo.block->symbols.add(p.identifier.string, p.type);
            }            
        }
        // declaration only
        else{
            expect(TOKEN_SEMI_COLON);
        }
    
        // function definitions are valid only in global scope
        if (scope == &ir->global){
            if (!ir->functions.existKey(foo.funcName.string)){
                ir->functions.add(foo.funcName.string, foo);
            }
            // if a function entry already exists
            else{
                Function f = ir->functions.getInfo(foo.funcName.string).info;
                // if func definition already exists, then it is redefinition
                if (f.block){
                    logErrorMessage(foo.funcName, "Redefinition of function \"%.*s\".", splicePrintf(foo.funcName.string));
                    errors++;
                }
                else{
                    // check definition params with declaration params and check return value
                    auto checkMatch = [&](Function prevFoo, Function newFoo) -> bool{
                        // check num of parameters 
                        if (prevFoo.parameters.size() != newFoo.parameters.size()){
                            logErrorMessage(foo.funcName, "Number of arguments %" PRIu64 " doesn't match with that in declaration %" PRIu64 " of function \"%.*s\".", 
                                            newFoo.parameters.size(), prevFoo.parameters.size(), splicePrintf(foo.funcName.string));
                            errors++;
                            return false;
                        }
                        
                        // check types of parameters 
                        for (int i=0; i< prevFoo.parameters.size(); i++){
                            if (!(prevFoo.parameters[i].type == newFoo.parameters[i].type)){
                                logErrorMessage(foo.funcName, "Conflicting types of parameter \"%.*s\": \"%s\" and \"%s\".", 
                                                splicePrintf(foo.parameters[i].identifier.string), 
                                                dataTypePrintf(prevFoo.parameters[i].type), dataTypePrintf(newFoo.parameters[i].type));
                                errors++;
                                return false;
                            }
                        }
                        
                        // check return type 
                        if (!(prevFoo.returnType == newFoo.returnType)){
                            logErrorMessage(foo.funcName, "Conflicting return type of function \"%.*s\": \"%s\" and \"%s\".", 
                                            splicePrintf(foo.funcName.string),
                                            dataTypePrintf(prevFoo.returnType), dataTypePrintf(newFoo.returnType));
                            errors++;
                            return false;
                        }


                        return true;
                    };

                    if(checkMatch(f, foo)){
                        ir->functions.update(foo.funcName.string, foo);
                    }
                }

                        
                
            }

        }
        else{
            logErrorMessage(foo.funcName, "Invalid function declaration \"%.*s\". Function declarations are valid only in global scope.",
                            splicePrintf(foo.funcName.string));
            errors++;
        }

        // TODO: maybe refactor so that no need to return NULL?
        return NULL;
    }
    // else it is var declaration
    else{        
        void *mem = arena->alloc(sizeof(Declaration));
        Declaration *d =  new (mem) Declaration;

        d->tag = Node::NODE_DECLARATION;
        
        rewindTo(identifier);

        // a kinda hacky solution: the first variable type has already been parsed 
        // eg: (const int * const) a, *b;
        // which is stored in "type". So the * tokens are not matched, and the whole type is used as the base. 
        // for the next ones, the actual base type is used as the base on parsing the pointer types  
        // for a, the * will not be matched, so base type is (const int * const)
        // for b, the * is parsed and the actual base type is used (const int)
        
        DataType base = type.getBaseType();
        
        do {
            // parse pointer info
            while (match(TOKEN_STAR)){
                type = parsePointerType(scope, type);
            }
            

            assert(match(TOKEN_IDENTIFIER));
            
            Declaration::DeclInfo var;
            var.identifier = consumeToken();
            var.initValue = 0;

            // parse array info
            var.type = parseArrayType(scope, type);

            
            // if there is an initializer value
            if (match(TOKEN_ASSIGNMENT)){
                consumeToken();
                var.initValue = parseSubexpr(INT32_MAX, scope);
            }


            if (!scope->symbols.existKey(var.identifier.string)){
                d->decln.push_back(var);
                scope->symbols.add(var.identifier.string, var.type);
            }
            else{
                DataType prevType = scope->symbols.getInfo(var.identifier.string).info;
                if (prevType.tag != DataType::TAG_ERROR){
                    logErrorMessage(var.identifier, "Redeclaration of \"%.*s\" with type \"%s\", previously defined with type \"%s\".",
                                    splicePrintf(var.identifier.string), 
                                    dataTypePrintf(var.type), dataTypePrintf(prevType));
                    errors++;
                }
            }

            
            type = base;
        }while (match(TOKEN_COMMA) && expect(TOKEN_COMMA));
        expect(TOKEN_SEMI_COLON);

        

        return d; 
    }
}

bool Parser :: isStartOfType(StatementBlock* scope){
    return matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))
        || matchv(TYPE_MODIFIER_TOKENS, ARRAY_COUNT(TYPE_MODIFIER_TOKENS))
        || matchv(TYPE_QUALIFIER_TOKENS, ARRAY_COUNT(TYPE_QUALIFIER_TOKENS))
        || scope->findTypedef(peekToken().string);
}



/*
    Parses different types of statements allowed.
*/
Node* Parser::parseStatement(StatementBlock *scope){
    didError = false;

    Node *statement = 0;
    // is declaration if starts with datatype token or a storage class specifier
    if (isStartOfType(scope) || matchv(STORAGE_CLASS_SPECIFIER_TOKENS, ARRAY_COUNT(STORAGE_CLASS_SPECIFIER_TOKENS))){
        statement = parseDeclaration(scope);
    }
    else if (match(TOKEN_IF)){
        statement = parseIf(scope);
    }
    else if (match(TOKEN_TYPEDEF)){
        parseTypedef(scope);
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
    else if (match(TOKEN_BREAK)){
        statement = parseBreak(scope);
    }
    else if (match(TOKEN_CONTINUE)){
        statement = parseContinue(scope);
    }
    // { represents a new scope/statment block
    else if (match(TOKEN_CURLY_OPEN)){
        statement = parseStatementBlock(scope, true);
    }
    else if (match(TOKEN_SEMI_COLON)){
        statement = NULL;
        consumeToken();
    }
    else if (isExprStart()){
        statement = parseSubexpr(INT32_MAX, scope);
        expect(TOKEN_SEMI_COLON);
    }
    else {
        errors++;
        logErrorMessage(peekToken(), "Unexpected token \"%.*s\".", (int)peekToken().string.len, peekToken().string.data);
        // skip until a semicolon/end of scope
        tryRecover();
        consumeToken();
    }



    return statement;
}



ReturnNode* Parser::parseReturn(StatementBlock *scope){
    ReturnNode* r = (ReturnNode*) arena->alloc(sizeof(ReturnNode));
    r->returnToken = consumeToken();
    r->returnVal = NULL;
    r->tag = Node::NODE_RETURN;
    
    // parse the return value
    if (!match(TOKEN_SEMI_COLON)){
        r->returnVal = parseSubexpr(INT32_MAX, scope);
    }
    
    expect(TOKEN_SEMI_COLON);

    return r;
}


BreakNode* Parser::parseBreak(StatementBlock *scope){
    BreakNode* b = (BreakNode*) arena->alloc(sizeof(BreakNode));
    b->breakToken = consumeToken();
    b->tag = Node::NODE_BREAK;
    
    expect(TOKEN_SEMI_COLON);

    return b;
}

ContinueNode* Parser::parseContinue(StatementBlock *scope){
    ContinueNode* c = (ContinueNode*) arena->alloc(sizeof(ContinueNode));
    c->continueToken = consumeToken();
    c->tag = Node::NODE_CONTINUE;
    
    expect(TOKEN_SEMI_COLON);

    return c;
}




StatementBlock* Parser::parseStatementBlock(StatementBlock *scope, bool blockMode){
    void *mem = arena->alloc(sizeof(StatementBlock));
    StatementBlock *block =  new (mem) StatementBlock;
    
    block->tag = Node::NODE_STMT_BLOCK;
    block->parent = scope;
    block->subtag = StatementBlock::BLOCK_UNNAMED;
    
    
    if (blockMode){
        expect(TOKEN_CURLY_OPEN);
        
        // parse statements
        while (!match(TOKEN_CURLY_CLOSE)){
            Node *stmt = parseStatement(block);
            if (stmt){
                block->statements.push_back(stmt);
            }
        }
        expect(TOKEN_CURLY_CLOSE);
    }
    else {
        Node *stmt = parseStatement(block);
        if (stmt){
            block->statements.push_back(stmt);
        }
    }

    return block;
}


Node* Parser::parseIf(StatementBlock *scope){
    IfNode *ifNode = (IfNode*) arena->alloc(sizeof(IfNode));

    ifNode->nextIf = NULL;
    ifNode->tag = Node::NODE_IF_BLOCK; 


    // parses 'if' with condition     
    if (match(TOKEN_IF)){
        consumeToken();

        // parse condition
        expect(TOKEN_PARENTHESIS_OPEN);
        
        if (isExprStart()){
            ifNode->condition = parseSubexpr(INT32_MAX, scope);
        }
        else{
            ifNode->condition = NULL;
            logErrorMessage(peekToken(), "Missing expression for if condition.");
            errors++;
        }

        expect(TOKEN_PARENTHESIS_CLOSE);
    
        ifNode->subtag = IfNode::IfNodeType::IF_NODE;
    }
    // else it is statement block of 'else'
    else{
        ifNode->subtag = IfNode::IfNodeType::ELSE_NODE;
    }
    
    bool isBlock = match(TOKEN_CURLY_OPEN);

    ifNode->block = (StatementBlock *)parseStatementBlock(scope, isBlock);
    ifNode->block->subtag = StatementBlock::BLOCK_IF;
    ifNode->block->scope  = ifNode;



    // if there is an 'else' or 'else if', then consumes the 'else' token and recursively parse new 'if' or statement block
    if (match(TOKEN_ELSE)){
        consumeToken();
        
        ifNode->nextIf = (IfNode *)parseIf(scope);
    }
    return ifNode;
}



Node* Parser::parseWhile(StatementBlock *scope){
    WhileNode *whileNode = (WhileNode*) arena->alloc(sizeof(WhileNode));

    whileNode->tag = Node::NODE_WHILE; 

    expect(TOKEN_WHILE);
    
    // parse condition
    expect(TOKEN_PARENTHESIS_OPEN);
    if (isExprStart()){
        whileNode->condition = parseSubexpr(INT32_MAX, scope);
    }
    else{
        whileNode->condition = NULL;
        logErrorMessage(peekToken(), "Missing expression for while condition.");
        errors++;
    }
    expect(TOKEN_PARENTHESIS_CLOSE);
    
    bool isBlock = match(TOKEN_CURLY_OPEN);
    
    whileNode->block = (StatementBlock *)parseStatementBlock(scope, isBlock);
    whileNode->block->subtag = StatementBlock::BLOCK_WHILE;
    whileNode->block->scope  = whileNode;
    

    return whileNode;
}




Node* Parser::parseFor(StatementBlock *scope){
    ForNode *forNode = (ForNode*) arena->alloc(sizeof(ForNode));

    forNode->tag = Node::NODE_FOR; 
    forNode->init = NULL; 
    forNode->exitCondition = NULL; 
    forNode->update = NULL; 

    expect(TOKEN_FOR);
    // parse init expr
    expect(TOKEN_PARENTHESIS_OPEN);
    if (isExprStart()){
        forNode->init = parseSubexpr(INT32_MAX, scope);
    }
    expect(TOKEN_SEMI_COLON);
    
    // parse condition expr
    if (isExprStart()){
        forNode->exitCondition = parseSubexpr(INT32_MAX, scope);
    }
    expect(TOKEN_SEMI_COLON);

    // porse update expr
    if (isExprStart()){
        forNode->update = parseSubexpr(INT32_MAX, scope);
    }
    expect(TOKEN_PARENTHESIS_CLOSE);
    
    bool isBlock = match(TOKEN_CURLY_OPEN);

    forNode->block = (StatementBlock *)parseStatementBlock(scope, isBlock);
    forNode->block->subtag = StatementBlock::BLOCK_FOR;
    forNode->block->scope  = forNode;
    

    return forNode;
}



/*
    Parse text as a proper C program.
    Returns the constructed AST on no errors.
    Returns NULL on error.
*/
AST *Parser::parseProgram(){
    while (peekToken().type != TOKEN_EOF){
        if (isStartOfType(&ir->global) || matchv(STORAGE_CLASS_SPECIFIER_TOKENS, ARRAY_COUNT(STORAGE_CLASS_SPECIFIER_TOKENS))){
            Node *stmt = this->parseDeclaration(&ir->global);
            if (stmt){
                ir->global.statements.push_back(stmt);
                checkContext(stmt, &ir->global);
            }
        }
        else if (match(TOKEN_TYPEDEF)){
            parseTypedef(&ir->global);
        }
        else if (match(TOKEN_SEMI_COLON)){
            consumeToken();
        }
        else{
            logErrorMessage(peekToken(), "Not a valid function or variable declaration.");
            errors++;
            tryRecover();
            consumeToken();
        }

    }

    for (auto &fooInfo : ir->functions.entries){
        Function &foo = fooInfo.second.info;
        checkContext(foo.block, &ir->global);
    }
    
    fprintf(stdout, "[Parser] %" PRIu64 " errors generated.\n", errors);
    return (errors == 0)? ir : NULL;

}


/*
    Check overall context of the program.
*/
bool Parser::checkContext(Node *n, StatementBlock *scope){
    if (!n){
        return false;
    }


    switch (n->tag){
    case Node::NODE_ERROR:{
        return false;
    }
    case Node::NODE_IF_BLOCK:{
        IfNode *i = (IfNode *)n;
        
        while (i){
            if (i->condition){    
                // check if condition expression can be converted to a boolean/arithmetic value
                DataType conditionType = checkSubexprType(i->condition, scope);

                if (!canBeConverted(i->condition, conditionType, DataTypes::Int, scope)){
                    Token subexprToken = getSubexprToken(i->condition);
                    
                    logErrorMessage(subexprToken, "Cannot convert from expression of type \"%s\" to an integer/arithmetic type",
                                    dataTypePrintf(conditionType));
                    errors++;
                }
            }
            checkContext(i->block, scope);
            
            i = i->nextIf;
        }

        break;
    }
    case Node::NODE_WHILE:{
        WhileNode *w = (WhileNode *)n;
        
        // check if condition expression can be converted to a boolean/arithmetic value
        DataType conditionType = checkSubexprType(w->condition, scope);

        if (!canBeConverted(w->condition, conditionType, DataTypes::Int, scope)){
            Token subexprToken = getSubexprToken(w->condition);
            
            logErrorMessage(subexprToken, "Cannot convert from expression of type \"%s\" to an integer/arithmetic type.",
                            dataTypePrintf(conditionType));
            errors++;
        }

        checkContext(w->block, scope);

        break;
    }
    case Node::NODE_FOR:{
        ForNode *f = (ForNode *)n;
        
        checkSubexprType(f->init, scope);
        checkSubexprType(f->update, scope);

        DataType conditionType = checkSubexprType(f->exitCondition, scope);

        // check if condition expression can be converted to a boolean/arithmetic value
        if (!canBeConverted(f->exitCondition, conditionType, DataTypes::Int, scope)){
            Token subexprToken = getSubexprToken(f->exitCondition);
            
            logErrorMessage(subexprToken, "Cannot convert from expression of type \"%s\" to an integer/arithmetic type.",
                            dataTypePrintf(conditionType));
            errors++;
        }

        checkContext(f->block, scope);

        break;
    }
    case Node::NODE_STMT_BLOCK:{
        StatementBlock *s = (StatementBlock *)n;

        for (auto &stmt: s->statements){
            checkContext(stmt, s);
        }

        break;
    }
    
    case Node::NODE_RETURN:{
        ReturnNode *r = (ReturnNode *)n;

        StatementBlock *functionScope = scope->getParentFunction();
        
        // check if return is inside function
        if (!functionScope){
            logErrorMessage(r->returnToken, "Return statement can only be used in a function body.");
            errors++;
            return false;
        }

        assert(ir->functions.existKey(functionScope->funcName.string));
        
        // check if return expression can be converted to the expected return type
        DataType expectedRetType = ir->functions.getInfo(functionScope->funcName.string).info.returnType;
        DataType retExprType = checkSubexprType(r->returnVal, scope);

        if (!canBeConverted(r->returnVal, retExprType, expectedRetType, scope)){
            Token errorToken = (r->returnVal)? getSubexprToken(r->returnVal) : r->returnToken;
            
            logErrorMessage(errorToken, "Cannot convert subexpr of type \"%s\" to expected return type \"%s\".",
                            dataTypePrintf(retExprType), dataTypePrintf(expectedRetType));
            errors++;
            return false;
        }

        break;
        
    }
    
    case Node::NODE_BREAK:{
        BreakNode *b = (BreakNode *)n;

        auto isValidBreak = [&]() -> bool{
            StatementBlock *currentScope = scope;
            while (currentScope){
                if (currentScope->subtag == StatementBlock::BLOCK_FOR
                    || currentScope->subtag == StatementBlock::BLOCK_WHILE){
                    return true;
                }
                currentScope = currentScope->parent;

            }
            return false;
        };

        // check if break is inside a for/while loop
        if (!isValidBreak()){
            logErrorMessage(b->breakToken, "No control statement to break out of.");
            errors++;
            return false;
        }

        break;
    }
    
    case Node::NODE_CONTINUE:{
        BreakNode *c = (BreakNode *)n;

        auto isValidContinue = [&]() -> bool{
            StatementBlock *currentScope = scope;
            while (currentScope){
                if (currentScope->subtag == StatementBlock::BLOCK_FOR
                    || currentScope->subtag == StatementBlock::BLOCK_WHILE){
                    return true;
                }
                currentScope = currentScope->parent;

            }
            return false;
        };

        // check if continue is inside a for/while loop
        if (!isValidContinue()){
            logErrorMessage(c->breakToken, "\"continue\" can only be used inside a loop body.");
            errors++;
            return false;
        }

        break;
    }

    case Node::NODE_DECLARATION:{
        Declaration *d = (Declaration*)n;
        
        for (auto &decln: d->decln){
            
            // void type not allowed
            if (decln.type.tag == DataType::TAG_VOID && decln.type.indirectionLevel() == 0){
                logErrorMessage(decln.type.type, "void type is not allowed.");
                errors++;
                continue;
            }


            if (decln.initValue){
                DataType initType = checkSubexprType(decln.initValue, scope);
                if (!canBeConverted(decln.initValue, initType, decln.type, scope)){
                    logErrorMessage(getSubexprToken(decln.initValue), "Cannot initialize variable of type \"%s\" with value of type \"%s\".", 
                        dataTypePrintf(decln.type), dataTypePrintf(initType)
                    );
                    errors++;
                }   
            }
            
            // DataType type = checkSubexprType(decln.count, scope);
            
            // if (!(type == DataTypes::Int)){
            //     logErrorMessage(getSubexprToken(decln.count), "An array must have an integer count.");
            //     errors++;
            //     continue;
            // }

        }
        break;
    }

    case Node::NODE_SUBEXPR:{
        Subexpr *s = (Subexpr *)n;

        checkSubexprType(s, scope);
        break;
    }

    
    default:
        return false;
    }

    return true;

}



