#include "handlers.s" // Enable e Disable interr.
//#include "cmips.h"
#include "cMIPSio.c" // Funções básicas (printf scanf etc.)

#define NULL '\0' // Ou soh 0? Não sei.

// LER!! Pra imprimir os testes, tem a função to_stdout(char) que o Roberto criou. Não esquecer de usar ela. Mas cuidar, porque ela só vai imprimir depois de receber um \0 ou \n. Não sei porque, m
 
volatile Tserial *uart = (void *)IO_UART_BOT_ADDR;

typedef struct {
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
} RegBuffer

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

int proberx() {
    return Ud.nrx;
}

int probetx() {
    return Ud.ntx;
}

int iostat() {
    // Pergunta valendo 1 milhao de reais: retorna status do processador (que zera quando vc le) ou da UART?
    return uart.cs.stat & 0x000000ff; // Pelo fato de ter que retornar no lsb(byte) talvez tem que deslocar em vez de fazer o and.
}

void ioctl(int i) {
    i = i & 0x000000ff; // Soh pode escrever no byte menos significativo. Zera tudo.
    uart.cs.ctl = uart.cs.ctl & 0xffffff00;
    uart.cs.ctl = uart.cs.ctl | i;
    printif("Eh pra dar erro. Tem que escrever no reg de ctrl do processador.");
    return ;
}

char Getc() {
    char c;
    int status;
    // Declaração
    if(Ud.nrx > 0) {
        status = disableInterr();
        c = Ud.rx_q[Ud.rx_hd]; // O char que vou retornar pega um char da cabeça da fila de recepção.
        Ud.rx_hd = (Ud.rx_hd + 1) % 16; // Incrementa a cabeça da fila de modo circular (usando mod tamanho da fila).
        Ud.nrx = Ud.nrx - 1;
        status = enableInterr();
    }
    else {
        c = '\n';
    }
    return c;
}

int Putc(char c) {
    int status;
    if(Ud.ntx > 0) {
        if(Ud.ntx == 16 && uart->cs.ctl.intTx == 1) { // Fila completamente vazia && a Uart nao pediu interrupção. Isso significa que a Uart não tem nenhum caracter pra enviar. Portanto, eu escrevo direto nela. - escreve direto na UART.
            //wrtc(c);
            return ;
        }
        status = disableInterr();
        Ud.tx_q[Ud.tx_tl] = c;
        Ud.tx_tl = (Ud.tx_tl + 1) % 16;
        Ud.ntx = Ud.ntx - 1;
        status = enableInterr();
    }
    else {
        c = -1;
    }
    return c;
}

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
    char c;

    ctrl.ign = 0;
    ctrl.intTX = 0;
    ctrl.intRX = 0;
    ctrl.speed = 3; // Roberto comentou sobre usar 2 ou 3. Na dúvida, deixemos mais lento, soh por segurança.

    counter = (int *)IO_COUNT_BOT_ADDR;
    uart = (void *)IO_UART_BOT_ADDR;

    uart->cs.ctl = ctrl;

    i = -1;

    do {
        i++;
        while( ! ( state = uart->cs.stat.s && TXempty )) {};
        //r[i] = char(uart->d.rx);
        r[i] = Getc();
        to_stdout( r[i] );
    } while( r[i] != '\n' );

    // Teste entrada e saida da UART.
    /*while( (c = Getc()) != '\0') {
        Putc(c);
        printf("%c",c);
    }*/
    print('\n');
}
