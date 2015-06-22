#include "cMIPS.h"  
#define NULL '\0'

char r[16]="0000000000000";

typedef struct UD {
    char    rx_q[16];   // Reception Queue
    int     rx_hd;      // Reception Queue Head Index
    int     rx_tl;      // Reception Queue Tail Index
    char    tx_q[16];   // Transmission Queue
    int     tx_hd;      // Transmission Queue Head Index
    int     tx_tl;      // Transmission Queue Tail Index
    int     nrx;        // Number of Characters in rx_q
    int     ntx;        // Number of Spaces in tx_q
} UARTdriver;

extern UARTdriver Ud;

typedef struct {
    int buff[8];
} RegBuffer;

extern RegBuffer _uart_buff;

typedef struct control {
    int ign : 24,
        ign567 : 3,
        intTX : 1,
        intRX : 1,
        speed : 3;
} Tcontrol;

typedef struct status {
    int s;
} Tstatus;

#define RXfull 0x00000020
#define TXempty 0x00000040

typedef union ctlStat {
    Tcontrol ctl;
    Tstatus  stat;
} TctlStat;

typedef union data {
    int tx;
    int rx;
} Tdata;

typedef struct serial {
    TctlStat cs;
    Tdata d;
} Tserial;

volatile Tserial *uart;

int proberx() {
    volatile UARTdriver *Ud_p = &(Ud);   
    return Ud_p->nrx;
}

int probetx() {
    return Ud.ntx;
}

int iostat() {
    return uart->cs.stat.s & 0x000000ff; 
}

void ioctl(int i) {
    i = i & 0x000000ff; // Escreve no byte menos significativo. Zera tudo.
    //uart->cs.ctl.AQUIII = uart->cs.ctl.AQUIII & 0xffffff00;
    //uart->cs.ctl.AQUIII = uart->cs.ctl.AQUIII | i;
    //printif("Eh pra dar erro. Tem que escrever no reg de ctrl do processador.");
    return ;
}

char Getc(void) {
    char c;
    int status;
    if(Ud.nrx > 0) {
        status = disableInterr();
        Ud.nrx = Ud.nrx - 1;
        c = Ud.rx_q[Ud.rx_hd]; // O char que vou retornar pega um char da cabeça da fila de recepção.
        Ud.rx_hd = (Ud.rx_hd + 1) % 16; // Incrementa a cabeça da fila de modo circular (usando mod tamanho da fila).
        status = enableInterr();
    }
    else {
        c = '\n';
    }
    return c;
}

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


int pot(x,i){
    int cont,pot=1;
    for(cont=0;cont<i;cont++)
        pot=pot*x;
    return pot;
}

int chartoint(char *s) {
    int i,j,k,soma=0,v[8];
    for(i=0;s[i]!='\n';i++) // Conta quantos chars tem.
        {};
    v[i] =48; // Transforma o \n em 0 pra nao atrapalhar calculos. Ele vai ser atribuido novamente no inttochar.
    for(j=0;j<8;j++) // Deixa todas as posicoes zeradas.
        v[j] = 48;
    k = i-1;
    for(j=7;j>7-i;j--,k--) {
        v[j] = s[k];
    }

    for(j=0;j<8;j++) {
        if(v[j] >= 48 && v[j] <= 57) // Eh um numero (0-9). Subtrai 48 pra ficar 0,1,..,9, e nao 48,49,..,57
            v[j] -= 48;
        else if(v[j] >= 97 && v[j] <= 102)   // Letra (a,b,..,f). Subtrai 87 pra ficar o valor certo
            v[j] -= 87;                      // (a = 97 -> 97-87 = 10. b = 98 -> 98-87=11.)
        else // Eh algo que nao eh hexadecimal, zera.
            v[j] = 0;
    }

    for(i=0;i<8;i++) {
        soma += v[i] * ( pot(16,7-i) );
    }
    print(soma);
    return soma;
}

void inttochar(int x, char s[]) {
    int a,i,j,n;
// f0000000 - 0f000000 - 00f00000
// 1234abcd
    for(i=0; i < 8; i++) {
        n = 15; // n = 00000000f
        for(j=7; j > i; j--)
            n = n << 4;
        a = x & n;
        for(j=7; j > i; j--)
            a = a >> 4;
        if(a >= 0 && a <= 9)
            a += 48;
        else if(a >= 10 && a <= 15)
            a += 87;
        else // Caso tenha algum problema e nao seja um valor em hexadecimal.
            a = 48;
        s[i] = a;
    }
    s[i+1] = 10;
    while(s[0]  == 30) { // Se o caracter for 0, nao quero ele.
        for(i=0;i<15;i++) // Ignora os 0's.
            s[i] = s[i+1];
    }
}

int Putc(char c) {
    int status = uart->cs.stat.s & TXempty;
    if(Ud.ntx > 0) {
        if( Ud.ntx == 16 && status > 0) { // Fila completamente vazia && a Uart nao pediu interrupção. Isso significa que a Uart não tem nenhum caracter pra enviar. Portanto, eu escrevo direto nela. - escreve direto na UART.
            uart->d.tx = c;
            return ;
        }
        status = disableInterr();
        //print(c);
        Ud.tx_q[Ud.tx_tl] = c;
        Ud.tx_tl = (Ud.tx_tl + 1) & 0xf;
        Ud.ntx = Ud.ntx - 1;
        status = enableInterr();
    }
    else {
        c = -1;
    }
    return c;
}

int main() {
    int i,j=0,k,v[16];
    volatile int state;
    volatile int *counter;
    Tcontrol ctrl;
    volatile Tstatus status;
    char c, s[]="umastringbacan\n";
    char cadeia[16][9];

    counter = (int *)IO_COUNT_ADDR; // Deu problema no endereco, por isso comentei. Verificar depois.
    uart = (void *)IO_UART_ADDR;    // Tambem deu problema. Tem que ver isso urgente.

    state = disableInterr();
    Ud.rx_hd = 1;
    Ud.rx_tl = 0;
    Ud.tx_hd = 0;
    Ud.tx_tl = 0;
    Ud.nrx = 0;
    Ud.ntx = 16;
    state = enableInterr();

    ctrl.ign = 0;
    ctrl.ign567 = 0;
    ctrl.intTX = 0;
    ctrl.intRX = 1;
    ctrl.speed = 2; // Roberto comentou sobre usar 2 ou 3. Na dúvida, deixemos mais lento, soh por segurança.

    uart->cs.ctl = ctrl;

    i = -1;
    do {    
        do {
            i++;
            while(proberx() == 0); // Usa proberx para que o gcc nao otimize o codigo.
            r[i] = Getc();
            print(r[i]);
        } while( r[i] != '\n');
        for(k=0; k <= i; k++) {
            cadeia[j][k]=r[k];
            //print(cadeia[j][k]);
        }
        i=0;
        while(proberx() == 0); // pra gcc nao otimizar
        r[i]=Getc();
        print(r[i]);
        j++;
    } while (r[i] != '\n');
    
    //for(i=0;i<j;i++)
    //    for(k=0;k<6;k++)
    //        print(cadeia[i][k]);
    
    for(i=0;i<7;i++)
        r[i] = 48;
    r[7] = 10;
    print(512);
    for(i=0; i<j; i++) {
        v[i] = chartoint(cadeia[i]);
    }
    
    sort(v,0,j-1); // Ordena ateh a posicao j-1 pra nao ordenar o ultimo \n, que gera o fim da entrada.

    for(i=0;i<j;i++) {
        //print(v[i]);
        //print(512);
        inttochar(v[i], r);
        //TRANSMITE R
    }

    //for(i=0;s[i]!='\n';i++) // Conta quantos chars tem.
    //    {};
/*
13356527
74565
11250607
624485
*/
  /*  
   ctrl.intTX = 1;
   ctrl.intRX = 0;
   uart->cs.ctl = ctrl;
   //uart->d.tx = 'a';
   //for(i=0;i<100;i++) {};
   i = -1;
   //r = "0123456789abcde";
  do {
      i++;
      while ( ! ( ( state = uart-> cs.stat.s ) & TXempty ) ) {
      //while(Ud.ntx == 0) {
       //  print(12);
      };
      Putc(s[i]);
      
      //print(s[i]);
       
   } while ( s[i] != '\n'); // end of string ?
   for(i=0; i<200; i++); */
}
