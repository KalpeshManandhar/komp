struct A{
    char ac1;
    int ai1;
    long int ai2;

    struct B{
        int bi1;
    }b1;
};


int main(){
    struct A a;

    a.ai1 = 1;
    a.b1.bi1 = 2 + a.ai1;

    return a.ai1 + a.b1.bi1;
}