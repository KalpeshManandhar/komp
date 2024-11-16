int a;
int *b = 0;
int **c = &b;
int d[1];
int e[1][2];
int* f[1][2], g[3];

a = b[0];
a = c[0][0];
b = c[a];
a = c[b[a]];

a = f[1][2];
b = &g[0];
f[1][2] = 0;


