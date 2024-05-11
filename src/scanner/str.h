#pragma once

#include <stdint.h>
#include <cstring>
#include <iostream>
#include <fstream>


struct Splice{
    const char *data;
    size_t len;
};

static int copyToArr(Splice src, char *dest, int destSize){
    int nToCopy = std::min<size_t>(destSize - 1, src.len);
    memcpy(dest, src.data, nToCopy);
    dest[nToCopy] = 0;
    return nToCopy;
}

static bool compare(Splice s, const char* str){
    int last = std::min(strlen(str), s.len);
    return strncmp(s.data, str, last);
}

static std::ostream & operator <<(std::ostream &out, Splice s){
    if (s.len > 0){
        out.write(s.data, s.len);
    }
    return out;
}
