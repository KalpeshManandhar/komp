int foo(int a, int b, int c, ...){
    return a;
}

void fooWrong(int a, int b, ..., int c){
    return;
}

int call(){
    foo(1,2,3,4,5,6,7);
    return 0;
}