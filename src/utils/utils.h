#pragma once

#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>

static int max(int a, int b){
    return (a>b)?a:b;
}

static int min(int a, int b){
    return (a<b)?a:b;
}

static size_t alignUpPowerOf2(size_t address, size_t align){
    return((address + (align - 1)) & ~(align -1));
};

static bool inRange(int64_t val, int64_t a, int64_t b){
    return (val >= a) && (val <= b);
}

#define assertFalse(cond) assert((cond) && false)