int a; // simple declaration
float b = 12; // with initializer
char c = 12 + 32; // initializer with operation
int d, e; // multiple var declaration 
int f = 0x23, g = 0b1011; // w/initializer

// type specifiers
unsigned h;
unsigned long int i;
unsigned short int j;
unsigned int k;
unsigned long long int l;
signed m;
signed int n;
signed short o;
signed long p;
signed long long q;
long int r;

// error
{
    signed unsigned int a; // signed unsigned conflict
    long short int b;  // long short conflict
    unsigned long short c; 
    long long long d; // extra long
    short short e; // extra short

    // type modifiers with real types
    long double f;  
    short double f;
    short float f;
    unsigned float f;
    signed float f;
    signed double f;
    unsigned double f;

    // type modifiers with void
    unsigned void *g;

    // will not print errors for these
    a = 1;
    b = 2; 

}