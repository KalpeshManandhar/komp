int a = 0;
int *b = &a;
int c = 0;

c = a + a; // should work: int and int
c = a + b; // currently doesnt work: int * and int
c = a + *b; // should work : int and int
c = a + b[0]; // should work : int and int
c = *a + b; // should error : deferencing an int

c = &1; // error: address of a literal
c = a + "abc"; // error: int and const char *

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