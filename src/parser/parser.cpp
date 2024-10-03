#include <stdio.h>
#include <assert.h>
#include <cstdarg>

#include <logger/logger.h>

#include "parser.h"



// referenced from https://en.cppreference.com/w/c/language/operator_precedence
int getPrecedence(Token opToken){
    switch (opToken.type){
    case TOKEN_PLUS_PLUS:
    case TOKEN_MINUS_MINUS:
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


// error recovery: skip until the next semi colon, end of scope or until EOF
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



// consumes expected token
bool Parser::expect(TokenType type){
    // unexpected token
    if (!match(type)) {
        if (didError){
            return false;
        }

        logErrorMessage(peekToken(), "Expected token %s but found \"%.*s\"", TOKEN_TYPE_STRING[type], (int)currentToken.string.len, currentToken.string.data);
        errors++;
        
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


bool Parser::isExprStart(){
    return match(TOKEN_IDENTIFIER) || matchv(UNARY_OP_TOKENS, ARRAY_COUNT(UNARY_OP_TOKENS))
            || matchv(LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES));
}


Token Parser::parseStructDefinition(StatementBlock *scope){
    assert(expect(TOKEN_STRUCT));

    Struct s;
    s.defined = false;
    

    if (match(TOKEN_IDENTIFIER)){
        s.structName = consumeToken();
    }
    else {
        logErrorMessage(peekToken(), "Missing struct identifier.");
        errors++;
        s.structName = peekToken();
        s.structName.string.data = "unnamed-type";
        s.structName.string.len = strlen("unnamed-type");
    }
    

    if (match(TOKEN_CURLY_OPEN)){
        expect(TOKEN_CURLY_OPEN);
        

        // TODO: maybe change this to a declaration?
        while (matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))){
            Struct::MemberInfo member;
            member.type = parseDataType(scope);

            if (member.type.tag == DataType::TAG_STRUCT){
                // struct declarations can also occur without identifiers
                if (match(TOKEN_SEMI_COLON)){
                    consumeToken();
                    continue;
                }
                
                // if the struct type is incomplete
                if (member.type.indirectionLevel() == 0 && !findStructDeclaration(member.type.structName, scope)){
                    logErrorMessage(member.type.structName, "Struct \"%.*s\" incomplete.", splicePrintf(member.type.structName.string));
                    errors++;
                }
            }


            do{
                if (match(TOKEN_IDENTIFIER)){
                    member.memberName = consumeToken();
                    s.members.add(member.memberName.string, member);   
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
    if (scope->structs.existKey(s.structName.string)){
        if (s.defined){
            if (scope->structs.getInfo(s.structName.string).info.defined){
                logErrorMessage(s.structName, "Redefinition of struct \"%.*s\".", splicePrintf(s.structName.string));
                errors++; 
            }
            else{
                scope->structs.update(s.structName.string, s);
            }
        }
    }
    else{
        // unnamed structs aren't supported
        if (!compare(s.structName.string, "unnamed-struct"))
            scope->structs.add(s.structName.string, s);
    }

    return s.structName;
}


DataType Parser::parseDataType(StatementBlock *scope){
    assert(matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS)) 
            || matchv(TYPE_MODIFIER_TOKENS, ARRAY_COUNT(TYPE_MODIFIER_TOKENS)));

    bool didError = false;

    DataType d;
    d.specifierFlags = DataType::Specifiers::NONE;
    
    // type modifiers: signed unsigned long and short
    while (matchv(TYPE_MODIFIER_TOKENS, ARRAY_COUNT(TYPE_MODIFIER_TOKENS))){
        if (match(TOKEN_UNSIGNED)){
            if (d.specifierFlags & DataType::Specifiers::UNSIGNED){
                didError = true;
            }
            else{
                d.specifierFlags |= DataType::Specifiers::UNSIGNED;
            }   
        }
        else if (match(TOKEN_SIGNED)){
            if (d.specifierFlags & DataType::Specifiers::SIGNED){
                didError = true;
            }
            else{
                d.specifierFlags |= DataType::Specifiers::SIGNED;
            }
        }
        else if (match(TOKEN_LONG)){
            if (d.specifierFlags & DataType::Specifiers::LONG_LONG){
                didError = true;
            }
            else if (d.specifierFlags & DataType::Specifiers::LONG){
                d.specifierFlags |= DataType::Specifiers::LONG_LONG;
                d.specifierFlags ^= DataType::Specifiers::LONG;
            }
            else{
                d.specifierFlags |= DataType::Specifiers::LONG;
            }
        }
        else if (match(TOKEN_SHORT)){
            if (d.specifierFlags & DataType::Specifiers::SHORT){
                didError = true;
            }
            else{
                d.specifierFlags |= DataType::Specifiers::SHORT;
            }
        }
        consumeToken();
    }

    // long and short conflict
    if ((d.specifierFlags & (DataType::Specifiers::LONG | DataType::Specifiers::LONG_LONG) )
        && (d.specifierFlags & DataType::Specifiers::SHORT)){
        didError = true;
    }
    // signed and unsigned conflict
    if ((d.specifierFlags & (DataType::Specifiers::SIGNED))
        && (d.specifierFlags & DataType::Specifiers::UNSIGNED)){
        didError = true;
    }

    // default type is int if there are specifiers but no type
    if (!matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))){
        d.type = DataTypes::Int.type;
    }
    // if struct then parse the struct type or struct definition as well
    else if (match(TOKEN_STRUCT)){
        d.structName = parseStructDefinition(scope);
        d.tag = DataType::TAG_STRUCT;
    }
    // else other primary data type
    else{
        d.type = consumeToken();
        d.tag = DataType::TAG_PRIMARY;
    }


    // if a pointer
    while (match(TOKEN_STAR)){
        DataType ptr; 
        ptr.tag = DataType::TAG_PTR;
        ptr.ptrTo = new DataType;
        *ptr.ptrTo = d;
        
        d = ptr;
        
        consumeToken(); 
    }
    
    // cannot use type modifiers with floats and doubles 
    if ((d.isSet(DataType::Specifiers::LONG_LONG) || d.isSet(DataType::Specifiers::LONG)
        || d.isSet(DataType::Specifiers::SHORT) || d.isSet(DataType::Specifiers::SIGNED)
        || d.isSet(DataType::Specifiers::UNSIGNED))  
        && (match(d.type, TOKEN_FLOAT) || match(d.type, TOKEN_DOUBLE) 
            || match(d.type, TOKEN_VOID) || match(d.type, TOKEN_STRUCT))){
        
        logErrorMessage(d.type, "Invalid use of modifiers with type \"%.*s\".", splicePrintf(d.type.string));
        return DataTypes::Error;
    }
    // cannot use long, short with char 
    if ((d.isSet(DataType::Specifiers::LONG_LONG) || d.isSet(DataType::Specifiers::LONG)
        || d.isSet(DataType::Specifiers::SHORT)) 
        && match(d.type, TOKEN_CHAR) ){
        logErrorMessage(d.type, "Invalid use of modifiers with type \"%.*s\".", splicePrintf(d.type.string));
        return DataTypes::Error;
    }



    if (didError){
        logErrorMessage(peekToken(), "Invalid combination of type modifiers.");
        errors++;
        return DataTypes::Error;
    }

    // if not unsigned, then signed by default
    if (!d.isSet(DataType::Specifiers::UNSIGNED) && match(d.type, TOKEN_INT)){
        d.specifierFlags |= DataType::Specifiers::SIGNED;
    }

    if (match(d.type, TOKEN_VOID)){
        d.tag = DataType::TAG_VOID;
    }

    return d;
}


Token Parser::getSubexprToken(Subexpr *expr) {
    switch (expr->subtag){
        case Subexpr::SUBEXPR_LEAF:
            return expr->leaf; 
        
        case Subexpr::SUBEXPR_BINARY_OP:
            return expr->op;

        case Subexpr::SUBEXPR_UNARY:
            return expr->unaryOp;

        case Subexpr::SUBEXPR_FUNCTION_CALL:
            return expr->functionCall->funcName;

        case Subexpr::SUBEXPR_RECURSE_PARENTHESIS:
            return getSubexprToken(expr->inside);

        default:
            return expr->leaf;
    }
};



bool Parser::canBeConverted(Subexpr *from, DataType fromType, DataType toType){
    if (fromType.tag == DataType::TAG_ERROR || toType.tag == DataType::TAG_ERROR){
        return false;
    }
    if (fromType.tag == DataType::TAG_VOID || toType.tag == DataType::TAG_VOID){
        return true;
    }

    // void cannot be converted to or from anything
    if ((fromType.tag == DataType::TAG_VOID) || (toType.tag == DataType::TAG_VOID)){
        return false;
    }
    
    Token subexprToken = getSubexprToken(from);

    // pointers can be converted to other pointers and to integers
    if (fromType.indirectionLevel() > 0){
        if (toType.indirectionLevel() > 0){
            // dont log an error with void pointers?
            if (fromType.tag == DataType::TAG_VOID || toType.tag == DataType::TAG_VOID){
                return true;
            }
            else if (fromType.indirectionLevel() != toType.indirectionLevel() || fromType.tag != toType.tag){
                logWarningMessage(subexprToken, "Conversion from pointer of type \"%s\" to \"%s\".",
                                dataTypePrintf(fromType), dataTypePrintf(toType));
            }
            return true;
        }

        if (toType.tag == DataType::TAG_PRIMARY){
            if (match(toType.type, TOKEN_INT) || match(toType.type, TOKEN_CHAR)){
                logWarningMessage(subexprToken, "Conversion from pointer of type \"%s\" to integer type \"%s\".",
                                dataTypePrintf(fromType), dataTypePrintf(toType));
                return true;
            }
        }
        
        return false;
    }

    // structs can only be converted to the same struct
    if (fromType.tag == DataType::TAG_STRUCT){
        if (toType.tag == DataType::TAG_STRUCT && compare(fromType.structName.string, toType.structName.string)){
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
        // primary cannot be converted to structs
        return false;
    }

    return false;


}






// get the expected type of a subexpr while checking for errors
// reference from https://en.cppreference.com/w/c/language/conversion
DataType Parser::checkSubexprType(Subexpr *expr, StatementBlock *scope){
    if (!expr){
        return DataTypes::Void;
    }


    switch (expr->subtag){
    case Subexpr::SUBEXPR_BINARY_OP:{
        
        DataType left = checkSubexprType(expr->left, scope);

        // okay this is a pain in the ass cause the right operand (member name) will not have a type from the identifier itself
        // that member name should not be checked for declaration by itself. 
        // so a valid memberName right operand will require to not be checked and hence, only left operand is checked at first for struct accesses 
        
        // struct accesses: struct.member and struct_ptr->member
        if (matchv(expr->op, STRUCT_ACCESS_OP, ARRAY_COUNT(STRUCT_ACCESS_OP))){
            if (left.tag == DataType::TAG_ERROR){
                return DataTypes::Error;
            }
            
            // left must be of type struct
            if (left.tag != DataType::TAG_STRUCT){
                logErrorMessage(expr->op, "Not a valid struct.");
                errors++;
                return DataTypes::Error;
            }
            
            // left must be struct * if ->
            if (match(expr->op, TOKEN_ARROW)){
                if (left.indirectionLevel() != 1){
                    logErrorMessage(expr->op, "Not a valid struct pointer.");
                    errors++;
                    return DataTypes::Error;
                }
            }
            
            StatementBlock *structDeclScope = findStructDeclaration(left.structName, scope);

            if (!structDeclScope){
                logErrorMessage(expr->op, "Not a valid struct.");
                errors++;
                return DataTypes::Error;
            }
            
            // the right operand must be valid member name, ie, identifier
            if (!(expr->right->subtag == Subexpr::SUBEXPR_LEAF && expr->right->leaf.type == TOKEN_IDENTIFIER)){
                logErrorMessage(expr->op, "Not a valid member identifier.");
                errors++;
                return DataTypes::Error;
            }
            
            Splice memberName = expr->right->leaf.string;
            Struct st = structDeclScope->structs.getInfo(left.structName.string).info;
            
            // the right identifier must be a valid member name in the struct
            if (!st.members.existKey(memberName)){
                logErrorMessage(expr->right->leaf, "No \"%.*s\" member exists in struct \"%.*s\".",
                                splicePrintf(memberName), splicePrintf(st.structName.string));
                errors++;
                return DataTypes::Error;
            }

            DataType memberType = st.members.getInfo(memberName).info.type;

            return memberType;
            
        }



        DataType right = checkSubexprType(expr->right, scope);
        
        
        // if error, just return; dont log any errors
        if (left.tag == DataType::TAG_ERROR || right.tag == DataType::TAG_ERROR){
            return DataTypes::Error;
        }
        // void types cannot be used in operations
        if ((left.tag == DataType::TAG_VOID && left.indirectionLevel() == 0)
            || right.tag == DataType::TAG_VOID && right.indirectionLevel() == 0){
            logErrorMessage(expr->op, "Cannot perform operation \"%.*s\" with void type.", 
                                splicePrintf(expr->op.string));
            errors++;
            return DataTypes::Error;
        }


        // indexing only works with integers
        if (match(expr->op, TOKEN_SQUARE_OPEN)){
            if (!match(right.type,TOKEN_INT)){
                logErrorMessage(expr->op, "Indexing only works with integer type.");
                errors++;
                return DataTypes::Error;
            }
            
            return *(left.ptrTo);
        }


        // check for lvalue validity
        else if (matchv(expr->op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP))){
            auto isValidLvalue = [&](DataType leftOperandType, Subexpr *leftOperand) -> bool{
                // const type
                if (leftOperandType.isSet(DataType::Specifiers::CONST)){
                    return false;
                }
                
                TokenType lvalueOp[] = {
                    TOKEN_ARROW,
                    TOKEN_DOT,
                    TOKEN_SQUARE_OPEN,
                };

                if (leftOperand->subtag == Subexpr::SUBEXPR_BINARY_OP){
                    return matchv(leftOperand->op, lvalueOp, ARRAY_COUNT(lvalueOp));
                }
                
                // single variable identifier
                if (leftOperand->subtag == Subexpr::SUBEXPR_LEAF 
                    && matchv(leftOperand->leaf, LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES))){
                    return false;
                }

                return true;

            };

            if (!isValidLvalue(left, expr->left)){
                logErrorMessage(expr->op, "Not a valid lvalue.");
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

            if (matchv(expr->op, invalidOpForFloatingTypes, ARRAY_COUNT(invalidOpForFloatingTypes))){
                logErrorMessage(expr->op, "The operation \"%.*s\" cannot be used with floating point types.", splicePrintf(expr->op.string));
                errors++;
                return DataTypes::Error;
            }
        }

    

        
        auto getIntegerConversionRank = [&](DataType d) -> int{
            if (d.isSet(DataType::Specifiers::LONG_LONG)){
                return 4;
            }
            if (d.isSet(DataType::Specifiers::LONG)){
                return 3;
            }
            if (!d.isSet(DataType::Specifiers::LONG) && !d.isSet(DataType::Specifiers::SHORT)){
                if (match(d.type, TOKEN_INT)){
                    return 2;
                }
                if (match(d.type, TOKEN_CHAR)){
                    return 0;
                }
                
            }
            if (d.isSet(DataType::Specifiers::SHORT)){
                return 1;
            }
            return -1;
        };


        auto getResultantType = [&](DataType left, DataType right) -> DataType{
            bool didError = false;
            
            // diff level of indirection
            if (left.indirectionLevel() != right.indirectionLevel()){
                // pointer arithmetic 
                // (ptr + int)/(ptr - int)/(ptr += int)/(ptr -= int)
                if (left.indirectionLevel() > 0 && (match(right.type, TOKEN_INT) || match(right.type, TOKEN_CHAR))){ 
                    if (match(expr->op, TOKEN_PLUS) || match(expr->op, TOKEN_MINUS) 
                    || match(expr->op, TOKEN_PLUS_ASSIGN) || match(expr->op, TOKEN_MINUS_ASSIGN) ){
                        return left;
                    }
                    else if (match(expr->op, TOKEN_ASSIGNMENT)){
                        logWarningMessage(expr->op, "Incompatible conversion from integer to pointer.");
                        return left;
                    }
                }
                // (int + ptr) 
                else if (right.indirectionLevel() > 0 && (match(left.type, TOKEN_INT) || match(left.type, TOKEN_CHAR))){
                    if (match(expr->op, TOKEN_PLUS)){
                        return right;
                    }
                    if (match(expr->op, TOKEN_ASSIGNMENT)){
                        logWarningMessage(expr->op, "Incompatible conversion from pointer to integer.");
                        return right;
                    }
                }
                // both pointers of different level of indirection
                // can only assign but log a warning
                else if (left.indirectionLevel() > 0 && right.indirectionLevel() > 0){
                    if (match(expr->op, TOKEN_ASSIGNMENT)){
                        logWarningMessage(expr->op, "Assignment of pointer type \"%s\" to type \"%s\".",
                                            dataTypePrintf(right), dataTypePrintf(left));
                        return left;
                    }
                }

                didError = true;
            }
            // same level of indirection
            else{
                // both are pointers
                if (left.indirectionLevel() > 0){
                    // can assign, but log error if of different types
                    if (match(expr->op, TOKEN_ASSIGNMENT)){
                        if (!(left == right)){
                            logWarningMessage(expr->op, "Assignment of pointer type \"%s\" to type \"%s\".",
                                            dataTypePrintf(right), dataTypePrintf(left));
                        }
                        return left;
                    }

                    if (left == right){
                        // ptr difference: (ptr - ptr)
                        if (match(expr->op, TOKEN_MINUS)){
                            return DataTypes::Long_Long;
                        }
                    }

                    didError = true;
                }
                // same type but not pointers
                else if (left == right){
                    // assignment is defined for all operands of the same type  
                    if (match(expr->op, TOKEN_ASSIGNMENT)){
                        return left;
                    }
                    
                    // all other primary operands work with all other operators 
                    // (floating point exceptions have been handled at the start)
                    if (left.tag == DataType::TAG_PRIMARY){
                        return left;
                    }

                    // if struct, only assignment between same structs is allowed 
                    if (match(left.type, TOKEN_STRUCT)){
                        if (match(TOKEN_ASSIGNMENT) && compare(left.structName.string, right.structName.string)){
                            return left;
                        }
                    }
                    didError = true;

                }
                
                // different types of primary types
                else if (left.tag == DataType::TAG_PRIMARY && right.tag == DataType::TAG_PRIMARY){
                    
                    // for valid assignment operations, the resultant type is the type of the left operand
                    if (matchv(expr->op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP))){
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
                            signedType.specifierFlags |= DataType::Specifiers::UNSIGNED;
                            signedType.specifierFlags ^= DataType::Specifiers::SIGNED;
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
                logErrorMessage(expr->op, "No \"%.*s\" operator defined for type \"%s\" and \"%s\".", 
                                splicePrintf(expr->op.string),
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

        DataType operand = checkSubexprType(expr->unarySubexpr, scope);
        // *ptr
        if (match(expr->unaryOp, TOKEN_STAR)){
            if (operand.indirectionLevel() > 0){
                return *(operand.ptrTo);
            }
            else{
                logErrorMessage(expr->unaryOp, "Cannot be dereferenced. Not a valid pointer.");
                errors++;
                return DataTypes::Error;
            }
        }
        // &var
        else if (match(expr->unaryOp, TOKEN_AMPERSAND)){
            // cannot get the address of an address
            if (operand.tag == DataType::TAG_ADDRESS){
                logErrorMessage(expr->unaryOp, "Cannot get the address of an address literal.");
                errors++;
                return DataTypes::Error;
            }
            
            // cannot get the address of a literal
            if (expr->unarySubexpr->subtag == Subexpr::SUBEXPR_LEAF && 
                matchv(expr->unarySubexpr->leaf, LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES))){
                
                logErrorMessage(expr->unaryOp, "\"%.*s\" is not a valid identifier. Cannot get the address of a literal.",
                                splicePrintf(expr->unarySubexpr->leaf.string));
                errors++;
                return DataTypes::Error;
            }
            
            // &val is valid only if &identifier, &struct.member, &array[i]
            bool isValid = false;
            
            if (expr->unarySubexpr->subtag == Subexpr::SUBEXPR_LEAF && match(expr->unarySubexpr->leaf, TOKEN_IDENTIFIER)){
                isValid = true;
            }
            else if (expr->unarySubexpr->subtag == Subexpr::SUBEXPR_BINARY_OP){
                TokenType VALID_OP[] = {
                    TOKEN_SQUARE_OPEN,
                    TOKEN_DOT,
                    TOKEN_ARROW,
                };
                
                isValid = isValid || matchv(expr->unarySubexpr->op, VALID_OP, ARRAY_COUNT(VALID_OP));
            }
            
            if (isValid){
                DataType d;
                d.tag = DataType::TAG_ADDRESS;
                d.ptrTo = new DataType;
                *(d.ptrTo) = operand;

                return d;
            }
            else{
                logErrorMessage(expr->unaryOp, "Not a valid identifier.");
                errors++;
                return DataTypes::Error;
            }
        }
        // if struct then, no other unary operator other than & are defined
        else if (operand.tag == DataType::TAG_STRUCT){
            return DataTypes::Error;
        }

        return operand;
        break;
    }

    case Subexpr::SUBEXPR_LEAF:{
        // check if identifier has been declared
        auto findVarDeclaration = [&](Splice name) -> StatementBlock *{
            StatementBlock *currentScope = scope;
            while(currentScope){
                if (currentScope->symbols.existKey(name)){
                    return currentScope;
                }
                currentScope = currentScope->parent;
            }
            return NULL;
        };

        if (match(expr->leaf, TOKEN_IDENTIFIER)){
            StatementBlock *varDeclScope = findVarDeclaration(expr->leaf.string);
            
            if (!varDeclScope){
                logErrorMessage(expr->leaf, "Undeclared identifier \"%.*s\"", splicePrintf(expr->leaf.string));
                errors++;

                return DataTypes::Error;
            }

            DataType type = varDeclScope->symbols.getInfo(expr->leaf.string).info;
            return type;
        }
        
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
        if (foo.parameters.size() != fooCall->arguments.size()){
            logErrorMessage(fooCall->funcName, "In function \"%.*s\", required %llu but found %llu arguments.", 
                        splicePrintf(fooCall->funcName.string), foo.parameters.size(), fooCall->arguments.size());
            errors++;
        }
        else{
            // check if arguments are of correct type/can be implicitly converted to the correct type
            auto matchArgType = [&](Function *foo, FunctionCall *fooCall){
                for (int i=0; i<foo->parameters.size(); i++){
                    DataType fromType = checkSubexprType(fooCall->arguments[i], scope);
                    
                    if (!canBeConverted(fooCall->arguments[i], fromType, foo->parameters[i].type)){
                        logErrorMessage(getSubexprToken(fooCall->arguments[i]), "Cannot convert argument of type \"%s\" to \"%s\"",
                                        dataTypePrintf(fromType), dataTypePrintf(foo->parameters[i].type));
                        errors++;
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

}




Subexpr* Parser::parseSubexpr(int precedence, StatementBlock *scope){
    Subexpr *left = (Subexpr*)parsePrimary(scope);

    Subexpr *s = left;
    
    // while next token is an operator and its precedence is higher (value is lower) than current one, add to the tree 
    while (matchv(BINARY_OP_TOKENS, ARRAY_COUNT(BINARY_OP_TOKENS))){
        bool isRtoLAssociative = matchv(ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP));
        isRtoLAssociative = isRtoLAssociative || match(TOKEN_SQUARE_OPEN);
        
        
        // for left to right associativity, break out when next op has a lower or equal precedence than current one
        if (getPrecedence(peekToken()) >= precedence){
            // right to left associativity for assignment operators, ie dont break out for same precedence
            if (isRtoLAssociative){
                if (getPrecedence(peekToken()) > precedence)
                    break;
            }
            else{
                break;
            }

        }
        
        s = new Subexpr;
        s->tag = Node::NODE_SUBEXPR;
        
        s->left = left;
        s->op = consumeToken();
        

        Subexpr *next = (Subexpr*)parseSubexpr(getPrecedence(s->op), scope);

        s->right  = next;
        s->subtag = Subexpr::SUBEXPR_BINARY_OP;
        
        if (match(s->op,TOKEN_SQUARE_OPEN)){
            expect(TOKEN_SQUARE_CLOSE);
        }
        
        if (next->tag == Node::NODE_ERROR){
            s->tag = Node::NODE_ERROR;
        }
        
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
        Token identifier = consumeToken();
        // parse function call
        if (match(TOKEN_PARENTHESIS_OPEN)){
            expect(TOKEN_PARENTHESIS_OPEN);

            FunctionCall *fooCall = new FunctionCall;
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




StatementBlock* Parser::findStructDeclaration(Token structName, StatementBlock *scope){
    StatementBlock *currentScope = scope;
    while (currentScope){
        if (currentScope->structs.existKey(structName.string)){
            if (currentScope->structs.getInfo(structName.string).info.defined){
                return currentScope;
            }
        }

        currentScope = currentScope->parent;
    }
    return NULL;
}



// parse variable declaration, function definition and struct definition
Node* Parser::parseDeclaration(StatementBlock *scope){
    DataType type = parseDataType(scope);
    

    // only struct declaration w/o variable declaration
    if (type.tag == DataType::TAG_STRUCT 
        && match(TOKEN_SEMI_COLON)){
        consumeToken();
        return NULL;
    }

    // check if struct has been defined: only defined structs can be used for declaration
    if (type.tag == DataType::TAG_STRUCT){
        
        if (!findStructDeclaration(type.structName, scope)){
            logErrorMessage(type.structName, "Struct \"%.*s\" incomplete.", splicePrintf(type.structName.string));
            errors++;
        }
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
        
        // parse parameters
        while (matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))){
            Function::Parameter p;
            p.type = parseDataType(scope);
            
            assert(match(TOKEN_IDENTIFIER));
            p.identifier = consumeToken();

            foo.parameters.push_back(p);
            
            
            if (!match(TOKEN_COMMA)){
                break;
            }
            else{
                consumeToken();
            }
        };

        expect(TOKEN_PARENTHESIS_CLOSE);
        
        
        // if definition exists
        if (match(TOKEN_CURLY_OPEN)){
            
            // parse function body
            /* NOTE: i wouldve liked to use parseStatementBlock() but that function is pretty much a standalone function that parses an independent block 
            for function definition, the parameters need to be preadded to the symbol table before calling parseStatementBlock(), 
            which would require modifying the function somehow, hence the repetition
            
            maybe keeping the symboltable as a reference instead of a member in a statement block might work, 
            by ensuring that the table is allocated by the calling function
            */
            foo.block = new StatementBlock;
            foo.block->tag = Node::NODE_STMT_BLOCK;
            foo.block->parent = &ir->global;
            foo.block->subtag = StatementBlock::BLOCK_FUNCTION_BODY;
            foo.block->funcName = foo.funcName;

            // add parameters to symbol table
            for (auto &p : foo.parameters){
                foo.block->symbols.add(p.identifier.string, p.type);
            }

            
            expect(TOKEN_CURLY_OPEN);
            

            while (!match(TOKEN_CURLY_CLOSE)){
                Node *stmt = parseStatement(foo.block);
                if (stmt){
                    foo.block->statements.push_back(stmt);
                }
            }
            expect(TOKEN_CURLY_CLOSE);
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
                            logErrorMessage(foo.funcName, "Number of arguments %llu doesn't match with that in declaration %llu of function \"%.*s\".", 
                                            newFoo.parameters.size(), prevFoo.parameters.size(), splicePrintf(foo.funcName.string));
                            errors++;
                            return false;
                        }
                        
                        // check types of parameters 
                        for (int i=0; i< prevFoo.parameters.size(); i++){
                            if (prevFoo.parameters[i].type != newFoo.parameters[i].type){
                                logErrorMessage(foo.funcName, "Conflicting types of parameter \"%.*s\": \"%s\" and \"%s\".", 
                                                splicePrintf(foo.parameters[i].identifier.string), 
                                                dataTypePrintf(prevFoo.parameters[i].type), dataTypePrintf(newFoo.parameters[i].type));
                                errors++;
                                return false;
                            }
                        }
                        
                        // check return type 
                        if (prevFoo.returnType != newFoo.returnType){
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
                checkSubexprType(var.initValue, scope);

            }


            if (!scope->symbols.existKey(var.identifier.string)){
                d->decln.push_back(var);
                scope->symbols.add(var.identifier.string, d->type);
            }
            else{
                logErrorMessage(var.identifier, "Redeclaration of \"%.*s\" with type \"%s\", previously defined with type \"%s\".",
                                splicePrintf(var.identifier.string), 
                                dataTypePrintf(type), dataTypePrintf(scope->symbols.getInfo(var.identifier.string).info));
            }

                
            
        }while (match(TOKEN_COMMA) && expect(TOKEN_COMMA));
        expect(TOKEN_SEMI_COLON);

        // void type not allowed
        if (type.tag == DataType::TAG_VOID && type.indirectionLevel() == 0){
            errors++;
            logErrorMessage(type.type, "void type is not allowed.");
        }

        return d; 
    }
}


Node* Parser::parseStatement(StatementBlock *scope){
    didError = false;

    Node *statement;
    if (matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))
        || matchv(TYPE_MODIFIER_TOKENS, ARRAY_COUNT(TYPE_MODIFIER_TOKENS))){
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
    else if (match(TOKEN_BREAK)){
        statement = parseBreak(scope);
    }
    else if (match(TOKEN_CONTINUE)){
        statement = parseContinue(scope);
    }
    else if (match(TOKEN_CURLY_OPEN)){
        statement = parseStatementBlock(scope);
    }
    else if (match(TOKEN_SEMI_COLON)){
        statement = NULL;
        consumeToken();
    }
    else if (isExprStart()){
        statement = parseSubexpr(INT32_MAX, scope);

        // if (!didError){
        //     checkSubexprType((Subexpr *)statement, scope);
        // }
        
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
    ReturnNode* r = new ReturnNode;
    r->returnToken = consumeToken();
    r->returnVal = NULL;
    r->tag = Node::NODE_RETURN;
    
    if (!match(TOKEN_SEMI_COLON)){
        r->returnVal = parseSubexpr(INT32_MAX, scope);
    }
    
    expect(TOKEN_SEMI_COLON);

    return r;
}


BreakNode* Parser::parseBreak(StatementBlock *scope){
    BreakNode* b = new BreakNode;
    b->breakToken = consumeToken();
    b->tag = Node::NODE_BREAK;
    
    expect(TOKEN_SEMI_COLON);

    return b;
}

ContinueNode* Parser::parseContinue(StatementBlock *scope){
    ContinueNode* c = new ContinueNode;
    c->continueToken = consumeToken();
    c->tag = Node::NODE_CONTINUE;
    
    expect(TOKEN_SEMI_COLON);

    return c;
}




StatementBlock* Parser::parseStatementBlock(StatementBlock *scope){
    StatementBlock *block = new StatementBlock;
    block->tag = Node::NODE_STMT_BLOCK;
    block->parent = scope;
    block->subtag = StatementBlock::BLOCK_UNNAMED;
    
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
        
        if (isExprStart()){
            ifNode->condition = (Subexpr *)parseSubexpr(INT32_MAX, scope);
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

    ifNode->block = (StatementBlock *)parseStatementBlock(scope);
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
    WhileNode *whileNode = new WhileNode;

    whileNode->tag = Node::NODE_WHILE; 

    expect(TOKEN_WHILE);
    // parse condition
    expect(TOKEN_PARENTHESIS_OPEN);

    if (isExprStart()){
        whileNode->condition = (Subexpr *)parseSubexpr(INT32_MAX, scope);
    }
    else{
        whileNode->condition = NULL;
        logErrorMessage(peekToken(), "Missing expression for while condition.");
        errors++;
    }

    expect(TOKEN_PARENTHESIS_CLOSE);
    
    whileNode->block = (StatementBlock *)parseStatementBlock(scope);
    whileNode->block->subtag = StatementBlock::BLOCK_WHILE;
    whileNode->block->scope  = whileNode;
    

    return whileNode;
}


Node* Parser::parseFor(StatementBlock *scope){
    ForNode *forNode = new ForNode;

    forNode->tag = Node::NODE_FOR; 
    forNode->init = NULL; 
    forNode->exitCondition = NULL; 
    forNode->update = NULL; 

    expect(TOKEN_FOR);
    // parse condition
    expect(TOKEN_PARENTHESIS_OPEN);
    if (isExprStart()){
        forNode->init = (Subexpr *)parseSubexpr(INT32_MAX, scope);
    }
    expect(TOKEN_SEMI_COLON);
    
    if (isExprStart()){
        forNode->exitCondition = (Subexpr *)parseSubexpr(INT32_MAX, scope);
    }
    expect(TOKEN_SEMI_COLON);

    if (isExprStart()){
        forNode->update = (Subexpr *)parseSubexpr(INT32_MAX, scope);
    }
    expect(TOKEN_PARENTHESIS_CLOSE);
    
    forNode->block = (StatementBlock *)parseStatementBlock(scope);
    forNode->block->subtag = StatementBlock::BLOCK_FOR;
    forNode->block->scope  = forNode;
    

    return forNode;
}


IR *Parser::parseProgram(){
    while (peekToken().type != TOKEN_EOF){
        if (matchv(DATA_TYPE_TOKENS, ARRAY_COUNT(DATA_TYPE_TOKENS))){
            Node *stmt = this->parseDeclaration(&ir->global);
            if (stmt){
                ir->global.statements.push_back(stmt);
                checkContext(stmt, &ir->global);
            }
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
    
    fprintf(stdout, "[Parser] %llu errors generated.\n", errors);
    return (errors == 0)? ir : NULL;

}



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
        
        // check if condition expression can be converted to a boolean/arithmetic value
        DataType conditionType = checkSubexprType(i->condition, scope);

        if (!canBeConverted(i->condition, conditionType, DataTypes::Int)){
            Token subexprToken = getSubexprToken(i->condition);
            
            logErrorMessage(subexprToken, "Cannot convert from expression of type \"%s\" to an integer/arithmetic type",
                            dataTypePrintf(conditionType));
            errors++;
        }

        checkContext(i->block, scope);

        break;
    }
    case Node::NODE_WHILE:{
        WhileNode *w = (WhileNode *)n;
        
        // check if condition expression can be converted to a boolean/arithmetic value
        DataType conditionType = checkSubexprType(w->condition, scope);

        if (!canBeConverted(w->condition, conditionType, DataTypes::Int)){
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
        if (!canBeConverted(f->exitCondition, conditionType, DataTypes::Int)){
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

        if (!canBeConverted(r->returnVal, retExprType, expectedRetType)){
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

        // check if return is inside function
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

        // check if return is inside function
        if (!isValidContinue()){
            logErrorMessage(c->breakToken, "\"continue\" can only be used inside a loop body.");
            errors++;
            return false;
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
        case Subexpr::SUBEXPR_BINARY_OP :{
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
    case Node::NODE_DECLARATION: {
        Declaration *d = (Declaration*) current;
        printTabs(depth + 1);

        std::cout<< "type: ";
        for (int level = 0; level < d->type.indirectionLevel(); level++){
            std::cout<<"*";
        }
        std::cout<<d->type.type.string<<"\n";


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
        
        if (b->symbols.count() > 0){
            printTabs(depth+1);
            std::cout<<"Symbol table:\n";
            for (auto &pair : b->symbols.entries){
                printTabs(depth + 2);
                std::cout<<pair.second.identifier <<": ";
                DataType *type = &pair.second.info;
                for (int level = 0; level < type->indirectionLevel(); level++){
                    std::cout<<"*";
                }
                std::cout<<type->type.string<<"\n";
            }
        }
        
        if (b->structs.count() > 0){
            printTabs(depth+1);
            std::cout<<"Struct table:\n";
            for (auto &pair : b->structs.entries){
                Struct strct = pair.second.info;
                printTabs(depth + 2);
                std::cout<<"\tStruct " << strct.structName.string<< "{\n";

                for (auto &m: strct.members.entries){
                    printTabs(depth + 3);
                    std::cout<<m.second.info.memberName.string <<": " << dataTypePrintf(m.second.info.type)<<"\n"; 
                }
                printTabs(depth + 2);
                std::cout<<"\t}\n"; 

            }
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