#include "code-gen.h"
#include <parser/parser.h>


int main(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "Usage: %s <c file> [p]\n \t p: for parse program proper", argv[0]);
        return EXIT_FAILURE;
    }

    Tokenizer t;
    t.init();
    t.loadFileToBuffer(argv[1]);

    Parser p;
    p.init(&t);

    IR *ir = p.parseProgram();
    
    if (ir){
        CodeGenerator gen;
        gen.generateAssembly(ir);
        gen.printAssembly();
    }
    

}