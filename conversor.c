#include <stdio.h>

char inttochar(int i) {
    return (char)i;
}

int chartoint(char c) {
    return (int)c;
}

int main() {
    // Testa chartoint
    char c;
    printf("Digite um char: ");
    scanf("%c",&c);
    int i = chartoint(c);
    printf("Inteiro correspondente: %d\n",i);

    // Testa inttochar
    printf("Digite um inteiro: ");
    scanf("%d",&i);
    c = inttochar(i);
    printf("Char correspondente: %c\n",c);
    return 1;
}
