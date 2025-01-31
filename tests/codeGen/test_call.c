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

int main(){
    int a = 2;
    int b = 1;
    int *c;
    c = &b;
    modify(c, 12);
    // 2 + 1 + 2 + 12 + 2 + 13 + 12 + 4 = 48
    return a + returns1() + returnsN(2) + add(12 + returnsN(2), 13) + b + returnsFloat(1.25, 3); 
}