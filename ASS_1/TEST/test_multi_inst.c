#include <stdio.h>

int foo(int a, int b, int c)
{

    int x2 = b + a;
    int x3 = x2 - a;

    int z1 = 5 * c;
    int z2 = z1 / 5;

    printf("%d %d\n", x2, x3);
    printf("%d %d\n", z1, z2);

    return 0;
}