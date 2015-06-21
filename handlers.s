
	# interrupt handlers
	.include "cMIPS.s"
	.text
	.set noreorder
    .align 2

	.set M_StatusIEn,0x0000ff09     # STATUS.intEn=1, user mode

	#----------------------------------------------------------------
	# interrupt handler for external counter attached to IP2=HW0
	# Counter address -- see vhdl/packageMemory.vhd

	.bss
	.align  2
	.global _counter_val             # accumulate number of interrupts
	.comm   _counter_val 4
	.comm   _counter_buffer 8*4       # area to save up to 8 registers
	# _counter_buffer[0]=$a0, [1]=$a1, [2]=$a2, ...

	.set HW_counter_value,0xc00000c8 # Count 200 clock pulses & interr

	.text
	.set    noreorder
	.global extCounter
	.ent    extCounter

extCounter:
	lui   $k0, %hi(HW_counter_addr)
	ori   $k0, $k0, %lo(HW_counter_addr)
	sw    $zero, 0($k0) 	# Reset counter, remove interrupt request

	#----------------------------------
	# save additional registers
	# la $k1, _counter_buffer
	# sw $a0, 0*4($k1)
	# sw $a1, 1*4($k1)
	#----------------------------------

     # save additional registers
     lui   $k1, %hi(_counter_buffer)
     ori   $k1, $k1, %lo(_counter_buffer)
     sw    $ra, 4($k1)

	lui   $k1, %hi(HW_counter_value)
	ori   $k1, $k1, %lo(HW_counter_value)
	sw    $k1, 0($k0)	      # Reload counter so it starts again

	lui   $k0, %hi(_counter_val)  # Increment interrupt event counter
	ori   $k0, $k0, %lo(_counter_val)
	lw    $k1,0($k0)
	nop
	addiu $k1,$k1,1
	sw    $k1,0($k0)

	addi $k0,$zero,10
	mult $k0,$k1
	mflo $k0
	jal print
	nop

	#----------------------------------
	# and then restore same registers
	# la $k1, _counter_buffer
	# lw $a0, 0*4($k1)
	# lw $a1, 1*4($k1)
	#----------------------------------

     # restore same registers
     lui   $k1, %hi(_counter_buffer)
     ori   $k1, $k1, %lo(_counter_buffer)
     lw    $ra, 0*4($k1)

	mfc0  $k0, cop0_STATUS	    # Read STATUS register
	ori   $k0, $k0, M_StatusIEn #   but do not modify its contents
	addiu $k1, $zero, -7        #   except for re-enabling interrupts
	and   $k0, $k1, $k0	    #   -7 = 0xffff.fff9
	mtc0  $k0, cop0_STATUS
	eret			    # Return from interrupt
	.end extCounter
	#----------------------------------------------------------------
	# interrupt handler for UART attached to IP3=HW1

    .bss
    .align  2
    .set    noreorder
    .global Ud, Ubuff, UARTinterr

Ud:     .comm   rx_queue 16
        .comm   rx_hd 4
        .comm   rx_tl 4
        .comm   tx_queue 16
        .comm   tx_hd 4
        .comm   tx_tl 4
        .comm   nrx 4                  # characters in RX_queue
        .comm   ntx 4                  # spaces left in TX_queue
Ubuff:  .comm   _uart_buff 16*4        # save space to save registers

    .extern uart

    .set UART_rx_irq,0x08
    .set UART_tx_irq,0x10

    .text
    .set    noreorder
    .global UARTinterr
    .ent    UARTinterr

    # Ud[0-15]=rx_queue; Ud[16-19]=rx_hd; Ud[20-23]=rx_tl; Ud[24-39]=tx_queue;
    # Ud[40-43]=tx_head; Ud[44-47]=tx_tl; Ud[48-51]=nrx; Ud[52-55]=ntx;

    # _uart_buff[0]=UARTstatus, [1]=UARTcontrol, [2]=data_inp, [3]=new,
    #           [4]=$ra, [5]=$a0, [6]=$a1, [7]=$a2, [8]=$a3, [9]=$v0

UARTinterr:
    lui   $k0, %hi(HW_uart_addr)
    ori   $k0, $k0, %lo(HW_uart_addr)   # $k1 = endereço uart
    lw    $k1, 0($k0)                   # Read status, remove interrupt request - CONFERIR ENDERECO

    lui   $k0, %hi(_uart_buff)
    ori   $k0, $k0, %lo(_uart_buff)     # and save UART status to memory
    sw    $k1, 0($k0)

    sw    $ra, 16($k0)                  # save some registers
    sw    $a0, 20($k0)
    sw    $a1, 24($k0)
    sw    $v0, 36($k0)

	#----------------------------------
    # while you are developing the complete handler,
    #    uncomment the line below and comment out lines up to UARTret
    #.include "../tests/handlerUART.s"
    #----------------------------------

 RX_Interr:
	 andi  $a0, $k1, UART_rx_irq     # is this reception?
	 beq   $a0, $zero, TX_Interr     # if not, go to TX_interr
     nop

     lui   $k0, %hi(Ud)              # carrega endereco da uart
     ori   $k0, $k0, %lo(Ud)

     lw    $a0, 48($k0)              # carrega nrx (rxqueue + rxhead + rxtail + txqueue + txhead + tail + Utype) = (16+4+4+16+4+4+HW_uart_addr) - CONFERIR ENDERECO
     nop

     slti  $a1, $a0, 16              # confere se nrx < 16
     nop
     beq   $a1, $zero, overrun       # quando n tem espaço deu overrun, pula pra lah
     nop

     addiu $a0, $a0, 1               # incrementa nrx
     sw    $a0, 48($k0)              # salva nrx - CONFERIR ENDEREC
	 lw    $a0, 20($k0)              # carrega rxtail - CONFERIR ENDERECO
 	 nop

	 addiu $a0, $a0, 1               # incrementa rxtail # notas de aula tah -1, porque seria -1? GG - CONFERIR SE INCREMENTA OU DECREMENTA
	 andi  $a0, $a0, 0xf             # modu    lo 16
	 sw    $a0, 20($k0)              # salva rxtail - CONFERIR ENDERECO

     lui   $k1, %hi(HW_uart_addr)    # $k1 = endereço uart
     ori   $k1, $k1, %lo(HW_uart_addr)
     nop
     lw    $a1, 4($k1)               # ler data do uart rxreg - CONFERIR ENDERECO

     addu  $a0, $a0, $k0             # adicionar tail index to &(Ud)
     sb    $a1, 0($a0)               # coloca na posicao rxtail - CONFERIR ENDERECO

     # j UARTret                      # Comment - i have to check if theres transmission.
     # nop

     # Ud[0-15]=rx_queue; Ud[16-19]=rx_hd; Ud[20-23]=rx_tl; Ud[24-39]=tx_queue;
     # Ud[40-43]=tx_head; Ud[44-47]=tx_tl; Ud[48-51]=nrx; Ud[52-55]=ntx;
 TX_Interr:
     lui   $k0, %hi(_uart_buff)      # nao le do HW_uart_addr porque jah foi lido anteriormente e isso zera as interrupcoes.
     ori   $k0, $k0, %lo(_uart_buff) # depois da primeira leitura salvamos na _uart_buff e agora vamos ler de lah.
     nop                             # PRECISA?

     lw    $a0, 0($k0)               # ler registrador de status
     nop

     lui   $k0, %hi(HW_uart_addr)
     ori   $k0, $k0, %lo(HW_uart_addr)
     lw    $k1, 0($k0)

     or    $k1, $a0, $k1             # Or entre status lido na interpretacao de recepcoes e status lido agora.

     andi  $a0, $k1, UART_tx_irq     # is this transmission?
     nop                             # PRECISA?
     beq   $a0, $zero, UARTret       # if not, leave
     nop

     lui   $k0, %hi(Ud)
     ori   $k0, $k0, %lo(Ud)
     nop                             # PRECISA?

     lw    $a0, 52($k0)              # load ntx - CONFERIR ENDERECO *
     nop

     slti  $a1, $a0, 16              # checks if ntx < 16 (theres something in there)
     nop                             # PRECISA?
     beq   $a1, $zero, UARTret       # if it isnt ((ntx < 16) != 0), do nothing. Leave.
     nop

    #------------------------------------------------------------------------------#
    # Resolvendo esse maldito slti e beq/bne.
    # Se ntx = 16 -> slti faz $a1 receber 0. Depois, se $a1==0, saio (go to UARTret). Portanto, beq.
    # Se ntx < 16 -> slti faz $a1 receber 1. Depois, se $a1==1, continuo. Portanto, beq.
    #------------------------------------------------------------------------------#

     addi  $a0, $a0, 1               # incrementa ntx (retirou um elemento, tem mais um espaco.) - CONFERIR SE INCREMENTA OU DECREMENTA

     lw    $k1, 40($k0)              # pega txhead - CONFERIR ENDERECO *
     nop

     addu  $a0, $k1, $k0             # calcula posicao do elemento (txqueue[txhead])
     nop                             # PRECISA?
     lb    $a1, 24($a0)              # copy txqueue[txhead] to tx buffer. - CONFERIR ENDEREÇO * (Ud + txhead + 24)

     addi  $k1, $k1, 1               # update txhead
     and   $k1, $k1, 15              # mod 16
     nop                             # PRECISA?
     nop                             # PRECISA?
     sw    $k1, 40($k0)              # salva txhead - CONFERIR ENDERECO *

     lui   $k0, %hi(HW_uart_addr)
     ori   $k0, $k0, %lo(HW_uart_addr)
     nop                             # PRECISA?

     sw    $a1, 4($k0)               # salva o dado na buffer de transmissao da uart - CONFERIR ENDERECO *

UARTret:

    lui   $k0, %hi(_uart_buff)
    ori   $k0, $k0, %lo(_uart_buff)

    lw    $ra, 16($k0)               # restore registers $ra, $a0, $a1, $v0
    lw    $a0, 20($k0)
    lw    $a1, 24($k0)
    lw    $v0, 36($k0)

    mfc0  $k0, cop0_STATUS	         # Read STATUS register
    ori   $k0, $k0, M_StatusIEn      # but do not modify its contents
    addiu $k1, $zero, -7             # except for re-enabling interrupts
    and   $k0, $k1, $k0	             # -7 = 0xffff.fff9 = user mode
    mtc0  $k0, cop0_STATUS
    eret			                 # Return from interrupt

overrun: #faz o syscall de erro

lui   $v0, %hi(_uart_buff)
ori   $v0, $v0, %lo(_uart_buff)
sw $k0, 8($v0)
sw $k1, 12($v0)
nop
la $k1,x_IO_BASE_ADDR
addi $k0, $zero, 1
nop
sw $k0,0($k1)
nop
lw $k0, 8($v0)
lw $k1, 12($v0)
nop

    #eret

	.end UARTinterr
	#----------------------------------------------------------------


	#----------------------------------------------------------------
	# handler for COUNT-COMPARE registers -- IP7=HW5
	.text
	.set    noreorder
	.global countCompare
	.ent    countCompare
countCompare:
	mfc0  $k1,cop0_COUNT    # read COMPARE and clear IRQ
	addiu $k1,$k1,64	# set next interrupt in 64 ticks
	mtc0  $k1,cop0_COMPARE

	mfc0 $k0, cop0_STATUS	# Read STATUS register
	ori  $k0, $k0, M_StatusIEn #   but do not modify its contents
	lui  $k1, 0xffff        #   except for re-enabling interrupts
	ori  $k1, $k1, 0xfff9   #   and going into user mode
	and  $k0, $k1, $k0
	mtc0 $k0, cop0_STATUS
	eret			# Return from interrupt
	.end countCompare
	#----------------------------------------------------------------


	#----------------------------------------------------------------
	# functions to enable and disable interrupts, both return STATUS
	.text
	.set    noreorder
	.global enableInterr,disableInterr
	.ent    enableInterr
enableInterr:
	mfc0  $v0, cop0_STATUS	    # Read STATUS register
	ori   $v0, $v0, 1           #   and enable interrupts
	mtc0  $v0, cop0_STATUS
	nop
	jr $ra                      # return updated STATUS
	nop
	.end enableInterr

	.ent disableInterr
disableInterr:
	mfc0  $v0, cop0_STATUS	    # Read STATUS register
	addiu $v1, $zero, -2        #   and disable interrupts
	and   $v0, $v0, $v1         # -2 = 0xffff.fffe
	mtc0  $v0, cop0_STATUS
	nop
	jr $ra                      # return updated STATUS
	nop
	.end disableInterr
	#----------------------------------------------------------------


	#----------------------------------------------------------------
	# delays processing by approx 4*$a4 processor cycles
	.text
	.set    noreorder
	.global cmips_delay
	.ent    cmips_delay
cmips_delay:
	addiu $4, $4, -1
        nop
        bne $4, $zero, cmips_delay
        nop
        jr $ra
        nop
	.end    cmips_delay
	#----------------------------------------------------------------

print:  la $k1,x_IO_BASE_ADDR
		jr $31
		sw $k0,0($k1)
