#pragma once

#include <IR/ir.h>
#include <sstream>
#include <fstream>


#include "storage.h"


struct GlobalSymbolInfo {
    size_t label;
    Splice value;
    MIR_Datatype type;
};

struct CodeGenerator{
    
    SymbolTable<GlobalSymbolInfo> data;
    SymbolTable<GlobalSymbolInfo> rodata;

    std::stringstream rodataSection;
    std::stringstream dataSection;
    std::stringstream buffer;
    std::stringstream textSection;
    const std::string assemblyFilePath="out.s" ;

    RegisterAllocator regAlloc;
    StackAllocator stackAlloc;
    Labeller labeller;

    AST *ir;
    MIR *mir;
    Arena *arena;

    
    
    // RV64D specific info
    const size_t XLEN = 8;
    const size_t FLEN = 8;
    
    void generateFunctionMIR(MIR_Function *foo, MIR_Scope* global, ScopeInfo *storageScope);
    void generatePrimitiveMIR(MIR_Primitive* p, MIR_Scope* scope , ScopeInfo *storageScope);
    void generateExprMIR(MIR_Expr *current, Register dest, ScopeInfo *storageScope);
    size_t allocStackSpaceMIR(MIR_Scope* scope, ScopeInfo* storage);
    void saveRegisters(RegisterState &rstate, std::stringstream &buffer);
    void restoreRegisters(RegisterState &rstate, std::stringstream &buffer);


    // output
    void writeAssemblyToFile(const char *filename);
    void printAssembly();

public:
    void generateAssemblyFromMIR(MIR *mir);

};