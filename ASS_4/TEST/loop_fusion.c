#include <stdio.h>

void fun(int d, int k){
    int a[10];
    int b[10];
    int n = 16;
    int x= 5;
    int f = 0;

    for(int i=0; i<n; i++){
        a[i] = 16;
    }
    for(int j=0; j<n; j++){
       a[j] = 16;
    }

    if(x < 16){
        int i = 1;
        do{
            a[i] = 0;
            i ++;
        }while(i < 6);
    }
    if( x < 32){
        int i = 0;
        do{
            a[i] = 44;
            f = a[i] + 1;
            i ++;
        }while(i < 5);
    }
}