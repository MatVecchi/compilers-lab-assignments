#include <stdio.h>

void fun(int d, int k){
    int a[10];
    int b[10];
    int n = 16;
    int x= 5;
    int f = 0;

    for(int i=0; i<n; i++){
        printf("%d", i);
    }
    for(int j=0; j<n; j++){
        printf("%d", j);
    }

    if(x < 16){
        for(int i=1; i<6; i++){
            printf("%d", i);
            a[i] = 0;
        }
        //printf("ciao");
    }
    if( x < 32){
        
        for(int j=0; j<5; j++){
            printf("%d", j);
            //a[j] = 16;
            //f = a[j+1];
            a[j] = 44;
            f = a[j]+1;
        }
    }



}