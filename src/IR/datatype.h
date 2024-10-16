#include <tokenizer/token.h>


struct DataType{
    // Pointer types have the ptrTo member set to their corresponding data types
    // example: an "int*" datatype would have its ptrTo pointing to an "int" datatype
    enum {
        TAG_PRIMARY,
        TAG_STRUCT,
        TAG_PTR,
        TAG_VOID,
        TAG_ERROR,
        TAG_ADDRESS,
        TAG_ARRAY,
    }tag;

    enum {
        
    }subtag;


    
    union{
        // for primary typess
        Token type;

        // for structs
        Token structName;

        // for pointer types
        DataType *ptrTo;  
    };
    
    enum Specifiers{
        NONE = 0x0,
        // signedness
        UNSIGNED = (0x1 << 0), 
        SIGNED = (0x1 << 1),

        // long/short
        LONG = (0x1 << 2), 
        LONG_LONG = (0x1 << 3), 
        SHORT = (0x1 << 4),

        // type qualifiers
        VOLATILE = (0x1 << 5), 
        CONST = (0x1 << 6),

        // storage class specifiers
        EXTERN = (0x1 << 7), 
        STATIC = (0x1 << 8), 
        INLINE = (0x1 << 9), 
        REGISTER = (0x1 << 10), 
    };
    int flags = 0;
    
    int indirectionLevel(){
        if (tag == TAG_ADDRESS){
            return 1;
        }

        int level = 0;
        DataType *current = this;
        while (current->tag == TAG_PTR){
            current = current->ptrTo;
            level++;
        }
        return level;
    }

    DataType getBaseType(){
        if (tag == TAG_ADDRESS){
            return *ptrTo;
        }

        DataType *current = this;
        while (current->tag == TAG_PTR){
            current = current->ptrTo;
        }
        return *current;
    }

    bool isSet(Specifiers flag){
        return flags & flag;
    }

};

namespace DataTypes{
    inline DataType Char  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_CHAR, {"char", sizeof("char") - 1}, 0, 0}};
    inline DataType Int  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}};
    inline DataType Short  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}, .flags = DataType::Specifiers::SHORT};
    inline DataType Long  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}, .flags = DataType::Specifiers::LONG};
    inline DataType Long_Long  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_INT, {"int", sizeof("int") - 1}, 0, 0}, .flags = DataType::Specifiers::LONG_LONG};
    inline DataType Float  = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_FLOAT, {"float", sizeof("float") - 1}, 0, 0}};
    inline DataType Double = {.tag = DataType::TAG_PRIMARY, .type = {TOKEN_DOUBLE, {"double", sizeof("double") - 1}, 0, 0}};
    inline DataType String = {.tag = DataType::TAG_PTR, .ptrTo = &Char};
    inline DataType Void = {.tag = DataType::TAG_VOID, .type = {TOKEN_VOID, {"void", sizeof("void") - 1}, 0, 0}};
    inline DataType Error = {.tag = DataType::TAG_ERROR, .type = {TOKEN_ERROR, {"error", sizeof("error") - 1}, 0, 0}};
    inline DataType Struct = {.tag = DataType::TAG_STRUCT};
};

static void _recursePrintf(DataType d, char *scratchpad, int *sp){
    auto append = [&](const char *str){
        strcpy_s(&scratchpad[*sp], 1024 - *sp, str);
        (*sp) += strlen(str);
    };
    
    
    switch (d.tag){
        case DataType::TAG_ADDRESS :{
            scratchpad[(*sp)++] = '&';
            _recursePrintf(*(d.ptrTo), scratchpad, sp);
            break;
        }
        case DataType::TAG_PTR :{
            _recursePrintf(*(d.ptrTo), scratchpad, sp);
            append("*");

            if (d.isSet(DataType::Specifiers::CONST)){
                append("const ");
            }
            if (d.isSet(DataType::Specifiers::VOLATILE)){
                append("volatile ");
            }
            
            break;
        }
        case DataType::TAG_PRIMARY :{
            if (d.isSet(DataType::Specifiers::CONST)){
                append("const ");
            }
            if (d.isSet(DataType::Specifiers::VOLATILE)){
                append("volatile ");
            }
            
            if (d.flags & DataType::Specifiers::UNSIGNED){
                append("unsigned ");
            }
            if (d.flags & DataType::Specifiers::LONG){
                append("long ");
            }
            else if (d.flags & DataType::Specifiers::LONG_LONG){
                append("long long ");
            }
            else if (d.flags & DataType::Specifiers::SHORT){
                append("short ");
            }
            
            for (int i = 0; i<d.type.string.len; i++){
                scratchpad[(*sp)++] = d.type.string.data[i];
            }
            scratchpad[(*sp)++] = ' ';

            break;
        }
        default:
            for (int i = 0; i<d.type.string.len; i++){
                scratchpad[(*sp)++] = d.type.string.data[i];
            }
            break;
    }
}

static const char* dataTypePrintf(DataType d){
    static char scratchpad[1024];
    static int sp = 0;

    if (sp > 512){
        sp = 0;
    }

    int start = sp;

    _recursePrintf(d, scratchpad, &sp);
    
    scratchpad[sp++] = 0;

    return &scratchpad[start];
}


static bool operator==(DataType a, DataType b){
    if (a.indirectionLevel() != b.indirectionLevel()){
        return false;
    }


    if (a.flags != b.flags){
        return false;
    }

    while (a.indirectionLevel() > 0){
        a = *(a.ptrTo);
    }
    while (b.indirectionLevel() > 0){
        b = *(b.ptrTo);
    }
    

    if (a.tag != b.tag){
        return false;
    }
    if (!compare(a.type.string, b.type.string)){
        return false;
    }
    return true;
}
