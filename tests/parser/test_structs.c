struct A; // only declared

struct A { // struct definition
    int a;
    int b;
    float c;
};

struct B { // struct member of struct
    int b;
    struct A a; 
};

struct C {
    struct A *a;

    struct D{ // struct declaration inside
        int d;
    };
    struct D d; 

    struct E{  // struct definition and inline declaration
        int e;
    }e; 
};

struct A afoo, *bfoo;
afoo.a = 1;
afoo.b = 2;

bfoo = &afoo;
bfoo->a = 1;
(*bfoo).a = 1;

void foo(){
    struct P{
        int p;
    }p;
    
    p.p = 1;
}


// ERRORS
// bfoo = 1; // error: A = int
// afoo.c = "abc"; // error: float = char*
// afoo.d = 1; // no .d member
// afoo.1 = 2; // invalid member
// afoo1.a = 3; // only check left operand for declaration


// struct { // unnamed struct not supported
//     int a;
// };

// struct A{ // struct redefinition
//     int b;
// };

// struct F;

// struct G{
//     struct F f; // struct F not defined till this point
//     struct F *f; // struct F not defined till this point
// };




