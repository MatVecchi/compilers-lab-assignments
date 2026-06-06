void fun(int d, int k){
    int a[10];
    int b[10];

    d=5;
    int l = d+100;
    k = 10;
    int i = 0;
    int j=0;
    
    /*
    if(d < k){
       while(i<d){
        i++;
       }
    }
    if(d < k ){
        while(j<d){
        j++;
       }
    } */
    

    
    d = 5;
    int kplus = k+5;

    /*if(d < k){
        for(int i=0; i<d; i++){
            b[i] = 44;
        }
    }*/

    if(d < kplus){
        for(int i=1; i<d+1; i++){
            b[i-1] = 16 + b[i-1];

            int a[20];
            for(int n=0; n<10; n++){
                a[i]=44;
            }
            for(int n=0; n<10; n++){
                a[i]=55;
            }
            for(int n=0; n<10; n++){
                a[i]=1;
            }
            for(int n=0; n<10; n++){
                a[i]=16;
            }
            
        }
    }
    
    
    int x = b[3]; 
}