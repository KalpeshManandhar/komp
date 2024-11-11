struct A{
    char ac1;
    int ai1;
    int ai2;
    long int ali1;

    struct B{
        int bi1;
        int bi2;
    }b1;
};


int main(){
    struct A a, *ap;
    struct B *bp;



    ap = &a;
    bp = &a.b1;

    

    a.ai1 = 1; // 1
    a.b1.bi1 = 2 + a.ai1; // 3
    ap->ai2 = a.ai1 + 10; // 11
    ap->b1.bi2 = ap->ai2 + 1; // 12
    bp->bi1 = bp->bi1 + 100; // 103

    return a.ai1 + a.b1.bi1 + ap->b1.bi2; // 1 + 103 + 12 = 116
}