#pragma once

#include <unordered_map>
#include <tokenizer/str.h>
#include <tokenizer/token.h>

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






struct DataType{
    // Pointer types have the ptrTo member set to their corresponding data types
    // example: an "int*" datatype would have its ptrTo pointing to an "int" datatype
    enum {
        TYPE_PRIMARY,
        TYPE_STRUCT,
        TYPE_PTR,
        TYPE_VOID,
        TYPE_ERROR,
    }tag;
    Token type;
    int indirectionLevel;

};

namespace DataTypes{
    inline DataType Char  = {.tag = DataType::TYPE_PRIMARY, .type = {TOKEN_CHAR, {"char", sizeof("char") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Short  = {.tag = DataType::TYPE_PRIMARY, .type = {TOKEN_SHORT, {"short", sizeof("short") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Int  = {.tag = DataType::TYPE_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Float  = {.tag = DataType::TYPE_PRIMARY, .type = {TOKEN_FLOAT, {"float", sizeof("float") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Long  = {.tag = DataType::TYPE_PRIMARY, .type = {TOKEN_LONG, {"long", sizeof("long") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Double = {.tag = DataType::TYPE_PRIMARY, .type = {TOKEN_DOUBLE, {"double", sizeof("double") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType String = {.tag = DataType::TYPE_PRIMARY, .type = {TOKEN_STRING_LITERAL, {"char", sizeof("char") - 1}, 0, 0}, .indirectionLevel = 1};
    inline DataType Void = {.tag = DataType::TYPE_VOID, .type = {TOKEN_VOID, {"void", sizeof("void") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Error = {.tag = DataType::TYPE_ERROR, .type = {TOKEN_ERROR, {"error", sizeof("error") - 1}, 0, 0}, .indirectionLevel = 0};
};

static const char* dataTypePrintf(DataType d){
    static char scratchpad[1024];
    static int sp = 0;

    if (sp > 512){
        sp = 0;
    }

    int start = sp;
    for (int i = 0; i<d.type.string.len; i++){
        scratchpad[sp++] = d.type.string.data[i];
    }
    for (int level = 0; level < d.indirectionLevel; level++){
        scratchpad[sp++] = '*';
    }
    scratchpad[sp++] = 0;

    return &scratchpad[start];
}




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

    std::unordered_map<uint32_t, SymbolTableEntry> entries;

    void add(Splice name, T info){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);

        entries.insert({hash, {name, info}});
    }

    bool existKey(Splice name){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);
        return entries.contains(hash);

    }
    SymbolTableEntry getInfo(Splice name){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);
        return entries[hash];
    }

};
