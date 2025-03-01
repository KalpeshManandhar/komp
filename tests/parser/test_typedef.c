typedef int i32;
typedef struct A{
    int a;
}AS;


int foo(){
    i32 a;
    AS *asp, as;
    a = 12;
    asp = &as;
    asp->a = a;
    return a;
}