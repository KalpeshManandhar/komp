#pragma once

#include <IR/ir.h>
#include <sstream>
#include <fstream>


struct CodeGenerator{
    std::stringstream buffer;
    std::stringstream outputBuffer;
    const std::string assemblyFilePath="out.s" ;
    

    void generateNode(const Node *current, StatementBlock *scope);
    void generateFunction(Function *foo);
    void generateSubexpr(const Subexpr *expr, StatementBlock *scope, const char *destReg, const char* tempReg);
    

    void writeAssemblyToFile(const char *filename);
    void printAssembly();


public:
    void generateAssembly(IR *ir);

};