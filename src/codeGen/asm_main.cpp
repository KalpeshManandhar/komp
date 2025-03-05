#include "code-gen.h"
#include <parser/parser.h>
#include <debug/debug-print.h>

struct {
    bool print = true;
    const char* outputTo = "./codegen_output.s";
    const char* input;
}config;


int main(int argc, char **argv){
    
    
    if (argc < 2){
        fprintf(stderr, "Usage: %s <c file>", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 0; i<argc; i++){
        if (strcmp(argv[i], "-o") == 0){
            config.outputTo = argv[i+1];
            i++;
        }
        
        else if (strcmp(argv[i], "-no-print") == 0){
            config.print = false;
        }
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
    
    if (!ir){
        printf("Failed! \n");
        return 1;
    }

    

    Arena b;
    b.init(PAGE_SIZE * 2);
    b.createFrame();

    CodeGenerator gen;
    gen.arena = &b;
    
    MIR* mir = transform(ir, &b);
    
    a.destroyFrame();
    a.destroy();
    

    if (config.print){
        printMIR(mir);    
        printMIRDot(mir);
    }
    
    gen.generateAssemblyFromMIR(mir);

    
    gen.writeAssemblyToFile(config.outputTo);
    if (config.print){
        gen.printAssembly();
    }


    printf("Successfully generated! \n");
}