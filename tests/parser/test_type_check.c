int a = 0;
int *b = &a;
int c = 0;
int *d = &a;

c = a + a; // should work: int and int
c = a + *b; // should work : int and int
c = a + b[0]; // should work : int and int
d = a + b; // should work: int* = int*
d = b - a ; // should work: int* = int*
d = a - b; // should error: int - int*

d = a * b; // should error: int * int*
c = a + b; // should error: int = int*
c = *a + b; // should error : deferencing an int

a = d - b; // works: int = int
d = d - b; // error: ptr = int
d = d+ b; // error: ptr + ptr

c = &1; // error: address of a literal
c = a + "abc"; // error: int and const char *
c = *(a + "abc"); // works: int = char (implicit typecast to int)

void d = 1; // void type not allowed
void *e = 1; // should work

int returnInt(){
    return 0;
}

void returnVoid(){
    return;
}

c = a + returnInt();
c = a + returnVoid();