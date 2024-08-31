#pragma once

#include <string>
#include <unordered_map>

struct DataType{
    const char *name;
    size_t sizeInBytes;
    
    enum Type{
        DATATYPE_CHAR,
        DATATYPE_SHORT,
        DATATYPE_INT,
        DATATYPE_LONG,
        DATATYPE_DOUBLE,
        DATATYPE_FLOAT,
        DATATYPE_POINTER,
        DATATYPE_STRUCT,
    }type;
};



namespace DataTypes{
    inline DataType Char  = {.name = "char", .sizeInBytes = 1, .type = DataType::Type::DATATYPE_CHAR};
    inline DataType Short =  {.name = "short", .sizeInBytes = 2, .type = DataType::Type::DATATYPE_SHORT};
    inline DataType Int   = {.name = "int",  .sizeInBytes = 4, .type = DataType::Type::DATATYPE_INT};
    inline DataType Float = {.name = "float", .sizeInBytes = 4, .type = DataType::Type::DATATYPE_FLOAT};
    inline DataType Long =  {.name = "long", .sizeInBytes = 4, .type = DataType::Type::DATATYPE_LONG};
    inline DataType Double = {.name = "double", .sizeInBytes = 8, .type = DataType::Type::DATATYPE_DOUBLE};
    inline DataType Pointer = {.name = "pointer", .sizeInBytes = 8, .type = DataType::Type::DATATYPE_POINTER};
};


struct SymbolTableEntry{
    DataType datatype;
};


struct SymbolTable{
    std::unordered_map<std::string, std::string> variables;
};

extern SymbolTable SYMBOL_TABLE;