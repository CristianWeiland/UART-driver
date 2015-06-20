//#include "handlers.s" // Enable e Disable interr.
#include "cMIPS.h"  // Comentado porque jah tah sendo incluido pelo cMIPSio.c
//#include "cMIPSio.c" // Funções básicas (printf scanf etc.)

#define NULL '\0' // Ou soh 0? Não sei.

char r[16]="---------------";

// LER!! Pra imprimir os testes, tem a função to_stdout(char) que o Roberto criou. Não esquecer de usar ela. Mas cuidar, porque ela só vai imprimir depois de receber um \0 ou \n. Não sei porque, m

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
    return Ud.nrx;
}

int probetx() {
    return Ud.ntx;
}

int iostat() {
    // Pergunta valendo 1 milhao de reais: retorna status do processador (que zera quando vc le) ou da UART?
    return uart->cs.stat.s & 0x000000ff; // Pelo fato de ter que retornar no lsb(byte) talvez tem que deslocar em vez de fazer o and.
}

void ioctl(int i) {
    i = i & 0x000000ff; // Soh pode escrever no byte menos significativo. Zera tudo.
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
/*
0110 0010 - 0110 0001
abcdfe/n
00abcdfe
0*16^8 + 0*16^7 + 10(a)*16^6...
f0
0a
fa = 1111 1010
abcdef
*/
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
    //print(c);
    return c;
}
/*
int Putc(char c) {
   disableInterr();
   uart->d.tx = c;
   enableInterr();
   return 1;
} */

/*
int Putc(char c){
        if(Ud.ntx > 0){
                disableInterr();
                --Ud.ntx;
                //print(Ud.tx_tl);
                //print(Ud.tx_hd);
                Ud.tx_q[Ud.tx_tl] = c;
                Ud.tx_tl = (Ud.tx_tl + 1) & 15;
                if((iostat() & TXempty) == TXempty) {
                        uart->d.tx = c;
                        Ud.tx_hd = (Ud.tx_hd + 1) & 15;
                        ++Ud.ntx;
                }
                enableInterr();
                return 1;            
        }

        return 0;
}
*/

/*
int wrtc(char c) {
    return uart->d.tx = (int)c;
}
*/

int main() {
    int i;
    volatile int state;
    volatile int *counter;
    Tcontrol ctrl;
    volatile Tstatus status;
    char c, s[]="umastringbacan\n";

    counter = (int *)IO_COUNT_ADDR; // Deu problema no endereco, por isso comentei. Verificar depois.
    uart = (void *)IO_UART_ADDR;    // Tambem deu problema. Tem que ver isso urgente.

    state = disableInterr();
    Ud.rx_hd = 0;
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

    //uart->cs.ctl = ctrl;
/*
    i = -1;
    do {
        i++;
        //while( ! ( (state = uart->cs.stat.s) & RXfull )) {};
        while(Ud.nrx == 0) {};
        r[i] = Getc();
        print(r[i]);
        //to_stdout(r[i]);
    } while( r[i] != '\0' );
*/
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
for(i=0; i<200; i++);
}
