int main(){
    int a = 0; // i32 to i32: no cast needed
    char b = 1; // i32 to i8: no asm generated
    unsigned int c = 12; // i32 to u32: no asm generated

    unsigned long long d = b; // i8 to u64: no asm generated
    
    float e = 12; // i32 to f32: asm generated
    float f = e; // f32 to f32: no asm generated

    double g = 1.05f; // f32 to f64
    float h = (float)12/15 * 10; // explicit cast: i32 to f32

    // 1 + 12 + 8 = 21 
    return d + c + (int)h; // i32(u64 + u64(u32))
}