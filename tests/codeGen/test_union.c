union A{
    int a;
    float b;
    unsigned long c;
    int d[2];
};

struct B{
    union A a;
    int c;
};


int main(){
    union A a;
    a.d[1] = 255;
    a.d[0] = 12;

    struct B b;
    b.a = a;
    b.c = 1;

    return (a.a == 12) && (a.c == 0x000000ff0000000c) && b.a.a == 12 && b.c == 1;
}