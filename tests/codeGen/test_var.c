int main(){
    int a = 0, b;
    a = 12;
    b = 12/3;
    
    int *ap;
    ap = &a;

    *ap = 20;
    

    return *ap + b;
}