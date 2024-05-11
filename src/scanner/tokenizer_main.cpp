#include "tokenizer.h"
#include <iostream>



int main(int argc, char ** argv){
    if (argc < 2){
        std::cout<<"[USAGE] tokenizer.exe <path to c file to tokenize>";
        return -1; 
    }

    Tokenizer t;
    t.init();
    t.loadFileToBuffer(argv[1]);
    Token token = Token{TOKEN_EOF};

    while (true){
        token = t.nextToken();
        std::cout<<token.string<<":\t"<<TOKEN_PRIMARY_TYPE_STRING[token.type]<<"\t"<<TOKEN_SECONDARY_TYPE_STRING[token.type2]<<"\n";
        if (token.type == TOKEN_EOF){
            break;
        }
    }

    return 0;
    
    
}