int a = 0;

// undeclared identifier: d
int b = a + d;

{
    // a and b are from parent scope
    int c = a;
    int e = b;
}

// c has gone out of scope 
int f = c;