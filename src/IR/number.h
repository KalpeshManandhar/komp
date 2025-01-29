#pragma once

#include "mir-datatype.h"
#include <stdint.h>
#include <cstdlib>

struct Number{
    MIR_Datatype type;
    union{
        uint64_t u64[1];
        uint32_t u32[2];
        uint16_t u16[4];
        uint8_t u8[8];
        
        int64_t i64[1];
        int32_t i32[2];
        int16_t i16[4];
        int8_t i8[8];

        float f32[2];
        double f64[1];
    };
};


Number f32FromString(const char* value){
    return Number{.type = MIR_Datatypes::_f32, .f32 = {strtof(value, NULL)}};
}
Number f64FromString(const char* value){
    return Number{.type = MIR_Datatypes::_f64, .f64 = {strtod(value, NULL)}};
}
