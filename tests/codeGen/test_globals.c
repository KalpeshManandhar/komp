int a = 0;

int foo(){
    a++;
    return a;
}

int main(){
    int a = foo();
    int b = foo();
    return a + b + foo();
}