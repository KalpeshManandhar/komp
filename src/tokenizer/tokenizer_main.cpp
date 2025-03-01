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

    // Output to md file
    std::ofstream mdFile("token_output.md");
    mdFile << "| Token Value | Token Type |\n";
    mdFile << "|-------------|------------|\n";

    while (true) {
        Token token = t.nextToken();
        // std out write
        std::cout<<token.string<<":\t"<<TOKEN_TYPE_STRING[token.type]<<"\n";

        // md file write
        mdFile << "| " << std::string(token.string.data, token.string.len) << " | " << TOKEN_TYPE_STRING[token.type] << " |\n";

        if (token.type == TOKEN_EOF) {
            break;
        }
    }

    mdFile.close();
    return 0;
    
    
}