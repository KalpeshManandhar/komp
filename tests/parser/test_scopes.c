
// global scope
int a = 5;
float c = 12.0f;

// block scope
if (a > c){
    char d = 12;
    int f = 1;
    // nested block
    {
        int e = 12;
    }
}
