#pragma once

#include <string.h>
#include <unordered_map>
#include <tokenizer/str.h>




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

template <typename T>
struct SymbolTableOrdered{

    struct SymbolTableEntry{
        Splice identifier;
        T info;
    };

    std::unordered_map<uint32_t, SymbolTableEntry> entries;
    std::vector<Splice> order;

    void add(Splice name, T info){
        uint32_t hash = adler32((unsigned char *)name.data, name.len);

        entries.insert({hash, {name, info}});
        order.push_back(name);
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
