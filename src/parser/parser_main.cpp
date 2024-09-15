#include "parser.h"

std::ostream& operator<<(std::ostream& o, DataType d){
    DataType *type = &d;
    for (int level = 0; level < d.indirectionLevel; level++){
        std::cout<<"*";
    }
    o<<type->type.string;
    return o;
}






int main(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "Usage: %s <c file to parse> [p]\n \t p: for parse program proper", argv[0]);
        return EXIT_FAILURE;
    }

    Tokenizer t;
    t.init();
    t.loadFileToBuffer(argv[1]);
    
    Parser p;
    p.init(&t);

    bool success = false;
    if (argc == 3 && strcmp(argv[2], "p") == 0){
        success = p.parseProgram();
    }
    else{
        success = p.parse();
    }

    if (success){
        fprintf(stdout, "Parse succeeded :)\n");
        
        for (auto &stmt : p.statements){
            printParseTree(stmt);
        }

        for (auto &pair : p.global.symbols.entries){
            std::cout<<pair.second.identifier <<": ";
            std::cout<<pair.second.info<<"\n";
        }

        for (auto &pair: p.functions.entries){
            std::cout<<"Function: " <<pair.second.identifier <<"{\n";
            Function *foo = &pair.second.info;
            std::cout<<"\tReturn type: " <<foo->returnType<<"\n";
            std::cout<<"\tParameters: " <<"{\n";
            for (auto &param: foo->parameters){
                std::cout<<"\t\t"<<param.identifier.string<< " : " <<param.type<<"\n";
            }
            std::cout<<"\t}\n";
            
            printParseTree(foo->block, 1);
            std::cout<<"}\n";
            
        }
    }

    
    return EXIT_SUCCESS;
}