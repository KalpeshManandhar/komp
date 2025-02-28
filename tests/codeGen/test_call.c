void modify(int *p, int n){
    *p = n;
}

int returnsN(int n){
    return n;
}

int add(int a, int b){
    return a + b;
}

int returns1(){
    return 1;
}

float returnsFloat(float f, int i){
    return f + i;
}


struct A{
    int a;
    int b;
    long d;
};

struct B{
    int a;
    int b;
    int c;
    long d;
};

// fits in 2 XLEN registers
int takesSmallStruct(struct A a){
    return a.a + a.d; // check if argument is saved correctly
}

// all these should be in registers
int takesManyArgs(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7){
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
}

// m0 and m1 should be in stack
int takesManyArgsThatWontFitInRegisters(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int m0, long m1){
    return m0 + m1;
}

// sizeof(B) > 2*XLEN
int takesLargeStruct(struct B b){
    return b.c;
}


// struct is half in register, half in stack
int takesPartOfStructInRegister(int a0, int a1, int a2, int a3, int a4, int a5, int a6, struct A a){
    return a.d + a.a;
}


int main(){
    int a = 2;
    int b = 1;
    int *c;
    c = &b;
    modify(c, 12);
    
    struct A as = {1,2,4};
    struct B bs = {11,22,44,88};
    

    
    // 2 + 1 + 2 + 12 + 2 + 13 + 12 + 4 + 5 + 36 + 19 + 44 + 5 = 157
    return 
        a  
        + returns1() 
        + returnsN(2) 
        + add(12 + returnsN(2), 13) 
        + b 
        + returnsFloat(1.25, 3) 
        + takesSmallStruct(as) // check if argument is passed correctly
        + takesManyArgs(1,2,3,4,5,6,7,8)
        + takesManyArgsThatWontFitInRegisters(1,2,3,4,5,6,7,8,9,10)
        + takesLargeStruct(bs)
        + takesPartOfStructInRegister(1,2,3,4,5,6,7,as)
        ; 
}