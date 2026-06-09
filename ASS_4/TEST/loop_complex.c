#include <stdio.h>

void fun(int a, int b){
    int c[10];

    int x = 16;
    int y = 44;

    // cond 
    for(int i = 1; i<5; i ++){

        //cond 1
        for(int j = 0; j<5; j ++){
            c[j] = 16;
        }

        //cond 5
        for(int j = 0; j<5; j ++){
            c[j] = 16;
        }

        // cond 13
        for(int j = 5; j<10; j ++){
            c[j-5] = 16;
        }
    }

    // cond 23
    for(int z = 0; z<4; z ++){
        c[z] = 16;
    }

    // cond 31
    for(int i = 0; i<4; i ++){

        //cond 34
        for(int k = 0; k<4; k ++){
            if(x<y){
                int n =5;
                do{
                    n++;
                }while(n<10);
            }
            if(x<y){
                int n=5;
                do{
                    n++;
                }while(n<10);
            }
        }
    }

    /*
    for(int i=0; i<4; i++){
        for(int j=0; j<5; j++){
            print("%d", j);
        }
            
        print("%d", z);

        for(int k=0; k<4; k++){
            if(x<y){
                for(int m=0; m<4; m++){
                    print("%d", m);
                    print("%d", m);
                }
            }
        }
    }
    
    
    */
}