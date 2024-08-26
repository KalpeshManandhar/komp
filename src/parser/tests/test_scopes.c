
// global scope
int a = 5;
float c = 12.0f;

// block scope
if (a > c){
    char d = 12;
    d = a + c;
    // nested block
    {
        int e = 12;
    }
}
