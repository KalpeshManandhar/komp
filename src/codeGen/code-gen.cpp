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


void CodeGenerator::generateAssembly(IR *ir){
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
