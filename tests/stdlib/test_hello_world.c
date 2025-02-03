#include <io.h>

int main(){
    char buf[12];
    buf[0] = 0x68;
    buf[1] = 0x65;
    buf[2] = 0x6c;
    buf[3] = 0x6c;
    buf[4] = 0x6f;
    buf[5] = 0x20;
    buf[6] = 0x74;
    buf[7] = 0x68;
    buf[8] = 0x65;
    buf[9] = 0x72;
    buf[10] = 0x65;
    buf[11] = 0x0a;

    write(1, &buf[0], 12);

    return 12;
}