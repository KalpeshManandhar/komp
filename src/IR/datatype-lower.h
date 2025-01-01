#pragma once

struct Datatype_Low{
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
    }tag;

    size_t size;
    size_t alignment;

};


namespace DatatypeLower{

    inline Datatype_Low _u8 = Datatype_Low {.tag = Datatype_Low::TYPE_U8, .size = 1, .alignment = 1};
    inline Datatype_Low _u16 = Datatype_Low {.tag = Datatype_Low::TYPE_U16, .size = 2, .alignment = 2};
    inline Datatype_Low _u32 = Datatype_Low {.tag = Datatype_Low::TYPE_U32, .size = 4, .alignment = 4};
    inline Datatype_Low _u64 = Datatype_Low {.tag = Datatype_Low::TYPE_U64, .size = 8, .alignment = 8};
    inline Datatype_Low _u128 = Datatype_Low {.tag = Datatype_Low::TYPE_U128, .size = 16, .alignment = 16};
    inline Datatype_Low _i8 = Datatype_Low {.tag = Datatype_Low::TYPE_I8, .size = 1, .alignment = 1};
    inline Datatype_Low _i16 = Datatype_Low {.tag = Datatype_Low::TYPE_I16, .size = 2, .alignment = 2};
    inline Datatype_Low _i32 = Datatype_Low {.tag = Datatype_Low::TYPE_I32, .size = 4, .alignment = 4};
    inline Datatype_Low _i64 = Datatype_Low {.tag = Datatype_Low::TYPE_I64, .size = 8, .alignment = 8};
    inline Datatype_Low _i128 = Datatype_Low {.tag = Datatype_Low::TYPE_I128, .size = 16, .alignment = 16};
    inline Datatype_Low _f16 = Datatype_Low {.tag = Datatype_Low::TYPE_F16, .size = 2, .alignment = 2};
    inline Datatype_Low _f32 = Datatype_Low {.tag = Datatype_Low::TYPE_F32, .size = 4, .alignment = 4};
    inline Datatype_Low _f64 = Datatype_Low {.tag = Datatype_Low::TYPE_F64, .size = 8, .alignment = 8};
    inline Datatype_Low _f128 = Datatype_Low {.tag = Datatype_Low::TYPE_F128, .size = 16, .alignment = 16};
    inline Datatype_Low _struct = Datatype_Low {.tag = Datatype_Low::TYPE_STRUCT};

};




