#pragma once

#include <stdint.h>

static int max(int a, int b){
    return (a>b)?a:b;
}

static int min(int a, int b){
    return (a<b)?a:b;
}

static size_t alignUpPowerOf2(size_t address, size_t align){
    return((address + (align - 1)) & ~(align -1));
};