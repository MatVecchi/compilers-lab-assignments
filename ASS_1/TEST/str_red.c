#include <stdio.h>

int foo(int a, int b)
{
    int prod_a = a * 2;
    int prod_a_1 = a * 3;
    int prod_a_gen = a * 31;
    int prod_zero = 0 * a;

    int div_ten = a / 10;

    int div_b = b / 4;
    int prod_b_1 = 15 * b;
    int div_one = b / 1;

    printf("%d", prod_a);
    printf("%d", prod_a_1);
    printf("%d", div_b);
    printf("%d", div_one);
    printf("%d", prod_a_gen);
    printf("%d", prod_zero);
    printf("%d", div_ten);

    return 0;
}