#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void sort(int v[],int a,int b) {
    if(a >= b)
        return ;
    sort(v,a,b-1);
    int chave,i;
    chave = v[b];
    i = b-1;
    while(i>=0 && v[i]>chave)
    {
        v[i+1] = v[i];
        i--;
    }
    v[i+1] = chave;
}

int main() {
    srand(time(NULL));
    int v[10],i,a = 0,b = 9;
    for(i=0;i<10;i++) {
        v[i] = (rand()%100);
    }
    for(i=0;i<10;i++) {
        printf("%d ",v[i]);
    }
    printf("\n");

    sort(v,a,b);

    for(i=0;i<10;i++) {
        printf("%d ",v[i]);
    }
    printf("\n");
}
