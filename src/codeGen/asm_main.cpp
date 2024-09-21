#include "asm.h"

int main(int argc, char **argv)
{
    if (argc < 2){
        fprintf(stderr, "Usage: %s <c file to parse> [p]\n \t p: for parse program proper", argv[0]);
        return EXIT_FAILURE;
    }

    IR ir;
    ir.t.init();
    ir.t.loadFileToBuffer(argv[1]);

    ir.p.init(&ir.t);
    ir.p.parse();
    std::stringstream buffer;


    for (auto &pair: ir.p.functions.entries)
    {
        ir.funcAssembly(pair.second.identifier,&pair.second.info,buffer);

        Function *foo = &pair.second.info;

        // Appending the function assembly to outputBuffer
        ir.outputBuffer << buffer.str();
        buffer.str("");
        buffer.clear();
        
    }
    ir.writeAssemblyToFile();
    ir.printAssemblyCode();
        
}