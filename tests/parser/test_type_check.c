int a = 0;
int *b = &a;
int c = 0;
int *d = &a;

c = a + a; // work: int and int
c = a + *b; // work : int and int
c = a + b[0]; // work : int and int
d = a + b; // work: int* = int*
d = b - a ; // work: int* = int*
d = a - b; // error: int - int*

d = a * b; // error: int * int*
c = a + b; // warning: int = int*
c = *a + b; // error : deferencing an int

a = d - b; // works: int = int
d = d - b; // warning: ptr = int
d = d + b; // error: ptr + ptr

c = &1; // error: address of a literal
c = a + "abc"; // error: int and const char *
c = *(a + "abc"); // works: int = char (implicit typecast to int)



void d = 1; // void type not allowed
void *e = 1; // work

int returnInt(){
    return 0;
}

void returnVoid(){
    return;
}

c = a + returnInt();
c = a + returnVoid(); // error: void return type


void takesPtr(int *a){
    *a = 1;
}

takesPtr(1.0f);