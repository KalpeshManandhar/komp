#pragma once

#include <IR/ir.h>
#include <sstream>
#include <fstream>


#include "storage.h"
#include "label.h"


struct SymbolInfo {
    size_t label;
    Splice value;
    MIR_Datatype type;
};

struct CodeGenerator{
    
    SymbolTable<SymbolInfo> rodata;

    std::stringstream rodataSection;
    std::stringstream dataSection;
    std::stringstream buffer;
    std::stringstream textSection;
    const std::string assemblyFilePath="out.s" ;

    RegisterAllocator regAlloc;
    StackAllocator stackAlloc;
    Labeller labeller;

    AST *ir;
    Arena *arena;
    
    // RV64 specific info
    const size_t XLEN = 8;
    const size_t FLEN = 8;

    
    size_t sizeOfType(DataType d, StatementBlock* scope);
    size_t alignmentOfType(DataType d, StatementBlock* scope);
    


    // expand Subexpr to lower level
    MIR_Expr* expandSubexpr(const Subexpr *expr, StatementBlock *scope);
    void insertTypeCast(MIR_Expr *d);
    void calcStructMemberOffsets(StatementBlock *scope);
    MIR_Datatype convertToLowerLevelType(DataType d, StatementBlock *scope);

    

    // assembly generation (for AST)
    void generateNode(const Node *current, StatementBlock *scope, ScopeInfo *storageScope);
    void generateFunction(Function *foo, ScopeInfo *storageScope);
    void generateSubexpr(const Subexpr *expr, StatementBlock *scope, Register destReg, ScopeInfo *storageScope);
    void generateExpandedExpr(MIR_Expr *current, Register dest, StatementBlock *scope, ScopeInfo *storageScope);
    void generateTypeCasts(const MIR_Expr *cast, Register destReg, StatementBlock *scope, ScopeInfo *storageScope);


    void generateFunctionMIR(MIR_Function *foo, MIR_Scope* global, ScopeInfo *storageScope);
    void generatePrimitiveMIR(MIR_Primitive* p, MIR_Scope* scope , ScopeInfo *storageScope);
    void generateExprMIR(MIR_Expr *current, Register dest, MIR_Scope* scope, ScopeInfo *storageScope);
    size_t allocStackSpaceMIR(MIR_Scope* scope, ScopeInfo* storage);

    



    size_t allocStackSpace(StatementBlock *scope, ScopeInfo *storage);
    
    // output
    void writeAssemblyToFile(const char *filename);
    void printAssembly();

public:
    void generateAssembly(AST *ir);
    void generateAssemblyFromMIR(MIR *mir);


};