struct A{
    int a;
    int b;
    int c;
};

struct A foo(){
    struct A a;
    a.a = 1;
    a.b = 1;
    a.c = 1;
    return *(&a);
}


union B{
    int a;
    int b; 
    long c;
};

union B foo2(){
    union B b;
    b.c = 0x0000000f0000000f;
    return b;
}

int main(){
    struct A a = foo();
    union B b = foo2();
    return a.a + a.b + a.c + b.a + b.b;
}

