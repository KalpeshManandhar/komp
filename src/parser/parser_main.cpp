#include "parser.h"

std::ostream& operator<<(std::ostream& o, DataType d){
    DataType *type = &d;
    for (; type->tag == DataType::TYPE_PTR; type = type->ptrTo){
        o<<"*";
    }
    o<<type->type.string;
    return o;
}






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