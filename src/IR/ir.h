#pragma once

#include "node.h"
#include "symbol-table.h"

struct IR{
    StatementBlock global;
    SymbolTable<Function> functions;
};