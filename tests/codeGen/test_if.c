int main(){
    int a, b;
    a = 0;
    
    // all blocks: if, if else and else
    if (a< -1){
        b = -1;
        return b;
    }
    else if(a <2){
        b = 2;
    }
    else{
        b = 3;
    }
    
    // only if
    if (a<6){

        b = 6;
    }

    // if and if else, no else
    if (a < 12){
        b = 12;
    }
    else if (a < 15){
        b = 15;
    }

    return b;
}