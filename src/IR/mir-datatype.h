#pragma once

struct MIR_Datatype {
    enum Tag{
        TYPE_U8,
        TYPE_U16,
        TYPE_U32,
        TYPE_U64,
        TYPE_U128,
        TYPE_I8,
        TYPE_I16,
        TYPE_I32,
        TYPE_I64,
        TYPE_I128,
        TYPE_F16,
        TYPE_F32,
        TYPE_F64,
        TYPE_F128,
        TYPE_STRUCT,
        TYPE_PTR,
        TYPE_ARRAY,
        TYPE_VOID,
    }tag;

    size_t size;
    size_t alignment;
    const char* name;
};


namespace MIR_Datatypes{

    inline MIR_Datatype _void = MIR_Datatype {.tag = MIR_Datatype::TYPE_VOID, .size = 0, .alignment = 0, .name = "_void"};
    inline MIR_Datatype _u8 = MIR_Datatype {.tag = MIR_Datatype::TYPE_U8, .size = 1, .alignment = 1, .name = "_u8"};
    inline MIR_Datatype _u16 = MIR_Datatype {.tag = MIR_Datatype::TYPE_U16, .size = 2, .alignment = 2, .name = "_u16"};
    inline MIR_Datatype _u32 = MIR_Datatype {.tag = MIR_Datatype::TYPE_U32, .size = 4, .alignment = 4, .name = "_u32"};
    inline MIR_Datatype _u64 = MIR_Datatype {.tag = MIR_Datatype::TYPE_U64, .size = 8, .alignment = 8, .name = "_u64"};
    inline MIR_Datatype _u128 = MIR_Datatype {.tag = MIR_Datatype::TYPE_U128, .size = 16, .alignment = 16, .name = "_u128"};
    inline MIR_Datatype _i8 = MIR_Datatype {.tag = MIR_Datatype::TYPE_I8, .size = 1, .alignment = 1, .name = "_i8"};
    inline MIR_Datatype _i16 = MIR_Datatype {.tag = MIR_Datatype::TYPE_I16, .size = 2, .alignment = 2, .name = "_i16"};
    inline MIR_Datatype _i32 = MIR_Datatype {.tag = MIR_Datatype::TYPE_I32, .size = 4, .alignment = 4, .name = "_i32"};
    inline MIR_Datatype _i64 = MIR_Datatype {.tag = MIR_Datatype::TYPE_I64, .size = 8, .alignment = 8, .name = "_i64"};
    inline MIR_Datatype _i128 = MIR_Datatype {.tag = MIR_Datatype::TYPE_I128, .size = 16, .alignment = 16, .name = "_i128"};
    inline MIR_Datatype _f16 = MIR_Datatype {.tag = MIR_Datatype::TYPE_F16, .size = 2, .alignment = 2, .name = "_f16"};
    inline MIR_Datatype _f32 = MIR_Datatype {.tag = MIR_Datatype::TYPE_F32, .size = 4, .alignment = 4, .name = "_f32"};
    inline MIR_Datatype _f64 = MIR_Datatype {.tag = MIR_Datatype::TYPE_F64, .size = 8, .alignment = 8, .name = "_f64"};
    inline MIR_Datatype _f128 = MIR_Datatype {.tag = MIR_Datatype::TYPE_F128, .size = 16, .alignment = 16, .name = "_f128"};
    inline MIR_Datatype _ptr = MIR_Datatype {.tag = MIR_Datatype::TYPE_PTR, .size = 8, .alignment = 8, .name = "_ptr"};
    
    inline MIR_Datatype _struct = MIR_Datatype {.tag = MIR_Datatype::TYPE_STRUCT, .name = "_struct"};
};

static bool isIntegerType(MIR_Datatype type){
    return (type.tag >= MIR_Datatype::TYPE_U8 && type.tag <= MIR_Datatype::TYPE_I128) || type.tag == MIR_Datatype::TYPE_PTR;
}

static bool isFloatType(MIR_Datatype type){
    return type.tag >= MIR_Datatype::TYPE_F16 && type.tag <= MIR_Datatype::TYPE_F128;
}