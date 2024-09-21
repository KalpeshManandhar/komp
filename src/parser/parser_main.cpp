#include "parser.h"



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

        std::cout<<"Symbol table:\n";
        for (auto &pair : p.global.symbols.entries){
            std::cout<<pair.second.identifier <<": ";
            std::cout<<dataTypePrintf(pair.second.info)<<"\n";
        }

        for (auto &pair: p.functions.entries){
            std::cout<<"Function: " <<pair.second.identifier <<"{\n";
            Function *foo = &pair.second.info;
            std::cout<<"\tReturn type: " <<dataTypePrintf(foo->returnType)<<"\n";
            std::cout<<"\tParameters: " <<"{\n";
            for (auto &param: foo->parameters){
                std::cout<<"\t\t"<<param.identifier.string<< " : " <<dataTypePrintf(param.type)<<"\n";
            }
            std::cout<<"\t}\n";
            
            printParseTree(foo->block, 1);
            std::cout<<"}\n";
            
        }

        std::cout<<"Struct table:\n";
        for (auto &pair : p.global.structs.entries){
            Struct strct = pair.second.info;
            std::cout<<"\tStruct " << strct.structName.string<< "{\n";

            for (auto &m: strct.members.entries){
                std::cout<<"\t\t"<<m.second.info.memberName.string <<": " << dataTypePrintf(m.second.info.type)<<"\n"; 
            }
            std::cout<<"\t}\n"; 

        }
    }

    
    return EXIT_SUCCESS;
}