#pragma once

#include <IR/ir.h>
#include <sstream>
#include <fstream>


#include "storage.h"



struct CodeGenerator{
    std::stringstream buffer;
    std::stringstream outputBuffer;
    const std::string assemblyFilePath="out.s" ;

    RegisterAllocator regAlloc;
    StackAllocator stackAlloc;

    AST *ir;
    Arena *arena;
    
    
    // RV64 specific info
    size_t sizeOfType(DataType d, StatementBlock* scope);
    


    // expand Subexpr to lower level
    Exp_Expr* expandSubexpr(const Subexpr *expr, StatementBlock *scope);
    void insertTypeCast(Exp_Expr *d);
    void calcStructMemberOffsets(StatementBlock *scope);

    

    // assembly generation
    void generateNode(const Node *current, StatementBlock *scope, ScopeInfo *storageScope);
    void generateFunction(Function *foo, ScopeInfo *storageScope);
    void generateSubexpr(const Subexpr *expr, StatementBlock *scope, Register destReg, ScopeInfo *storageScope);
    void generateExpandedExpr(Exp_Expr *current, Register dest, StatementBlock *scope, ScopeInfo *storageScope);
    size_t allocStackSpace(StatementBlock *scope, ScopeInfo *storage);
    
    // output
    void writeAssemblyToFile(const char *filename);
    void printAssembly();

public:
    void generateAssembly(AST *ir);

};