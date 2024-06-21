#include "parser.h"


int main(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "Usage: %s <c file to parse>", argv[0]);
        return EXIT_FAILURE;
    }

    Tokenizer t;
    t.init();
    t.loadFileToBuffer(argv[1]);

    Parser p;
    p.init(&t);

    if (p.parse()){
        fprintf(stdout, "Parse succeeded :)\n");

        printParseTree(p.root);
    }

    
    return EXIT_SUCCESS;
}