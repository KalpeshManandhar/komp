int main(){
    int a = 0; // i32 to i32: no cast needed
    char b = 1; // i32 to i8: no asm generated
    unsigned int c = 12; // i32 to u32: no asm generated

    unsigned long long d = b; // i8 to u64: no asm generated
    
    return d + c; // 1 + 12 = 13
}