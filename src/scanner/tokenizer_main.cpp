#include "tokenizer.h"
#include <iostream>

int main(int argc, char ** argv){
    if (argc < 2){
        std::cout<<"[USAGE] tokenizer.exe <path to c file to tokenize>";
        return -1; 
    }

    Tokenizer t;
    t.loadFileToBuffer(argv[1]);
    Token token = Token{TOKEN_EOF};

    while (true){
        token = t.nextToken();
        std::cout<<TOKEN_STRINGS[token.type]<<"\n";
        if (token.type == TOKEN_EOF){
            break;
        }
    }

    return 0;
    
    
}