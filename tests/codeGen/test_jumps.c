int main(){
    int acc = 0;
    int i = 0;
    for (i=0; i<100; i = i + 1){
        if (i == 5){
            continue;
        }
        if (i == 10){
            break;
        }
        acc = acc + i;
    }
    
    i = 0;
    while (i<100){
        if (i == 5){
            i = i + 1;
            continue;
        }
        if (i==10){
            break;
        }
        acc = acc + i;
        i = i+1;
    }
    
    return acc;
}