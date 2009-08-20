#include "ns16550.h"
#include "intrinsics.h"
#include <stdio.h>
#include "kernel/ioapic.h"
#include "kernel/int-assign.h"
#include "kernel/lapic.h"

#define PORT 0x3f8

#define REG_RBR 0		/* r */
#define REG_THR 0		/* w */
#define REG_DLL 0               /* w (dlab = 1) */
#define REG_DLM 1               /* w (dlab = 1) */
#define REG_IER 1		/* rw */
#define REG_FCR 2		/* w */
#define REG_IIR 2		/* r */
#define REG_LCR 3		/* rw */
#define REG_MCR 4		/* rw */
#define REG_LSR 5		/* rw */
#define REG_MSR 6		/* rw */
#define REG_SCR 7		/* rw */

#define FCR_FIFO_ENABLE (1<<0)
#define FCR_RESET_RCVR (1<<1)
#define FCR_RESET_XMIT (1<<2)
#define FCR_DMA_MODE_SELECT (1<<3)
#define FCR_RCVR_TRIGGER_TRIGGER_1 (0<<6)
#define FCR_RCVR_TRIGGER_TRIGGER_4 (1<<6)
#define FCR_RCVR_TRIGGER_TRIGGER_8 (2<<6)
#define FCR_RCVR_TRIGGER_TRIGGER_14 (3<<6)

#define IER_ERBFI (1<<0)	/* received data available */
#define IER_ETBEI (1<<1)	/* transmitter holding registere empty */
#define IER_ELSI (1<<2)		/* receiver line */
#define IER_EDSSI (1<<3)	/* modem */

#define LSR_DR (1<<0)
#define LSR_OE (1<<1)
#define LSR_PE (1<<2)
#define LSR_FE (1<<3)
#define LSR_BI (1<<4)
#define LSR_THR (1<<5)
#define LSR_TEMT (1<<6)
#define LSR_ERROR_RCVR (1<<7)

#define LCR_WLS_5 (0<<0)
#define LCR_WLS_6 (1<<0)
#define LCR_WLS_7 (2<<0)
#define LCR_WLS_8 (3<<0)
#define LCR_STB2 (1<<2)
#define LCR_STB1 (0<<2)
#define LCR_PEN (1<<3)
#define LCR_EPS (1<<4)
#define LCR_STICK_PARITY (1<<5)
#define LCR_SET_BREAK (1<<6)
#define LCR_DLAB (1<<7)

void
ns16550_init(void)
{
        outb(PORT+REG_LCR, LCR_WLS_8|LCR_STB2);
        outb(PORT+REG_FCR, FCR_FIFO_ENABLE|FCR_RESET_RCVR|FCR_RESET_XMIT);

        /* set baud rate (115200) */
        outb(PORT+REG_LCR, LCR_DLAB);
        outb(PORT+REG_DLL, 0x01);
        outb(PORT+REG_DLM, 0x00);
        outb(PORT+REG_LCR, 0);

        outb(PORT+REG_LCR, LCR_WLS_8|LCR_STB1);
}

void
ns16550_init_intr(void)
{
	ioapic_set_redirect32(COM0_IRQ,
			      IOAPIC_DESTINATION_ID32(0),
			      IOAPIC_DELIVERY_FIXED|IOAPIC_VECTOR(COM0_VEC));
}

void
ns16550_write_poll(const char *buffer, int n)
{
	int i;
	for (i=0; i<n; i++) {
		unsigned char c;

		while (! ((c=inb(PORT+REG_LSR)) & LSR_THR))
			;

		outb(PORT + REG_THR, buffer[i]);
		eieio();
	}
}

void
ns16550_write_text_poll(const char *buffer, int n)
{
	int i;
	for (i=0; i<n; i++) {
		unsigned char c;

		while (! ((c=inb(PORT+REG_LSR)) & LSR_THR))
			;

		c = buffer[i];
		if (c == '\n') {
			outb(PORT + REG_THR, '\r');
			while (! ((c=inb(PORT+REG_LSR)) & LSR_THR))
				;
		}

		outb(PORT + REG_THR, buffer[i]);
		eieio();
	}
}


void
ns16550_read_poll(char *buffer, int n)
{
	int i;
	for (i=0; i<n; i++) {
		unsigned char c;

		while (! ((c=inb(PORT+REG_LSR)) & LSR_DR))
			;

		eieio();
		buffer[i] = inb(PORT+REG_RBR);
	}
}

static const unsigned char hex_table[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
};

void
ns16550_dump_registers(void)
{
	char buffer[3];
	unsigned char data;
	buffer[2] = '\0';

#define DUMP(x)					\
	data = inb(PORT+x);			\
	buffer[0] = hex_table[data>>4];		\
	buffer[1] = hex_table[data&0xf];	\
	puts(buffer);

	puts("LCR=");
	DUMP(REG_LCR);
	puts("IER=");
	DUMP(REG_IER);
	puts("IIR=");
	DUMP(REG_IIR);
	puts("LCR=");
	DUMP(REG_LCR);
	puts("MCR=");
	DUMP(REG_MCR);
	puts("LSR=");
	DUMP(REG_LSR);
	puts("MSR=");
	DUMP(REG_MSR);
	puts("SCR=");
	DUMP(REG_SCR);
}

struct ns16550_request_set {
	event_bits_t *ready_ptr;
	event_bits_t ready_bit;
	int read_remain;
	char *read_ptr;
};

static struct ns16550_request_set request_set;

void
cns16550_intr(void)
{
	int ier;
	unsigned char c;
	while (inb(PORT+REG_LSR) & LSR_DR) {
		eieio();
		c = inb(PORT+REG_RBR);

		*(request_set.read_ptr) = c;
		request_set.read_ptr++;
		request_set.read_remain--;

		if (request_set.read_remain==0) {
			*(request_set.ready_ptr) |= request_set.ready_bit;
			sfence();
			ier = inb(PORT+REG_IER);
			ier &= ~(IER_ERBFI|IER_ELSI);
			outb(PORT + REG_IER, ier);
		}
	}

	write_local_apic(LAPIC_EOI, 0);
}

void
ns16550_read(char *buffer, int n, event_bits_t *ready_ptr, event_bits_t ready_bit)
{
	int ier = inb(PORT+REG_IER);
	request_set.ready_ptr = ready_ptr;
	request_set.ready_bit = ready_bit;

	request_set.read_remain = n;
	request_set.read_ptr = buffer;
	sfence();

	ier |= IER_ERBFI|IER_ELSI;

	outb(PORT+REG_IER, ier);
}
