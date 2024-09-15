int a;
int *b = 0;
int **c = &b;

a = b[0];
a = c[0][0];
b = c[a];

