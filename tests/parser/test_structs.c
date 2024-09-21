struct A; // only declared

struct A { // struct definition
    int a;
    int b;
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
