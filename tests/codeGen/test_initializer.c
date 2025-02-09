struct A {
    int a;
    long long b;
    int *c;
};


struct B {
    int a;
    int b;
    struct A c;
};

struct C {
    int a;
    int b[5];
};


int main(){
    int i = 0;
    struct A a = {1,2, &i}; // struct
    struct B b = {1,2, {3,4,&i}}; // nested struct

    int c[5] = {6,7,8,9,10}; // array
    int d[2][2] = {{1,2}, {3,4}}; // 2D array

    struct A e[2] = { // array of struct
        {1,2,&i},
        {3,4,&i}
    };

    struct C f = { // struct of array
        1, {1,2,3,4,5}
    };

    // 1 + 2 + 4 + 8 + 1 + 3 + 5 = 24
    return a.a + a.b + b.c.b + c[2] + d[0][0] + e[1].a + f.b[4]; 
}

 