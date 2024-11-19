#include "code-gen.h"
#include <tokenizer/token.h>


int max(int a, int b){
    return (a>b)?a:b;
}

int min(int a, int b){
    return (a<b)?a:b;
}


// get depth of a Subexpr node
static int getDepth(const Subexpr *expr){
    if (!expr){
        return 0;
    }
    
    switch (expr->subtag){
    case Subexpr::SUBEXPR_FUNCTION_CALL: 
    case Subexpr::SUBEXPR_LEAF: 
        return 1;
    
    case Subexpr::SUBEXPR_BINARY_OP: {
        int left = getDepth(expr->left);
        int right = getDepth(expr->right);

        return max(left, right) + 1;
    }
    case Subexpr::SUBEXPR_UNARY: {
        int operand = getDepth(expr->unarySubexpr);

        return operand + 1;
    }
    case Subexpr::SUBEXPR_RECURSE_PARENTHESIS: {
        int inside = getDepth(expr->inside);

        return inside + 1;
    }
    default:
        return 0;
    }

}


// get depth of an expanded expr node
static int getDepth(const Exp_Expr *expr){
    if (!expr){
        return 0;
    }
    
    switch (expr->tag){
    case Exp_Expr::EXPR_ADDRESSOF: {
        return 1;
    }
    case Exp_Expr::EXPR_DEREF: {
        int operand = getDepth(expr->deref.base);
        return operand + 1;
    }
    case Exp_Expr::EXPR_INDEX: {
        int base = getDepth(expr->index.base);
        int index = getDepth(expr->index.index);
        return max(base, index) + 1;
    }
    case Exp_Expr::EXPR_LEAF: {
        return 1;
    }
    case Exp_Expr::EXPR_LOAD_ADDRESS: {
        int address = getDepth(expr->loadAddress.base);
        return address + 1;
    }
    case Exp_Expr::EXPR_LOAD_IMMEDIATE: {
        return 1;
    }
    case Exp_Expr::EXPR_STORE: {
        int lval = getDepth(expr->store.left);
        int rval = getDepth(expr->store.right);
        return max(lval, rval) + 1;
    }
    case Exp_Expr::EXPR_CAST: {
        int operand = getDepth(expr->cast.expr);
        return operand + 1;
    }
    case Exp_Expr::EXPR_BINARY: {
        int left = getDepth(expr->binary.left);
        int right = getDepth(expr->binary.right);

        return max(left, right) + 1;
    }
    case Exp_Expr::EXPR_UNARY: {
        int operand = getDepth(expr->unary.unarySubexpr);
        return operand + 1;
    }
    case Exp_Expr::EXPR_FUNCTION_CALL: {
        return 1;
    }
    default:
        return 0;
    }

}


/*
    Allocate stack space for the local variables in a block scope.
    Returns the size allocated.
*/
size_t CodeGenerator::allocStackSpace(StatementBlock *scope, ScopeInfo *storage){
    size_t totalSize = 0;
    
    // compute size
    for (auto &var: scope->symbols.order){
        DataType dt = scope->symbols.getInfo(var).info;

        size_t size = sizeOfType(dt, scope);
        size_t alignment = alignmentOfType(dt, scope);

        totalSize = alignUpPowerOf2(totalSize, alignment);
        totalSize += size;
    }

    // align sp to 16 bytes
    totalSize = alignUpPowerOf2(totalSize, 16);

    
    if (totalSize > 0){
        // allocate stack space
        size_t mem = stackAlloc.allocate(totalSize);
        size_t offset = 0;
        
        // assign memory offsets as storage info for each variable 
        for (auto &var: scope->symbols.order){
            DataType dt = scope->symbols.getInfo(var).info;
            
            size_t size = sizeOfType(dt, scope);
            size_t alignment = alignmentOfType(dt, scope);
            
            offset = alignUpPowerOf2(offset, alignment);
            offset += size;

            StorageInfo s;
            s.memAddress = mem + offset;
            s.tag = StorageInfo::STORAGE_MEMORY;

            storage->storage.add(var, s);

        }    

    }

    return totalSize;
}


/*
    Generate assembly for a subexpr node.
*/
void CodeGenerator::generateSubexpr(const Subexpr *expr, StatementBlock *scope, Register dest, ScopeInfo *storageScope){
    arena->createFrame();
    
    // expand into a lower level IR
    Exp_Expr *expanded = expandSubexpr(expr, scope); 
    
    // generate assembly using the lower level IR
    generateExpandedExpr(expanded, dest, scope, storageScope);
    
    arena->destroyFrame();
    
    return;
    
    // after this is no longer used 
    
    if (!expr){
        return;
    }

    switch (expr->subtag){
    case Subexpr::SUBEXPR_LEAF: {
        RV64_Register destReg = regAlloc.resolveRegister(dest);
        const char *destName = RV64_RegisterName[destReg];
        

        if (_matchv(expr->leaf, LITERAL_TOKEN_TYPES, ARRAY_COUNT(LITERAL_TOKEN_TYPES))){
            buffer << "    li " << destName << ", " << expr->leaf.string << "\n";
            return;
        }
        
        if (_match(expr->leaf, TOKEN_IDENTIFIER)){
            StorageInfo sInfo = storageScope->storage.getInfo(expr->leaf.string).info;
            
            int64_t offset = storageScope->frameBase - sInfo.memAddress;

            buffer << "    lw " << destName << ", " << offset << "(fp)" << "\n";
        }

        break;
    }
    case Subexpr::SUBEXPR_RECURSE_PARENTHESIS: {
        generateSubexpr(expr->inside, scope, dest, storageScope);    
        break;
    }
    case Subexpr::SUBEXPR_UNARY: {
        generateSubexpr(expr->unarySubexpr, scope, dest, storageScope);  
        
        RV64_Register destReg = regAlloc.resolveRegister(dest);
        const char *destName = RV64_RegisterName[destReg];
        
        switch (expr->unaryOp.type){

        case TOKEN_PLUS:
            break;
        case TOKEN_MINUS:
            buffer << "    neg " << destName << ", " << destName << "\n";
            break;
        case TOKEN_STAR:
            break;
        case TOKEN_LOGICAL_NOT:
            buffer << "    seqz " << destName << ", " << destName << "\n";
            break;
        case TOKEN_BITWISE_NOT:
            buffer << "    not " << destName << ", " << destName << "\n";
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
    
    case Subexpr::SUBEXPR_BINARY_OP: {
        Register temp = regAlloc.allocVRegister(RegisterType::REG_TEMPORARY);
        
        int leftDepth = getDepth(expr->left);
        int rightDepth = getDepth(expr->right);
        
        if (leftDepth < rightDepth){
            generateSubexpr(expr->right, scope, temp, storageScope);    
            generateSubexpr(expr->left, scope, dest, storageScope);
        }
        else{
            generateSubexpr(expr->left, scope, dest, storageScope);
            generateSubexpr(expr->right, scope, temp, storageScope);
        }

        RV64_Register leftReg = regAlloc.resolveRegister(dest);
        RV64_Register rightReg = regAlloc.resolveRegister(temp);

        const char *lRegName = RV64_RegisterName[leftReg];
        const char *rRegName = RV64_RegisterName[rightReg];
        
        switch (expr->op.type){
        case TOKEN_PLUS:
            buffer << "    add " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_MINUS:
            buffer << "    sub " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_STAR:
            buffer << "    mul " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_SLASH:
            buffer << "    div " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_AMPERSAND:
            buffer << "    and " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_BITWISE_OR:
            buffer << "    or " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_BITWISE_XOR:
            buffer << "    xor " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_MODULO:
            break;
        case TOKEN_SHIFT_LEFT:
            break;
        case TOKEN_SHIFT_RIGHT:
            break;
        
        // TODO: the values may not be boolean
        case TOKEN_LOGICAL_AND:
            buffer << "    and " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_LOGICAL_OR:
            buffer << "    or " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        
        case TOKEN_EQUALITY_CHECK:
            buffer << "    sub " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            buffer << "    seqz " << lRegName << ", " << lRegName << "\n";
            break;
        case TOKEN_NOT_EQUALS:
            buffer << "    sub " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            buffer << "    snez " << lRegName << ", " << lRegName << "\n";
            break;
        case TOKEN_GREATER_EQUALS:
            buffer << "    slt " << lRegName << ", " << rRegName << ", " << lRegName << "\n";
            break;
        case TOKEN_GREATER_THAN:
            buffer << "    sub " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            buffer << "    sgtz " << lRegName << ", " << lRegName << "\n";
            break;
        case TOKEN_LESS_EQUALS:
            buffer << "    sub " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            buffer << "    sgtz " << lRegName << ", " << lRegName << "\n";
            buffer << "    xori " << lRegName << ", " << lRegName << ", " << "1" << "\n";
            break;
        case TOKEN_LESS_THAN:
            buffer << "    slt " << lRegName << ", " << lRegName << ", " << rRegName << "\n";
            break;
        case TOKEN_ASSIGNMENT:
            break;
        case TOKEN_PLUS_ASSIGN:
            break;
        case TOKEN_MINUS_ASSIGN:
            break;
        case TOKEN_MUL_ASSIGN:
            break;
        case TOKEN_DIV_ASSIGN:
            break;
        case TOKEN_SQUARE_OPEN:
            break;
        case TOKEN_LSHIFT_ASSIGN:
            break;
        case TOKEN_RSHIFT_ASSIGN:
            break;
        case TOKEN_BITWISE_AND_ASSIGN:
            break;
        case TOKEN_BITWISE_OR_ASSIGN:
            break;
        case TOKEN_BITWISE_XOR_ASSIGN:
            break;
        case TOKEN_ARROW:
            break;
        case TOKEN_DOT:
            break;
        default:
            break;
        }

        regAlloc.freeRegister(temp);

        break;
    }
    default:
        break;
    }
}


/*
    The instruction suffix for the size of load/store 
*/
const char* sizeSuffix(size_t size){
    /*
        8 bytes = "d"ouble word
        4 bytes = "w"ord
        2 bytes = "h"alf word
        1 bytes = "b"yte
    */
    if (size == 8){
        return "d";
    }
    if (size == 4){
        return "w";
    }
    if (size == 2){
        return "h";
    }
    if (size == 1){
        return "b";
    }
    return "d";
}



/*
    Generate assembly for the expanded IR.
    current : The node to generate assembly for.
    dest    : The register to put the result in.    

*/
void CodeGenerator::generateExpandedExpr(Exp_Expr *current, Register dest, StatementBlock *scope, ScopeInfo *storageScope){
    switch (current->tag)
    {
    case Exp_Expr::EXPR_LOAD_IMMEDIATE:{
        // Puts the immediate value in the destination register.
        RV64_Register destReg = regAlloc.resolveRegister(dest);
        const char *destName = RV64_RegisterName[destReg];
        
        // load immediate value into a register
        buffer << "    li " << destName << ", " << current->immediate.val.string << "\n";
        break;
    }

    case Exp_Expr::EXPR_ADDRESSOF:{
        // Resolve the address of the given variable. Doesn't use the destination register.
        Exp_Expr *of = current->addressOf.of;
        
        if (of->tag == Exp_Expr::EXPR_LEAF){
            // find the scope where the variable is found
            auto getAddressScope = [&](Splice symbol) -> ScopeInfo*{
                ScopeInfo *current = storageScope;

                while (current){
                    if (current->storage.existKey(symbol)){
                        return current;
                    }
                    current = current->parent;
                }

                return 0;
            };
            
            // resolve the variable into an address
            ScopeInfo *storage = getAddressScope(of->leaf.val.string);
            assert(storage != NULL);
            StorageInfo sInfo = storage->storage.getInfo(of->leaf.val.string).info;
            current->addressOf.offset = storage->frameBase - sInfo.memAddress;
            break;
        }

        break;
    }
    
    case Exp_Expr::EXPR_LOAD_ADDRESS:{
        // Loads the destination register with the given address.
        Exp_Expr *base = current->loadAddress.base;
        
        // resolve variable into address/load address into regsister
        generateExpandedExpr(base, dest, scope, storageScope);
        
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        // address is just resolved
        if(base->tag == Exp_Expr::EXPR_ADDRESSOF){
            // load address + offset into a register
            buffer << "    addi " << destName << ", fp, " << base->addressOf.offset + current->loadAddress.offset << "\n";
        }
        // address is loaded into register
        else{
            // load address + offset into a register
            buffer << "    addi " << destName << ", " << destName << ", " << current->loadAddress.offset << "\n";
        }

        break;
    }
    
    case Exp_Expr::EXPR_DEREF:{
        // Dereference the value at given address + offset and put it into the destination register.

        // load/resolve the address into the register first
        generateExpandedExpr(current->deref.base, dest, scope, storageScope); 
        
        
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        // if the given address is a direct AddressOf node, then the address can be used instead of loading it into a register first.
        if (current->deref.base->tag == Exp_Expr::EXPR_ADDRESSOF){
            Exp_Expr *base = current->deref.base;
            
            // dereference the value and load into destination register
            buffer << "    l" << sizeSuffix(current->deref.size) << " " << destName << ", " << base->addressOf.offset + current->deref.offset << "(fp)\n";
            return;
        }

        // dereference value and load   
        buffer << "    l" << sizeSuffix(current->deref.size) << " " << destName << ", " << current->deref.offset << "(" << destName << ")\n";
        
        break;
    }
    
    case Exp_Expr::EXPR_STORE:{
        // Store the given rvalue in the address of the lvalue, and also load it into the destination register.

        // Load the rvalue into the destination register
        generateExpandedExpr(current->store.right, dest, scope, storageScope);
        

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        // if the lvalue has a direct address, use that directly instead of loading it to a register first
        if (current->store.left->tag == Exp_Expr::EXPR_ADDRESSOF){
            // resolve the base adddress
            generateExpandedExpr(current->store.left, dest, scope, storageScope);
            Exp_Expr *base = current->store.left;
            
            // store the value at (address + offset)
            buffer << "    s" << sizeSuffix(current->store.size) << " " << destName << ", " << base->addressOf.offset + current->store.offset << "(fp)\n";
            return;
        }
        
        // else, load the address into a temporary register first 
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        
        // get address of lvalue
        generateExpandedExpr(current->store.left, temp, scope, storageScope);    
        

        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];

        // store the value at (address + offset)
        buffer << "    s" << sizeSuffix(current->store.size) << " " << destName << ", " << current->store.offset << "(" << tempName << ")\n";
        
        regAlloc.freeRegister(temp);
        break;
    }
    
    case Exp_Expr::EXPR_INDEX:{
        // Adds a given index to a given address, to get the correct offset.

        
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        
        // load the given base address into register
        generateExpandedExpr(current->index.base, dest, scope, storageScope);


        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        if (current->index.base->tag == Exp_Expr::EXPR_ADDRESSOF){
            Exp_Expr *address = current->index.base;
            buffer << "    addi " << destName << ", fp, " << address->addressOf.offset << "\n";
        }


        // calculate index and load it into register
        generateExpandedExpr(current->index.index, temp, scope, storageScope);
        

        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];
        
        // multiply the index with the size to get correct offset 
        if (current->index.size > 1){
            // index x size
            Register indexSize = regAlloc.allocVRegister(REG_TEMPORARY);
            const char *indexName = RV64_RegisterName[regAlloc.resolveRegister(indexSize)];
            
            buffer << "    li " <<  indexName << ", " << current->index.size << "\n";
            buffer << "    mul " << tempName << ", " << tempName << ", " << indexName << "\n";
            
            regAlloc.freeRegister(indexSize);
            
        }

        
        // add the offset to get the correct address
        buffer << "    add " << destName << ", " << destName << ", " << tempName << "\n";

        regAlloc.freeRegister(temp);
        break;
    }
    
    case Exp_Expr::EXPR_BINARY:{
        // Computes a binary operation and stores the result in the destination register.

        
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        
        int leftDepth = getDepth(current->binary.left);
        int rightDepth = getDepth(current->binary.right);

        // Generate the one with the greatest depth first so that intermediate values need not be stored.
        if (leftDepth < rightDepth){
            generateExpandedExpr(current->binary.right, temp, scope, storageScope);
            generateExpandedExpr(current->binary.left, dest, scope, storageScope);
        }
        else{
            generateExpandedExpr(current->binary.left, dest, scope, storageScope);
            generateExpandedExpr(current->binary.right, temp, scope, storageScope);
        }

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];
        
        switch (current->binary.op){
            case Exp_Expr::BinaryOp::EXPR_UADD:
            case Exp_Expr::BinaryOp::EXPR_IADD:{
                buffer << "    add " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_USUB:
            case Exp_Expr::BinaryOp::EXPR_ISUB:{
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_UMUL:
            case Exp_Expr::BinaryOp::EXPR_IMUL:{
                buffer << "    mul " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_UDIV:
            case Exp_Expr::BinaryOp::EXPR_IDIV:{
                buffer << "    div " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }

            
            case Exp_Expr::BinaryOp::EXPR_IBITWISE_AND:{
                buffer << "    and " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_IBITWISE_OR:{
                buffer << "    or " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_IBITWISE_XOR:{
                buffer << "    xor " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            

            // TODO: boolean values are considered to be 0 or 1, the cast would convert all other values into these
            case Exp_Expr::BinaryOp::EXPR_LOGICAL_AND:{
                buffer << "    and " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_LOGICAL_OR:{
                buffer << "    or " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            
            case Exp_Expr::BinaryOp::EXPR_IBITWISE_LSHIFT:{
                buffer << "    sll " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_IBITWISE_RSHIFT:{
                buffer << "    srl " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            
            
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_LT:{
                // set if less than
                buffer << "    slt " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_GT:{
                // subtract and set if greater than 0
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                buffer << "    sgtz " << destName << ", " << destName << "\n";
                break;
            } 
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_LE:{
                // subtract and set if greater than 0 (aka gt) then xor with 0x1 
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                buffer << "    sgtz " << destName << ", " << destName << "\n";
                buffer << "    xori " << destName << ", " << destName << ", " << "1" << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_GE:{
                // set if less than with operands swapped
                buffer << "    slt " << destName << ", " << tempName << ", " << destName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_EQ:{
                // subtract and set if eq to 0
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                buffer << "    seqz " << destName << ", " << destName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_NEQ:{
                // subtract and set if neq to 0
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                buffer << "    snez " << destName << ", " << destName << "\n";
                break;
            }

            case Exp_Expr::BinaryOp::EXPR_FADD:
            case Exp_Expr::BinaryOp::EXPR_FSUB:
            case Exp_Expr::BinaryOp::EXPR_FMUL:
            case Exp_Expr::BinaryOp::EXPR_FDIV:
            case Exp_Expr::BinaryOp::EXPR_FCOMPARE_LT:
            case Exp_Expr::BinaryOp::EXPR_FCOMPARE_GT:
            case Exp_Expr::BinaryOp::EXPR_FCOMPARE_LE:
            case Exp_Expr::BinaryOp::EXPR_FCOMPARE_GE:
            case Exp_Expr::BinaryOp::EXPR_FCOMPARE_EQ:
            case Exp_Expr::BinaryOp::EXPR_FCOMPARE_NEQ:
                assert(false && "Floating point operations unimplemented.");
                break;
            default:
                break;
        }

        regAlloc.freeRegister(temp);
        break;
    }
    
    case Exp_Expr::EXPR_CAST:{
        assert(false && "Casts not implemented yet.");
        break;
    }
    case Exp_Expr::EXPR_UNARY:{
        // load the expr into register
        generateExpandedExpr(current->unary.unarySubexpr, dest, scope, storageScope);

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        switch (current->unary.op){
            case Exp_Expr::UnaryOp::EXPR_INEGATE:{
                buffer << "    neg " << destName << ", " << destName << "\n";
                break;
            }
            case Exp_Expr::UnaryOp::EXPR_IBITWISE_NOT:{
                buffer << "    not " << destName << ", " << destName << "\n";
                break;
            }
            case Exp_Expr::UnaryOp::EXPR_LOGICAL_NOT:{
                // set if equal to 0
                buffer << "    seqz " << destName << ", " << destName << "\n";
                break;
            }
            default:
                break;
        }

        break;
    }
    case Exp_Expr::EXPR_FUNCTION_CALL:{
        assert(false && "Function calls not implemented yet.");
        break;
    }
    default:
        break;
    }
}



/*
    Generate assembly for node.
*/
void CodeGenerator::generateNode(const Node *current, StatementBlock *scope, ScopeInfo *storageScope){
    if (!current){
        return;
    }

    switch (current->tag){
    
    case Node::NODE_DECLARATION:{
        // No assembly generated for declaration in of itself. The stack allocation is done at start of scope.

        Declaration *d = (Declaration *)current;
        
        for (auto &decl: d->decln){
            // generate asm to store init values
            // TODO: Convert this to a separate assignment in parser.
            if (decl.initValue){
                Register temp = regAlloc.allocVRegister(RegisterType::REG_TEMPORARY);
                
                generateSubexpr(decl.initValue, scope, temp, storageScope);
                            
                const char *name = RV64_RegisterName[regAlloc.resolveRegister(temp)]; 
                
                StorageInfo sInfo = storageScope->storage.getInfo(decl.identifier.string).info;
                int64_t offset = storageScope->frameBase - sInfo.memAddress;
                
                // Note: This is a 32-bit store and wont work for others.
                buffer << "    sw " << name << ", " << offset << "(fp)" << "\n";
                regAlloc.freeRegister(temp);
            }
        }
        break;
    }

    case Node::NODE_STMT_BLOCK:{
        // Allocate stack space, assign memory offsets to each variable, and generate asm for each statement.

        StatementBlock *b = (StatementBlock *)current;
        
        // fill in the struct member offsets and the struct size
        calcStructMemberOffsets(b);

        ScopeInfo currentStorageScope;
        // the frame is the same
        currentStorageScope.frameBase = storageScope->frameBase;
        currentStorageScope.parent = storageScope;

        // allocate stack space for all variables
        size_t totalSize = allocStackSpace(b, &currentStorageScope);
        buffer << "    addi sp, sp, -" << totalSize << "\n"; 
        
        // generate statements
        for (auto &stmt : b->statements){
            generateNode(stmt, scope, &currentStorageScope);
        }

        // deallocate stack space
        buffer << "    addi sp, sp, " << totalSize << "\n"; 
        stackAlloc.deallocate(totalSize);

        break;
    }

    case Node::NODE_RETURN:{
        // Loads return value into A0 and jump to function epilogue.

        ReturnNode *r = (ReturnNode *)current;
        
        // claim register A0 for return value
        Register a0 = regAlloc.allocRegister(REG_A0);
        
        // load return value into register A0
        generateSubexpr(r->returnVal, scope, a0, storageScope);

        StatementBlock *funcScope = scope->getParentFunction();
        // jump to function epilogue instead of ret
        buffer << "    j ."<< funcScope->funcName.string << "_ep\n";
        regAlloc.freeRegister(a0);
        break;
    }
    
    case Node::NODE_SUBEXPR:{
        // Generate assembly for expression.

        Subexpr *s = (Subexpr *)current;
        
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        generateSubexpr(s, scope, temp, storageScope);

        regAlloc.freeRegister(temp);

        break;
    }

    default:
        break;
    }
}



/*
    Generate assembly for a function.
*/
void CodeGenerator::generateFunction(Function *foo, ScopeInfo *storageScope){
    buffer << "    .globl " << foo->funcName.string << "\n";
    buffer << foo->funcName.string << ":\n";
    // function prologue
    buffer << "    addi sp, sp, -16\n"; // allocate stack space for return address and previous frame pointer.
    buffer << "    sd ra, 8(sp)\n";     // save return address
    buffer << "    sd fp, 0(sp)\n";     // save prev frame pointer
    buffer << "    mv fp, sp\n";        // save current stack pointer 

    // Moving the parameters to registers
    for (int i = 5; auto &param : foo->parameters)
    {
        buffer << "mv a" << (i--) << ", a0\n";
    }

    generateNode(foo->block, foo->block, storageScope);

    // function epilogue
    buffer << "."<<foo->funcName.string << "_ep:\n";
    buffer << "    mv sp, fp\n";       // restore stack pointer
    buffer << "    ld fp, 0(sp)\n";    // restore previous frame pointer
    buffer << "    ld ra, 8(sp)\n";    // restore return address
    buffer << "    addi sp, sp, 16\n"; // deallocate stack space
    buffer << "    ret\n";             // return from function
}


/*
    Write assembly out to a given file.
*/
void CodeGenerator::writeAssemblyToFile(const char *filename){
    std::ofstream outFile(filename);
    if (outFile.is_open())
    {
        outFile << outputBuffer.str(); // Write the combined assembly to file
        outFile.close();
        std::cout << "Assembly written to " << filename << std::endl;
    }
    else
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}

void CodeGenerator::printAssembly(){
    std::cout << outputBuffer.str() << std::endl;
}


void CodeGenerator::generateAssembly(AST *ir){
    this->ir = ir;

    calcStructMemberOffsets(&ir->global);

    ScopeInfo s;
    s.frameBase = 0;
    s.parent = 0;

    outputBuffer << "    .text\n";


    for (auto &pair: ir->functions.entries){
        generateFunction(&pair.second.info, &s);

        Function *foo = &pair.second.info;

        // Appending the function assembly to outputBuffer
        outputBuffer << buffer.str();
        buffer.str("");
        buffer.clear();
        
    }
}


/*
    Inserts a typecast node to convert an operand of a binary expression to its resulting type.
    TODO: Add typecasts for stores as well
*/
void CodeGenerator::insertTypeCast(Exp_Expr *d){
    Exp_Expr *left = d->binary.left;
    Exp_Expr *right = d->binary.right;

    if (!(left->type == d->type)){
        Exp_Expr *cast = (Exp_Expr*)arena->alloc(sizeof(Exp_Expr));
        cast->tag = Exp_Expr::EXPR_CAST;
        cast->cast.from = left->type; 
        cast->cast.to = d->type; 
        cast->cast.expr = left;

        d->binary.left = cast;
    }
    if (!(right->type == d->type)){
        Exp_Expr *cast = (Exp_Expr*)arena->alloc(sizeof(Exp_Expr));
        cast->tag = Exp_Expr::EXPR_CAST;
        cast->cast.from = right->type; 
        cast->cast.to = d->type; 
        cast->cast.expr = right;
        
        d->binary.right = cast;
    }
}


/*
    Fill in the offsets of each member of each struct within a scope.
*/
void CodeGenerator::calcStructMemberOffsets(StatementBlock *scope){
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







/*
    Expand subexpr nodes to a lower level IR.
*/
Exp_Expr* CodeGenerator::expandSubexpr(const Subexpr *expr, StatementBlock *scope){
    Exp_Expr *d = (Exp_Expr *)arena->alloc(sizeof(Exp_Expr));
    
    switch (expr->subtag){
    case Subexpr::SUBEXPR_BINARY_OP :{
        if (_match(expr->op, TOKEN_DOT)){
            /*
                Struct member dereference is expanded into 
                a.x

                            DEREF
                           /     \
                         /        \
                offset /           \ base
                     /              \
              OFFSET OF x        ADDRESS    
               in struct           of a 
            
            */ 


            d = expandSubexpr(expr->left, scope);

            DataType structType = d->type;
            assert(structType.tag == DataType::TAG_STRUCT);
            assert(expr->right->subtag == Subexpr::SUBEXPR_LEAF);

            StatementBlock *structDeclScope = scope->findStructDeclaration(structType.structName);
            
            assert(structDeclScope != NULL);
            Struct &structInfo = structDeclScope->structs.getInfo(structType.structName.string).info;
            
            assert(structInfo.members.existKey(expr->right->leaf.string));
            Struct::MemberInfo member = structInfo.members.getInfo(expr->right->leaf.string).info;
            
            d->type = member.type;
            d->deref.size = sizeOfType(member.type, scope);
            d->deref.offset += member.offset;
            return d;
        }
        
        if (_match(expr->op, TOKEN_ARROW)){
            /*
                Struct member dereference through pointer is expanded into 
                a->x

                            DEREF
                           /     \
                         /        \
                offset /           \ base
                     /              \
              OFFSET OF x         DEREF    
               in struct            |   \
                                    |    \
                              base  |     \ offset
                                    |      \
                                ADDRESS of  0
                                    a 
            */ 
            
            
            d->deref.base = expandSubexpr(expr->left, scope);

            DataType structType = d->deref.base->type.getBaseType();
            assert(structType.tag == DataType::TAG_STRUCT);
            assert(expr->right->subtag == Subexpr::SUBEXPR_LEAF);

            StatementBlock *structDeclScope = scope->findStructDeclaration(structType.structName);
            
            assert(structDeclScope != NULL);
            Struct &structInfo = structDeclScope->structs.getInfo(structType.structName.string).info;
            
            assert(structInfo.members.existKey(expr->right->leaf.string));
            Struct::MemberInfo member = structInfo.members.getInfo(expr->right->leaf.string).info;
            
            d->type = member.type;
            d->deref.size = sizeOfType(member.type, scope);
            d->deref.offset = member.offset;
            d->tag = Exp_Expr::EXPR_DEREF;
            return d;
        }


        if (_match(expr->op, TOKEN_SQUARE_OPEN)){
            /*
                Pointer indexing is expanded into 
                a[i]

                            DEREF
                           /     \
                         /        \
                offset /           \ base
                     /              \
              OFFSET OF a         INDEX    
             if in struct           |   \
                                    |    \
                              base  |     \ index
                                    |      \
                                  DEREF   value of   
                                    |       i
                                    |       
                                    |       
                                ADDRESS     
                                  of a 
            
            
            Arrays have implicit address on the stack so no dereferencing is required.
            Array indexing is expanded into 
                a[i]

                            DEREF
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


            Exp_Expr *left = expandSubexpr(expr->left, scope);
            Exp_Expr *right = expandSubexpr(expr->right, scope);
            
            assert(left->type.tag == DataType::TAG_PTR || 
                    left->type.tag == DataType::TAG_ARRAY ||
                    left->type.tag == DataType::TAG_ADDRESS);
        

            d->tag = Exp_Expr::EXPR_DEREF;
            d->type = *(left->type.ptrTo);

            int64_t derefOffset = 0;

            // if an array, then the address is implicit, so there is no need to deref the array 
            if (left->type.tag == DataType::TAG_ARRAY){
                // remove the deref but save the offset
                assert(left->tag == Exp_Expr::EXPR_DEREF);
                derefOffset = left->deref.offset;
                left = left->deref.base;
            }


            Exp_Expr *index = (Exp_Expr *)arena->alloc(sizeof(Exp_Expr));
            index->tag = Exp_Expr::EXPR_INDEX;
            index->index.index = right;
            index->index.base = left;
            index->index.size = sizeOfType(d->type, scope);
            index->type = DataType{.tag = DataType::TAG_ADDRESS};

            d->deref.base = index;
            d->deref.offset = derefOffset;
            d->deref.size = sizeOfType(d->type, scope);

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



            Exp_Expr *left = expandSubexpr(expr->left, scope);
            Exp_Expr *right = expandSubexpr(expr->right, scope);


            switch (expr->op.type){
            case TOKEN_ASSIGNMENT:{
                break;
            }

            // something-assign are expanded to a store with that node as rvalue   
            case TOKEN_PLUS_ASSIGN:{
                Exp_Expr *add = (Exp_Expr*)arena->alloc(sizeof(Exp_Expr));
                Exp_Expr *addLeft = (Exp_Expr*)arena->alloc(sizeof(Exp_Expr));
                
                // the new add node
                add->tag = Exp_Expr::EXPR_BINARY;
                add->binary.left = addLeft;
                add->binary.right = right;
                add->binary.op = Exp_Expr::BinaryOp::EXPR_IADD;
                add->type = getResultantType(left->type, right->type, expr->op);
                
                // the left operand is same as the left node of the store
                *addLeft = *left;
                
                // insert typecast if the add operands need to be typecasted
                insertTypeCast(add);
                
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
            d->tag = Exp_Expr::EXPR_STORE;
            

            /* 
                Any variable node will generate a deref node.
                For lvalue, we need an address in the left node of the store node.
                So, remove the deref node: 
                - set the base address of the deref as the left node. 
                - set the deref offset as the offset in the store node.
            */
            assert(left->tag == Exp_Expr::EXPR_DEREF);
            d->store.offset = left->deref.offset;
            
            left = left->deref.base;
            d->store.left = left;
            d->store.right = right;
            d->store.size = sizeOfType(d->type, scope);

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
        Exp_Expr *left = expandSubexpr(expr->left, scope);
        Exp_Expr *right = expandSubexpr(expr->right, scope);
        
        // get resultant type
        d->type = getResultantType(left->type, right->type, expr->op);
        d->tag = Exp_Expr::EXPR_BINARY;

        switch (expr->op.type){
        case TOKEN_PLUS:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IADD;
            break;
        } 
        case TOKEN_MINUS:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_ISUB;
            break;
        } 
        case TOKEN_STAR:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IMUL;
            break;
        } 
        case TOKEN_SLASH:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IDIV;
            break;
        } 
        case TOKEN_MODULO:{
            assert(false);
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IDIV;
            break;
        } 
        case TOKEN_AMPERSAND:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IBITWISE_AND;
            break;
        } 
        case TOKEN_BITWISE_OR:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IBITWISE_OR;
            break;
        } 
        case TOKEN_BITWISE_XOR:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IBITWISE_XOR;
            break;
        } 
        case TOKEN_SHIFT_LEFT:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IBITWISE_LSHIFT;
            break;
        } 
        case TOKEN_SHIFT_RIGHT:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_IBITWISE_RSHIFT;
            break;
        }
        case TOKEN_LOGICAL_AND:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_LOGICAL_AND;
            break;
        }
        case TOKEN_LOGICAL_OR:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_LOGICAL_OR;
            break;
        }
        case TOKEN_EQUALITY_CHECK:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_ICOMPARE_EQ;
            break;
        }
        case TOKEN_NOT_EQUALS:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_ICOMPARE_NEQ;
            break;
        }
        case TOKEN_GREATER_EQUALS:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_ICOMPARE_GE;
            break;
        }
        case TOKEN_GREATER_THAN:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_ICOMPARE_GT;
            break;
        }
        case TOKEN_LESS_EQUALS:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_ICOMPARE_LE;
            break;
        }
        case TOKEN_LESS_THAN:{
            d->binary.op = Exp_Expr::BinaryOp::EXPR_ICOMPARE_LT;
            break;
        }
        



        default:
            break;
        }

        
        d->binary.left = left;
        d->binary.right = right;
        
        // insert type cast if needed
        insertTypeCast(d);

        break;
    }
    
    case Subexpr::SUBEXPR_LEAF :{
        // if variable/identifier
        if (_match(expr->leaf, TOKEN_IDENTIFIER)){
            /*
                Expanded into 
                
                        DEREF 
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
            d->tag = Exp_Expr::EXPR_DEREF;
            
            Exp_Expr *leaf = (Exp_Expr*) arena->alloc(sizeof(Exp_Expr));
            leaf->leaf.val = expr->leaf;
            leaf->tag = Exp_Expr::EXPR_LEAF;

            Exp_Expr *address = (Exp_Expr*) arena->alloc(sizeof(Exp_Expr));
            address->addressOf.of = leaf;
            address->tag = Exp_Expr::EXPR_ADDRESSOF;

            d->deref.base = address;
            d->deref.offset = 0;
            d->deref.size = sizeOfType(d->type, scope);
        
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

        d->tag = Exp_Expr::EXPR_LOAD_IMMEDIATE;
        d->immediate.val = expr->leaf;
        return d;
    }

    case Subexpr::SUBEXPR_UNARY: {

        Exp_Expr *operand = expandSubexpr(expr->unarySubexpr, scope);
        
        if (_match(expr->unaryOp, TOKEN_STAR)){
            /*
                *x is expanded to

                    DEREF
                base|   \ offset 
                    |    \ 
                    x     0
            */
            d->tag = Exp_Expr::EXPR_DEREF;
            d->type = *(operand->type.ptrTo);

            d->deref.base = operand;
            d->deref.offset = 0;
            d->deref.size = sizeOfType(d->type, scope);

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
        
            d->tag = Exp_Expr::EXPR_LOAD_ADDRESS;


            assert(operand->tag == Exp_Expr::EXPR_DEREF);
            d->loadAddress.base = operand->deref.base;
            d->loadAddress.offset = operand->deref.offset;

            
            d->type.tag = DataType::TAG_ADDRESS;
            d->type.ptrTo = (DataType*) arena->alloc(sizeof(DataType));
            *(d->type.ptrTo) = operand->type;

            return d;
        }
        
        /*
            Unary expressions are expanded to

            UNARY_EXPR
                |
                |
              expr
        */
        
        d->tag = Exp_Expr::EXPR_UNARY;
        d->unary.unarySubexpr = operand; 
        d->type = operand->type;

        switch (expr->unaryOp.type){

        case TOKEN_PLUS:
            break;
        case TOKEN_MINUS:
            d->unary.op = Exp_Expr::UnaryOp::EXPR_INEGATE;
            break;
        case TOKEN_STAR:
            break;
        case TOKEN_LOGICAL_NOT:
            d->unary.op = Exp_Expr::UnaryOp::EXPR_LOGICAL_NOT;
            break;
        case TOKEN_BITWISE_NOT:
            d->unary.op = Exp_Expr::UnaryOp::EXPR_IBITWISE_NOT;
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
        return expandSubexpr(expr->inside, scope);
    }
    
    case Subexpr::SUBEXPR_FUNCTION_CALL:{
        // TODO: finish this
        FunctionCall *fooCall = expr->functionCall;
        Function foo = ir->functions.getInfo(fooCall->funcName.string).info;
        

        d->type = foo.returnType;
        break;
    }
        
    
    default:
        break;
    }

    return d;

}



/*
    RV64 specific sizes of types.
*/
size_t CodeGenerator::sizeOfType(DataType d, StatementBlock* scope){
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
size_t CodeGenerator::alignmentOfType(DataType d, StatementBlock* scope){
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