#pragma once

#include <string.h>
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
//         DATATAG_STRUCT,
//     }type;
// };






struct DataType{
    // Pointer types have the ptrTo member set to their corresponding data types
    // example: an "int*" datatype would have its ptrTo pointing to an "int" datatype
    enum {
        TAG_PRIMARY,
        TAG_STRUCT,
        TAG_PTR,
        TAG_VOID,
        TAG_ERROR,
    }tag;

    
    union{
        Token type;
        Token structName;
    };
    int indirectionLevel;
    
    enum Specifiers{
        NONE = 0x0,
        UNSIGNED = (0x1 << 0), 
        VOLATILE = (0x1 << 1), 
        EXTERN = (0x1 << 2), 
        STATIC = (0x1 << 3), 
        CONST = (0x1 << 4), 
        LONG = (0x1 << 5), 
        LONG_LONG = (0x1 << 6), 
        SHORT = (0x1 << 7), 
        SIGNED = (0x1 << 8), 
    };
    int specifierFlags;
    

    bool isSet(Specifiers flag){
        return specifierFlags & flag;
    }

};

namespace DataTypes{
    inline DataType Char  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_CHAR, {"char", sizeof("char") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Int  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Short  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}, .indirectionLevel = 0, .specifierFlags = DataType::Specifiers::SHORT};
    inline DataType Long  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}, .indirectionLevel = 0, .specifierFlags = DataType::Specifiers::LONG};
    inline DataType Long_Long  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}, .indirectionLevel = 0, .specifierFlags = DataType::Specifiers::LONG_LONG};
    inline DataType Float  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_FLOAT, {"float", sizeof("float") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Double = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_DOUBLE, {"double", sizeof("double") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType String = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_STRING_LITERAL, {"char", sizeof("char") - 1}, 0, 0}, .indirectionLevel = 1};
    inline DataType Void = {.tag = DataType::TAG_VOID, .type = {TOKEN_VOID, {"void", sizeof("void") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Error = {.tag = DataType::TAG_ERROR, .type = {TOKEN_ERROR, {"error", sizeof("error") - 1}, 0, 0}, .indirectionLevel = 0};
    inline DataType Struct = {.tag = DataType::TAG_STRUCT, .indirectionLevel = 0};
};

static const char* dataTypePrintf(DataType d){
    static char scratchpad[1024];
    static int sp = 0;

    if (sp > 512){
        sp = 0;
    }

    int start = sp;
    if (d.specifierFlags & DataType::Specifiers::UNSIGNED){
        strcpy_s(&scratchpad[sp], 1024 - sp, "unsigned ");
        sp += strlen("unsigned ");
    }
    if (d.specifierFlags & DataType::Specifiers::LONG){
        strcpy_s(&scratchpad[sp], 1024 - sp, "long ");
        sp += strlen("long ");
    }
    else if (d.specifierFlags & DataType::Specifiers::LONG_LONG){
        strcpy_s(&scratchpad[sp], 1024 - sp, "long long ");
        sp += strlen("long long ");
    }
    else if (d.specifierFlags & DataType::Specifiers::SHORT){
        strcpy_s(&scratchpad[sp], 1024 - sp, "short ");
        sp += strlen("short ");
    }



    for (int i = 0; i<d.type.string.len; i++){
        scratchpad[sp++] = d.type.string.data[i];
    }
    for (int level = 0; level < d.indirectionLevel; level++){
        scratchpad[sp++] = '*';
    }
    scratchpad[sp++] = 0;

    return &scratchpad[start];
}


static bool operator==(DataType a, DataType b){
    return (a.tag == b.tag) && (a.indirectionLevel == b.indirectionLevel) 
            && (a.specifierFlags == b.specifierFlags) && compare(a.type.string, b.type.string);
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
    SymbolTableEntry &getInfo(Splice name){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);
        return entries[hash];
    }
    size_t count(){
        return entries.size();
    }
    void update(Splice name, T info){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);

        entries[hash] = {name, info};
    }

};