#include <stdio.h>
#include <math.h>

// gello i am a comment
/*

hello i am another comment*/

typedef struct{
    int x,y;
}Vec2;

Vec2 moves(int n){
    int sqLen = ceil(sqrt(n)); // 2
    
    int i = n - (sqLen - 1)*(sqLen - 1); // 3
    int d = sqLen*sqLen - (sqLen - 1)*(sqLen - 1); // 4-1=3
    
    Vec2 pos;
    if (sqLen % 2 == 0){
        pos.x = (i <= (d+1)/2)?(sqLen-1):(sqLen - 1 - abs(i - ((d+1)/2)));
        pos.y = (i >= (d+1)/2)?(sqLen-1):(i-1);
    }
    else{
        pos.x = (i >= (d+1)/2)?(sqLen-1):(i-1);
        pos.y = (i <= (d+1)/2)?(sqLen-1):(sqLen - 1 - abs(i - ((d+1)/2)));
    }
    return pos;
}



int main(){
    const int N = 0b124;
    for (int i=0; i<N*N; i++){
        Vec2 v = moves(i+1);
        printf("(%d %d)\t", v.x, v.y);
    }
    return 0;
}