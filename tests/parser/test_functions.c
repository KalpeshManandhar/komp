
void foo1();
void foo1(int a){} // conflicting num of params

int foo2(int a);
float foo2(int a){return 0;} // conflicting return type 

void foo3(int a);
void foo3(float a){} // conflicting param type

void foo4(float b){b = 2;}
void foo4(int  b){b = 2;} // function redefinition


int foo(int a, int b);

int foo(int a, int b){
    return 0;
}




int main(){
    int a = foo(1,2);
    a = foo(foo(1,2),2);
    foo(1,2, 3); // mismatch in number of arguments

    fooUndeclared(); // implicit declaration of function

    return 0;
}