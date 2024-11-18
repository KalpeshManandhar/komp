int main(){
    int a, *b, c[3], d[2][2];
    
    a = 0;
    b = &a;
    
    b[0] = 1; // pointer indexing
    c[1] = 12; // array indexing
    d[0][0] = 13; // 2D indexing


    return b[0] + a + c[1] + d[0][0];
}