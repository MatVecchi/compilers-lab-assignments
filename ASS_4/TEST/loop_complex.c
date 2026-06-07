#include <stdio.h>

void fun(int a, int b){
    int c[10];
    int d[10];

    int x = 16;
    int y = 44;

    // cond 
    for(int i = 1; i<5; i ++){

        //cond 1
        for(int j = 0; j<5; j ++){
            printf("%d", j);
        }

        //cond 5
        for(int j = 0; j<5; j ++){
            printf("%d", j);
        }


        // cond 13
        for(int j = 5; j<10; j ++){
            printf("%d", j);
        }
    }

    // cond 23
    for(int z = 0; z<4; z ++){
        printf("%d", z);
    }

    // cond 31
    for(int i = 0; i<4; i ++){

        //cond 34
        for(int k = 0; k<4; k ++){
            if(x<y){
                // cond 38
                for(int m = 0; m<4; m ++){
                    printf("%d", m);
                }
            }
            if(x<y){
                // cond 48
                for(int m = 0; m<4; m ++){
                    printf("%d", m);
                }
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