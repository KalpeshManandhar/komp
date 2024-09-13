int foo(int a, int b){
    return 0;
}

int main(){
    int a = foo(1,2,3);
    a = foo(1,2);
    a = foo();
    foo(1,2);
    
    return 0;
}