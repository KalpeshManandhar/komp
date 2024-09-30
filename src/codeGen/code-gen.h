#pragma once

#include <IR/ir.h>
#include <sstream>
#include <fstream>


struct CodeGenerator{
    std::stringstream buffer;
    std::stringstream outputBuffer;
    const std::string assemblyFilePath="out.s" ;
    

    void generateNode(Node *const current);
    void generateFunction(Function *foo);
    void writeAssemblyToFile();
    void printAssembly();


public:
    void generateAssembly(IR *ir);

};