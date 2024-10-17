#include "parser.h"



int main(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "Usage: %s <c file to parse> [p]\n \t p: for parse program proper", argv[0]);
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

    IR *ir = NULL;
    if (argc == 3 && strcmp(argv[2], "p") == 0){
        ir = p.parseProgram();
    }
    else{
        ir = p.parse();
    }

    if (ir){
        fprintf(stdout, "Parse succeeded :)\n");
        
        for (auto &stmt : ir->global.statements){
            printParseTree(stmt);
        }


        for (auto &pair: ir->functions.entries){
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
        
        if (ir->global.symbols.count() > 0){
            std::cout<<"Symbol table:\n";
            for (auto &pair : ir->global.symbols.entries){
                std::cout<<"\t"<<pair.second.identifier <<": " << dataTypePrintf(pair.second.info)<<"\n";
            }
        }
        
        if (ir->global.structs.count() > 0){
            std::cout<<"Struct table:\n";
            for (auto &pair : ir->global.structs.entries){
                Struct strct = pair.second.info;
                std::cout<<"\tStruct " << strct.structName.string<< "{\n";

                for (auto &m: strct.members.entries){
                    std::cout<<"\t\t"<<m.second.info.memberName.string <<": " << dataTypePrintf(m.second.info.type)<<"\n"; 
                }
                std::cout<<"\t}\n"; 

            }
        }
        
    }

    a.destroyFrame();
    a.destroy();
    
    return p.errors;
}