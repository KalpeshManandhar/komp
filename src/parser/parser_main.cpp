#include "parser.h"

SymbolTable SYMBOL_TABLE;

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
        
        for (auto &stmt : p.statements){
            printParseTree(stmt);
        }

        for (auto &pair : p.global.symbols.variables){
            std::cout<<pair.second.identifier <<": ";
            DataType *type = &pair.second.info;
            for (; type->tag == DataType::TYPE_PTR; type = type->ptrTo){
                std::cout<<"*";
            }
            std::cout<<type->type.string<<"\n";
        }
    }

    
    return EXIT_SUCCESS;
}