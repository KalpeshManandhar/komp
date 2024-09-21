#include <parser/parser.h>
#include <sstream>
#include <fstream>

struct IR
{
    Tokenizer t;
    Parser p;
    std::stringstream buffer;
    std::stringstream outputBuffer;
    const std::string assemblyFilePath="out.s" ;


    void generateAssembly(Node *const current, std::stringstream &buffer);
    void funcAssembly(Splice funcName, Function *foo, std::stringstream &buffer);
    void writeAssemblyToFile();
    void printAssemblyCode();

};