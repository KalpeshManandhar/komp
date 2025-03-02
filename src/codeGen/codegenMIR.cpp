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




/*
    Get depth of an MIR_Expr node
*/ 
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
        int operand = getDepth(expr->unary.expr);
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
    Assign memory locations to each variable and return the space required.
*/
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
        MemBlock mem = stackAlloc.allocate(totalSize);
        size_t offset = 0;
        
        // assign memory offsets as storage info for each variable 
        for (auto &var: scope->symbols.order){
            MIR_Datatype dt = scope->symbols.getInfo(var).info;
            
            size_t size = dt.size;
            size_t alignment = dt.alignment;
            
            offset = alignUpPowerOf2(offset, alignment);

            StorageInfo s;
            s.memAddress.start = mem.start + offset;
            s.memAddress.size = size;
            
            offset += size;
            s.tag = StorageInfo::STORAGE_MEMORY;

            storage->symbols.add(var, s);

        }    

    }

    return totalSize;
}




/*
    Generate assembly for an MIR_Primitive node.
*/
void CodeGenerator :: generatePrimitiveMIR(MIR_Primitive* p, MIR_Scope* scope, ScopeInfo *storageScope){
    if (!p){
        return;
    }

    switch (p->ptag){
        case MIR_Primitive::PRIM_IF:{
            MIR_If* inode = (MIR_If*) p;
            

            while (inode){
                if (inode->condition){
                    Register condition = regAlloc.allocVRegister(REG_SAVED);
                    // compute the condition
                    generateExprMIR(inode->condition, condition, storageScope);
                    
                
                    const char *regName = RV64_RegisterName[regAlloc.resolveRegister(condition)];
                
                    // branch if condition is false
                    buffer << "    beqz " << regName << ", " << ".L"<< inode->falseLabel <<"\n";

                    regAlloc.freeRegister(condition);
                    
                    // generate the block
                    generatePrimitiveMIR(inode->scope, scope, storageScope);
                    
                    // only jump if there is something between the label and code block
                    if (inode->next){
                        buffer << "    j " << ".L"<< inode->endLabel<<"\n";
                    }
                    
                    // the label for if the condition is false
                    buffer << ".L" << inode->falseLabel << ":\n";
                    
                }
                else{
                    assert(inode->next == NULL && "Else cannot have a 'next' branch block. Must be NULL.");
                    generatePrimitiveMIR(inode->scope, scope, storageScope);

                    buffer << ".L" << inode->endLabel << ":\n";
                }

                inode = inode->next;
            }

            break;
        }
        case MIR_Primitive::PRIM_LOOP:{
            MIR_Loop* lnode = (MIR_Loop*) p;
        
            Register condition = regAlloc.allocVRegister(REG_SAVED);

            
            // start of while loop 
            buffer << ".L" << lnode->startLabel << ":\n";
            
            // check condition
            generateExprMIR(lnode->condition, condition, storageScope);
            
            const char *regName = RV64_RegisterName[regAlloc.resolveRegister(condition)];
            
            // break out if condition is false
            buffer << "    beqz " << regName << ", " << ".L"<< lnode->endLabel <<"\n";
            
            regAlloc.freeRegister(condition);
            
            // generate the block
            generatePrimitiveMIR(lnode->scope, scope, storageScope);
            
            
            Register update = regAlloc.allocVRegister(REG_SAVED);
            buffer << ".L" << lnode->updateLabel << ":\n";
            
            generateExprMIR(lnode->update, update, storageScope);
            
            // jump to loop start
            buffer << "    j " << ".L"<< lnode->startLabel<<"\n";

            // out of loop
            buffer << ".L" << lnode->endLabel << ":\n";
            
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
                generateExprMIR(rnode->returnValue, a0, storageScope);
                regAlloc.freeRegister(a0);
            }
            else if (isFloatType(foo.returnType)){
                assert(foo.returnType.size <= FLEN && "Return values are only supported in one register.");

                Register fa0 = regAlloc.allocRegister(REG_FA0);
                generateExprMIR(rnode->returnValue, fa0, storageScope);
                
                regAlloc.freeRegister(fa0);
            }
            else {
                assert(false && "Struct returns not implemented yet.");
            }
            buffer << "    j ." << rnode->funcName << "_ep\n";


            break;
        }
        case MIR_Primitive::PRIM_JUMP:{
            MIR_Jump* jnode = (MIR_Jump*) p;
            buffer << "    j .L" << jnode->jumpLabel << "\n";
            break;
        }
        case MIR_Primitive::PRIM_EXPR:{
            MIR_Expr* enode = (MIR_Expr*) p;

            bool isFloatExpr = isFloatType(enode->_type);
            int mask = isFloatExpr? REG_FLOATING_POINT : 0; 

            Register rtmp = regAlloc.allocVRegister(RegisterType(REG_SAVED | mask));

            generateExprMIR(enode, rtmp, storageScope);
            
            regAlloc.freeRegister(rtmp);

            break;
        }
        case MIR_Primitive::PRIM_SCOPE:{
            MIR_Scope* snode = (MIR_Scope*) p;

            ScopeInfo storage;
            storage.parent = storageScope;

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
    
    // allocate stack space for parameters and local variables
    ScopeInfo storage;
    storage.parent = storageScope;

    size_t totalSize = allocStackSpaceMIR((MIR_Scope*) foo, &storage);
    
    for (auto &prim : foo->statements){
        generatePrimitiveMIR(prim, (MIR_Scope*) foo, &storage);
    }

    
    // function prologue
    std::stringstream prologue;
    int64_t prologueOffset = 16;
    
    
    
    // save the callee saved registers which were actually used by the function
    RegisterState isUsedX = regAlloc.getRegisterUsage(RegisterType(REG_CALLEE_SAVED));
    RegisterState isUsedF = regAlloc.getRegisterUsage(RegisterType(REG_CALLEE_SAVED | REG_FLOATING_POINT));
    RegisterState state = {0};
    
    int count = 0;
    for (int i=0; i<REG_COUNT; i++){
        state.reg[i] = isUsedX.reg[i].occupied? isUsedX.reg[i] : isUsedF.reg[i];
        if (state.reg[i].occupied) 
        count++;
    }
    
    prologue << "    .globl " << foo->funcName << "\n";
    prologue << foo->funcName << ":\n";
    
    prologue << "    addi sp, sp, " << -prologueOffset << "\n"; // allocate stack space for return address and previous frame pointer.
    prologue << "    sd ra, 8(sp)\n";     // save return address
    prologue << "    sd fp, 0(sp)\n";     // save prev frame pointer
    saveRegisters(state, prologue);
    
    prologue << "    mv fp, sp\n";        // save current stack pointer 
    
    // allocate space for local variables
    if (totalSize > 0)
        prologue << "    addi sp, sp, -" << totalSize << "\n"; 
    

    
    // follows the LP64D ABI
    // available register counts
    int totalAvailableXA = (REG_A7 - REG_A0 + 1);
    int totalAvailableFA = (REG_FA7 - REG_FA0 + 1);
    int occupiedXA = 0;
    int occupiedFA = 0;


    int64_t stackOffset = prologueOffset + alignUpPowerOf2(count * XLEN, 16);

    // copy parameters from registers to stack
    for (int paramNo = 0; paramNo < foo->parameters.size(); paramNo++){
        MIR_Datatype typeOfParam = foo->parameters[paramNo].type;
        size_t sizeOfParam = typeOfParam.size;
        
        StorageInfo sInfo = storage.symbols.getInfo(foo->parameters[paramNo].identifier).info;
        
        int *registerFileInUse = 0;
        int registerSize = 0;
        int totalAvailable = 0;
        RV64_Register registerFileStart = REG_A0;
        const char* prefix = "";

        // integer types
        if (isIntegerType(typeOfParam)){
            registerFileInUse = &occupiedXA;
            registerSize = XLEN;
            registerFileStart = REG_A0;
            totalAvailable = totalAvailableXA;
        }
        // floating point types
        else if (isFloatType(typeOfParam)){
            registerFileInUse = &occupiedFA;
            registerSize = FLEN;
            registerFileStart = REG_FA0;
            totalAvailable = totalAvailableFA;
            prefix = "f";
        }
        else {
            registerFileInUse = &occupiedXA;
            registerSize = XLEN;
            registerFileStart = REG_A0;
            totalAvailable = totalAvailableXA;
        }

        int nRegistersRequired = alignUpPowerOf2(sizeOfParam, registerSize) / registerSize;

        
        // for each reglen required
        for (int i=0; i<nRegistersRequired; i++){
            int64_t destOffset = stackAlloc.offsetFromBase(sInfo.memAddress) + i*registerSize;
            int64_t remaining = max(sizeOfParam - i * registerSize, 0); 
            int64_t sizeToUse = min(remaining, registerSize);

            // if doesnt fit in 2*RLEN, it is in memory
            // OR if fits in 2*RLEN but out of arg registers, it is in memory
            if (nRegistersRequired > 2 || *registerFileInUse == totalAvailable){
                Register value = regAlloc.allocVRegister(REG_ANY);
                const char* regName = RV64_RegisterName[regAlloc.resolveRegister(value)];
                
                // load from memory to register
                stackOffset = alignUpPowerOf2(stackOffset, typeOfParam.alignment);
                int64_t srcOffset = stackOffset;
                prologue << "    l" << iInsIntegerSuffix(sizeToUse) << " " << regName << ", " << srcOffset << "(fp)\n";
                
                // put into temp region 
                prologue << "    s" << iInsIntegerSuffix(sizeToUse) << " " << regName << ", " << destOffset << "(fp)\n"; 
                
                regAlloc.freeRegister(value);
                stackOffset += sizeToUse;
            }
            // argument is in register, so store to temp region 
            else {
                Register argRegister = regAlloc.allocRegister(RV64_Register(registerFileStart + (*registerFileInUse)));
                const char* argRegName = RV64_RegisterName[regAlloc.resolveRegister(argRegister)];
                    
                prologue << "    " << prefix << "s" << iInsIntegerSuffix(sizeToUse) << " " << argRegName << ", " << destOffset << "(fp)\n"; 
                regAlloc.freeRegister(argRegister);
                
                (*registerFileInUse)++;
            }

        }

    }

    


    
    
    // function epilogue
    std::stringstream epilogue;
    epilogue << "."<<foo->funcName << "_ep:\n";

    
    // deallocate stack space
    stackAlloc.deallocate(totalSize);
    if (totalSize > 0)
        epilogue << "    addi sp, sp, " << totalSize << "\n"; 
    
    
    epilogue << "    mv sp, fp\n";       // restore stack pointer

    // restore the callee saved registers
    restoreRegisters(state, epilogue);

    epilogue << "    ld fp, 0(sp)\n";    // restore previous frame pointer
    epilogue << "    ld ra, 8(sp)\n";    // restore return address
    epilogue << "    addi sp, sp, " << prologueOffset << "\n"; // deallocate stack space
    epilogue << "    ret\n\n\n";             // return from function
    

    buffer.str(prologue.str() + buffer.str() + epilogue.str());
}





/*
    Generate assembly from MIR. Self explanatory really.
*/
void CodeGenerator :: generateAssemblyFromMIR(MIR *mir){
    this->mir = mir;

    ScopeInfo s;
    s.parent = 0;
    
    // add the global symbols to global symbol table
    for (auto &symbolName : mir->global->symbols.order){
        MIR_Datatype type = mir->global->symbols.getInfo(symbolName).info;
        
        Label label = labeller.label();
        StorageInfo inDataSection;
        inDataSection.tag = StorageInfo::STORAGE_LABEL;
        inDataSection.label = label;
        inDataSection.size = type.size;
        s.symbols.add(symbolName, inDataSection);
        
        GlobalSymbolInfo gSymbol;
        gSymbol.label = label;
        gSymbol.type = type;
        gSymbol.value = Splice{.data = "0", .len = 1};
        data.add(symbolName, gSymbol);
        
    }
    
    // change the init values for each of the global symbols
    for (auto &statement : mir->global->statements){
        assert(statement->ptag == MIR_Primitive::PRIM_EXPR);
        MIR_Expr* assignment = (MIR_Expr*) statement;
        
        assert(assignment->tag == MIR_Expr::EXPR_STORE);
        assert(assignment->store.right->tag == MIR_Expr::EXPR_LOAD_IMMEDIATE);
        
        assert(assignment->store.left->tag == MIR_Expr::EXPR_ADDRESSOF);
        MIR_Expr* addressOf = assignment->store.left;
        
        GlobalSymbolInfo symbol = data.getInfo(addressOf->addressOf.symbol).info;
        symbol.value = assignment->store.right->immediate.val;

        data.update(addressOf->addressOf.symbol, symbol);
    }
    

    textSection << "    .section     .text\n";
    for (auto &pair: mir->global->statements){

    }


    
    // generate functions
    for (auto &pair : mir->functions.entries){
        // register allocation and stack allocation for each function is independent
        // register allocation is independent as who-calls-who isn't tracked for global allocation.
        // stack allocation is independent as all allocations are done with respect to the stack base which is always at 0.
        regAlloc = RegisterAllocator{0};
        stackAlloc = StackAllocator{0};

        generateFunctionMIR(&pair.second.info, mir->global, &s);

        textSection << buffer.str();
        buffer.str("");
        buffer.clear();
    }

    rodataSection << "    .section     .rodata\n";
    dataSection << "    .section     .data\n";
    
    // write out all the symbols in .rodata section
    for (auto &rodataSymbol : rodata.entries){
        GlobalSymbolInfo symbol = rodataSymbol.second.info;
        
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
        // string
        case MIR_Datatype::TYPE_PTR:
        case MIR_Datatype::TYPE_ARRAY:{
            rodataSection << "    .string " << symbol.value << "\n";
            break;
        }
        default:
            break;
        }        
    }

    // write out all the symbols in .rodata section
    for (auto &dataSymbol : data.entries){
        GlobalSymbolInfo symbol = dataSymbol.second.info;
        
        dataSection << ".symbol" << symbol.label << ":\n";
        
        switch (symbol.type.tag) {
        case MIR_Datatype::TYPE_U8:
        case MIR_Datatype::TYPE_I8:{
            dataSection << "    .byte "  << symbol.value << "\n";
            break;
        }
        case MIR_Datatype::TYPE_U16:
        case MIR_Datatype::TYPE_I16:{
            dataSection << "    .half "  << symbol.value << "\n";
            break;
        }
        case MIR_Datatype::TYPE_U32:
        case MIR_Datatype::TYPE_I32:{
            dataSection << "    .word "  << symbol.value << "\n";
            break;
        }
        case MIR_Datatype::TYPE_U64:
        case MIR_Datatype::TYPE_I64:{
            dataSection << "    .dword "  << symbol.value << "\n";
            break;
        }
        case MIR_Datatype::TYPE_F32:{
            Number value = f32FromString(symbol.value.data);
    
            dataSection << "    .word "  << value.u32[0] << "\n";
            break;
        }
        case MIR_Datatype::TYPE_F64:{
            Number value = f64FromString(symbol.value.data);
    
            dataSection << "    .word "  << value.u32[0] << "\n";
            dataSection << "    .word "  << value.u32[1] << "\n";
            break;
        }
        // string
        case MIR_Datatype::TYPE_PTR:
        case MIR_Datatype::TYPE_ARRAY:{
            dataSection << "    .string " << symbol.value << "\n";
            break;
        }
        default:
            break;
        }        
    }

}


StorageInfo CodeGenerator :: accessLocation(Splice symbolName, ScopeInfo* storageScope){
    // find the scope where the variable is found
    auto getAddressScope = [&](Splice symbol) -> ScopeInfo*{
        ScopeInfo *current = storageScope;

        while (current){
            if (current->symbols.existKey(symbol)){
                return current;
            }
            current = current->parent;
        }

        return 0;
    };
        
    // resolve the variable into an address
    ScopeInfo *storage = getAddressScope(symbolName);
    assert(storage != NULL);
    
    return storage->symbols.getInfo(symbolName).info;
}




/*
    Generate assembly for the expanded IR.
    current : The node to generate assembly for.
    dest    : The register to put the result in.    

*/
void CodeGenerator::generateExprMIR(MIR_Expr *current, Register dest, ScopeInfo *storageScope){
    if (!current){
        return;
    }

    switch (current->tag)
    {
    case MIR_Expr::EXPR_LOAD_IMMEDIATE:{
        // Puts the immediate value in the destination register.
        RV64_Register destReg = regAlloc.resolveRegister(dest);
        const char *destName = RV64_RegisterName[destReg];
        
        // load immediate value into a register
        if (isIntegerType(current->_type)){
            // for string literal, load address
            if (current->_type.tag == MIR_Datatype::TYPE_PTR || current->_type.tag == MIR_Datatype::TYPE_ARRAY){
                if (!rodata.existKey(current->immediate.val)){
                    rodata.add(current->immediate.val, GlobalSymbolInfo{.label = labeller.label(), .value = current->immediate.val, .type = current->_type});
                }
                
                GlobalSymbolInfo stringLiteralInfo = rodata.getInfo(current->immediate.val).info;

                buffer << "    la " << destName << ", .symbol" << stringLiteralInfo.label << "\n";
            }
            else{
                buffer << "    li " << destName << ", " << current->immediate.val << "\n";
            }
        }

        // since there is no instruction in RV64 to load a immediate value into a floating point register
        else if (isFloatType(current->_type)){
            if (!rodata.existKey(current->immediate.val)){
                rodata.add(current->immediate.val, GlobalSymbolInfo{.label = labeller.label(), .value = current->immediate.val, .type = current->_type});
            }
            
            GlobalSymbolInfo fpLiteralInfo = rodata.getInfo(current->immediate.val).info;

            Register fpLiteralAddress = regAlloc.allocVRegister(RegisterType::REG_ANY);
            const char* fpLiteralAddressName = RV64_RegisterName[regAlloc.resolveRegister(fpLiteralAddress)];
            
            buffer << "    lui " << fpLiteralAddressName << ", \%hi(.symbol" << fpLiteralInfo.label << ")" << "\n";
            buffer << "    fl" << iInsIntegerSuffix(current->_type.size) << " " << destName << ", \%lo(.symbol" << fpLiteralInfo.label << ")(" << fpLiteralAddressName << ")\n";
        
            regAlloc.freeRegister(fpLiteralAddress);
        }
        
        break;
    }

    case MIR_Expr::EXPR_ADDRESSOF:{
        break;
    }
    
    case MIR_Expr::EXPR_LOAD_ADDRESS:{
        // Loads the destination register with the given address.
        MIR_Expr *base = current->loadAddress.base;
        
        
        
        // address is just resolved
        if(base->tag == MIR_Expr::EXPR_ADDRESSOF){
            StorageInfo location = accessLocation(base->addressOf.symbol, storageScope);
            
            const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
            
            if (location.tag == StorageInfo::STORAGE_MEMORY){
                // load address + offset into a register
                buffer << "    addi " << destName << ", fp, " << stackAlloc.offsetFromBase(location.memAddress) + current->loadAddress.offset << "\n";
            }
            else if (location.tag == StorageInfo::STORAGE_LABEL){
                buffer << "    la " << destName << ", .symbol" << location.label << "\n";
            }
            
        }
        // address is loaded into register
        else{
            // resolve variable into address/load address into regsister
            generateExprMIR(base, dest, storageScope);

            const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];

            
            // load address + offset into a register
            buffer << "    addi " << destName << ", " << destName << ", " << current->loadAddress.offset << "\n";
        }

        break;
    }
    
    case MIR_Expr::EXPR_LOAD:{
        // Load the value at given address + offset and put it into the destination register.

        
        bool isFloatExpr = current->load.type == MIR_Expr::LoadType::EXPR_FLOAD;
        
        const char* prefix = isFloatExpr? "f" : "";
        
        
        // if the given address is a direct AddressOf node, then the address can be used instead of loading it into a register first.
        if (current->load.base->tag == MIR_Expr::EXPR_ADDRESSOF){
            const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
            MIR_Expr *base = current->load.base;
            StorageInfo location = accessLocation(base->addressOf.symbol, storageScope);
            
            if (location.tag == StorageInfo::STORAGE_MEMORY){
                // load the value and load into destination register
                buffer << "    " << prefix << "l" << iInsIntegerSuffix(current->load.size) << " " << destName << ", " << stackAlloc.offsetFromBase(location.memAddress) + current->load.offset << "(fp)\n";
                return;
            }
            
            else if (location.tag == StorageInfo::STORAGE_LABEL){
                buffer << "    la " << destName << ", .symbol" << location.label << "\n";
            }
        }
        else{
            // load/resolve the address into the register first
            generateExprMIR(current->load.base, dest, storageScope); 
        }

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];

        // load value and load   
        buffer << "    " << prefix << "l" << iInsIntegerSuffix(current->load.size) << " " << destName << ", " << current->load.offset << "(" << destName << ")\n";
        
        break;
    }
    
    case MIR_Expr::EXPR_STORE:{
        // Store the given rvalue in the address of the lvalue, and also load it into the destination register.
        
        // Load the rvalue into the destination register
        generateExprMIR(current->store.right, dest, storageScope);
        

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        bool isFloatExpr = isFloatType(current->_type);
        
        const char* prefix = isFloatExpr? "f" : "";
        
        // else, load the address into a temporary register first 
        Register temp = regAlloc.allocVRegister(REG_SAVED);

        // if the lvalue has a direct address, use that directly instead of loading it to a register first
        if (current->store.left->tag == MIR_Expr::EXPR_ADDRESSOF){
            MIR_Expr* leftAddress = current->store.left;
            StorageInfo location = accessLocation(leftAddress->addressOf.symbol, storageScope);

            if (location.tag  == StorageInfo::STORAGE_MEMORY){
                // store the value at (address + offset)
                buffer << "    " << prefix << "s" << iInsIntegerSuffix(current->store.size) << " " << destName << ", " << stackAlloc.offsetFromBase(location.memAddress) + current->store.offset << "(fp)\n";
                return;
            }
            else if (location.tag == StorageInfo::STORAGE_LABEL){
                const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];
                
                buffer << "    la " << tempName << ", .symbol" << location.label << "\n";
            }
        }
        else {
            // get address of lvalue
            generateExprMIR(current->store.left, temp, storageScope);    
        }

        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];

        // store the value at (address + offset)
        buffer << "    " << prefix << "s" << iInsIntegerSuffix(current->store.size) << " " << destName << ", " << current->store.offset << "(" << tempName << ")\n";
        
        regAlloc.freeRegister(temp);
        break;
    }
    
    case MIR_Expr::EXPR_INDEX:{
        // Adds a given index to a given address, to get the correct offset.

        
        Register temp = regAlloc.allocVRegister(REG_SAVED);
        
        // load the given base address into register
        generateExprMIR(current->index.base, dest, storageScope);


        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        if (current->index.base->tag == MIR_Expr::EXPR_ADDRESSOF){
            MIR_Expr *address = current->index.base;
            StorageInfo location = accessLocation(address->addressOf.symbol, storageScope);
            
            if (location.tag == StorageInfo::STORAGE_MEMORY){
                buffer << "    addi " << destName << ", fp, " << stackAlloc.offsetFromBase(location.memAddress) << "\n";
            }
            else if (location.tag == StorageInfo::STORAGE_LABEL){
                buffer << "    la " << destName << ", .symbol" << location.label << "\n";
            }
        }


        // calculate index and load it into register
        generateExprMIR(current->index.index, temp, storageScope);
        

        const char *tempName = RV64_RegisterName[regAlloc.resolveRegister(temp)];
        
        // multiply the index with the size to get correct offset 
        if (current->index.size > 1){
            // index x size
            Register indexSize = regAlloc.allocVRegister(REG_SAVED);
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
        left = canDestBeUsed? dest : regAlloc.allocVRegister(RegisterType(((dest.type & REG_FLOATING_POINT) ^ REG_FLOATING_POINT) | REG_SAVED));
        // right allocates of the same type as left 
        right = regAlloc.allocVRegister(RegisterType((left.type & REG_FLOATING_POINT) | REG_SAVED));


        int leftDepth = getDepth(current->binary.left);
        int rightDepth = getDepth(current->binary.right);

        // Generate the one with the greatest depth first so that intermediate values need not be stored.
        if (leftDepth < rightDepth){
            generateExprMIR(current->binary.right, right, storageScope);
            generateExprMIR(current->binary.left, left, storageScope);
        }
        else{
            generateExprMIR(current->binary.left, left, storageScope);
            generateExprMIR(current->binary.right, right, storageScope);
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
            
            case MIR_Expr::BinaryOp::EXPR_UMOD:{
                buffer << "    remu " << destName << ", " << leftName << ", " << rightName << "\n";
                break;
            }
            case MIR_Expr::BinaryOp::EXPR_IMOD:{
                buffer << "    rem " << destName << ", " << leftName << ", " << rightName << "\n";
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
            exprIn = regAlloc.allocVRegister(RegisterType(((dest.type ^ REG_FLOATING_POINT) & REG_FLOATING_POINT) | REG_SAVED));
        }

        // generate the expr to be cast
        generateExprMIR(current->cast.expr, exprIn, storageScope);

        const char* exprInName = RV64_RegisterName[regAlloc.resolveRegister(exprIn)];
        const char* exprDestName = RV64_RegisterName[regAlloc.resolveRegister(dest)];

        
        switch (current->cast._from.tag){
        case MIR_Datatype::TYPE_BOOL:
        case MIR_Datatype::TYPE_PTR:
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
                // since sign is extended by default, there is no need for explicit asm for converting between integer types
                    break;
            }

            
            break;
        }
        
        case MIR_Datatype::TYPE_F32:
        case MIR_Datatype::TYPE_F64:{
            switch (current->cast._to.tag) {
                case MIR_Datatype::TYPE_PTR:
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
        generateExprMIR(current->unary.expr, dest, storageScope);

        const char *destName = RV64_RegisterName[regAlloc.resolveRegister(dest)];
        
        switch (current->unary.op){
            case MIR_Expr::UnaryOp::EXPR_INEGATE:{
                buffer << "    neg " << destName << ", " << destName << "\n";
                break;
            }
            case MIR_Expr::UnaryOp::EXPR_FNEGATE:{
                Register fpZero = regAlloc.allocVRegister(RegisterType(REG_FLOATING_POINT | REG_SAVED));
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
        
        
        RegisterState Xstate = regAlloc.getRegisterState(REG_CALLER_SAVED);
        RegisterState Fstate = regAlloc.getRegisterState(RegisterType(REG_CALLER_SAVED | REG_FLOATING_POINT));
        RegisterState state = {0};
        
        // the number of caller saved registers in use
        int count = 0;
        for (int i = 0; i<RV64_Register::REG_COUNT; i++){
            // dont get state of destination register as it will be overwritten anyway, and should not be restored as well
            // if (i == destReg){
                //     continue;
                // }
            count += (Xstate.reg[i].occupied)? 1 : 0;
            count += (Fstate.reg[i].occupied)? 1 : 0;
            state.reg[i] = (Xstate.reg[i].occupied)? Xstate.reg[i] : Fstate.reg[i];
        }

        // save the caller saved registers
        saveRegisters(state, buffer);


        // follows the LP64D ABI
        // available register counts
        int totalAvailableXA = (REG_A7 - REG_A0 + 1);
        int totalAvailableFA = (REG_FA7 - REG_FA0 + 1);
        int occupiedXA = 0;
        int occupiedFA = 0;
        
        // calculate stack space required
        int stackSpaceRequired = 0;
        for (int argNo = 0; argNo < current->functionCall->arguments.size(); argNo++){
            MIR_Expr* arg = current->functionCall->arguments[argNo];
            size_t sizeOfArg = arg->_type.size;
            
            int *registerFileInUse = 0;
            int registerSize = 0;
            int totalAvailable = 0;

            // integer types
            if (isIntegerType(arg->_type)){
                registerFileInUse = &occupiedXA;
                registerSize = XLEN;
                totalAvailable = totalAvailableXA;
            }
            // floating point types
            else if (isFloatType(arg->_type)){
                registerFileInUse = &occupiedFA;
                registerSize = FLEN;
                totalAvailable = totalAvailableFA;
            }
            else {
                registerFileInUse = &occupiedXA;
                registerSize = XLEN;
                totalAvailable = totalAvailableXA;
            }

            int nRegistersRequired = alignUpPowerOf2(sizeOfArg, registerSize) / registerSize;

            MIR_Expr argCopy = *arg;

            // if doesnt fit in 2*RLEN, then put pointer in register if available
            if (nRegistersRequired > 2 && (*registerFileInUse) < totalAvailable){
                (*registerFileInUse)++;
            }

            // for each reglen required
            for (int i=0; i<nRegistersRequired; i++){
                // if doesnt fit in 2*RLEN, put it into memory
                // OR if fits in 2*RLEN but out of arg registers, put it into memory
                if (nRegistersRequired > 2 || (*registerFileInUse) == totalAvailable){
                    int64_t remaining = max(sizeOfArg - i*registerSize, 0);
                    int64_t sizeToUse = min(remaining, registerSize);
                    stackSpaceRequired = alignUpPowerOf2(stackSpaceRequired, arg->_type.alignment);
                    stackSpaceRequired += sizeToUse;
                }

                // registers are available, so put in registers
                else {
                    (*registerFileInUse)++;
                }

            }
        }
        stackSpaceRequired = alignUpPowerOf2(stackSpaceRequired, 16);
        
        // allocate stack space if needed
        MemBlock stackSpace = stackAlloc.allocate(stackSpaceRequired);
        if (stackSpaceRequired > 0){
            buffer << "    addi sp, sp, " << -stackSpaceRequired << "\n";
        }

        
        // pass the arguments in registers/memory
        occupiedXA = 0;
        occupiedFA = 0;
        
        
        Register argRegisters[16];
        int64_t stackOffset = 0;

        for (int argNo = 0; argNo < current->functionCall->arguments.size(); argNo++){
            MIR_Expr* arg = current->functionCall->arguments[argNo];
            size_t sizeOfArg = arg->_type.size;
            
            int *registerFileInUse = 0;
            int registerSize = 0;
            int totalAvailable = 0;
            RV64_Register registerFileStart = REG_A0;
            const char* prefix = "";

            // integer types
            if (isIntegerType(arg->_type)){
                registerFileInUse = &occupiedXA;
                registerSize = XLEN;
                registerFileStart = REG_A0;
                totalAvailable = totalAvailableXA;
            }
            // floating point types
            else if (isFloatType(arg->_type)){
                registerFileInUse = &occupiedFA;
                registerSize = FLEN;
                registerFileStart = REG_FA0;
                totalAvailable = totalAvailableFA;
                prefix = "f";
            }
            else {
                registerFileInUse = &occupiedXA;
                registerSize = XLEN;
                registerFileStart = REG_A0;
                totalAvailable = totalAvailableXA;
            }

            int nRegistersRequired = alignUpPowerOf2(sizeOfArg, registerSize) / registerSize;
            int64_t stackOffsetStart = stackOffset;

            MIR_Expr argCopy = *arg;
            // for each reglen required
            for (int i=0; i<nRegistersRequired; i++){
                int64_t remaining = max(sizeOfArg - i*registerSize, 0);
                int64_t sizeToUse = min(remaining, registerSize);
    
                if (arg->tag == MIR_Expr::EXPR_LOAD){
                    *arg = argCopy;
                    arg->load.offset += i * registerSize;
                    arg->load.size = sizeToUse;
                }
                
                // if doesnt fit in 2*RLEN, put it into memory
                // OR if fits in 2*RLEN but out of arg registers, put it into memory
                if (nRegistersRequired > 2 || (*registerFileInUse) == totalAvailable){
                    // load value into temporary register
                    Register value = regAlloc.allocVRegister(REG_ANY);
                    generateExprMIR(arg, value, storageScope);
                    
                    const char* regName = RV64_RegisterName[regAlloc.resolveRegister(value)];
                    
                    // get offset 
                    stackOffset = alignUpPowerOf2(stackOffset, arg->_type.alignment);
                    int64_t offset = stackAlloc.offsetFromBase(stackSpace) + stackOffset;

                    // store value in stack
                    buffer << "    " << prefix << "s" << iInsIntegerSuffix(sizeToUse) << " " << regName << ", " << offset << "(fp)\n";

                    regAlloc.freeRegister(value);
                    stackOffset += sizeToUse;
                }

                // registers are available, so put in registers
                else {
                    Register argRegister = regAlloc.allocRegister(RV64_Register(registerFileStart + (*registerFileInUse)));
                    
                    generateExprMIR(arg, argRegister, storageScope);
                    
                    argRegisters[occupiedXA + occupiedFA] = argRegister;
                    (*registerFileInUse)++;
                }

            }

            // if doesnt fit in 2*RLEN, then put pointer in register if available
            if (nRegistersRequired > 2 && (*registerFileInUse) < totalAvailable){
                Register argRegister = regAlloc.allocRegister(RV64_Register(REG_A0 + (*registerFileInUse)));
                const char* regName = RV64_RegisterName[regAlloc.resolveRegister(argRegister)];
                
                // load address of the arg in stack in register
                buffer << "    addi " << regName << ", fp, " << stackAlloc.offsetFromBase(stackSpace) + stackOffsetStart << "\n";
                
                (*registerFileInUse)++;
            }

        }

        buffer << "    call " << current->functionCall->funcName << "\n";
        
        // deallocate stack space if any allocated
        stackAlloc.deallocate(stackSpaceRequired);
        if (stackSpaceRequired > 0){
            buffer << "    addi sp, sp, " << stackSpaceRequired << "\n";
        }

        // free the argument registers
        for (int i=0; i<occupiedXA + occupiedFA; i++){
            regAlloc.freeRegister(argRegisters[i]);
        }
        
        
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

            // move return value into destination register
            RV64_Register destReg = regAlloc.resolveRegister(dest);
            const char *destName = RV64_RegisterName[destReg];

            buffer << "    " << prefix << "mv" << suffix1 << suffix2 << " " << destName << ", " << returnValInName << "\n";
        
            regAlloc.freeRegister(returnValIn);
        }
        
        // restore the saved registers
        restoreRegisters(state, buffer);
        regAlloc.setRegisterState(RegisterType::REG_CALLER_SAVED, state);

        break;
    }
    default:
        assert(false && "Some stuff is not accounted for.");
        break;
    }
}




/*
    Save all the occupied registers in rState to memory.
*/
void CodeGenerator :: saveRegisters(RegisterState &rState, std::stringstream &buffer){
    // save the caller saved registers used in memory
    int count = 0;
    for (int i = 0; i<RV64_Register::REG_COUNT; i++){
        count += (rState.reg[i].occupied)? 1 : 0;
    }

    int n = 0;
    int allocSize = alignUpPowerOf2(count * XLEN, 16);
    MemBlock memAddress = stackAlloc.allocate(allocSize);
    if (count > 0){
        buffer << "    addi sp, sp, -" << allocSize << "\n";
        
        for (int i = 0; i<RV64_Register::REG_COUNT; i++){
            // if (i == destReg){
                //     continue;
                // }
            if(rState.reg[i].occupied){
                int64_t offset = n * XLEN; 
                    
                bool isFloatReg = (RV64Registers[i].type & REG_FLOATING_POINT) != 0;
                const char* prefix = isFloatReg? "f": "";
                
                buffer << "    " << prefix << "s"<< iInsIntegerSuffix(XLEN) << " " << RV64_RegisterName[i] << ", "<< offset << "(sp) \n";
                // buffer << "    " << prefix << "s"<< iInsIntegerSuffix(XLEN) << " " << RV64_RegisterName[i] << ", "<< (n-1)*XLEN << "(sp) \n";
                regAlloc.freeRegister(Register{.id = size_t(i)});
                n++;
            }
        }
    }
}


/*
    Restore all the occupied registers from memory back to their corresponding registers.
*/
void CodeGenerator :: restoreRegisters(RegisterState &rState, std::stringstream &buffer){
    int count = 0;
    for (int i = 0; i<RV64_Register::REG_COUNT; i++){
        count += (rState.reg[i].occupied)? 1 : 0;
    }
    
    int n = count - 1;
    int allocSize = alignUpPowerOf2(XLEN * count, 16);
    MemBlock memAddress = stackAlloc.deallocate(allocSize);
    if (count > 0){
        for (int i = 0; i<RV64_Register::REG_COUNT; i++){
            int regIndex = RV64_Register::REG_COUNT - i - 1;
            // if (i == destReg){
            //     continue;
            // }


            if(rState.reg[regIndex].occupied){
                int64_t offset = n * XLEN; 

                bool isFloatReg = (RV64Registers[regIndex].type & REG_FLOATING_POINT) != 0;
                const char* prefix = isFloatReg? "f": "";
                
                buffer << "    " << prefix << "l"<< iInsIntegerSuffix(XLEN) << " " << RV64_RegisterName[regIndex] << ", "<< offset << "(sp) \n";
                // buffer << "    " << prefix << "l"<< iInsIntegerSuffix(XLEN) << " " << RV64_RegisterName[i] << ", "<< (n-1)*XLEN << "(sp) \n";
                n--;
            }
        }
        buffer << "    addi sp, sp, " << allocSize << "\n";
    }
}





/*
    Write assembly out to a given file.
*/
void CodeGenerator::writeAssemblyToFile(const char *filename){
    std::ofstream outFile(filename);
    if (outFile.is_open())
    {
        outFile << rodataSection.str();
        outFile << dataSection.str();
        outFile << textSection.str();
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
