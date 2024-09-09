#pragma once

#include <string>
#include <unordered_map>
#include <tokenizer/str.h>

// struct DataType{
//     const char *name;
//     size_t sizeInBytes;
    
//     enum Type{
//         DATATYPE_CHAR,
//         DATATYPE_SHORT,
//         DATATYPE_INT,
//         DATATYPE_LONG,
//         DATATYPE_DOUBLE,
//         DATATYPE_FLOAT,
//         DATATYPE_POINTER,
//         DATATYPE_STRUCT,
//     }type;
// };



// namespace DataTypes{
//     inline DataType Char  = {.name = "char", .sizeInBytes = 1, .type = DataType::Type::DATATYPE_CHAR};
//     inline DataType Short =  {.name = "short", .sizeInBytes = 2, .type = DataType::Type::DATATYPE_SHORT};
//     inline DataType Int   = {.name = "int",  .sizeInBytes = 4, .type = DataType::Type::DATATYPE_INT};
//     inline DataType Float = {.name = "float", .sizeInBytes = 4, .type = DataType::Type::DATATYPE_FLOAT};
//     inline DataType Long =  {.name = "long", .sizeInBytes = 4, .type = DataType::Type::DATATYPE_LONG};
//     inline DataType Double = {.name = "double", .sizeInBytes = 8, .type = DataType::Type::DATATYPE_DOUBLE};
//     inline DataType Pointer = {.name = "pointer", .sizeInBytes = 8, .type = DataType::Type::DATATYPE_POINTER};
// };

struct DataType{
    enum {
        TYPE_PRIMARY,
        TYPE_STRUCT,
        TYPE_PTR,
        TYPE_VOID,
    }tag;
    union{
        Token type;
        DataType *ptrTo;
    };
};




// yoinked with courtesy from https://en.wikipedia.org/wiki/Adler-32
static uint32_t adler32(unsigned char *data, size_t len) 
{
    const uint32_t MOD_ADLER = 65521;
    uint32_t a = 1, b = 0;
    size_t index;
    
    for (index = 0; index < len; ++index)
    {
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    
    return (b << 16) | a;
}


template <typename T>
struct SymbolTable{
    struct SymbolTableEntry{
        Splice identifier;
        T info;
    };


    // TODO: create a separate declaration entry and also add to declaration node
    std::unordered_map<uint32_t, SymbolTableEntry> entries;

    void add(Splice name, T type){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);

        entries.insert({hash, {name, type}});
    }

    bool existKey(Splice name){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);
        return entries.contains(hash);
    }

    T getInfo(Splice name){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);
        return entries[hash];
    }

};
