#include "ir.h"

#include <utils/utils.h>



void* scratchPad[1024];


struct MIR_Primitives {
    MIR_Primitive** primitives;
    int n;
};



struct MiddleEnd {
    AST* ast;
    MIR* mir;

    MIR_Expr* typeCastTo(MIR_Expr* expr, MIR_Datatype to, Arena* arena);
    MIR_Datatype convertToLowerLevelType(DataType d, StatementBlock *scope);
    void calcStructMemberOffsets(StatementBlock *scope);
    MIR_Primitives transformSubexpr(const Subexpr* expr, StatementBlock* scope, Arena* arena);
    MIR_Primitives transformNode(const Node* current, StatementBlock *scope, Arena* arena, MIR_Scope* mScope);  
    MIR_Primitives resolveInitializerLists(const Subexpr* expr, DataType d, MIR_Expr* left, size_t offset, StatementBlock* scope, Arena* arena);
    Splice copySplice(Splice s, Arena* arena);
    MIR_Expr* getStoreNode(MIR_Expr* left, MIR_Expr* right, MIR_Datatype dt, size_t storeOffset, Arena* arena);

};


Splice MiddleEnd :: copySplice(Splice s, Arena* arena){
    char* str = (char*) arena->alloc(s.len + 1);
    memcpy_s(str, s.len, s.data, s.len);
    str[s.len] = 0;
    return Splice{.data = str, .len = s.len};
}




/*
    Convert an expression to a given type.
*/
MIR_Expr* MiddleEnd :: typeCastTo(MIR_Expr* expr, MIR_Datatype to, Arena* arena){
    if (!expr || expr->_type.tag == to.tag){
        return expr;
    }

    MIR_Expr* e = expr;
    
    // if conversion from array to pointer, just change the load to a load address 
    if (e->_type.tag == MIR_Datatype::TYPE_ARRAY && to.tag == MIR_Datatype::TYPE_PTR){
        if (e->tag == MIR_Expr::EXPR_LOAD){
            assert(e->tag == MIR_Expr::EXPR_LOAD);
            assert(e->load.base->tag == MIR_Expr::EXPR_ADDRESSOF);

            MIR_Expr loadAddressOfArray = {};
            loadAddressOfArray.ptag = MIR_Primitive::PRIM_EXPR;
            loadAddressOfArray.tag = MIR_Expr::EXPR_LOAD_ADDRESS;
            loadAddressOfArray.loadAddress.base = e->load.base;
            loadAddressOfArray.loadAddress.offset = e->load.offset;
            loadAddressOfArray._type = to;
            
            *e = loadAddressOfArray;
        }
    }
    else{    
        MIR_Expr *cast = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));
        cast->ptag = MIR_Primitive::PRIM_EXPR;
        cast->tag = MIR_Expr::EXPR_CAST;
        cast->_type = to;
        cast->cast._from = expr->_type; 
        cast->cast._to = to; 
        cast->cast.expr = expr;
        
        e = cast;
    
    }
    


    return e;
}


/*
    RV-64 specific datatypes
*/
MIR_Datatype MiddleEnd :: convertToLowerLevelType(DataType d, StatementBlock *scope){
    switch (d.tag){
    case DataType::TAG_ADDRESS:
    case DataType::TAG_PTR:
        return MIR_Datatypes::_ptr;
        break;
    case DataType::TAG_ARRAY:{
        MIR_Datatype arrayOf = convertToLowerLevelType(*d.ptrTo, scope);
        return MIR_Datatype{
            .tag = MIR_Datatype::TYPE_ARRAY,
            .size = d.arrayCount * arrayOf.size,
            .alignment = arrayOf.alignment, 
            .name = "_array"
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
    case DataType::TAG_VOID:{
        return MIR_Datatypes::_void;
    }

    default:
        break;
    }
    
    assertFalse(printf("Some type hasnt been accounted for: %d\n", d.tag));
    return MIR_Datatypes::_i32;
}


MIR_Expr* MiddleEnd :: getStoreNode(MIR_Expr* left, MIR_Expr* right, MIR_Datatype dt, size_t storeOffset, Arena* arena) {
    MIR_Expr* node = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));

    node->_type = dt;

    

    /* 
        Any variable node will generate a load node.
        For lvalue, we need an address in the left node of the store node.
        So, remove the load node: 
        - set the base address of the load as the left node. 
        - set the load offset as the offset in the store node.
    */
    assert(left->tag == MIR_Expr::EXPR_LOAD);
    node->store.offset = left->load.offset + storeOffset;
    node->store.size = dt.size;
    
    node->tag = MIR_Expr::EXPR_STORE;
    left = left->load.base;
    node->store.left = left;
    node->store.right = typeCastTo(right, node->_type, arena);
    node->ptag = MIR_Primitive::PRIM_EXPR;

    return node;
};




MIR_Primitives MiddleEnd :: resolveInitializerLists(const Subexpr* expr, DataType d, MIR_Expr* left, size_t offset, StatementBlock* scope, Arena* arena){
    if (expr->subtag != Subexpr::SUBEXPR_INITIALIZER_LIST){
        MIR_Primitives exprsRight = transformSubexpr(expr, scope, arena);
        assert(exprsRight.n == 1);
        MIR_Expr* right = (MIR_Expr*) exprsRight.primitives[0];
        
        MIR_Datatype dt = convertToLowerLevelType(d, scope);
        
        MIR_Primitives m = {
            .primitives = (MIR_Primitive**)scratchPad,
            .n = 0
        };
        


        m.primitives[m.n] = getStoreNode(left, right, dt, offset, arena);
        m.n++;

        return m;
    }



    bool isStructType = d.tag == DataType::TAG_STRUCT;
    bool isArrayType = d.tag == DataType::TAG_ARRAY;

    MIR_Primitives returnExprs = {.primitives = (MIR_Primitive**)scratchPad, .n = 0};

    InitializerList* initlist = expr->initList;
    std::vector <MIR_Primitive*> exprsCopy;
    if (isStructType){
        StatementBlock* declnScope = scope->findStructDeclaration(d.structName);
        Struct &structInfo = declnScope->structs.getInfo(d.structName.string).info;
        
        
        for (int i=0; i < initlist->values.size(); i++){

            Splice structMemberName = structInfo.members.order[i];
            Struct::MemberInfo &member = structInfo.members.getInfo(structMemberName).info;

            MIR_Primitives exprs = resolveInitializerLists(initlist->values[i], member.type, left, offset + member.offset, scope, arena);
            
            for (int j=0; j<exprs.n; j++){
                exprsCopy.push_back(exprs.primitives[j]);
            }

        }
        

        for (auto &expr : exprsCopy){
            returnExprs.primitives[returnExprs.n] = expr;
            returnExprs.n++;
        }

        return returnExprs;
    }
    else if (isArrayType){
            
        for (int i=0; i<initlist->values.size(); i++){
            size_t sizeOfType = convertToLowerLevelType(*(d.ptrTo), scope).size;

            MIR_Primitives exprs = resolveInitializerLists(initlist->values[i], *(d.ptrTo), left, offset + sizeOfType * i, scope, arena);
            
            for (int j=0; j<exprs.n; j++){
                exprsCopy.push_back(exprs.primitives[j]);
            }

        }

        
    
    }
    else {
        assert(false && "Initializer list/Expected datatype should either be a struct or array type.");
    }

    for (auto &expr : exprsCopy){
        returnExprs.primitives[returnExprs.n] = expr;
        returnExprs.n++;
    }

    return returnExprs;

}




/*
    Expand subexpr nodes to a lower level IR.
*/
MIR_Primitives MiddleEnd :: transformSubexpr(const Subexpr* expr, StatementBlock* scope, Arena* arena){
    if (!expr){
        return MIR_Primitives{.n = 0};
    }
    
    MIR_Primitives returnExprs = {.primitives = (MIR_Primitive**)scratchPad, .n = 0};
    MIR_Expr *d = (MIR_Expr *)arena->alloc(sizeof(MIR_Expr));
    
    switch (expr->subtag){
    case Subexpr::SUBEXPR_INITIALIZER_LIST: {
        std::vector <MIR_Primitive*> values;

        for (auto &value : expr->initList->values){
            MIR_Primitives exprs = transformSubexpr(value, scope, arena);
            assert(exprs.n == 1);
            
            for (int i=0; i<exprs.n; i++){
                values.push_back(exprs.primitives[i]);
            }
        }
        
        for (auto &value : values){
            returnExprs.primitives[returnExprs.n] = value;
            returnExprs.n++;
        }

        break;
    }


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


            MIR_Primitives exprs = transformSubexpr(expr->left, scope, arena);
            assert(exprs.n == 1);
            d = (MIR_Expr*)exprs.primitives[0];

            DataType structType = expr->left->type;
            assert(structType.tag == DataType::TAG_STRUCT);
            assert(expr->right->subtag == Subexpr::SUBEXPR_LEAF);

            StatementBlock *structDeclScope = scope->findStructDeclaration(structType.structName);
            
            assert(structDeclScope != NULL);
            Struct &structInfo = structDeclScope->structs.getInfo(structType.structName.string).info;
            
            assert(structInfo.members.existKey(expr->right->leaf.string));
            Struct::MemberInfo member = structInfo.members.getInfo(expr->right->leaf.string).info;
            
            // d->type = member.type;
            d->_type = convertToLowerLevelType(member.type, scope);

            d->load.size = d->_type.size;
            d->load.offset += member.offset;
            d->ptag = MIR_Primitive::PRIM_EXPR;
            
            returnExprs.primitives[returnExprs.n] = d;
            returnExprs.n++;
            break;
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
            
            MIR_Primitives exprs = transformSubexpr(expr->left, scope, arena);
            assert(exprs.n == 1);
            d->load.base = (MIR_Expr*)exprs.primitives[0];
            
            DataType structType = expr->left->type.getBaseType();

            // DataType structType = d->load.base->type.getBaseType();
            assert(structType.tag == DataType::TAG_STRUCT);
            assert(expr->right->subtag == Subexpr::SUBEXPR_LEAF);

            StatementBlock *structDeclScope = scope->findStructDeclaration(structType.structName);
            
            assert(structDeclScope != NULL);
            Struct &structInfo = structDeclScope->structs.getInfo(structType.structName.string).info;
            
            assert(structInfo.members.existKey(expr->right->leaf.string));
            Struct::MemberInfo member = structInfo.members.getInfo(expr->right->leaf.string).info;
            
            d->type = member.type;
            d->_type = convertToLowerLevelType(member.type, scope);

            d->load.size = d->_type.size;
            d->load.offset = member.offset;
            d->tag = MIR_Expr::EXPR_LOAD;
            d->ptag = MIR_Primitive::PRIM_EXPR;
            
            returnExprs.primitives[returnExprs.n] = d;
            returnExprs.n++;
            break;
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
            
            
            MIR_Primitives exprLeft = transformSubexpr(expr->left, scope, arena);
            assert(exprLeft.n == 1);
            MIR_Expr *left = (MIR_Expr*) exprLeft.primitives[0];

            MIR_Primitives exprRight = transformSubexpr(expr->right, scope, arena);
            assert(exprRight.n == 1);
            MIR_Expr *right = (MIR_Expr*) exprRight.primitives[0];
            
            assert(expr->left->type.tag == DataType::TAG_PTR || 
                    expr->left->type.tag == DataType::TAG_ARRAY ||
                    expr->left->type.tag == DataType::TAG_ADDRESS);
        

            d->tag = MIR_Expr::EXPR_LOAD;
            
            DataType dt = *(expr->left->type.ptrTo);
            d->type = dt;
            d->_type = convertToLowerLevelType(dt, scope);


            int64_t loadOffset = 0;

            // if an array, then the address is implicit, so there is no need to load the array 
            if (expr->left->type.tag == DataType::TAG_ARRAY){
                // remove the load but save the offset
                assert(left->tag == MIR_Expr::EXPR_LOAD);
                loadOffset = left->load.offset;
                left = left->load.base;
            }


            MIR_Expr *index = (MIR_Expr *)arena->alloc(sizeof(MIR_Expr));
            index->tag = MIR_Expr::EXPR_INDEX;
            index->index.index = right;
            index->index.base = left;
            index->index.size = convertToLowerLevelType(dt, scope).size;
            index->type = DataType{.tag = DataType::TAG_ADDRESS};
            index->_type = convertToLowerLevelType(index->type, scope);
            index->ptag = MIR_Primitive::PRIM_EXPR;


            d->load.base = index;
            d->load.offset = loadOffset;
            d->load.size = convertToLowerLevelType(dt, scope).size;
            d->ptag = MIR_Primitive::PRIM_EXPR;

            returnExprs.primitives[returnExprs.n] = d;
            returnExprs.n++;
            break;
        }



        bool isAssignment = _matchv(expr->op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP));
        
        // assignments are converted into stores
        if (isAssignment){
            MIR_Primitives exprLeft = transformSubexpr(expr->left, scope, arena);
            assert(exprLeft.n == 1);
            MIR_Expr *left = (MIR_Expr*) exprLeft.primitives[0];


            if (expr->right->subtag == Subexpr::SUBEXPR_INITIALIZER_LIST){
                return resolveInitializerLists(expr->right, expr->left->type, left, 0, scope, arena);
            }


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

            




            MIR_Primitives exprRight = transformSubexpr(expr->right, scope, arena);
            assert(exprRight.n == 1);
            MIR_Expr* right = (MIR_Expr*) exprRight.primitives[0];

            
            switch (expr->op.type){
            case TOKEN_ASSIGNMENT:{
                break;
            }

            // something-assign are expanded to a store with that node as rvalue   
            case TOKEN_PLUS_ASSIGN:{
                MIR_Expr *add = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));
                MIR_Expr *addLeft = (MIR_Expr*)arena->alloc(sizeof(MIR_Expr));
                
                
                DataType addType = getResultantType(expr->left->type, expr->right->type, expr->op);
                
                add->type = addType;
                add->_type = convertToLowerLevelType(add->type, scope);
                
                // the left operand is same as the left node of the store
                *addLeft = *left;
                
                // the new add node
                add->tag = MIR_Expr::EXPR_BINARY;
                add->binary.left = typeCastTo(addLeft, add->_type, arena);
                add->binary.right = typeCastTo(right, add->_type, arena);
                add->binary.op = MIR_Expr::BinaryOp::EXPR_IADD;
                
                
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


            returnExprs.primitives[returnExprs.n] = getStoreNode(left, right, left->_type, 0, arena);
            returnExprs.n++;
            

            break;

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
        MIR_Primitives exprLeft = transformSubexpr(expr->left, scope, arena);
        assert(exprLeft.n == 1);
        MIR_Expr *left = (MIR_Expr*) exprLeft.primitives[0];

        MIR_Primitives exprRight = transformSubexpr(expr->right, scope, arena);
        assert(exprRight.n == 1);
        MIR_Expr *right = (MIR_Expr*) exprRight.primitives[0];
        
        // get resultant type
        DataType dt = getResultantType(expr->left->type, expr->right->type, expr->op);
        d->type = dt;
        d->_type = convertToLowerLevelType(dt, scope);
                
        d->binary.left = typeCastTo(left, d->_type, arena);
        d->binary.right = typeCastTo(right, d->_type, arena);
        d->binary.size = d->_type.size;
        d->tag = MIR_Expr::EXPR_BINARY;
        d->ptag = MIR_Primitive::PRIM_EXPR;

        bool isIntegerOperation = isIntegerType(d->_type);

        switch (expr->op.type){
        case TOKEN_PLUS:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_IADD : MIR_Expr::BinaryOp::EXPR_FADD;
            break;
        } 
        case TOKEN_MINUS:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_ISUB : MIR_Expr::BinaryOp::EXPR_FSUB;
            break;
        } 
        case TOKEN_STAR:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_IMUL : MIR_Expr::BinaryOp::EXPR_FMUL;
            break;
        } 
        case TOKEN_SLASH:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_IDIV : MIR_Expr::BinaryOp::EXPR_FDIV;
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
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_ICOMPARE_EQ : MIR_Expr::BinaryOp::EXPR_FCOMPARE_EQ;
            d->_type = MIR_Datatypes::_bool;
            break;
        }
        case TOKEN_NOT_EQUALS:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_ICOMPARE_NEQ : MIR_Expr::BinaryOp::EXPR_FCOMPARE_NEQ;
            d->_type = MIR_Datatypes::_bool;
            break;
        }
        case TOKEN_GREATER_EQUALS:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_ICOMPARE_GE : MIR_Expr::BinaryOp::EXPR_FCOMPARE_GE;
            d->_type = MIR_Datatypes::_bool;
            break;
        }
        case TOKEN_GREATER_THAN:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_ICOMPARE_GT : MIR_Expr::BinaryOp::EXPR_FCOMPARE_GT;
            d->_type = MIR_Datatypes::_bool;
            break;
        }
        case TOKEN_LESS_EQUALS:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_ICOMPARE_LE : MIR_Expr::BinaryOp::EXPR_FCOMPARE_LE;
            d->_type = MIR_Datatypes::_bool;
            break;
        }
        case TOKEN_LESS_THAN:{
            d->binary.op = (isIntegerOperation)? MIR_Expr::BinaryOp::EXPR_ICOMPARE_LT : MIR_Expr::BinaryOp::EXPR_FCOMPARE_LT;
            d->_type = MIR_Datatypes::_bool;
            break;
        }
        



        default:
            break;
        }


        returnExprs.primitives[returnExprs.n] = d;
        returnExprs.n++;
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
            
            DataType loadType = varDeclScope->symbols.getInfo(expr->leaf.string).info;
            d->type = loadType;
            d->_type = convertToLowerLevelType(loadType, scope);
            d->tag = MIR_Expr::EXPR_LOAD;
            
            MIR_Expr *leaf = (MIR_Expr*) arena->alloc(sizeof(MIR_Expr));
            leaf->leaf.val = copySplice(expr->leaf.string, arena);
            leaf->tag = MIR_Expr::EXPR_LEAF;
            leaf->ptag = MIR_Primitive::PRIM_EXPR;

            MIR_Expr *address = (MIR_Expr*) arena->alloc(sizeof(MIR_Expr));
            address->addressOf.of = leaf;
            address->tag = MIR_Expr::EXPR_ADDRESSOF;
            address->ptag = MIR_Primitive::PRIM_EXPR;

            d->load.base = address;
            d->load.offset = 0;
            d->load.size = d->_type.size;
            d->ptag = MIR_Primitive::PRIM_EXPR;

            if (isIntegerType(d->_type)){
                d->load.type = MIR_Expr::LoadType::EXPR_ILOAD;
            }
            else if (isFloatType(d->_type)){
                d->load.type = MIR_Expr::LoadType::EXPR_FLOAD;
            }
            else{
                d->load.type = MIR_Expr::LoadType::EXPR_MEMLOAD;
            }

            returnExprs.primitives[returnExprs.n] = d;
            returnExprs.n++;
            break;
        }
        
        // else is an immediate value
        /*
            Expanded into 
            
            IMMEDIATE
                |
                |
               val

        */ 

        DataType dt;
        switch (expr->leaf.type){
            case TOKEN_CHARACTER_LITERAL:
                dt = DataTypes::Char;
                break;
            case TOKEN_NUMERIC_FLOAT:
                dt = DataTypes::Float;
                break;
            case TOKEN_NUMERIC_DOUBLE:
                dt = DataTypes::Double;
                break;
            case TOKEN_NUMERIC_DEC:
            case TOKEN_NUMERIC_BIN:
            case TOKEN_NUMERIC_HEX:
            case TOKEN_NUMERIC_OCT:
                dt = DataTypes::Int;
                break;
            case TOKEN_STRING_LITERAL:
                dt = DataTypes::String;
                break;
            default:
                break;
        }
        
        d->type = dt;
        d->_type = convertToLowerLevelType(dt, scope);

        d->tag = MIR_Expr::EXPR_LOAD_IMMEDIATE;
        d->immediate.val = copySplice(expr->leaf.string, arena);
        d->ptag = MIR_Primitive::PRIM_EXPR;
        
        returnExprs.primitives[returnExprs.n] = d;
        returnExprs.n++;
        break;
    }

    case Subexpr::SUBEXPR_UNARY: {
        
        MIR_Primitives exprs = transformSubexpr(expr->unarySubexpr, scope, arena);
        assert(exprs.n == 1);
        MIR_Expr* operand = (MIR_Expr*)exprs.primitives[0];
        
        if (_match(expr->unaryOp, TOKEN_STAR)){
            /*
                *x is expanded to

                    LOAD
                base|   \ offset 
                    |    \ 
                    x     0
            */
            d->tag = MIR_Expr::EXPR_LOAD;

            DataType dt = *(expr->unarySubexpr->type.ptrTo);

            d->type = dt;
            d->_type = convertToLowerLevelType(dt, scope);

            d->load.base = operand;
            d->load.offset = 0;
            d->load.size = d->_type.size;
            d->ptag = MIR_Primitive::PRIM_EXPR;

            returnExprs.primitives[returnExprs.n] = d;
            returnExprs.n++;
            break;
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
            
            DataType dt;
            
            dt.tag = DataType::TAG_ADDRESS;
            dt.ptrTo = (DataType*) arena->alloc(sizeof(DataType));
            *(dt.ptrTo) = expr->unarySubexpr->type;
            d->type = dt;
            d->_type = convertToLowerLevelType(dt, scope);

            d->ptag = MIR_Primitive::PRIM_EXPR;

            returnExprs.primitives[returnExprs.n] = d;
            returnExprs.n++;
            break;
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
        
        DataType dt = expr->unarySubexpr->type;
        
        d->type = dt;
        d->_type = convertToLowerLevelType(dt, scope);
        d->ptag = MIR_Primitive::PRIM_EXPR;
        
        bool isIntegerOperation = isIntegerType(d->_type);


        switch (expr->unaryOp.type){

        case TOKEN_PLUS:
            break;
        case TOKEN_MINUS:
            d->unary.op = (isIntegerOperation)? MIR_Expr::UnaryOp::EXPR_INEGATE : MIR_Expr::UnaryOp::EXPR_FNEGATE;
            break;
        case TOKEN_STAR:
            break;
        case TOKEN_LOGICAL_NOT:{
            d->unary.op = MIR_Expr::UnaryOp::EXPR_LOGICAL_NOT;
            d->_type = MIR_Datatypes::_bool;
            break;
        }
        case TOKEN_BITWISE_NOT:
            d->unary.op = MIR_Expr::UnaryOp::EXPR_IBITWISE_NOT;
            break;
        case TOKEN_PLUS_PLUS:
            break;
        case TOKEN_MINUS_MINUS:
            break;
        
        default:
            break;
        }

        returnExprs.primitives[returnExprs.n] = d;
        returnExprs.n++;
        break;
    }

    case Subexpr::SUBEXPR_RECURSE_PARENTHESIS :{
        return transformSubexpr(expr->inside, scope, arena);
    }
    
    case Subexpr::SUBEXPR_FUNCTION_CALL:{
        FunctionCall *fooCall = expr->functionCall;
        Function foo = ast->functions.getInfo(fooCall->funcName.string).info;
        
        void* mem = arena->alloc(sizeof(MIR_FunctionCall));
        MIR_FunctionCall* mfooCall = new (mem) MIR_FunctionCall;
        mfooCall->funcName = copySplice(fooCall->funcName.string, arena);
        
        // convert the arguments
        for (int i=0; i<fooCall->arguments.size(); i++){
            DataType reqType = foo.parameters[i].type;
            MIR_Datatype mReqType = convertToLowerLevelType(reqType, scope);
            
            // convert
            MIR_Primitives exprs = transformSubexpr(fooCall->arguments[i], scope, arena);
            assert(exprs.n == 1);
            MIR_Expr* mArg = (MIR_Expr*)exprs.primitives[0];

            // type cast to required type
            mArg = typeCastTo(mArg, mReqType, arena);
            mfooCall->arguments.push_back(mArg);
        }

        
        d->ptag = MIR_Primitive::PRIM_EXPR;
        d->tag = MIR_Expr::EXPR_CALL;
        d->type = foo.returnType;
        d->_type = convertToLowerLevelType(foo.returnType, scope);
        d->functionCall = mfooCall;

        returnExprs.primitives[returnExprs.n] = d;
        returnExprs.n++;
        break;
    }
    
    case Subexpr::SUBEXPR_CAST:{
        d->cast._from = convertToLowerLevelType(expr->expr->type, scope);
        d->cast._to = convertToLowerLevelType(expr->to, scope);
        
        MIR_Primitives exprs = transformSubexpr(expr->expr, scope, arena);
        assert(exprs.n == 1);

        d->cast.expr = (MIR_Expr*)exprs.primitives[0];
        d->ptag = MIR_Primitive::PRIM_EXPR;
        d->tag = MIR_Expr::EXPR_CAST;
        d->_type = d->cast._to;

        returnExprs.primitives[returnExprs.n] = d;
        returnExprs.n++;
        break;
    }
        
    
    default:
        break;
    }
    

    return returnExprs;

}





/*
    Fill in the offsets of each member of each struct within a scope.
*/
void MiddleEnd :: calcStructMemberOffsets(StatementBlock *scope){
    for (auto &structName: scope->structs.order){

        Struct &structInfo = scope->structs.getInfo(structName).info;
        size_t offset = 0;
        size_t structAlignment = 0;
        
        for(auto &memberName: structInfo.members.order){
            Struct::MemberInfo &member = structInfo.members.getInfo(memberName).info;
            
            MIR_Datatype dt = convertToLowerLevelType(member.type, scope);

            size_t size = dt.size;
            size_t alignment = dt.alignment;

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





MIR_Primitives MiddleEnd :: transformNode(const Node* current, StatementBlock *scope, Arena* arena, MIR_Scope* mScope){
    
    if (!current){
        return MIR_Primitives{.n = 0};
    }

    switch (current->tag){
    
    // convert initialization to assignment
    case Node::NODE_DECLARATION:{
        Declaration* d = (Declaration*) current;
        for (auto const &decln : d->decln){
            if (decln.initValue){
                Subexpr left = Subexpr {0};
                left.tag = Node::NODE_SUBEXPR;
                left.subtag = Subexpr::SUBEXPR_LEAF;
                left.leaf = decln.identifier;
                left.type = decln.type;
                
                Subexpr assignment = Subexpr {0};
                assignment.tag = Node::NODE_SUBEXPR;
                assignment.subtag = Subexpr::SUBEXPR_BINARY_OP;
                assignment.op = Token{.type = TOKEN_ASSIGNMENT};
                assignment.left = &left;
                assignment.right = decln.initValue;
                assignment.type = decln.type;

                MIR_Primitives convertedExprs = transformSubexpr(&assignment, scope, arena);

                return convertedExprs;
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

        scratchPad[0] = mir;
        return MIR_Primitives{.primitives = (MIR_Primitive**)&scratchPad[0], .n = 1};
        break;
    }

    case Node::NODE_RETURN:{
        ReturnNode* AST_rnode = (ReturnNode*) current;
        MIR_Return* rnode = (MIR_Return*) arena->alloc(sizeof(MIR_Return));
        
        StatementBlock* parentFunc = scope->getParentFunction();

        rnode->ptag = MIR_Primitive::PRIM_RETURN;
        
        MIR_Primitives retVal = transformSubexpr(AST_rnode->returnVal, scope, arena);
        assert(retVal.n == 1);
        rnode->returnValue = (MIR_Expr*)retVal.primitives[0];
        rnode->funcName = parentFunc->funcName.string;

        // type cast to the return type
        MIR_Datatype retType = convertToLowerLevelType(ast->functions.getInfo(parentFunc->funcName.string).info.returnType, scope);
        rnode->returnValue = typeCastTo(rnode->returnValue, retType, arena);
        
        scratchPad[0] = rnode;
        return MIR_Primitives{.primitives = (MIR_Primitive**)&scratchPad[0], .n = 1};
        break;
    }
    
    case Node::NODE_SUBEXPR:{
        Subexpr* AST_expr = (Subexpr*) current;
        return transformSubexpr(AST_expr, scope, arena);
        break;
    }

    case Node::NODE_IF_BLOCK:{
        IfNode* AST_current = (IfNode*) current;
        
        MIR_If* start = NULL;
        MIR_If** inode = &start;
        while (AST_current){
            *inode = (MIR_If*) arena->alloc(sizeof(MIR_If));

            MIR_Expr* condition = NULL;
            
            if (AST_current->condition) {
                MIR_Primitives exprs = transformSubexpr(AST_current->condition, scope, arena);
                assert(exprs.n == 1);
                condition = (MIR_Expr*) exprs.primitives[0];
            }
            
            MIR_Primitives stmts = transformNode(AST_current->block, scope, arena, mScope);
            assert(stmts.n == 1);
            assert(stmts.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);
            
            (*inode)->ptag = MIR_Primitive::PRIM_IF;
            (*inode)->condition = typeCastTo(condition, MIR_Datatypes::_bool, arena);
            (*inode)->scope = (MIR_Scope*) stmts.primitives[0]; 
            (*inode)->next = NULL;
            
            inode = &(*inode)->next;
            AST_current = AST_current->nextIf;
        }
        
        scratchPad[0] = start;
        return MIR_Primitives{.primitives = (MIR_Primitive**)&scratchPad[0], .n = 1};
        break;
    }


    case Node::NODE_WHILE:{
        WhileNode* AST_wnode = (WhileNode*) current;
        
        MIR_Loop* loop = (MIR_Loop*) arena->alloc(sizeof(MIR_Loop));

        MIR_Primitives exprs = transformSubexpr(AST_wnode->condition, scope, arena);
        assert(exprs.n == 1);
        MIR_Expr* condition = (MIR_Expr*) exprs.primitives[0];
        
        MIR_Primitives stmts = transformNode(AST_wnode->block, scope, arena, mScope);
        assert(stmts.n == 1);
        assert(stmts.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);
        
        loop->ptag = MIR_Primitive::PRIM_LOOP;
        loop->condition = typeCastTo(condition, MIR_Datatypes::_bool, arena);
        loop->scope = (MIR_Scope*) stmts.primitives[0];
        
        scratchPad[0] = loop;
        return MIR_Primitives{.primitives = (MIR_Primitive**)&scratchPad[0], .n = 1};
        break;
    }
    
    case Node::NODE_FOR:{
        ForNode* AST_fnode = (ForNode*) current;
        
        MIR_Loop* loop = (MIR_Loop*) arena->alloc(sizeof(MIR_Loop));
        
        // transform body and loop condition

        MIR_Primitives exprs = transformSubexpr(AST_fnode->exitCondition, scope, arena);
        assert(exprs.n == 1);
        MIR_Expr* condition = (MIR_Expr*) exprs.primitives[0];
        
        exprs = transformSubexpr(AST_fnode->update, scope, arena);
        assert(exprs.n == 1);
        MIR_Expr* update = (MIR_Expr*) exprs.primitives[0];
        
        MIR_Primitives stmts = transformNode(AST_fnode->block, scope, arena, mScope);
        assert(stmts.n == 1);
        assert(stmts.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);
        
        loop->ptag = MIR_Primitive::PRIM_LOOP;
        loop->scope = (MIR_Scope*) stmts.primitives[0];
        loop->condition = typeCastTo(condition, MIR_Datatypes::_bool, arena);

        
        // add the update statement to the loop body
        loop->scope->statements.push_back(update);

        // the init statement
        stmts = transformNode(AST_fnode->init, scope, arena, mScope);
        assert(stmts.n == 1);
        MIR_Primitive* initStmt = stmts.primitives[0];
        
        scratchPad[0] = initStmt;
        scratchPad[1] = loop;
        return MIR_Primitives{.primitives = (MIR_Primitive**)&scratchPad[0], .n = 2};
        break;
    }
    
    case Node::NODE_BREAK:
    case Node::NODE_CONTINUE:{
        assert(false && "break and continue havent been supported huhu");
        break;
    }

    default:
        break;
    }

    assert(false && "Huhu");
    return MIR_Primitives{.n = 0};
}





MIR* transform(AST *ast, Arena *arena){

    MiddleEnd middleEnd;
    middleEnd.ast = ast;
    middleEnd.mir = new MIR;
    
    MIR_Primitives global = middleEnd.transformNode(&ast->global, NULL, arena, NULL);
    assert(global.n == 1 && global.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);
    middleEnd.mir->global = (MIR_Scope*) global.primitives[0];


    for (auto &func: ast->functions.entries){
        Function foo = func.second.info;
        
        
        MIR_Function f;
        f.funcName = foo.funcName.string;
        f.returnType = middleEnd.convertToLowerLevelType(foo.returnType, &ast->global);
        for (auto &param: foo.parameters) {
            f.parameters.push_back(MIR_Function::Parameter{
                .type = middleEnd.convertToLowerLevelType(param.type, &ast->global),
                .identifier = middleEnd.copySplice(param.identifier.string, arena)
            });
        }
        
        // if function definition doesnt exist, then no need to generate
        f.isExtern = true;

        if (foo.block){
            MIR_Primitives scopeNode = middleEnd.transformNode(foo.block, &ast->global, arena, middleEnd.mir->global);
            assert(scopeNode.n == 1 && scopeNode.primitives[0]->ptag == MIR_Primitive::PRIM_SCOPE);
            MIR_Scope* scope = (MIR_Scope*) scopeNode.primitives[0];
            f.parent = scope->parent;
            f.statements = scope->statements;
            f.symbols = scope->symbols;


            f.ptag = MIR_Primitive::PRIM_SCOPE;
            f.isExtern = false;
        }

        middleEnd.mir->functions.add(f.funcName, f);
    }

    return middleEnd.mir;
}










