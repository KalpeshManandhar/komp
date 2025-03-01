int main(){
    int a = 0;
    int b = a++; // b=0, a=1
    int c = ++a + a; // c=4, a=2
    int d = ++a + a++ + ++a; // d=3+3+5=11, a=5

    return (a==5) && (b==0) && (c==4) && (d==11);
}