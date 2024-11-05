#include "code-gen.h"
#include <tokenizer/token.h>


int max(int a, int b){
    return (a>b)?a:b;
}

int min(int a, int b){
    return (a<b)?a:b;
}


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

static int getDepth(const Exp_Expr *expr){
    if (!expr){
        return 0;
    }
    
    switch (expr->tag){
    case Exp_Expr::EXPR_ADDRESSOF: {
        return 1;
    }
    case Exp_Expr::EXPR_BINARY: {
        int left = getDepth(expr->binary.left);
        int right = getDepth(expr->binary.right);

        return max(left, right) + 1;
    }
    case Exp_Expr::EXPR_CAST: {
        int operand = getDepth(expr->cast.expr);
        return operand + 1;
    }
    case Exp_Expr::EXPR_DEREF: {
        int operand = getDepth(expr->unary.unarySubexpr);
        return operand + 1;
    }
    case Exp_Expr::EXPR_FUNCTION_CALL: {
        return 1;
    }
    case Exp_Expr::EXPR_LOAD_IMMEDIATE: {
        return 1;
    }
    case Exp_Expr::EXPR_STORE: {
        int lval = getDepth(expr->store.left);
        int rval = getDepth(expr->store.right);
        return max(lval, rval) + 1;
    }
    case Exp_Expr::EXPR_UNARY: {
        int operand = getDepth(expr->unary.unarySubexpr);
        return operand + 1;
    }
    default:
        return 0;
    }

}

size_t CodeGenerator::allocStackSpace(StatementBlock *scope, ScopeInfo *storage){
    size_t totalSize = 0;

    for (auto &dt: scope->symbols.entries){
        int size = sizeOfType(dt.second.info);
        totalSize = alignUpPowerOf2(totalSize, size);
        totalSize += size;
    }
    totalSize = alignUpPowerOf2(totalSize, 16);

    if (totalSize > 0){
        size_t mem = stackAlloc.allocate(totalSize);
        size_t offset = 0;

        for (auto &dt: scope->symbols.entries){
            int size = sizeOfType(dt.second.info);
            offset = alignUpPowerOf2(offset, size);
            offset += size;

            StorageInfo s;
            s.memAddress = mem + offset;
            s.tag = StorageInfo::STORAGE_MEMORY;

            storage->storage.add(dt.second.identifier, s);

        }    
    }

    return totalSize;
}


// allocate the dest register before calling 
void CodeGenerator::generateSubexpr(const Subexpr *expr, StatementBlock *scope, Register dest, ScopeInfo *storageScope){
    Exp_Expr *expanded = expandSubexpr(expr, scope); 
    
    generateExpandedExpr(expanded, scope, dest, storageScope);
    return;
    
    
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


const char* sizeSuffix(size_t size){
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



void CodeGenerator::generateExpandedExpr(Exp_Expr *current, StatementBlock *scope, Register dest, ScopeInfo *storageScope){
    switch (current->tag)
    {
    case Exp_Expr::EXPR_LOAD_IMMEDIATE:{
        RV64_Register destReg = regAlloc.resolveRegister(dest);
        const char *destName = RV64_RegisterName[destReg];
        
        buffer << "    li " << destName << ", " << current->immediate.val.string << "\n";
        break;
    }

    case Exp_Expr::EXPR_ADDRESSOF:{
        Exp_Expr *of = current->addressOf.of;

        if (of->tag == Exp_Expr::EXPR_LEAF){
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

            ScopeInfo *storage = getAddressScope(of->leaf.val.string);
            StorageInfo sInfo = storage->storage.getInfo(of->leaf.val.string).info;
            current->addressOf.offset = storage->frameBase - sInfo.memAddress;
            break;
        }



        break;
    }
    
    case Exp_Expr::EXPR_DEREF:{
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        if (current->deref.base->tag == Exp_Expr::EXPR_ADDRESSOF){
            // generate for base address
            generateExpandedExpr(current->deref.base, scope, dest, storageScope);
            Exp_Expr *base = current->deref.base;
            buffer << "    l" << sizeSuffix(current->deref.size) << " " << destName << ", " << base->addressOf.offset + current->deref.offset << "(fp)\n";
            return;
        }

        generateExpandedExpr(current->deref.base, scope, dest, storageScope);    
        buffer << "    l" << sizeSuffix(current->deref.size) << " " << destName << ", " << current->deref.offset << "(" << destName << ")\n";
        
        break;
    }
    
    case Exp_Expr::EXPR_STORE:{
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];

        generateExpandedExpr(current->store.right, scope, dest, storageScope);
        
        if (current->store.left->tag == Exp_Expr::EXPR_ADDRESSOF){
            // generate for base address
            generateExpandedExpr(current->store.left, scope, dest, storageScope);
            Exp_Expr *base = current->store.left;
            buffer << "    s" << sizeSuffix(current->store.size) << " " << destName << ", " << base->addressOf.offset + current->store.offset << "(fp)\n";
            return;
        }
        
        
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];

        generateExpandedExpr(current->store.left, scope, temp, storageScope);    
        buffer << "    s" << sizeSuffix(current->store.size) << " " << destName << ", " << current->store.offset << "(" << tempName << ")\n";
        
        regAlloc.freeRegister(temp);
        break;
    }
    
    case Exp_Expr::EXPR_INDEX:{
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];
        
        // generate for offset
        if (current->deref.offset){
            generateExpandedExpr(current->index.index, scope, temp, storageScope);
            
            if (current->index.size > 1){
                // offset x size
                buffer << "    li " <<  destName << ", " << current->index.size << "\n";
                buffer << "    mul " << tempName << ", " << tempName << ", " << destName << "\n";
            }
        }

        generateExpandedExpr(current->index.base, scope, dest, storageScope);
        buffer << "    add " << destName << ", " << destName << ", " << tempName << "\n";

        regAlloc.freeRegister(temp);
        break;
    }
    
    case Exp_Expr::EXPR_BINARY:{
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];
        
        int leftDepth = getDepth(current->binary.left);
        int rightDepth = getDepth(current->binary.right);

        if (leftDepth < rightDepth){
            generateExpandedExpr(current->binary.right, scope, temp, storageScope);
            generateExpandedExpr(current->binary.left, scope, dest, storageScope);
        }
        else{
            generateExpandedExpr(current->binary.left, scope, dest, storageScope);
            generateExpandedExpr(current->binary.right, scope, temp, storageScope);
        }

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
                buffer << "    srl " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_GT:{
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                buffer << "    sgtz " << destName << ", " << destName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_LE:{
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                buffer << "    sgtz " << destName << ", " << destName << "\n";
                buffer << "    xori " << destName << ", " << destName << ", " << "1" << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_GE:{
                buffer << "    slt " << destName << ", " << tempName << ", " << destName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_EQ:{
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                buffer << "    seqz " << destName << ", " << destName << "\n";
                break;
            }
            case Exp_Expr::BinaryOp::EXPR_ICOMPARE_NEQ:{
                buffer << "    sub " << destName << ", " << destName << ", " << tempName << "\n";
                buffer << "    snez " << destName << ", " << destName << "\n";
                break;
            }

            case Exp_Expr::BinaryOp::EXPR_FCOMPARE_LT:{
                buffer << "    slt " << destName << ", " << destName << ", " << tempName << "\n";
                break;
            }

            case Exp_Expr::BinaryOp::EXPR_FADD:
            case Exp_Expr::BinaryOp::EXPR_FSUB:
            case Exp_Expr::BinaryOp::EXPR_FMUL:
            case Exp_Expr::BinaryOp::EXPR_FDIV:
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

        


        break;
    }
    
    case Exp_Expr::EXPR_CAST:{
        break;
    }
    case Exp_Expr::EXPR_UNARY:{
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];

        generateExpandedExpr(current->unary.unarySubexpr, scope, dest, storageScope);
        
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
                buffer << "    seqz " << destName << ", " << destName << "\n";
                break;
            }
            default:
                break;
        }

        break;
    }
    case Exp_Expr::EXPR_FUNCTION_CALL:{
        break;
    }
    default:
        break;
    }
}



void CodeGenerator::generateNode(const Node *current, StatementBlock *scope, ScopeInfo *storageScope){
    if (!current){
        return;
    }

    switch (current->tag){
    
    case Node::NODE_DECLARATION:{
        Declaration *d = (Declaration *)current;
        
        for (auto &decl: d->decln){
            if (decl.initValue){
                Register temp = regAlloc.allocVRegister(RegisterType::REG_TEMPORARY);
                
                generateSubexpr(decl.initValue, scope, temp, storageScope);
                            
                const char *name = RV64_RegisterName[regAlloc.resolveRegister(temp)]; 
                
                StorageInfo sInfo = storageScope->storage.getInfo(decl.identifier.string).info;
                int64_t offset = storageScope->frameBase - sInfo.memAddress;

                buffer << "    sw " << name << ", " << offset << "(fp)" << "\n";
                regAlloc.freeRegister(temp);
            }
        }
        break;
    }

    case Node::NODE_STMT_BLOCK:{
        StatementBlock *b = (StatementBlock *)current;
        
        ScopeInfo currentStorageScope;
        currentStorageScope.frameBase = storageScope->frameBase;
        currentStorageScope.parent = storageScope;
        
        size_t totalSize = allocStackSpace(b, &currentStorageScope);
        buffer << "    addi sp, sp, -" << totalSize << "\n"; 
        
        for (auto &stmt : b->statements){
            generateNode(stmt, scope, &currentStorageScope);
        }
        buffer << "    addi sp, sp, " << totalSize << "\n"; 
        stackAlloc.deallocate(totalSize);

        break;
    }

    case Node::NODE_RETURN:{
        ReturnNode *r = (ReturnNode *)current;

        Register a0 = regAlloc.allocRegister(REG_A0);
        generateSubexpr(r->returnVal, scope, a0, storageScope);

        StatementBlock *funcScope = scope->getParentFunction();
        // jump to function epilogue instead of ret
        buffer << "    j ."<< funcScope->funcName.string << "_ep\n";
        regAlloc.freeRegister(a0);
        break;
    }
    
    case Node::NODE_SUBEXPR:{
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

void CodeGenerator::generateFunction(Function *foo, ScopeInfo *storageScope){
    buffer << "    .globl " << foo->funcName.string << "\n";
    buffer << foo->funcName.string << ":\n";
    // Function prologue
    buffer << "    addi sp, sp, -16\n"; // Allocate stack space
    buffer << "    sd ra, 8(sp)\n";     // Save return address
    buffer << "    sd fp, 0(sp)\n";     // Save prev frame pointer
    buffer << "    mv fp, sp\n";        // Save current stack pointer 

    // Moving the parameters to registers
    for (int i = 5; auto &param : foo->parameters)
    {
        buffer << "mv a" << (i--) << ", a0\n";
    }

    generateNode(foo->block, foo->block, storageScope);

    // Function epilogue
    buffer << "."<<foo->funcName.string << "_ep:\n";
    buffer << "    mv sp, fp\n";       // Restore stack pointer
    buffer << "    ld fp, 0(sp)\n";    // Restore previous frame pointer
    buffer << "    ld ra, 8(sp)\n";    // Restore return address
    buffer << "    addi sp, sp, 16\n"; // Deallocate stack space
    buffer << "    ret\n";             // Return from function
}

// Generated from AI to write to a .s file
void CodeGenerator::writeAssemblyToFile(const char *filename)
{
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

    outputBuffer << "    .text\n";
    
    ScopeInfo s;
    s.frameBase = 0;
    s.parent = 0;

    for (auto &pair: ir->functions.entries){
        generateFunction(&pair.second.info, &s);

        Function *foo = &pair.second.info;

        // Appending the function assembly to outputBuffer
        outputBuffer << buffer.str();
        buffer.str("");
        buffer.clear();
        
    }
}


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



Exp_Expr* CodeGenerator::expandSubexpr(const Subexpr *expr, StatementBlock *scope){
    Exp_Expr *d = (Exp_Expr *)arena->alloc(sizeof(Exp_Expr));
    
    switch (expr->subtag){
    case Subexpr::SUBEXPR_BINARY_OP :{
        bool isAssignment = _matchv(expr->op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP));

        if (isAssignment){
            Exp_Expr *left = expandSubexpr(expr->left, scope);
            Exp_Expr *right = expandSubexpr(expr->right, scope);


            switch (expr->op.type){
            case TOKEN_ASSIGNMENT:{
                break;
            }
            case TOKEN_PLUS_ASSIGN:{
                Exp_Expr *add = (Exp_Expr*)arena->alloc(sizeof(Exp_Expr));
                Exp_Expr *addLeft = (Exp_Expr*)arena->alloc(sizeof(Exp_Expr));
                
                add->tag = Exp_Expr::EXPR_BINARY;
                add->binary.left = addLeft;
                add->binary.right = right;
                add->binary.op = Exp_Expr::BinaryOp::EXPR_IADD;
                add->type = getResultantType(left->type, right->type, expr->op);
                
                *addLeft = *left;

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
            - Any variable will generate a deref node.
            - For lvalue, we need an address in the left node of the store node.
            - So, remove that node: 
                - set the base address of the deref as the left node. 
                - set the offset as the offset in the store node.
            */
            assert(left->tag == Exp_Expr::EXPR_DEREF);
            d->store.offset = left->deref.offset;
            
            left = left->deref.base;
            d->store.left = left;
            d->store.right = right;
            d->store.size = sizeOfType(d->type);

            return d;
        }


        Exp_Expr *left = expandSubexpr(expr->left, scope);
        Exp_Expr *right = expandSubexpr(expr->right, scope);
        
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
        
        

        case TOKEN_ARROW:{
            break;
        }
        case TOKEN_DOT:{
            break;
        }



        default:
            break;
        }
        
        d->binary.left = left;
        d->binary.right = right;


        break;
    }

    case Subexpr::SUBEXPR_LEAF :{
        if (_match(expr->leaf, TOKEN_IDENTIFIER)){
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
            d->deref.size = sizeOfType(d->type);
        
            return d;
        }
        
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
            d->tag = Exp_Expr::EXPR_DEREF;
            d->type = *(operand->type.ptrTo);

            d->deref.base = operand;
            d->deref.offset = 0;
            d->deref.size = sizeOfType(d->type);
        }

        else if(_match(expr->unaryOp, TOKEN_AMPERSAND)){
            d->tag = Exp_Expr::EXPR_ADDRESSOF;
            
            d->type.tag = DataType::TAG_ADDRESS;
            d->type.ptrTo = (DataType*) arena->alloc(sizeof(DataType));
            *(d->type.ptrTo) = operand->type;

            // d->addressOf.symbol
        }
        
        
        d->tag = Exp_Expr::EXPR_UNARY;
        d->unary.unarySubexpr = operand; 

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

        FunctionCall *fooCall = expr->functionCall;
        Function foo = ir->functions.getInfo(fooCall->funcName.string).info;
        

        d->type = foo.returnType;
    }
        
        break;
    
    default:
        break;
    }

    return d;

}
