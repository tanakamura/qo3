%define	PORT 0x3f8

%define REG_RBR 0
%define REG_THR 0
%define REG_DLL 0
%define REG_DLM 1
%define REG_IER 1
%define REG_FCR 2
%define REG_IIR 2
%define REG_LCR 3
%define REG_MCR 4
%define REG_LSR 5
%define REG_MSR 6
%define REG_SCR 7

%define FCR_FIFO_ENABLE (1<<0)
%define FCR_RESET_RCVR (1<<1)
%define FCR_RESET_XMIT (1<<2)
%define FCR_DMA_MODE_SELECT (1<<3)
%define FCR_RCVR_TRIGGER_TRIGGER_1 (0<<6)
%define FCR_RCVR_TRIGGER_TRIGGER_4 (1<<6)
%define FCR_RCVR_TRIGGER_TRIGGER_8 (2<<6)
%define FCR_RCVR_TRIGGER_TRIGGER_14 (3<<6)

%define IER_ERBFI (1<<0)
%define IER_ETBEI (1<<1)
%define IER_ELSI (1<<2)
%define IER_EDSSI (1<<3)

%define LSR_DR (1<<0)
%define LSR_OE (1<<1)
%define LSR_PE (1<<2)
%define LSR_FE (1<<3)
%define LSR_BI (1<<4)
%define LSR_THR (1<<5)
%define LSR_TEMT (1<<6)
%define LSR_ERROR_RCVR (1<<7)

%define LCR_WLS_5 (0<<0)
%define LCR_WLS_6 (1<<0)
%define LCR_WLS_7 (2<<0)
%define LCR_WLS_8 (3<<0)
%define LCR_STB2 (1<<2)
%define LCR_STB1 (0<<2)
%define LCR_PEN (1<<3)
%define LCR_EPS (1<<4)
%define LCR_STICK_PARITY (1<<5)
%define LCR_SET_BREAK (1<<6)
%define LCR_DLAB (1<<7)

%macro	outb	2
	mov	al, %2
	mov	dx, %1
	out	dx, al
%endmacro

init_ns16550:
	outb PORT+REG_LCR, LCR_WLS_8|LCR_STB2
	outb PORT+REG_FCR, FCR_FIFO_ENABLE|FCR_RESET_RCVR|FCR_RESET_XMIT
	outb PORT+REG_LCR, LCR_DLAB
	outb PORT+REG_DLL, 0x01
	outb PORT+REG_DLM, 0x00
	outb PORT+REG_LCR, 0

	outb PORT+REG_LCR, LCR_WLS_8|LCR_STB1
	ret

	; esi : str
	; eax, edx : clobbered
puts:
puts_poll:
	mov	dx, PORT+REG_LSR
	in	al, dx
	test	al, LSR_THR
	jz	puts_poll

	mov	al, [esi]
	add	esi, 1
	cmp	al, 0
	je	exit_puts

	mov	dx, PORT + REG_THR
	out	dx, al

	jmp	puts

exit_puts:
	ret
