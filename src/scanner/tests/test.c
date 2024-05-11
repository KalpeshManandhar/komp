int main(){
    int b = 1;
    int a = 12 + b;
    char cb = 080.1;
    char ab = 'a';
    const char * abs = &ab;
    abs = &cb;
    *abs = '5';
    return a;
}