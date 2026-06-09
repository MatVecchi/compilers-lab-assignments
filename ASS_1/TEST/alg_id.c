#include <stdio.h>

int foo( int a, int b){
    //int a = 10;
    //int b = 6;

    int somma_a_zero = a + 0;
    int somma_b_zero = 0 + b;
    
    int mul_a_uno = a * 1;
    int mul_b_uno = 1 * b;

    int sub_zero = a - 0;
    int div_one = b/1;

    int somma = somma_a_zero + somma_b_zero;
    int mul = mul_a_uno * mul_b_uno;

    printf("Somma: %d", somma);
    printf("Mul: %d", mul);
    printf("Sub: %d", sub_zero);
    printf("Div: %d", div_one);

    return 0;
}