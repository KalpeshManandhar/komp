#include <parser/parser.h>

struct IR
{
    Tokenizer t;
    Parser p;

    void generateAssembly(Node *const current) ;
    void funcAssembly(Splice funcName, Function *foo) ;
    
};