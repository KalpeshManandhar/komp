#pragma once

#include <tokenizer/token.h>
#include <assert.h>
#include <stdlib.h>
#include <utils/utils.h>



struct DataType{
    // Pointer types have the ptrTo member set to their corresponding data types
    // example: an "int*" datatype would have its ptrTo pointing to an "int" datatype
    enum {
        TAG_PRIMARY,
        TAG_STRUCT,
        TAG_UNION,
        TAG_PTR,
        TAG_VOID,
        TAG_ERROR,
        TAG_ADDRESS,
        TAG_ARRAY,
        TAG_COMPOSITE_UNSPECIFIED,
    }tag;

    struct CompositeType{
        std::vector<DataType> types;
    };

    union{
        // for primary types
        Token type;

        // for structs
        Token compositeName;


        struct{
            // for pointer types
            DataType *ptrTo;  
            // element count for arrays
            size_t arrayCount;
        };
        
        
        CompositeType* composite;



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
        int level = 0;
        DataType *current = this;
        while (current->tag == TAG_ADDRESS || current->tag == TAG_PTR || current->tag == TAG_ARRAY){
            current = current->ptrTo;
            level++;
        }
        return level;
    }

    DataType getBaseType(){
        if (tag == TAG_ADDRESS){
            return *ptrTo;
        }
        
        if (tag == TAG_ARRAY){
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
    inline DataType String = {.tag = DataType::TAG_ARRAY, .ptrTo = &Char};
    inline DataType Void = {.tag = DataType::TAG_VOID, .type = {TOKEN_VOID, {"void", sizeof("void") - 1}, 0, 0}};
    inline DataType Error = {.tag = DataType::TAG_ERROR, .type = {TOKEN_ERROR, {"error", sizeof("error") - 1}, 0, 0}};
    inline DataType Struct = {.tag = DataType::TAG_STRUCT};
    inline DataType Union = {.tag = DataType::TAG_UNION};
    inline DataType MemBlock = {.tag = DataType::TAG_COMPOSITE_UNSPECIFIED};
};

static void _recursePrintf(DataType d, char *scratchpad, int *sp){
    auto append = [&](const char *str){
        strncpy(&scratchpad[*sp], str, min(1024 - *sp, strlen(str)));
        (*sp) += strlen(str);
    };
    
    
    switch (d.tag){
        case DataType::TAG_ADDRESS :{
            scratchpad[(*sp)++] = '&';
            _recursePrintf(*(d.ptrTo), scratchpad, sp);
            break;
        }
        case DataType::TAG_ARRAY :{
            append("[");
            int n = sprintf(&scratchpad[*sp], "%" PRIu64, d.arrayCount);
            (*sp) += n;
            append("]");
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

            break;
        }
        case DataType::TAG_COMPOSITE_UNSPECIFIED:{
            append("{");
            for (int i=0; i<d.composite->types.size(); i++){
                _recursePrintf(d.composite->types[i], scratchpad, sp);
                if (i == d.composite->types.size() - 1){
                    break;
                }
                append(",");
            }
            append("}");
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
    
    int mask = (DataType::Specifiers::SHORT | DataType::Specifiers::UNSIGNED 
              | DataType::Specifiers::LONG | DataType::Specifiers::LONG_LONG);
    a.flags &= mask;
    b.flags &= mask;


    if (a.flags != b.flags){
        return false;
    }
    if (a.tag != b.tag){
        return false;
    }

    if (a.indirectionLevel() > 0 && b.indirectionLevel() > 0){
        return *a.ptrTo == *b.ptrTo;
    }
    

    return compare(a.type.string, b.type.string);
}


static int getIntegerConversionRank(DataType d){
    if (d.isSet(DataType::Specifiers::LONG_LONG)){
        return 4;
    }
    if (d.isSet(DataType::Specifiers::LONG)){
        return 3;
    }
    if (!d.isSet(DataType::Specifiers::LONG) && !d.isSet(DataType::Specifiers::SHORT)){
        if (_match(d.type, TOKEN_INT)){
            return 2;
        }
        if (_match(d.type, TOKEN_CHAR)){
            return 0;
        }
        
    }
    if (d.isSet(DataType::Specifiers::SHORT)){
        return 1;
    }
    return -1;
};


static DataType getResultantType(DataType left, DataType right, Token op){
    
    // diff level of indirection
    if (left.indirectionLevel() != right.indirectionLevel()){
        // pointer arithmetic 
        // (ptr + int)/(ptr - int)/(ptr += int)/(ptr -= int)
        if (left.indirectionLevel() > 0 && (_match(right.type, TOKEN_INT) || _match(right.type, TOKEN_CHAR))){ 
            if (_match(op, TOKEN_PLUS) || _match(op, TOKEN_MINUS) 
            || _match(op, TOKEN_PLUS_ASSIGN) || _match(op, TOKEN_MINUS_ASSIGN) ){
                return left;
            }
            else if (_match(op, TOKEN_ASSIGNMENT)){
                return left;
            }
        }
        // (int + ptr) 
        else if (right.indirectionLevel() > 0 && (_match(left.type, TOKEN_INT) || _match(left.type, TOKEN_CHAR))){
            if (_match(op, TOKEN_PLUS)){
                return right;
            }
            if (_match(op, TOKEN_ASSIGNMENT)){
                return right;
            }
        }
        // both pointers of different level of indirection
        // can only assign but log a warning
        else if (left.indirectionLevel() > 0 && right.indirectionLevel() > 0){
            if (_match(op, TOKEN_ASSIGNMENT)){
                return left;
            }
        }

    }
    // same level of indirection
    else{
        // both are pointers
        if (left.indirectionLevel() > 0){
            // can assign, but log error if of different types
            if (_match(op, TOKEN_ASSIGNMENT)){
                return left;
            }

            if (left == right){
                // ptr difference: (ptr - ptr)
                if (_match(op, TOKEN_MINUS)){
                    return DataTypes::Long_Long;
                }
            }
        }
        // same type but not pointers
        else if (left == right){
            // assignment is defined for all operands of the same type  
            if (_match(op, TOKEN_ASSIGNMENT)){
                return left;
            }
            
            // all other primary operands work with all other operators 
            // (floating point exceptions have been handled at the start)
            if (left.tag == DataType::TAG_PRIMARY){
                return left;
            }

            // if struct, only assignment between same structs is allowed 
            if (_match(left.type, TOKEN_STRUCT)){
                if (_match(op, TOKEN_ASSIGNMENT) && compare(left.compositeName.string, right.compositeName.string)){
                    return left;
                }
            }
        }
        
        // different types of primary types
        else if (left.tag == DataType::TAG_PRIMARY && right.tag == DataType::TAG_PRIMARY){
            
            // for valid assignment operations, the resultant type is the type of the left operand
            if (_matchv(op, ASSIGNMENT_OP, ARRAY_COUNT(ASSIGNMENT_OP))){
                return left;
            }

            // if any is double or float, convert to that
            if (_match(left.type, TOKEN_DOUBLE) || _match(right.type, TOKEN_DOUBLE)){
                return DataTypes::Double;
            }   
            else if (_match(left.type, TOKEN_FLOAT) || _match(right.type, TOKEN_FLOAT)){
                return DataTypes::Float;
            }   

            // same signedness, conversion to greater conversion rank
            else if ((left.isSet(DataType::Specifiers::SIGNED) && right.isSet(DataType::Specifiers::SIGNED))
                    || (left.isSet(DataType::Specifiers::UNSIGNED) && right.isSet(DataType::Specifiers::UNSIGNED))){
                
                if (getIntegerConversionRank(left) > getIntegerConversionRank(right)){
                    return left;
                }
                
                return right;
                
            }   
            
            // different signedness
            else {
                // if unsigned has higher or equal rank, then unsigned
                // else, if signed can accomodate full range of unsigned then convert to signed, 
                // else convert to unsigned counterpart of the signed types
                
                auto signedUnsignedConversion = [&](DataType unsignedType, DataType signedType){
                    if (getIntegerConversionRank(unsignedType) >= getIntegerConversionRank(signedType)){
                        return unsignedType;
                    }
                    else if (signedType.isSet(DataType::Specifiers::LONG_LONG)){
                        return signedType;
                    }
                    else if (signedType.isSet(DataType::Specifiers::LONG)){
                        // signed long cannot accomodate unsigned int
                        if (!unsignedType.isSet(DataType::Specifiers::LONG)){
                            return signedType;
                        }
                    }
                    else if (!signedType.isSet(DataType::Specifiers::SHORT)){
                        // signed int can accomodate unsigned short and char
                        if (unsignedType.isSet(DataType::Specifiers::SHORT) || _match(unsignedType.type, TOKEN_CHAR)){
                            return signedType;
                        }
                    }
                    else if (signedType.isSet(DataType::Specifiers::SHORT)){
                        // signed short can accomodate unsigneds char
                        if (_match(unsignedType.type, TOKEN_CHAR)){
                            return signedType;
                        }
                    }
                    // unsigned counterpart of the signed types
                    signedType.flags |= DataType::Specifiers::UNSIGNED;
                    signedType.flags ^= DataType::Specifiers::SIGNED;
                    return signedType;
                };
                
                
                if (left.isSet(DataType::Specifiers::UNSIGNED)){
                    return signedUnsignedConversion(left, right);
                }
                else {
                    return signedUnsignedConversion(right, left);
                }
            }
            
        
        }

    }

    return DataTypes::Int;
};


static bool isCompositeType(DataType d){
    return d.tag == DataType::TAG_STRUCT || d.tag == DataType::TAG_UNION;
}



struct TypedefInfo{
    Splice identifier;
    DataType aliasFor;
};