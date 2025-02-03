#include "code-gen.h"
#include <utils/utils.h>
#include <IR/number.h>

/*
    The instruction suffix for the size of integer load/store 
*/
static const char* iInsIntegerSuffix(size_t size){
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
    The instruction suffix for the size of load/store of floats.
*/
static const char* fInsFloatSuffix(size_t size){
    /*
        8 bytes = "d"ouble precision
        4 bytes = "s"ingle precision
    */
    if (size == 8){
        return "d";
    }
    if (size == 4){
        return "s";
    }
    return "d";
}

/*
    The instruction suffix for the size of integer in float conversion instructions. 
*/
static const char* fInsIntegerSuffix(size_t size){
    /*
        8 bytes = "l"ong 
        4 bytes = "w"ord
    */
    if (size == 8){
        return "l";
    }
    if (size == 4){
        return "w";
    }
    return "l";
}




// get depth of an expanded expr node
static int getDepth(const MIR_Expr *expr){
    if (!expr){
        return 0;
    }
    
    switch (expr->tag){
    case MIR_Expr::EXPR_ADDRESSOF: {
        return 1;
    }
    case MIR_Expr::EXPR_LOAD: {
        int operand = getDepth(expr->load.base);
        return operand + 1;
    }
    case MIR_Expr::EXPR_INDEX: {
        int base = getDepth(expr->index.base);
        int index = getDepth(expr->index.index);
        return max(base, index) + 1;
    }
    case MIR_Expr::EXPR_LEAF: {
        return 1;
    }
    case MIR_Expr::EXPR_LOAD_ADDRESS: {
        int address = getDepth(expr->loadAddress.base);
        return address + 1;
    }
    case MIR_Expr::EXPR_LOAD_IMMEDIATE: {
        return 1;
    }
    case MIR_Expr::EXPR_STORE: {
        int lval = getDepth(expr->store.left);
        int rval = getDepth(expr->store.right);
        return max(lval, rval) + 1;
    }
    case MIR_Expr::EXPR_CAST: {
        int operand = getDepth(expr->cast.expr);
        return operand + 1;
    }
    case MIR_Expr::EXPR_BINARY: {
        int left = getDepth(expr->binary.left);
        int right = getDepth(expr->binary.right);

        return max(left, right) + 1;
    }
    case MIR_Expr::EXPR_UNARY: {
        int operand = getDepth(expr->unary.unarySubexpr);
        return operand + 1;
    }
    case MIR_Expr::EXPR_CALL: {
        return 1;
    }
    default:
        assert(false && "Some stuff is not accounted for.");
        return 0;
    }

}



/*
    RV-64 specific datatypes
*/
MIR_Datatype CodeGenerator::convertToLowerLevelType(DataType d, StatementBlock *scope){
    switch (d.tag){
    case DataType::TAG_ADDRESS:
    case DataType::TAG_PTR:
        return MIR_Datatypes::_u64;
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
    
    assertFalse(printf("Some type hasnt been accounted for: %d", d.tag));
    return MIR_Datatypes::_i32;
}



size_t CodeGenerator :: allocStackSpaceMIR(MIR_Scope* scope, ScopeInfo* storage){
    size_t totalSize = 0;
    
    // compute size
    for (auto &var: scope->symbols.order){
        MIR_Datatype dt = scope->symbols.getInfo(var).info;

        size_t size = dt.size;
        size_t alignment = dt.alignment;

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
            MIR_Datatype dt = scope->symbols.getInfo(var).info;
            
            size_t size = dt.size;
            size_t alignment = dt.alignment;
            
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





void CodeGenerator :: generatePrimitiveMIR(MIR_Primitive* p, MIR_Scope* scope, ScopeInfo *storageScope){
    if (!p){
        return;
    }

    switch (p->ptag){
        case MIR_Primitive::PRIM_IF:{
            MIR_If* inode = (MIR_If*) p;
            
            size_t ifEndLabel = labeller.label();

            while (inode){
                if (inode->condition){
                    Register condition = regAlloc.allocVRegister(REG_TEMPORARY);
                    // compute the condition
                    generateExprMIR(inode->condition, condition, scope, storageScope);
                    
                
                    const char *regName = RV64_RegisterName[regAlloc.resolveRegister(condition)];
                
                    // if final in chain then use the endLabel as the false label
                    size_t falseLabel = (inode->next)? labeller.label() : ifEndLabel;
                    // branch if condition is false
                    buffer << "    beqz " << regName << ", " << ".if_L"<< falseLabel <<"\n";

                    regAlloc.freeRegister(condition);
                    
                    // generate the block
                    generatePrimitiveMIR(inode->scope, scope, storageScope);
                    
                    // only jump if there is something between the label and code block
                    if (inode->next){
                        buffer << "    j " << ".if_L"<< ifEndLabel<<"\n";
                    }
                    
                    // the label for if the condition is false
                    buffer << ".if_L" << falseLabel << ":\n";
                    
                }
                else{
                    assert(inode->next == NULL && "Else cannot have a 'next' branch block. Must be NULL.");
                    generatePrimitiveMIR(inode->scope, scope, storageScope);

                    buffer << ".if_L" << ifEndLabel << ":\n";
                }

                inode = inode->next;
            }

            break;
        }
        case MIR_Primitive::PRIM_LOOP:{
            MIR_Loop* lnode = (MIR_Loop*) p;
        
            Register condition = regAlloc.allocVRegister(REG_TEMPORARY);

            size_t startLabel = labeller.label();
            size_t falseLabel = labeller.label();
            
            // start of while loop 
            buffer << ".while_L" << startLabel << ":\n";
            
            // check condition
            generateExprMIR(lnode->condition, condition, scope, storageScope);

            const char *regName = RV64_RegisterName[regAlloc.resolveRegister(condition)];
                    
            // break out if condition is false
            buffer << "    beqz " << regName << ", " << ".while_L"<< falseLabel <<"\n";

            regAlloc.freeRegister(condition);
            
            // generate the block
            generatePrimitiveMIR(lnode->scope, scope, storageScope);
            
            // jump to loop start
            buffer << "    j " << ".while_L"<< startLabel<<"\n";
            
            // out of loop
            buffer << ".while_L" << falseLabel << ":\n";
            
            break;
        }
        
        case MIR_Primitive::PRIM_STACK_ALLOC:{
            break;
        }
        case MIR_Primitive::PRIM_STACK_FREE:{
            break;
        }
        
        case MIR_Primitive::PRIM_RETURN:{
            MIR_Return* rnode = (MIR_Return*) p;
            MIR_Function &foo = this->mir->functions.getInfo(rnode->funcName).info;
            
            if (foo.returnType.tag == MIR_Datatype::TYPE_VOID){

            }
            else if (isIntegerType(foo.returnType)){
                assert(foo.returnType.size <= XLEN && "Return values are only supported in one register.");

                Register a0 = regAlloc.allocRegister(REG_A0);
                generateExprMIR(rnode->returnValue, a0, scope, storageScope);
                regAlloc.freeRegister(a0);
            }
            else if (isFloatType(foo.returnType)){
                assert(foo.returnType.size <= FLEN && "Return values are only supported in one register.");

                Register fa0 = regAlloc.allocRegister(REG_FA0);
                generateExprMIR(rnode->returnValue, fa0, scope, storageScope);
                
                regAlloc.freeRegister(fa0);
            }
            else {
                assert(false && "Struct returns not implemented yet.");
            }
            buffer << "    j ." << rnode->funcName << "_ep\n";


            break;
        }
        case MIR_Primitive::PRIM_JUMP:{
            break;
        }
        case MIR_Primitive::PRIM_EXPR:{
            MIR_Expr* enode = (MIR_Expr*) p;

            bool isFloatExpr = isFloatType(enode->_type);
            int mask = isFloatExpr? REG_FLOATING_POINT : 0; 

            Register rtmp = regAlloc.allocVRegister(RegisterType(REG_TEMPORARY | mask));

            generateExprMIR(enode, rtmp, scope, storageScope);
            
            regAlloc.freeRegister(rtmp);

            break;
        }
        case MIR_Primitive::PRIM_SCOPE:{
            MIR_Scope* snode = (MIR_Scope*) p;

            ScopeInfo storage;
            storage.parent = storageScope;
            storage.frameBase = storageScope->frameBase;

            size_t totalSize = allocStackSpaceMIR(snode, &storage);
            
            // allocate stack space
            if (totalSize > 0)
                buffer << "    addi sp, sp, -" << totalSize << "\n"; 

            for (auto &prim : snode->statements){
                generatePrimitiveMIR(prim, snode, &storage);
            }

            // deallocate stack space
            if (totalSize > 0)
                buffer << "    addi sp, sp, " << totalSize << "\n"; 
            
            stackAlloc.deallocate(totalSize);

            break;
        }
    
    default:
        assert(false && "Some thing unaccounted for.");
        break;
    }
}








/*
    Generate assembly for a function.
*/
void CodeGenerator :: generateFunctionMIR(MIR_Function *foo, MIR_Scope* global, ScopeInfo *storageScope){
    // if extern, then no need to generate
    if (foo->isExtern){
        return;
    }

    std::cout << "Generating for " << foo->funcName << "\n";


    buffer << "    .globl " << foo->funcName << "\n";
    buffer << foo->funcName << ":\n";
    // function prologue
    buffer << "    addi sp, sp, -16\n"; // allocate stack space for return address and previous frame pointer.
    buffer << "    sd ra, 8(sp)\n";     // save return address
    buffer << "    sd fp, 0(sp)\n";     // save prev frame pointer
    buffer << "    mv fp, sp\n";        // save current stack pointer 
    

    ScopeInfo storage;
    storage.parent = storageScope;
    storage.frameBase = 0;

    // allocate stack space for parameters and local variables
    size_t totalSize = allocStackSpaceMIR((MIR_Scope*) foo, &storage);
    
    if (totalSize > 0)
        buffer << "    addi sp, sp, -" << totalSize << "\n"; 
    

    
    // follows the LP64D ABI
    // available register counts
    int totalAvailableXA = (REG_A7 - REG_A0 + 1);
    int totalAvailableFA = (REG_FA7 - REG_FA0 + 1);
    int occupiedXA = 0;
    int occupiedFA = 0;
    // copy parameters from registers to stack
    for (int paramNo = 0; paramNo < foo->parameters.size(); paramNo++){
        MIR_Datatype typeOfParam = foo->parameters[paramNo].type;
        size_t sizeOfParam = typeOfParam.size;
        
        StorageInfo sInfo = storage.storage.getInfo(foo->parameters[paramNo].identifier).info;

        // integer types
        if (isIntegerType(typeOfParam)){
            int nRegistersRequired = alignUpPowerOf2(sizeOfParam, XLEN)/XLEN;
            
            // pass in registers
            if (nRegistersRequired <= 2){
                assert(nRegistersRequired != 2 && "Multiple registers load isnt currently supported.");
                
                Register argRegister = regAlloc.allocRegister(RV64_Register(REG_A0 + occupiedXA));
                const char* argRegName = RV64_RegisterName[regAlloc.resolveRegister(argRegister)];
                
                
                int64_t offset = sInfo.memAddress - storage.frameBase;
                buffer << "    s" << iInsIntegerSuffix(sizeOfParam) << " " << argRegName << ", -" << offset << "(fp)\n"; 
                regAlloc.freeRegister(argRegister);
                occupiedXA++;
                
            }
            else {
                assert(false && "Integer types should fit in one XLEN length register.");
            }

            
        }
        // floating types
        else if(isFloatType(typeOfParam)){
            int nRegistersRequired = alignUpPowerOf2(sizeOfParam, FLEN)/FLEN;
            
            if (nRegistersRequired <= 2){
                assert(nRegistersRequired != 2 && "Multiple registers load isnt currently supported.");
                
                Register argRegister = regAlloc.allocRegister(RV64_Register(REG_FA0 + occupiedFA));
                
                const char* argRegName = RV64_RegisterName[regAlloc.resolveRegister(argRegister)];
                
                int64_t offset = sInfo.memAddress - storage.frameBase;
                buffer << "    fs" << iInsIntegerSuffix(sizeOfParam) << " " << argRegName << ", -" << offset << "(fp)\n";
                regAlloc.freeRegister(argRegister);
                occupiedFA++;
            }
            else {
                assert(false && "Float types should fit in one FLEN length register.");
            }
        }
        // struct types
        else{
            assert(false && "Struct arguments are not supported yet.");
        }

    }

    for (auto &prim : foo->statements){
        generatePrimitiveMIR(prim, (MIR_Scope*) foo, &storage);
    }

    // deallocate stack space
    stackAlloc.deallocate(totalSize);
    if (totalSize > 0)
        buffer << "    addi sp, sp, " << totalSize << "\n"; 
    
    


    // function epilogue
    buffer << "."<<foo->funcName << "_ep:\n";
    buffer << "    mv sp, fp\n";       // restore stack pointer
    buffer << "    ld fp, 0(sp)\n";    // restore previous frame pointer
    buffer << "    ld ra, 8(sp)\n";    // restore return address
    buffer << "    addi sp, sp, 16\n"; // deallocate stack space
    buffer << "    ret\n\n\n";             // return from function
}






void CodeGenerator :: generateAssemblyFromMIR(MIR *mir){
    this->mir = mir;

    ScopeInfo s;
    s.frameBase = 0;
    s.parent = 0;
    
    textSection << "    .section     .text\n";

    for (auto &pair : mir->functions.entries){
        generateFunctionMIR(&pair.second.info, mir->global, &s);

        MIR_Function *foo = &pair.second.info;

        // Appending the function assembly to textSection
        textSection << buffer.str();
        buffer.str("");
        buffer.clear();
            
    }

    rodataSection << "    .section     .rodata\n";
    dataSection << "    .section     .data\n";
    

    for (auto &rodataSymbol : rodata.entries){
        SymbolInfo symbol = rodataSymbol.second.info;
        
        rodataSection << ".symbol" << symbol.label << ":\n";
        
        switch (symbol.type.tag) {
        case MIR_Datatype::TYPE_F32:{
            Number value = f32FromString(symbol.value.data);
    
            rodataSection << "    .word "  << value.u32[0] << "\n";
            break;
        }
        case MIR_Datatype::TYPE_F64:{
            Number value = f64FromString(symbol.value.data);
    
            rodataSection << "    .word "  << value.u32[0] << "\n";
            rodataSection << "    .word "  << value.u32[1] << "\n";
            break;
        }
        default:
            break;
        }


        
    }

}




/*
    Generate assembly for the expanded IR.
    current : The node to generate assembly for.
    dest    : The register to put the result in.    

*/
void CodeGenerator::generateExprMIR(MIR_Expr *current, Register dest, MIR_Scope* scope, ScopeInfo *storageScope){
    switch (current->tag)
    {
    case MIR_Expr::EXPR_LOAD_IMMEDIATE:{
        // Puts the immediate value in the destination register.
        RV64_Register destReg = regAlloc.resolveRegister(dest);
        const char *destName = RV64_RegisterName[destReg];
        
        // load immediate value into a register
        if (isIntegerType(current->_type)){
            buffer << "    li " << destName << ", " << current->immediate.val << "\n";
        }
        else if (isFloatType(current->_type)){
            
            if (!rodata.existKey(current->immediate.val)){
                rodata.add(current->immediate.val, SymbolInfo{.label = labeller.label(), .value = current->immediate.val, .type = current->_type});
            }
            
            SymbolInfo fpLiteralInfo = rodata.getInfo(current->immediate.val).info;

            Register fpLiteralAddress = regAlloc.allocVRegister(RegisterType::REG_ANY);
            const char* fpLiteralAddressName = RV64_RegisterName[regAlloc.resolveRegister(fpLiteralAddress)];

            buffer << "    lui " << fpLiteralAddressName << ", \%hi(.symbol" << fpLiteralInfo.label << ")" << "\n";
            buffer << "    fl" << iInsIntegerSuffix(current->_type.size) << " " << destName << ", \%lo(.symbol" << fpLiteralInfo.label << ")(" << fpLiteralAddressName << ")\n";
        
            regAlloc.freeRegister(fpLiteralAddress);
        }
        
        break;
    }

    case MIR_Expr::EXPR_ADDRESSOF:{
        // Resolve the address of the given variable. Doesn't use the destination register.
        MIR_Expr *of = current->addressOf.of;
        
        if (of->tag == MIR_Expr::EXPR_LEAF){
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
            ScopeInfo *storage = getAddressScope(of->leaf.val);
            assert(storage != NULL);
            StorageInfo sInfo = storage->storage.getInfo(of->leaf.val).info;
            current->addressOf.offset = storage->frameBase - sInfo.memAddress;
            break;
        }

        break;
    }
    
    case MIR_Expr::EXPR_LOAD_ADDRESS:{
        // Loads the destination register with the given address.
        MIR_Expr *base = current->loadAddress.base;
        
        // resolve variable into address/load address into regsister
        generateExprMIR(base, dest, scope, storageScope);
        
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        // address is just resolved
        if(base->tag == MIR_Expr::EXPR_ADDRESSOF){
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
    
    case MIR_Expr::EXPR_LOAD:{
        // Load the value at given address + offset and put it into the destination register.

        // load/resolve the address into the register first
        generateExprMIR(current->load.base, dest, scope, storageScope); 
        
        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        bool isFloatExpr = isFloatType(current->_type);
        
        const char* prefix = isFloatExpr? "f" : "";


        // if the given address is a direct AddressOf node, then the address can be used instead of loading it into a register first.
        if (current->load.base->tag == MIR_Expr::EXPR_ADDRESSOF){
            MIR_Expr *base = current->load.base;
            
            // load the value and load into destination register
            buffer << "    " << prefix << "l" << iInsIntegerSuffix(current->load.size) << " " << destName << ", " << base->addressOf.offset + current->load.offset << "(fp)\n";
            return;
        }

        // load value and load   
        buffer << "    " << prefix << "l" << iInsIntegerSuffix(current->load.size) << " " << destName << ", " << current->load.offset << "(" << destName << ")\n";
        
        break;
    }
    
    case MIR_Expr::EXPR_STORE:{
        // Store the given rvalue in the address of the lvalue, and also load it into the destination register.

        // Load the rvalue into the destination register
        generateExprMIR(current->store.right, dest, scope, storageScope);
        

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        bool isFloatExpr = isFloatType(current->_type);
        
        const char* prefix = isFloatExpr? "f" : "";
        
        // if the lvalue has a direct address, use that directly instead of loading it to a register first
        if (current->store.left->tag == MIR_Expr::EXPR_ADDRESSOF){
            // resolve the base adddress
            generateExprMIR(current->store.left, dest, scope, storageScope);
            MIR_Expr *base = current->store.left;
            
            // store the value at (address + offset)
            buffer << "    " << prefix << "s" << iInsIntegerSuffix(current->store.size) << " " << destName << ", " << base->addressOf.offset + current->store.offset << "(fp)\n";
            return;
        }
        
        // else, load the address into a temporary register first 
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        
        // get address of lvalue
        generateExprMIR(current->store.left, temp, scope, storageScope);    
        

        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];

        // store the value at (address + offset)
        buffer << "    " << prefix << "s" << iInsIntegerSuffix(current->store.size) << " " << destName << ", " << current->store.offset << "(" << tempName << ")\n";
        
        regAlloc.freeRegister(temp);
        break;
    }
    
    case MIR_Expr::EXPR_INDEX:{
        // Adds a given index to a given address, to get the correct offset.

        
        Register temp = regAlloc.allocVRegister(REG_TEMPORARY);
        
        // load the given base address into register
        generateExprMIR(current->index.base, dest, scope, storageScope);


        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        if (current->index.base->tag == MIR_Expr::EXPR_ADDRESSOF){
            MIR_Expr *address = current->index.base;
            buffer << "    addi " << destName << ", fp, " << address->addressOf.offset << "\n";
        }


        // calculate index and load it into register
        generateExprMIR(current->index.index, temp, scope, storageScope);
        

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
    
    case MIR_Expr::EXPR_BINARY:{
        // Computes a binary operation and stores the result in the destination register.
        
        Register left = {0}, right = {0};
        
        bool canDestBeUsed = (isIntegerType(current->binary.left->_type) && (dest.type & REG_FLOATING_POINT) == 0)
                              || (isFloatType(current->binary.left->_type) && (dest.type & REG_FLOATING_POINT));

        // check if the destination register can be used for one of the operands
        // if can be, then left uses the dest register
        // else, left allocates a temporary register of the opposite register file
        left = canDestBeUsed? dest : regAlloc.allocVRegister(RegisterType(((dest.type & REG_FLOATING_POINT) ^ REG_FLOATING_POINT) | REG_TEMPORARY));
        // right allocates of the same type as left 
        right = regAlloc.allocVRegister(RegisterType((left.type & REG_FLOATING_POINT) | REG_TEMPORARY));


        int leftDepth = getDepth(current->binary.left);
        int rightDepth = getDepth(current->binary.right);

        // Generate the one with the greatest depth first so that intermediate values need not be stored.
        if (leftDepth < rightDepth){
            generateExprMIR(current->binary.right, right, scope, storageScope);
            generateExprMIR(current->binary.left, left, scope, storageScope);
        }
        else{
            generateExprMIR(current->binary.left, left, scope, storageScope);
            generateExprMIR(current->binary.right, right, scope, storageScope);
        }

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        const char *leftName = RV64_RegisterName[regAlloc.resolveRegister(left)];
        const char *rightName = RV64_RegisterName[regAlloc.resolveRegister(right)];
        
        switch (current->binary.op){
            case MIR_Expr::BinaryOp::EXPR_UADD:
            case MIR_Expr::BinaryOp::EXPR_IADD:{
                buffer << "    add " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_USUB:
            case MIR_Expr::BinaryOp::EXPR_ISUB:{
                buffer << "    sub " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_UMUL:
            case MIR_Expr::BinaryOp::EXPR_IMUL:{
                buffer << "    mul " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_UDIV:
            case MIR_Expr::BinaryOp::EXPR_IDIV:{
                buffer << "    div " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }

            
            case MIR_Expr::BinaryOp::EXPR_IBITWISE_AND:{
                buffer << "    and " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_IBITWISE_OR:{
                buffer << "    or " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_IBITWISE_XOR:{
                buffer << "    xor " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            

            // TODO: boolean values are considered to be 0 or 1, the cast would convert all other values into these
            case MIR_Expr::BinaryOp::EXPR_LOGICAL_AND:{
                buffer << "    and " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_LOGICAL_OR:{
                buffer << "    or " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            
            case MIR_Expr::BinaryOp::EXPR_IBITWISE_LSHIFT:{
                buffer << "    sll " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_IBITWISE_RSHIFT:{
                buffer << "    srl " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            
            
            case MIR_Expr::BinaryOp::EXPR_ICOMPARE_LT:{
                // set if less than
                buffer << "    slt " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_ICOMPARE_GT:{
                // subtract and set if greater than 0
                buffer << "    sub " << destName << ", " << leftName << ", " << rightName << "\n";
                buffer << "    sgtz " << destName << ", " << destName << "\n";
                break;
            } 
            case MIR_Expr::BinaryOp::EXPR_ICOMPARE_LE:{
                // subtract and set if greater than 0 (aka gt) then xor with 0x1 
                buffer << "    sub " << destName << ", " << leftName << ", " << rightName << "\n";
                buffer << "    sgtz " << destName << ", " << destName << "\n";
                buffer << "    xori " << destName << ", " << destName << ", " << "1" << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_ICOMPARE_GE:{
                // set if less than with operands swapped
                buffer << "    slt " << destName << ", " << rightName << ", " << leftName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_ICOMPARE_EQ:{
                // subtract and set if eq to 0
                buffer << "    sub " << destName << ", " << leftName << ", " << rightName << "\n";
                buffer << "    seqz " << destName << ", " << destName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_ICOMPARE_NEQ:{
                // subtract and set if neq to 0
                buffer << "    sub " << destName << ", " << leftName << ", " << rightName << "\n";
                buffer << "    snez " << destName << ", " << destName << "\n";
                break;
            }

            case MIR_Expr::BinaryOp::EXPR_FADD:{
                buffer << "    fadd." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_FSUB:{
                buffer << "    fsub." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_FMUL:{
                buffer << "    fmul." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_FDIV:{
                buffer << "    fdiv." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }


            


            // these instructions get an integer destination register, and require two fp temporary registers
            case MIR_Expr::BinaryOp::EXPR_FCOMPARE_LT:{
                buffer << "    flt." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_FCOMPARE_GT:{
                buffer << "    fle." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                buffer << "    xori " << destName << ", " << destName << ", " << "1" << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_FCOMPARE_LE:{
                buffer << "    fle." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_FCOMPARE_GE:{
                buffer << "    flt." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                buffer << "    xori " << destName << ", " << destName << ", " << "1" << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_FCOMPARE_EQ:{
                buffer << "    feq." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_FCOMPARE_NEQ:{
                buffer << "    feq." << fInsFloatSuffix(current->binary.size) << " " << destName << ", " << leftName << ", " << rightName << "\n";
                buffer << "    xori " << destName << ", " << destName << ", " << "1" << "\n";
                break;
            }
            default:
                assert(false && "Some operation hasn't been accounted for.");
                break;
        }

        if (!canDestBeUsed) {
            regAlloc.freeRegister(left);
        }
        regAlloc.freeRegister(right);
        break;
    }
    
    case MIR_Expr::EXPR_CAST:{
        if (current->cast._from.tag == current->cast._to.tag){
            break;
        }

        bool canSameRegBeUsed = isIntegerType(current->cast._from) && isIntegerType(current->cast._to);
        canSameRegBeUsed = canSameRegBeUsed || (isFloatType(current->cast._from) && isFloatType(current->cast._to));
        
        Register exprIn = dest;
        // same register cannot be used, then allocate another register of the other type 
        if (!canSameRegBeUsed){
            exprIn = regAlloc.allocVRegister(RegisterType(((dest.type ^ REG_FLOATING_POINT) & REG_FLOATING_POINT) | REG_TEMPORARY));
        }

        // generate the expr to be cast
        generateExprMIR(current->cast.expr, exprIn, scope, storageScope);

        const char* exprInName = RV64_RegisterName[regAlloc.resolveRegister(exprIn)];
        const char* exprDestName = RV64_RegisterName[regAlloc.resolveRegister(dest)];

        
        switch (current->cast._from.tag){
        case MIR_Datatype::TYPE_BOOL:
        case MIR_Datatype::TYPE_I8:
        case MIR_Datatype::TYPE_I16:
        case MIR_Datatype::TYPE_I32:
        case MIR_Datatype::TYPE_I64:
        case MIR_Datatype::TYPE_U8:
        case MIR_Datatype::TYPE_U16:
        case MIR_Datatype::TYPE_U32:
        case MIR_Datatype::TYPE_U64:{
            
            switch (current->cast._to.tag) {
                // conversion to bool
                case MIR_Datatype::TYPE_BOOL :{
                    buffer << "    snez " << exprDestName << ", " << exprInName << "\n";
                    break;
                }
                
                case MIR_Datatype::TYPE_F32 : 
                case MIR_Datatype::TYPE_F64 :
                    buffer  << "    fcvt." << fInsFloatSuffix(current->cast._to.size) << "." << iInsIntegerSuffix(current->cast._from.size) 
                            << ((isUnsigned(current->cast._from))?"u":"") << " " << exprDestName << ", " << exprInName << "\n";
                    break;

                case MIR_Datatype::TYPE_F16 :
                case MIR_Datatype::TYPE_F128 :
                    assert(false && "f16 and f128 are not supported.");
                    

                default:
                    break;
            }

            // since sign is extended by default, there is no need for explicit asm for converting between integer types
            
            break;
        }
        
        case MIR_Datatype::TYPE_F32:
        case MIR_Datatype::TYPE_F64:{
            switch (current->cast._to.tag) {
                case MIR_Datatype::TYPE_I8:
                case MIR_Datatype::TYPE_I16:
                case MIR_Datatype::TYPE_I32:
                case MIR_Datatype::TYPE_I64:
                case MIR_Datatype::TYPE_U8:
                case MIR_Datatype::TYPE_U16:
                case MIR_Datatype::TYPE_U32:
                case MIR_Datatype::TYPE_U64:{
                    buffer  << "    fcvt." << iInsIntegerSuffix(current->cast._to.size) << ((isUnsigned(current->cast._from))?"u":"") << "."
                            << fInsFloatSuffix(current->cast._from.size)   << " " << exprDestName << ", " << exprInName << "\n";
                    break;
                }

                case MIR_Datatype::TYPE_F32:
                case MIR_Datatype::TYPE_F64:{
                    assert((current->cast._from.tag != current->cast._to.tag) && "Casts shouldn't have the same type on both ends.");
                    buffer  << "    fcvt." << fInsFloatSuffix(current->cast._to.size) << "."
                            << fInsFloatSuffix(current->cast._from.size)   << " " << exprDestName << ", " << exprInName << "\n";
                    break;
                }


                case MIR_Datatype::TYPE_F16 :
                case MIR_Datatype::TYPE_F128 :
                    assert(false && "f16 and f128 are not supported.");
                default:
                    break;
            }
            break;
        }

        case MIR_Datatype::TYPE_I128:
        case MIR_Datatype::TYPE_U128:
            assert(false && "128 bit integers aren't supported.");
            break;
        case MIR_Datatype::TYPE_F16:
        case MIR_Datatype::TYPE_F128:
            assert(false && "16 and 128 bit floating point numbers aren't supported.");
            break;
        
        case MIR_Datatype::TYPE_ARRAY:{
            assert(isIntegerType(current->cast._to) && "Arrays can only be converted to integer types.");
            break;
        
        }
        
        default:
            assert(false && "Support for casts not implemented fully yet.");
            break;
        }





        
        if (!canSameRegBeUsed){
            regAlloc.freeRegister(exprIn);
        }

        break;
    }
    case MIR_Expr::EXPR_UNARY:{
        // load the expr into register
        generateExprMIR(current->unary.unarySubexpr, dest, scope, storageScope);

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        switch (current->unary.op){
            case MIR_Expr::UnaryOp::EXPR_INEGATE:{
                buffer << "    neg " << destName << ", " << destName << "\n";
                break;
            }
            case MIR_Expr::UnaryOp::EXPR_FNEGATE:{
                Register fpZero = regAlloc.allocVRegister(RegisterType(REG_FLOATING_POINT | REG_TEMPORARY));
                const char* fpZeroName = RV64_RegisterName[regAlloc.resolveRegister(fpZero)];
                

                buffer << "    fcvt." << fInsFloatSuffix(current->_type.size) << "." << fInsIntegerSuffix(XLEN) << " " << fpZeroName << ", zero\n";
                buffer << "    fsub." << fInsFloatSuffix(current->_type.size) << " " << destName << ", " << fpZeroName << ", " << destName << "\n";

                regAlloc.freeRegister(fpZero);
                break;
            }
            case MIR_Expr::UnaryOp::EXPR_IBITWISE_NOT:{
                buffer << "    not " << destName << ", " << destName << "\n";
                break;
            }
            case MIR_Expr::UnaryOp::EXPR_LOGICAL_NOT:{
                // set if equal to 0
                buffer << "    seqz " << destName << ", " << destName << "\n";
                break;
            }
            default:
                break;
        }

        break;
    }
    case MIR_Expr::EXPR_CALL:{
        // move return value into destination register
        RV64_Register destReg = regAlloc.resolveRegister(dest);
        const char *destName = RV64_RegisterName[destReg];


        RegisterState Xstate = regAlloc.getRegisterState(REG_CALLER_SAVED);
        RegisterState Fstate = regAlloc.getRegisterState(RegisterType(REG_CALLER_SAVED | REG_FLOATING_POINT));
        RegisterState state = {0};

        // the number of caller saved registers in use
        int count = 0;
        for (int i = 0; i<RV64_Register::REG_COUNT; i++){
            if (i == destReg){
                continue;
            }
            count += (Xstate.reg[i].occupied)? 1 : 0;
            count += (Fstate.reg[i].occupied)? 1 : 0;
            state.reg[i] = (Xstate.reg[i].occupied)? Xstate.reg[i] : Fstate.reg[i];
        }

        auto saveRegisters = [&] (RegisterState rState) {
            // save the caller saved registers used in memory
            int n = count;
            int allocSize = count * XLEN;
            if (count > 0){
                buffer << "    addi sp, sp, -" << allocSize << "\n";

                for (int i = 0; i<RV64_Register::REG_COUNT; i++){
                    if (i == destReg){
                        continue;
                    }
                    if(rState.reg[i].occupied){
                        bool isFloatReg = (RV64Registers[i].type & REG_FLOATING_POINT) != 0;
                        const char* prefix = isFloatReg? "f": "";
                        
                        buffer << "    " << prefix << "s"<< iInsIntegerSuffix(XLEN) << " " << RV64_RegisterName[i] << ", "<< (n-1)*XLEN << "(sp) \n";
                        n--;
                        regAlloc.freeRegister(Register{.id = size_t(i)});
                    }
                }
            }

        };
        saveRegisters(state);


        // follows the LP64D ABI
        // available register counts
        int totalAvailableXA = (REG_A7 - REG_A0 + 1);
        int totalAvailableFA = (REG_FA7 - REG_FA0 + 1);
        int occupiedXA = 0;
        int occupiedFA = 0;
        
        for (int argNo = 0; argNo < current->functionCall->arguments.size(); argNo++){
            MIR_Expr* arg = current->functionCall->arguments[argNo];
            size_t sizeOfArg = arg->_type.size;
            
            // integer types
            if (isIntegerType(arg->_type)){
                int nRegistersRequired = alignUpPowerOf2(sizeOfArg, XLEN)/XLEN;
                
                // pass in registers
                if (nRegistersRequired <= 2){
                    assert(nRegistersRequired != 2 && "Multiple registers load isnt currently supported.");
                    
                    Register argRegister = regAlloc.allocRegister(RV64_Register(REG_A0 + occupiedXA));
                    generateExprMIR(arg, argRegister, scope, storageScope);
                    regAlloc.freeRegister(argRegister);
                    occupiedXA++;
                    
                }
                else {
                    assert(false && "Integer types should fit in one XLEN length register.");
                }

                
            }
            // floating types
            else if(isFloatType(arg->_type)){
                int nRegistersRequired = alignUpPowerOf2(sizeOfArg, FLEN)/FLEN;
                
                if (nRegistersRequired <= 2){
                    assert(nRegistersRequired != 2 && "Multiple registers load isnt currently supported.");
                    
                    Register argRegister = regAlloc.allocRegister(RV64_Register(REG_FA0 + occupiedFA));
                    generateExprMIR(arg, argRegister, scope, storageScope);
                    regAlloc.freeRegister(argRegister);
                    occupiedFA++;
                }
                else {
                    assert(false && "Float types should fit in one FLEN length register.");
                }
            }
            // struct types
            else{
                assert(false && "Struct arguments are not supported yet.");
                // int nRegistersRequired = alignUpPowerOf2(sizeOfArg, XLEN)/XLEN;
                
                // if (nRegistersRequired > 2){
                //     Register argRegister = regAlloc.allocRegister(RV64_Register(REG_A0 + occupiedXA));
                //     generateExprMIR(arg, argRegister, scope, storageScope);
                //     regAlloc.freeRegister(argRegister);
                //     occupiedXA++;
                // }

                // assert(false && "Float types should fit in one FLEN length register.");
            }

        }

        buffer << "    call " << current->functionCall->funcName << "\n";
        
        
        MIR_Function &foo = this->mir->functions.getInfo(current->functionCall->funcName).info;
        if (foo.returnType.tag != MIR_Datatype::TYPE_VOID){
            Register returnValIn;
            const char* prefix = "";
            const char* suffix1 = "";
            const char* suffix2 = "";

            if (isIntegerType(foo.returnType)){
                returnValIn = regAlloc.allocRegister(REG_A0);
            }
            else if (isFloatType(foo.returnType)){
                returnValIn = regAlloc.allocRegister(REG_FA0);
                prefix = "f";
                suffix1 = ".";
                suffix2 = fInsFloatSuffix(foo.returnType.size);
            }
            else {
                assert(false && "Return types other than integers and floats aren't submitted currently.");
            }

            const char *returnValInName = RV64_RegisterName[regAlloc.resolveRegister(returnValIn)];

            
            buffer << "    " << prefix << "mv" << suffix1 << suffix2 << " " << destName << ", " << returnValInName << "\n";
        
            regAlloc.freeRegister(returnValIn);
        }
        
        // load back register values after function call
        auto restoreRegisters = [&] (RegisterState rState){
            int n = count;
            int allocSize = XLEN * count;
            if (count > 0){
                for (int i = 0; i<RV64_Register::REG_COUNT; i++){
                    if (i == destReg){
                        continue;
                    }
                    if(rState.reg[i].occupied){
                        bool isFloatReg = (RV64Registers[i].type & REG_FLOATING_POINT) != 0;
                        const char* prefix = isFloatReg? "f": "";
                        // only restore the registers except the destination registers since the destination register now contains the return value
                        buffer << "    " << prefix << "l"<< iInsIntegerSuffix(XLEN) << " " << RV64_RegisterName[i] << ", "<< (n-1)*XLEN << "(sp) \n";
                        n--;
                    }
                }
                buffer << "    addi sp, sp, " << allocSize << "\n";
            }
        };
        restoreRegisters(state);
        regAlloc.setRegisterState(RegisterType::REG_CALLER_SAVED, state);

        break;
    }
    default:
        assert(false && "Some stuff is not accounted for.");
        break;
    }
}

/*
    Write assembly out to a given file.
*/
void CodeGenerator::writeAssemblyToFile(const char *filename){
    std::ofstream outFile(filename);
    if (outFile.is_open())
    {
        outFile << rodataSection.str(); // Write the combined assembly to file
        outFile << dataSection.str(); // Write the combined assembly to file
        outFile << textSection.str(); // Write the combined assembly to file
        outFile.close();
        std::cout << "Assembly written to " << filename << std::endl;
    }
    else
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}

void CodeGenerator::printAssembly(){
    std::cout << rodataSection.str() << std::endl;
    std::cout << dataSection.str() << std::endl;
    std::cout << textSection.str() << std::endl;
}
