	# interrupt handlers
	.include "cMIPS.s"
	.text
	.set noreorder
    .align 2
	.global rx_queue , rx_hd , rx_tl # reception queue and pointers
	.comm rx_queue 16
	.comm rx_hd 4
	.comm rx_tl 4
	.global tx_queue , tx_hd , tx_tl # transmission queue and pointers
	.comm tx_queue 16
	.comm tx_hd 4
	.comm tx_tl 4
	.global nrx , ntx
	.comm nrx 4 # characters in RX_queue
	.comm ntx 4 # spaces left in TX_queue
	.set M_StatusIEn,0x0000ff09     # STATUS.intEn=1, user mode
	
	#----------------------------------------------------------------
	# interrupt handler for external counter attached to IP2=HW0
	# Counter address -- see vhdl/packageMemory.vhd

	.bss
	.align  2
	.global _counter_val             # accumulate number of interrupts
	.comm   _counter_val 4
	.comm   _counter_saves 8*4       # area to save up to 8 registers
	# _counter_saves[0]=$a0, [1]=$a1, [2]=$a2, ...
	
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
	# la $k1, _counter_saves
	# sw $a0, 0*4($k1)
	# sw $a1, 1*4($k1)
	#----------------------------------
	
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
	# la $k1, _counter_saves
	# lw $a0, 0*4($k1)
	# lw $a1, 1*4($k1)
	#----------------------------------
	
	mfc0  $k0, cop0_STATUS	    # Read STATUS register
	ori   $k0, $k0, M_StatusIEn #   but do not modify its contents
	addiu $k1, $zero, -7        #   except for re-enabling interrupts
	and   $k0, $k1, $k0	    #   -7 = 0xffff.fff9
	mtc0  $k0, cop0_STATUS	
	eret			    # Return from interrupt
	.end extCounter
	#----------------------------------------------------------------

	
	#----------------------------------------------------------------
	# interrupt handler for UART attached to IP3=HW1

	.bss 
    .align  2
	.global rx_queue,rx_hd,rx_tl   # reception queue and pointers
	.comm   rx_queue 16
	.comm   rx_hd 4
	.comm   rx_tl 4
	.global tx_queue,tx_hd,tx_tl   # transmission queue and pointers
	.comm   tx_queue 16
	.comm   tx_hd 4
	.comm   tx_tl 4
	.global nrx,ntx
	.comm   nrx 4                  # characters in RX_queue
	.comm   ntx 4                  # spaces left in TX_queue
	.extern Ud #UART data structure ########### L

	.set UART_rx_irq,0x08
	.set UART_tx_irq,0x10

	.text
	.set    noreorder
	.global UARTinterr
	.ent    UARTinterr

	#Ud[0] = ctl; [1] = stat;
	#[2] = nrx; [3] = rxhead; [4] = rxtail; [5] = rxqueue;
	#(mais pra frente acredito que fique):
	#[9] = ntx; [10] = txhead; [11] = txtail; [12] = txqueue
	#[16] = a0; [17] = a1 
	
UARTinterr:
	lui   $k1, %hi(HW_uart_addr)
	ori   $k1, $k1, %lo(HW_uart_addr) #k1 = endereço uart
	lw    $k0, 0($k1) # ler status -- conferir
	#cris estou me batendo muito com essa historia de buffer	
	#se tu for no handlers.s original, tu vi ver que tem um buffer, mas o nosso é dentro da uart ja ne??
	
	lui $k0, %hi(Ud)
	ori $k0, k0, %lo(Ud) # k0 = endereco Ud (uart data str?)
	sw $k1, 4(k0)
	sw $a0, 16*4(k0)
	sw $a1, 17*4(k0) #salvando a0 e a1

	.include "../tests/handlerUART.s"

	andi $a0, $k1, UART_rx_irq # é recepção?
	beq $a0, $zero, UARTret # se nao, xau
	
	lw $a0, 2*4(k0) # carrega nrx
	slti $a1, $a0, 16 # confere se nrx < 16
	bne $a1, $zero, overrun # quando n tem espaço xau
	nop

	addiu $a0, $a0, 1 # incrementa nrx
	sw $a0, 2*4(k0) # salva nrx
	
	lw $a0, 9*4(k0) # carrega ntx
	nop
	addiu $a0, $a0, -1 # decrementa tail
	andi $a0, $a0, 0xf # modulo 16?? - conferir
	sw $a0, 9*4(k0) # salva ntx
	##
	lw $a1, 4(k1) # ler data do uart rxreg
	addu $a0, $a0, $k0 # adicionar tail index to &(Ud)
	sb $a1, 8*4(a0) # coloca no rabo gg
	j UARTret
	nop	 

    # is this transmission?
    # if not, leave

    # load ntx
    # checks if ntx < 16 (theres something in there)
    # if there isnt, do nothing. Leave.

    # copy txqueue[txhead] to tx buffer.
    # decrement ntx.
    # update txhead and txtail.

	lui   $a0, %hi(HW_uart_addr)
	ori   $a0, $a0, %lo(HW_uart_addr)
	lw    $a1, 4($a0) 	    # Read data
	nop                         #   and store it to UART's buffer
	sw    $a1, 4($k0)           #   and return from interrupt
	addiu $a1, $zero, 1
	sw    $a1, 8($k0)           # Signal new arrival 
	
UARTret:
	lw    $a1, 16($k0)          # restore registers $a0,$a1, others?
	lw    $a0, 12($k0)

	mfc0  $k0, cop0_STATUS	    # Read STATUS register
	ori   $k0, $k0, M_StatusIEn #   but do not modify its contents
	addiu $k1, $zero, -7        #   except for re-enabling interrupts
	and   $k0, $k1, $k0	    #   -7 = 0xffff.fff9 = user mode
	mtc0  $k0, cop0_STATUS	
	eret			    # Return from interrupt

overrun: #faz o syscall de erro
	
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
