#include <io.h>
#include <system.h>

int main(){
    char buf[12];
    buf[0] = 'h';
    buf[1] = 'e';
    buf[2] = 'l';
    buf[3] = 'l';
    buf[4] = 'o';
    buf[5] = ' ';
    buf[6] = 't';
    buf[7] = 'h';
    buf[8] = 'e';
    buf[9] = 'r';
    buf[10] = 'e';
    buf[11] = '\n';

    const char* buf2 = "omae wa mou shindeiru\n";
    write(1, &buf[0], 12);
    write(1, buf2, 22);

    int n = read(0, buf, 12);
    write(1, buf, n);

    return 12;
}