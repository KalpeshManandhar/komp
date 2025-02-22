#pragma once

#include <stdint.h>

typedef uint64_t Label;


struct Labeller{
    uint64_t number = 0;
    
    Label label(){
        return number++;
    }
        
};