#include "handlers.s" // Enable e Disable interr.
#include "cmips.h"
#include "cMIPSio.c" // Funções básicas (printf scanf etc.)

// LER!! Pra imprimir os testes, tem a função to_stdout(char) que o Roberto criou. Não esquecer de usar ela. Mas cuidar, porque ela só vai imprimir depois de receber um \0 ou \n. Não sei porque, mas é assim.

typedef struct {
    int nrx;
    int rxhead;
    int rxtail;
    char rxqueue[16];

    int ntx;
    int txhead;
    int txtail;
    char txqueue[16];
} Utype;

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

extern Utype U;

int proberx() {
    return U.nrx;
}

int probetx() {
    return U.ntx;
}

int iostat() {
    return uart.cs.stat & 0x000000ff; // Pelo fato de ter que retornar no lsb(byte) talvez tem que deslocar em vez de fazer o and.
}

void ioctl(int i) {
    i = i & 0x000000ff; // Soh pode escrever no byte menos significativo. Zera tudo.
    uart.cs.ctl = uart.cs.ctl & 0xffffff00;
    uart.cs.ctl = uart.cs.ctl | i;
    return ;
}

char getc() {
    char c;
    int status;
    // Declaração
    if(U.nrx > 0) {
        status = disableInterr();
        c = U.rxqueue[U.rxhead]; // O char que vou retornar pega um char da cabeça da fila de recepção.
        U.rxhead = (U.rxhead + 1) % 16; // Incrementa a cabeça da fila de modo circular (usando mod tamanho da fila).
        U.nrx = U.nrx - 1;
        status = enableInterr();
    }
    else {
        c = '\n';
    }
    return c;
}

void putc(char c) {
    int x,status;
    if(U.ntx > 0) {
        if(U.ntx == 16 && uart->cs.ctl.intTx == 0) { // Fila completamente vazia && a Uart nao pediu interrupção. Isso significa que a Uart não tem nenhum caracter pra enviar. Portanto, eu escrevo direto nela. - escreve direto na UART.
            wrtc(c);
            return ;
        }
        U.txqueue[U.txtail] = c;
        U.txtail = (U.txtail + 1) % 16;
        status = disableInterr();
        U.ntx = U.ntx - 1;
        status = enableInterr();
	    x = 1;
    }
    else {
        x = 0;
    }
    return ;
    //return x; // Eu retorno algo sendo void? GG! Maybe fazer o x ser global pra poder retornar o erro.
}


int wrtc(char c) {
    uart->d.tx = (int)c;
    return uart.d.tx;
}


volatile Tserial *uart = (void *)IO_UART_BOT_ADDR;

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
    ctrl.speed = 0;

    counter = (int *)IO_COUNT_BOT_ADDR;
    uart = (void *)IO_UART_BOT_ADDR;

    uart->cs.ctl = ctrl;

    i = -1;

    do {
        i++;
        while( ! ( state = uart->cs.stat.s && TXempty )) {};
        //r[i] = char(uart->d.rx);
        r[i] = getc();
        to_stdout( r[i] );
    } while( r[i] != '\n' );

    // Teste entrada e saida da UART.
    /*while( (c = getc()) != '\0') {
        putc(c);
        printf("%c",c);
    }*/
    print('\n');
}
