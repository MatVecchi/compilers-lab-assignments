#include <stdio.h>

void compute(int a, int b, int c, int n, int *out) {
    for (int i = 0; i < n; i++) {
        // Invariant 1: simple arithmetic with arguments
        int x = a + b;
        
        // Invariant 2: depends on another invariant and an argument
        int y = x * c;
        
        // Invariant 3: constant expression
        int z = 100 / 4;
        
        // Not invariant: depends on loop induction variable 'i'
        int w = y + i;
        
        out[i] = w + z;
    }
}

int main() {
    int out[10];
    compute(2, 3, 4, 10, out);
    
    for (int i = 0; i < 10; i++) {
        printf("%d ", out[i]);
    }
    printf("\n");
    
    return 0;
}
