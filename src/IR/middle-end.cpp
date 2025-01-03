#include "ir.h"

#include <utils/utils.h>

/*
    Inserts a typecast node to convert an operand of a binary expression to its resulting type.
    TODO: Add typecasts for stores as well
*/
void insertTypeCast(MIR_Expr *d, Arena* arena){
    MIR_Expr *left = d->binary.left;
    MIR_Expr *right = d->binary.right;

    if (!(left->type == d->type)){
        MIR_Expr *cast = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));
        cast->tag = MIR_Expr::EXPR_CAST;
        cast->cast.from = left->type; 
        cast->cast.to = d->type; 
        cast->cast.expr = left;

        d->binary.left = cast;
    }
    if (!(right->type == d->type)){
        MIR_Expr *cast = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));
        cast->tag = MIR_Expr::EXPR_CAST;
        cast->cast.from = right->type; 
        cast->cast.to = d->type; 
        cast->cast.expr = right;
        
        d->binary.right = cast;
    }
}

/*
    Convert an expression to a given type.
*/
MIR_Expr* typeCastTo(MIR_Expr* expr, DataType to, Arena* arena){
    MIR_Expr* e = expr;
    if (!(expr->type == to)){
        MIR_Expr *cast = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));
        cast->tag = MIR_Expr::EXPR_CAST;
        cast->cast.from = expr->type; 
        cast->cast.to = to; 
        cast->cast.expr = expr;
        
        e = cast;
    }
    return e;
}




/*
    RV64 specific sizes of types.
*/
size_t sizeOfType(DataType d, StatementBlock* scope){
    switch (d.tag)
    {
    case DataType::TAG_ADDRESS:
    case DataType::TAG_PTR:
        return 8;
    case DataType::TAG_ARRAY:
        return d.arrayCount * sizeOfType(*(d.ptrTo), scope);
    case DataType::TAG_PRIMARY:
        if (_match(d.type, TOKEN_CHAR))
            return 1;

        if (_match(d.type, TOKEN_INT)){
            if (d.isSet(DataType::Specifiers::SHORT))
                return 2;
            if (d.isSet(DataType::Specifiers::LONG))
                return 8;
            if (d.isSet(DataType::Specifiers::LONG_LONG))
                return 8;

            return 4;
        }

        if (_match(d.type, TOKEN_FLOAT))
            return 4;

        // Note: long double isnt supported currently
        if (_match(d.type, TOKEN_DOUBLE))
            return 8;

    case DataType::TAG_STRUCT:{

        StatementBlock *structDeclScope = scope->findStructDeclaration(d.structName);
        assert(structDeclScope != NULL);

        return structDeclScope->structs.getInfo(d.structName.string).info.size;
    }

    default:
        break;
    }

    return 8;
}

/*
    RV64 specific alignment of types.
*/
size_t alignmentOfType(DataType d, StatementBlock* scope){
    switch (d.tag)
    {
    case DataType::TAG_ADDRESS:
    case DataType::TAG_PTR:
        return 8;
    case DataType::TAG_ARRAY:
        return alignmentOfType(*(d.ptrTo), scope);
    case DataType::TAG_PRIMARY:
        if (_match(d.type, TOKEN_CHAR))
            return 1;

        if (_match(d.type, TOKEN_INT)){
            if (d.isSet(DataType::Specifiers::SHORT))
                return 2;
            if (d.isSet(DataType::Specifiers::LONG))
                return 8;
            if (d.isSet(DataType::Specifiers::LONG_LONG))
                return 8;

            return 4;
        }

        if (_match(d.type, TOKEN_FLOAT))
            return 4;

        // Note: long double isnt supported currently
        if (_match(d.type, TOKEN_DOUBLE))
            return 8;

    case DataType::TAG_STRUCT:{

        StatementBlock *structDeclScope = scope->findStructDeclaration(d.structName);
        assert(structDeclScope != NULL);

        return structDeclScope->structs.getInfo(d.structName.string).info.alignment;
    }

    default:
        break;
    }

    return 8;
}



/*
    RV-64 specific datatypes
*/
MIR_Datatype convertToLowerLevelType(DataType d, StatementBlock *scope){
    switch (d.tag){
    case DataType::TAG_ADDRESS:
    case DataType::TAG_PTR:
        return MIR_Datatypes::_u64;
        break;
    case DataType::TAG_ARRAY:{
        MIR_Datatype arrayOf = convertToLowerLevelType(*d.ptrTo, scope);
        return MIR_Datatype{
            .tag = MIR_Datatype::TYPE_ARRAY,
            .size = d.arrayCount * arrayOf.size,
            .alignment = arrayOf.alignment, 
        };
    }
    case DataType::TAG_PRIMARY:
        if (d.isSet(DataType::Specifiers::UNSIGNED)){
            if (_match(d.type, TOKEN_CHAR))
                return MIR_Datatypes::_u8;

            if (_match(d.type, TOKEN_INT)){
                if (d.isSet(DataType::Specifiers::SHORT))
                    return MIR_Datatypes::_u16;
                if (d.isSet(DataType::Specifiers::LONG))
                    return MIR_Datatypes::_u64;
                if (d.isSet(DataType::Specifiers::LONG_LONG))
                    return MIR_Datatypes::_u64;

                return MIR_Datatypes::_u32;
            }
        }
        
        if (_match(d.type, TOKEN_CHAR))
            return MIR_Datatypes::_i8;

        if (_match(d.type, TOKEN_INT)){
            if (d.isSet(DataType::Specifiers::SHORT))
                return MIR_Datatypes::_i16;
            if (d.isSet(DataType::Specifiers::LONG))
                return MIR_Datatypes::_i64;
            if (d.isSet(DataType::Specifiers::LONG_LONG))
                return MIR_Datatypes::_i64;

            return MIR_Datatypes::_i32;
        }

        if (_match(d.type, TOKEN_FLOAT))
            return MIR_Datatypes::_f32;

        // Note: long double isnt supported currently
        if (_match(d.type, TOKEN_DOUBLE))
            return MIR_Datatypes::_f64;

    case DataType::TAG_STRUCT:{

        StatementBlock *structDeclScope = scope->findStructDeclaration(d.structName);
        assert(structDeclScope != NULL);
        
        MIR_Datatype structType = MIR_Datatypes::_struct;
        structType.size = structDeclScope->structs.getInfo(d.structName.string).info.size;
        structType.alignment = structDeclScope->structs.getInfo(d.structName.string).info.alignment;
        return structType;
    }

    default:
        break;
    }
    
    assert(false && "Some type hasnt been accounted for.");
    return MIR_Datatypes::_i32;
}



/*
    Expand subexpr nodes to a lower level IR.
*/
MIR_Expr* transformSubexpr(const Subexpr* expr, StatementBlock* scope, Arena* arena){
    if (!expr){
        return NULL;
    }
    
    
    MIR_Expr *d = (MIR_Expr *)arena->alloc(sizeof(MIR_Expr));
    
    switch (expr->subtag){
    case Subexpr::SUBEXPR_BINARY_OP :{
        if (_match(expr->op, TOKEN_DOT)){
            /*
                Struct member load is expanded into 
                a.x

                            LOAD
                           /     \
                         /        \
                offset /           \ base
                     /              \
              OFFSET OF x        ADDRESS    
               in struct           of a 
            
            */ 


            d = transformSubexpr(expr->left, scope, arena);

            DataType structType = d->type;
            assert(structType.tag == DataType::TAG_STRUCT);
            assert(expr->right->subtag == Subexpr::SUBEXPR_LEAF);

            StatementBlock *structDeclScope = scope->findStructDeclaration(structType.structName);
            
            assert(structDeclScope != NULL);
            Struct &structInfo = structDeclScope->structs.getInfo(structType.structName.string).info;
            
            assert(structInfo.members.existKey(expr->right->leaf.string));
            Struct::MemberInfo member = structInfo.members.getInfo(expr->right->leaf.string).info;
            
            d->type = member.type;
            d->_type = convertToLowerLevelType(d->type, scope);

            d->load.size = sizeOfType(d->type, scope);
            d->load.offset += member.offset;
            d->ptag = MIR_Primitive::PRIM_EXPR;
            
            return d;
        }
        
        if (_match(expr->op, TOKEN_ARROW)){
            /*
                Struct member load through pointer is expanded into 
                a->x

                            LOAD
                           /     \
                         /        \
                offset /           \ base
                     /              \
              OFFSET OF x         LOAD    
               in struct            |   \
                                    |    \
                              base  |     \ offset
                                    |      \
                                ADDRESS of  0
                                    a 
            */ 
            
            
            d->load.base = transformSubexpr(expr->left, scope, arena);

            DataType structType = d->load.base->type.getBaseType();
            assert(structType.tag == DataType::TAG_STRUCT);
            assert(expr->right->subtag == Subexpr::SUBEXPR_LEAF);

            StatementBlock *structDeclScope = scope->findStructDeclaration(structType.structName);
            
            assert(structDeclScope != NULL);
            Struct &structInfo = structDeclScope->structs.getInfo(structType.structName.string).info;
            
            assert(structInfo.members.existKey(expr->right->leaf.string));
            Struct::MemberInfo member = structInfo.members.getInfo(expr->right->leaf.string).info;
            
            d->type = member.type;
            d->_type = convertToLowerLevelType(d->type, scope);

            d->load.size = sizeOfType(member.type, scope);
            d->load.offset = member.offset;
            d->tag = MIR_Expr::EXPR_LOAD;
            d->ptag = MIR_Primitive::PRIM_EXPR;
            
            return d;
        }


        if (_match(expr->op, TOKEN_SQUARE_OPEN)){
            /*
                Pointer indexing is expanded into 
                a[i]

                            LOAD
                           /     \
                         /        \
                offset /           \ base
                     /              \
              OFFSET OF a         INDEX    
             if in struct           |   \
                                    |    \
                              base  |     \ index
                                    |      \
                                  LOAD   value of   
                                    |       i
                                    |       
                                    |       
                                ADDRESS     
                                  of a 
            
            
            Arrays have implicit address on the stack so no loaderencing is required.
            Array indexing is expanded into 
                a[i]

                            LOAD
                           /     \
                         /        \
                offset /           \ base
                     /              \
              OFFSET OF a         INDEX    
             if in struct           |   \
                                    |    \
                              base  |     \ index
                                    |      \         
                                ADDRESS  value of
                                  of a      i
            */ 


            MIR_Expr *left = transformSubexpr(expr->left, scope, arena);
            MIR_Expr *right = transformSubexpr(expr->right, scope, arena);
            
            assert(left->type.tag == DataType::TAG_PTR || 
                    left->type.tag == DataType::TAG_ARRAY ||
                    left->type.tag == DataType::TAG_ADDRESS);
        

            d->tag = MIR_Expr::EXPR_LOAD;
            d->type = *(left->type.ptrTo);
            d->_type = convertToLowerLevelType(d->type, scope);


            int64_t loadOffset = 0;

            // if an array, then the address is implicit, so there is no need to load the array 
            if (left->type.tag == DataType::TAG_ARRAY){
                // remove the load but save the offset
                assert(left->tag == MIR_Expr::EXPR_LOAD);
                loadOffset = left->load.offset;
                left = left->load.base;
            }


            MIR_Expr *index = (MIR_Expr *)arena->alloc(sizeof(MIR_Expr));
            index->tag = MIR_Expr::EXPR_INDEX;
            index->index.index = right;
            index->index.base = left;
            index->index.size = sizeOfType(d->type, scope);
            index->type = DataType{.tag = DataType::TAG_ADDRESS};

            d->load.base = index;
            d->load.offset = loadOffset;
            d->load.size = sizeOfType(d->type, scope);
            d->ptag = MIR_Primitive::PRIM_EXPR;

            return d;
        }



        bool isAssignment = _matchv(expr->op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP));
        
        // assignments are converted into stores
        if (isAssignment){
            /*
                Assignment is expanded into 
                
                              STORE
                           /   |    \
                         /     |     \
                offset /       |left  \
                     /         |       \
                    0       ADDRESS    rvalue
                          of lvalue 
            
            
                Something-Assignment is expanded to
                              STORE
                           /   |    \
                         /     |     \
                offset /       |left  \
                     /         |       \
                    0       ADDRESS   BINARY_EXPR
                          of lvalue  /         \
                                   /            \ 
                                  /              \
                               lvalue           rvalue
            */ 



            MIR_Expr *left = transformSubexpr(expr->left, scope, arena);
            MIR_Expr *right = transformSubexpr(expr->right, scope, arena);


            switch (expr->op.type){
            case TOKEN_ASSIGNMENT:{
                break;
            }

            // something-assign are expanded to a store with that node as rvalue   
            case TOKEN_PLUS_ASSIGN:{
                MIR_Expr *add = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));
                MIR_Expr *addLeft = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));
                
                // the new add node
                add->tag = MIR_Expr::EXPR_BINARY;
                add->binary.left = addLeft;
                add->binary.right = right;
                add->binary.op = MIR_Expr::BinaryOp::EXPR_IADD;
                add->type = getResultantType(left->type, right->type, expr->op);
                
                // the left operand is same as the left node of the store
                *addLeft = *left;
                
                // insert typecast if the add operands need to be typecasted
                insertTypeCast(add, arena);
                
                right = add;
                break;
            }
            case TOKEN_MINUS_ASSIGN:{
                break;
            }
            case TOKEN_MUL_ASSIGN:{
                break;
            }
            case TOKEN_DIV_ASSIGN:{
                break;
            }
            case TOKEN_SQUARE_OPEN:{
                break;
            }
            case TOKEN_LSHIFT_ASSIGN:{
                break;
            }
            case TOKEN_RSHIFT_ASSIGN:{
                break;
            }
            case TOKEN_BITWISE_AND_ASSIGN:{
                break;
            }
            case TOKEN_BITWISE_OR_ASSIGN:{
                break;
            }
            case TOKEN_BITWISE_XOR_ASSIGN:{
                break;
            }
            default:
                break;
            }
            
            d->type = left->type;
            d->_type = convertToLowerLevelType(d->type, scope);
            d->tag = MIR_Expr::EXPR_STORE;

            

            /* 
                Any variable node will generate a load node.
                For lvalue, we need an address in the left node of the store node.
                So, remove the load node: 
                - set the base address of the load as the left node. 
                - set the load offset as the offset in the store node.
            */
            assert(left->tag == MIR_Expr::EXPR_LOAD);
            d->store.offset = left->load.offset;
            
            left = left->load.base;
            d->store.left = left;
            d->store.right = right;
            d->store.size = sizeOfType(d->type, scope);
            d->ptag = MIR_Primitive::PRIM_EXPR;

            return d;
        }

        /*
            Else is a normal binary expression expanded to

                   BINARY_EXPR
                   /         \
                 /            \ 
                /              \
             left            right
        */
        
        // expand left and right 
        MIR_Expr *left = transformSubexpr(expr->left, scope, arena);
        MIR_Expr *right = transformSubexpr(expr->right, scope, arena);
        
        // get resultant type
        d->type = getResultantType(left->type, right->type, expr->op);
        d->_type = convertToLowerLevelType(d->type, scope);
        d->tag = MIR_Expr::EXPR_BINARY;

        switch (expr->op.type){
        case TOKEN_PLUS:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IADD;
            break;
        } 
        case TOKEN_MINUS:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_ISUB;
            break;
        } 
        case TOKEN_STAR:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IMUL;
            break;
        } 
        case TOKEN_SLASH:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IDIV;
            break;
        } 
        case TOKEN_MODULO:{
            assert(false);
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IDIV;
            break;
        } 
        case TOKEN_AMPERSAND:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IBITWISE_AND;
            break;
        } 
        case TOKEN_BITWISE_OR:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IBITWISE_OR;
            break;
        } 
        case TOKEN_BITWISE_XOR:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IBITWISE_XOR;
            break;
        } 
        case TOKEN_SHIFT_LEFT:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IBITWISE_LSHIFT;
            break;
        } 
        case TOKEN_SHIFT_RIGHT:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_IBITWISE_RSHIFT;
            break;
        }
        case TOKEN_LOGICAL_AND:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_LOGICAL_AND;
            break;
        }
        case TOKEN_LOGICAL_OR:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_LOGICAL_OR;
            break;
        }
        case TOKEN_EQUALITY_CHECK:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_ICOMPARE_EQ;
            break;
        }
        case TOKEN_NOT_EQUALS:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_ICOMPARE_NEQ;
            break;
        }
        case TOKEN_GREATER_EQUALS:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_ICOMPARE_GE;
            break;
        }
        case TOKEN_GREATER_THAN:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_ICOMPARE_GT;
            break;
        }
        case TOKEN_LESS_EQUALS:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_ICOMPARE_LE;
            break;
        }
        case TOKEN_LESS_THAN:{
            d->binary.op = MIR_Expr::BinaryOp::EXPR_ICOMPARE_LT;
            break;
        }
        



        default:
            break;
        }

        
        d->binary.left = left;
        d->binary.right = right;
        d->ptag = MIR_Primitive::PRIM_EXPR;
        
        // insert type cast if needed
        insertTypeCast(d, arena);

        break;
    }
    
    case Subexpr::SUBEXPR_LEAF :{
        // if variable/identifier
        if (_match(expr->leaf, TOKEN_IDENTIFIER)){
            /*
                Expanded into 
                
                        LOAD 
                offset /      \ base
                     /         \
                    0       ADDRESS_OF
                                |   
                                |
                              LEAF             
                                |
                                |
                               var
            */ 
            
            StatementBlock *varDeclScope = scope->findVarDeclaration(expr->leaf.string);
            assert(varDeclScope != NULL);
            
            d->type = varDeclScope->symbols.getInfo(expr->leaf.string).info;
            d->_type = convertToLowerLevelType(d->type, scope);
            d->tag = MIR_Expr::EXPR_LOAD;
            
            MIR_Expr *leaf = (MIR_Expr*) arena->alloc(sizeof(MIR_Expr));
            leaf->leaf.val = expr->leaf;
            leaf->tag = MIR_Expr::EXPR_LEAF;

            MIR_Expr *address = (MIR_Expr*) arena->alloc(sizeof(MIR_Expr));
            address->addressOf.of = leaf;
            address->tag = MIR_Expr::EXPR_ADDRESSOF;

            d->load.base = address;
            d->load.offset = 0;
            d->load.size = sizeOfType(d->type, scope);
            d->ptag = MIR_Primitive::PRIM_EXPR;
        
            return d;
        }
        
        // else is an immediate value
        /*
            Expanded into 
            
            IMMEDIATE
                |
                |
               val

        */ 


        switch (expr->leaf.type){
            case TOKEN_CHARACTER_LITERAL:
                d->type = DataTypes::Char;
                break;
            case TOKEN_NUMERIC_FLOAT:
                d->type = DataTypes::Float;
                break;
            case TOKEN_NUMERIC_DOUBLE:
                d->type = DataTypes::Double;
                break;
            case TOKEN_NUMERIC_DEC:
            case TOKEN_NUMERIC_BIN:
            case TOKEN_NUMERIC_HEX:
            case TOKEN_NUMERIC_OCT:
                d->type = DataTypes::Int;
                break;
            case TOKEN_STRING_LITERAL:
                d->type = DataTypes::String;
                break;
            default:
                break;
        }

        d->_type = convertToLowerLevelType(d->type, scope);

        d->tag = MIR_Expr::EXPR_LOAD_IMMEDIATE;
        d->immediate.val = expr->leaf;
        d->ptag = MIR_Primitive::PRIM_EXPR;
        
        return d;
    }

    case Subexpr::SUBEXPR_UNARY: {

        MIR_Expr *operand = transformSubexpr(expr->unarySubexpr, scope, arena);
        
        if (_match(expr->unaryOp, TOKEN_STAR)){
            /*
                *x is expanded to

                    LOAD
                base|   \ offset 
                    |    \ 
                    x     0
            */
            d->tag = MIR_Expr::EXPR_LOAD;
            d->type = *(operand->type.ptrTo);
            d->_type = convertToLowerLevelType(d->type, scope);

            d->load.base = operand;
            d->load.offset = 0;
            d->load.size = sizeOfType(d->type, scope);
            d->ptag = MIR_Primitive::PRIM_EXPR;

            return d;
        }

        else if(_match(expr->unaryOp, TOKEN_AMPERSAND)){
            /*
                &x is expanded to

                LOAD_ADDRESS
                    |
                    |
                    x
            */
        
            d->tag = MIR_Expr::EXPR_LOAD_ADDRESS;


            assert(operand->tag == MIR_Expr::EXPR_LOAD);
            d->loadAddress.base = operand->load.base;
            d->loadAddress.offset = operand->load.offset;

            
            d->type.tag = DataType::TAG_ADDRESS;
            d->type.ptrTo = (DataType*) arena->alloc(sizeof(DataType));
            *(d->type.ptrTo) = operand->type;
            d->_type = convertToLowerLevelType(d->type, scope);

            d->ptag = MIR_Primitive::PRIM_EXPR;

            return d;
        }
        
        /*
            Unary expressions are expanded to

            UNARY_EXPR
                |
                |
              expr
        */
        
        d->tag = MIR_Expr::EXPR_UNARY;
        d->unary.unarySubexpr = operand; 
        d->type = operand->type;
        d->_type = convertToLowerLevelType(d->type, scope);
        d->ptag = MIR_Primitive::PRIM_EXPR;



        switch (expr->unaryOp.type){

        case TOKEN_PLUS:
            break;
        case TOKEN_MINUS:
            d->unary.op = MIR_Expr::UnaryOp::EXPR_INEGATE;
            break;
        case TOKEN_STAR:
            break;
        case TOKEN_LOGICAL_NOT:
            d->unary.op = MIR_Expr::UnaryOp::EXPR_LOGICAL_NOT;
            break;
        case TOKEN_BITWISE_NOT:
            d->unary.op = MIR_Expr::UnaryOp::EXPR_IBITWISE_NOT;
            break;
        case TOKEN_AMPERSAND:
            break;
        case TOKEN_PLUS_PLUS:
            break;
        case TOKEN_MINUS_MINUS:
            break;
        
        default:
            break;
        }

        break;
    }

    case Subexpr::SUBEXPR_RECURSE_PARENTHESIS :{
        return transformSubexpr(expr->inside, scope, arena);
    }
    
    case Subexpr::SUBEXPR_FUNCTION_CALL:{
        FunctionCall *fooCall = expr->functionCall;
        assert(false && "Function calls not supported huhu");
        // Function foo = ir->functions.getInfo(fooCall->funcName.string).info;
        
        // d->tag = MIR_Expr::EXPR_FUNCTION_CALL;
        // d->type = foo.returnType;
        // d->functionCall = expr->functionCall;
        break;
    }
        
    
    default:
        break;
    }
    

    return d;

}





/*
    Fill in the offsets of each member of each struct within a scope.
*/
void calcStructMemberOffsets(StatementBlock *scope){
    for (auto &structName: scope->structs.order){

        Struct &structInfo = scope->structs.getInfo(structName).info;
        size_t offset = 0;
        size_t structAlignment = 0;
        
        for(auto &memberName: structInfo.members.order){
            Struct::MemberInfo &member = structInfo.members.getInfo(memberName).info;

            size_t size = sizeOfType(member.type, scope);
            size_t alignment = alignmentOfType(member.type, scope);

            offset = alignUpPowerOf2(offset, alignment);
            member.offset = offset;
            
            structAlignment = max(structAlignment, alignment);
            offset += size;
        }
        
        offset = alignUpPowerOf2(offset, structAlignment);

        structInfo.size = offset;
        structInfo.alignment = structAlignment;
    }
}





struct MIR_Primitives {
    MIR_Primitive* primitives[8];
    int n;
};





MIR_Primitives transformNode(const Node* current, StatementBlock *scope, Arena* arena, MIR_Scope* mScope){
    
    if (!current){
        return MIR_Primitives{.n = 0};
    }

    switch (current->tag){
    
    // convert initialization to assignment
    case Node::NODE_DECLARATION:{
        Declaration* d = (Declaration*) current;
        for (auto const &decln : d->decln){
            if (decln.initValue){
                assert(decln.type.tag != DataType::TAG_STRUCT && "Struct assignments aren't supported currently.");

                MIR_Expr* var = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr)); 
                var->ptag = MIR_Primitive::PRIM_EXPR;
                var->tag = MIR_Expr::EXPR_LEAF;
                var->leaf.val = decln.identifier;
                
                MIR_Expr* left = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr)); 
                left->ptag = MIR_Primitive::PRIM_EXPR;
                left->tag = MIR_Expr::EXPR_ADDRESSOF;
                left->addressOf.of = var;

                MIR_Expr* declnAssignment = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr)); 
                declnAssignment->ptag = MIR_Primitive::PRIM_EXPR;
                declnAssignment->tag = MIR_Expr::EXPR_STORE;
                declnAssignment->store.left = left;
                declnAssignment->store.right = transformSubexpr(decln.initValue, scope, arena);
                declnAssignment->store.size = sizeOfType(decln.type, scope);
                declnAssignment->store.offset = 0;
                
                declnAssignment->store.right = typeCastTo(declnAssignment->store.right, decln.type, arena);
                
                return MIR_Primitives{.primitives = {declnAssignment}, .n = 1};
            }

        }
        return MIR_Primitives{.n = 0};
        break;
    }

    case Node::NODE_STMT_BLOCK:{
        StatementBlock* block = (StatementBlock*) current;
        
        void* mem = arena->alloc(sizeof(MIR_Scope));
        MIR_Scope* mir = new (mem) MIR_Scope;
        mir->ptag = MIR_Primitive::PRIM_SCOPE;
        mir->parent = mScope;
        
        
        calcStructMemberOffsets(block);

        // change datatypes to architecture specific types
        for (auto const &symbolName : block->symbols.order){
            DataType symbolType = block->symbols.getInfo(symbolName).info;
            mir->symbols.add(symbolName, convertToLowerLevelType(symbolType, block));
        }
        
        // change statements
        for (auto const &AST_stmt : block->statements){
            MIR_Primitives stmts = transformNode(AST_stmt, block, arena, mir);
            for (int i = 0; i < stmts.n; i++) {
                mir->statements.push_back(stmts.primitives[i]);
            }
        }


        return MIR_Primitives{.primitives = {mir}, .n = 1};
        break;
    }

    case Node::NODE_RETURN:{
        ReturnNode* AST_rnode = (ReturnNode*) current;
        MIR_Return* rnode = (MIR_Return*) arena->alloc(sizeof(MIR_Return));
        
        rnode->ptag = MIR_Primitive::PRIM_RETURN;
        rnode->returnValue = transformSubexpr(AST_rnode->returnVal, scope, arena);
        rnode->funcName = scope->getParentFunction()->funcName.string;
        
        return MIR_Primitives{.primitives = {rnode}, .n = 1};
        break;
    }
    
    case Node::NODE_SUBEXPR:{
        Subexpr* AST_expr = (Subexpr*) current;
        MIR_Expr* expr = transformSubexpr(AST_expr, scope, arena);
        
        return MIR_Primitives{.primitives = {expr}, .n = 1};
        break;
    }

    case Node::NODE_IF_BLOCK:{
        IfNode* AST_current = (IfNode*) current;
        
        MIR_If* start = NULL;
        MIR_If** inode = &start;
        while (AST_current){
            *inode = (MIR_If*) arena->alloc(sizeof(MIR_If));
            
            MIR_Primitives stmts = transformNode(AST_current->block, scope, arena, mScope);
            assert(stmts.n == 1);
            assert(stmts.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);

            (*inode)->ptag = MIR_Primitive::PRIM_IF;
            (*inode)->condition = transformSubexpr(AST_current->condition, scope, arena);
            (*inode)->scope = (MIR_Scope*) stmts.primitives[0]; 
            (*inode)->next = NULL;
            
            inode = &(*inode)->next;
            AST_current = AST_current->nextIf;
        }
        
        return MIR_Primitives{.primitives = {start}, .n = 1};
        break;
    }


    case Node::NODE_WHILE:{
        WhileNode* AST_wnode = (WhileNode*) current;
        
        MIR_Loop* loop = (MIR_Loop*) arena->alloc(sizeof(MIR_Loop));
        
        MIR_Primitives stmts = transformNode(AST_wnode->block, scope, arena, mScope);
        assert(stmts.n == 1);
        assert(stmts.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);


        loop->ptag = MIR_Primitive::PRIM_LOOP;
        loop->condition = transformSubexpr(AST_wnode->condition, scope, arena);
        loop->scope = (MIR_Scope*) stmts.primitives[0];

        return MIR_Primitives{.primitives = {loop}, .n = 1};
        break;
    }
    
    case Node::NODE_FOR:{
        ForNode* AST_fnode = (ForNode*) current;
        
        MIR_Loop* loop = (MIR_Loop*) arena->alloc(sizeof(MIR_Loop));
        
        // transform body and loop condition
        MIR_Primitives stmts = transformNode(AST_fnode->block, scope, arena, mScope);
        assert(stmts.n == 1);
        assert(stmts.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);
        
        loop->ptag = MIR_Primitive::PRIM_LOOP;
        loop->scope = (MIR_Scope*) stmts.primitives[0];
        loop->condition = transformSubexpr(AST_fnode->exitCondition, scope, arena);
        
        // add the update statement to the loop body
        loop->scope->statements.push_back(transformSubexpr(AST_fnode->update, scope, arena));

        // the init statement
        stmts = transformNode(AST_fnode->block, scope, arena, mScope);
        assert(stmts.n == 1);
        MIR_Primitive* initStmt = stmts.primitives[0];

        return MIR_Primitives{.primitives = {initStmt, loop}, .n = 2};
        break;
    }


    default:
        break;
    }

    assert(false && "Huhu");
    return MIR_Primitives{.n = 0};
}



MIR* transform(AST *ast, Arena *arena){
    MIR* mir = new MIR;
    
    MIR_Primitives global = transformNode(&ast->global, NULL, arena, NULL);
    assert(global.n == 1 && global.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);
    mir->global = (MIR_Scope*) global.primitives[0];


    for (auto &func: ast->functions.entries){
        Function foo = func.second.info;

        MIR_Function f;
        f.funcName = foo.funcName.string;
        f.returnType = convertToLowerLevelType(foo.returnType, &ast->global);
        
        MIR_Primitives scopeNode = transformNode(foo.block, &ast->global, arena, mir->global);
        assert(scopeNode.n == 1 && scopeNode.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);
        f.scope = (MIR_Scope*) scopeNode.primitives[0];

        mir->functions.add(f.funcName, f);
    }

    return mir;
}






