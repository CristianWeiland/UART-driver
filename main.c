//#include "handlers.s"
#include "cmips.h"
#include "cMIPSio.c"
//#include <stdio.h>

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

char newgetc() {
    char c;
    int status;
    // Declaração
    if(U.nrx > 0) {
        c = U.rxqueue[U.rxhead]; // O char que vou retornar pega um char da cabeça da fila de recepção.
        U.rxhead = (U.rxhead + 1) % 16; // Incrementa a cabeça da fila de modo circular (usando mod tamanho da fila).
        status = disableInterr();
        U.nrx = U.nrx - 1;
        status = enableInterr();
    }
    else {
        c = NULL;
    }
    return c;
}

void newputc(char c) {
    int x,status;
    if(U.ntx > 0) {
        if(U.ntx == 16 && uart.cs.ctl.intTx == 0) { // Fila completamente vazia && a Uart nao pediu interrupção. Isso significa que a Uart não tem nenhum caracter pra enviar. Portanto, eu escrevo direto nela. - escreve direto na UART.
            return wrtc(c);
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
    return x; // Eu retorno algo sendo void? GG!
}

int wrtc(char c) {
    uart.d.tx = (int)
    return uart.d.tx;
}

volatile Tserial *uart = (void *)IO_UART_BOT_ADDR;

int main() {
    volatile Tserial *uart;
    //volatile int *counter;
    char c;

    //counter = (int *)IO_COUNT_BOT_ADDR;
    uart = (void *)IO_UART_BOT_ADDR;

    // Teste entrada e saida da UART.
    while( (c = newgetc()) != '\0') {
        newputc(c);
        printf("%c",c);
    }
    printf("\n");
}
