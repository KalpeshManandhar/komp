#pragma once

#include <stdint.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string_view>

struct Splice{
    const char *data;
    size_t len;

    // Define equality operator so unordered_map can check for collisions
    bool operator==(const Splice& other) const{
        return len == other.len && strncmp(data, other.data, len) == 0;
    }
};

static int copyToArr(Splice src, char *dest, int destSize){
    int nToCopy = std::min<size_t>(destSize - 1, src.len);
    memcpy(dest, src.data, nToCopy);
    dest[nToCopy] = 0;
    return nToCopy;
}

static bool compare(Splice s, const char* str){
    if (strlen(str) != s.len) 
        return false; 
    return strncmp(s.data, str, s.len) == 0;
}


static bool compare(Splice a, Splice b){
    if (a.len != b.len) 
        return false; 
    return strncmp(a.data, b.data, a.len) == 0;
}

static std::ostream & operator <<(std::ostream &out, Splice s){
    if (s.len > 0){
        out.write(s.data, s.len);
    }
    return out;
}
