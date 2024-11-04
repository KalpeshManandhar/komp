#pragma once

#include "node.h"
#include "symbol-table.h"

#include "expanded-node.h"

struct AST{
    StatementBlock global;
    SymbolTable<Function> functions;
};

