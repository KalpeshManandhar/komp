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
    

    void generateNode(const Node *current, StatementBlock *scope, ScopeInfo *storageScope);
    void generateFunction(Function *foo, ScopeInfo *storageScope);
    void generateSubexpr(const Subexpr *expr, StatementBlock *scope, Register destReg, ScopeInfo *storageScope);
    size_t allocStackSpace(StatementBlock *scope, ScopeInfo *storage);
    

    void writeAssemblyToFile(const char *filename);
    void printAssembly();

public:
    void generateAssembly(IR *ir);

};