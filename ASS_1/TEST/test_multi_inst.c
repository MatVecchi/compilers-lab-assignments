#include <stdio.h>

int foo(int a, int b, int c) {
    // Sequenza 1: operazioni correlate sullo stesso valore
    int x2 = b + a;     // correlata a x1
    int x3 = x2 - a;        // ritorna a * 3

    // Sequenza 3: catena ridondante
    int z1 = c * 5;
    int z2 = z1 / 5;        // 5c + c = 6c

    printf("%d %d\n", x2, x3);
    printf("%d %d\n", z1, z2);

    return 0;
}