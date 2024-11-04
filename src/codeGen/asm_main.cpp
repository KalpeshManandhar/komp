#include "code-gen.h"
#include <parser/parser.h>


int main(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "Usage: %s <c file>", argv[0]);
        return EXIT_FAILURE;
    }

    Tokenizer t;
    t.init();
    t.loadFileToBuffer(argv[1]);
    
    Arena a;
    a.init(PAGE_SIZE * 2);
    a.createFrame();


    Parser p;
    p.init(&t, &a);

    AST *ir = p.parseProgram();
    
    if (ir){
        CodeGenerator gen;
        gen.arena = &a;
        
        gen.generateAssembly(ir);
        gen.printAssembly();
        gen.writeAssemblyToFile("./codegen_output.s");
    }

    a.destroyFrame();
    a.destroy();
    

}