#include <stdio.h>


int foo( int a, int b){
    int prod_a = a * 2;
    int prod_a_1 = a * 3;

    int div_b = b/4;
    int prod_b_1 = 15 * b;

    printf("%d", prod_a);
    printf("%d", prod_a_1);
    printf("%d", div_b);
    printf("%d", prod_b_1);
    return 0;
}