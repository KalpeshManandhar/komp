struct A {
    int a;
    int b;
    int *c;
    struct B{
        int d;
        float e;
    }bB;
};

int main(){
    int i = 0;
    struct A a = {1,2, &i, {12, 13.52f}};
    int b[5] = {1,2,3,4,5};
    return 0;
}

 