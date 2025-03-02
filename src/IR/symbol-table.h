#pragma once

#include <string.h>
#include <unordered_map>
#include <vector>
#include <tokenizer/str.h>



// Custom hash function using std::string_view
struct SpliceHash {
    std::size_t operator()(const Splice& s) const {
        return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(s.data), s.len));
    }
};


template <typename T>
struct SymbolTable{
    struct SymbolTableEntry{
        Splice identifier;
        T info;
    };

    std::unordered_map<Splice, SymbolTableEntry, SpliceHash>entries;

    void add(Splice name, T info){
        entries.insert({name, {name, info}});
    }

    bool existKey(Splice name){
        return entries.contains(name);
    }

    SymbolTableEntry &getInfo(Splice name){
        return entries[name];
    }
    size_t count(){
        return entries.size();
    }
    void update(Splice name, T info){
        entries[name] = {name, info};
    }

};

template <typename T>
struct SymbolTableOrdered{

    struct SymbolTableEntry{
        Splice identifier;
        T info;
    };

    std::unordered_map<Splice, SymbolTableEntry, SpliceHash> entries;
    std::vector<Splice> order;

    void add(Splice name, T info){
        entries.insert({name, {name, info}});
        order.push_back(name);
    }

    bool existKey(Splice name){
        return entries.contains(name);
    }

    SymbolTableEntry &getInfo(Splice name){
        return entries[name];
    }
    size_t count(){
        return entries.size();
    }
    void update(Splice name, T info){
        entries[name] = {name, info};
    }

};
