int main(){

    return 
      (((1 + 2 * 3 * 5/5 + (-5) * (-1)) == 12)  // basic arithmetic
    && ((2 | 3 | 4) == 7)   // bitwise or
    
    && (12 != 4)    // not equals 
    && (12 >= 4)    // ge 
    && (4 > 0)    // gt 
    && (0 < 4)    // lt 
    && (4 <= 4)    // le 
    
    && ((1 & 2 & 4) == 0)   // bitwise and
    && ((1 ^ 6 ^ 4) == 3)   // bitwise xor
    && (~0 == -1)   // bitwise not and unary -
    && 1  // 1 is true
    && !0 // !0 is true
    || 0); 
}