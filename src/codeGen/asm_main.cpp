#include "code-gen.h"
#include <parser/parser.h>
#include <debug/debug-print.h>


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
        Arena b;
        b.init(PAGE_SIZE * 2);
        b.createFrame();

        CodeGenerator gen;
        gen.arena = &b;
        
        MIR* mir = transform(ir, &b);

        printMIR(mir);
        printMIRDot(mir);

        gen.generateAssemblyFromMIR(mir);
        gen.printAssembly();
        gen.writeAssemblyToFile("./codegen_output.s");

    }

    a.destroyFrame();
    a.destroy();
    

}