struct Foo{
    int arr[5];
    int arr2[5][6];
};


int main(){
    int a, *b, c[3], d[2][2], *e[2], *f;
    
    
    struct Foo foo;


    a = 0;
    b = &a;
    
    b[0] = 1; // pointer indexing
    c[1] = 12; // array indexing
    d[0][0] = 13; // 2D indexing
    e[0] = &d[0][1];
    e[0][0] = 20;
    f = c;

    // inside struct arrays
    foo.arr[2] = 14;
    foo.arr2[3][5] = 15;
    
    // 1 + 1 + 12 + 13 + 20 + 14 + 15 + 12 = 88 
    return b[0] + a + c[1] + d[0][0] + d[0][1] + foo.arr[2] + foo.arr2[3][5] + f[1];
}