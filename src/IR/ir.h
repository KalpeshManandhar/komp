#pragma once

#include "node.h"
#include "symbol-table.h"

#include "mir-node.h"

struct AST{
    StatementBlock global;
    SymbolTable<Function> functions;
};

// mid level IR
struct MIR{
    MIR_Scope* global; 
    SymbolTable<MIR_Function> functions;
};

MIR* transform(AST *ast, Arena *arena);
