#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ns16550.h"
#include "bios.h"
#include "intrinsics.h"
#include "vga.h"
#include "msr.h"
#include "apic.h"
#include "ich7.h"

#define SIZEOF_STR(p) (sizeof(p) - 1)
#define WRITE_STR(p) ns16550_write(p, SIZEOF_STR(p))

static int
serial_gets(char *buffer, int buflen)
{
	int i;
	char c;
	for (i=0; i<buflen; i++) {
		ns16550_read(&c, 1);
		ns16550_write(&c, 1);

		if (c == '\r') {
			ns16550_write("\n", 1);
			break;
		}
		if (c == '\n') {
			break;
		}

		buffer[i] = c;
	}
	return i;
}

struct command {
	const char *com;
	int len;
	void (*command_func)(void);
};

static void
show_cpuid(void)
{
	char digits[16];
	int len, val, a, b, c, d;

	printf("eax?> ");
	len = serial_gets(digits, 15);
	digits[len] = '\0';
	val = atoi(digits);

	cpuid(val, a, b, c, d);

	printf("a=%08x b=%08x c=%08x d=%08x\r\n", a, b, c, d);
}

static void
show_msr(void)
{
	char digits[16];
	int len, val, a, d;
	printf("ecx?> ");
	len = serial_gets(digits, 15);
	digits[len] = '\0';
	val = atoi(digits);

	rdmsr(val, a, d);

	printf("a=%08x d=%08x\r\n", a, d);
}

static void
do_hlt(void)
{
	hlt();
}

static void
show_mtrr(void)
{
	unsigned long long base, mask;
	int i;
	for (i=0; i<8; i++) {
		rdmsrll(IA32_MTRR_PHYSBASE0+i*2, base);
		rdmsrll(IA32_MTRR_PHYSMASK0+i*2, mask);

		printf("%d: %016llx %016llx\r\n", i, base, mask);
		printf("%d: base=%08x\r\n", i, ((unsigned int)base)&(0-(1<<12)));
		printf("%d: mask=%08x\r\n", i, ((unsigned int)mask)&(0-(1<<12)));
		printf("%d: v=%d\r\n", i, (((unsigned int)mask)&(1<<11))>>11);
		printf("%d: type=%08x\r\n", i, ((unsigned int)base)&0xff);
	}
}

static void
boot_ap(void)
{
	
}

static void
div0(void)
{
	int a = 0;
	a = 1/(int)a;
}

static void
int3(void)
{
	__asm__ __volatile__ ("int3");
}


#define NAME_LEN(name) name, sizeof(name)-1

static struct command commands[] = {
	{NAME_LEN("reset"), bios_system_reset},
	{NAME_LEN("rdmsr"), show_msr},
	{NAME_LEN("cpuid"), show_cpuid},
	{NAME_LEN("show_mtrr"), show_mtrr},
	{NAME_LEN("boot_ap"), boot_ap},
	{NAME_LEN("div0"), div0},
	{NAME_LEN("int3"), int3},
	{NAME_LEN("hlt"), do_hlt},
};

#define APBOOT_ADDR_4K 0xbd
#define APBOOT_ADDR (APBOOT_ADDR_4K*4096)

static void
ap_main(void)
{
	while (1) {
	}
}

void
cinterrupt_main(void)
{
	while (1);
}

static void
dump_info(void)
{
	unsigned int ioapic_ver, bsp_apic_id, ebda_addr;
	unsigned long long msr_base;

	/* find apic ver */
	ich7_write(IOAPIC_INDEX,IOAPIC_VER);
	ioapic_ver = ich7_read(IOAPIC_DATA);
	printf("IOAPIC ver. = %08x\r\n", ioapic_ver);

	bsp_apic_id = read_local_apic(LOCAL_APIC_ID);
	printf("BSP APIC ID. = %08x\r\n", bsp_apic_id);

	ebda_addr = *(uint16_t*)0x40e;

	printf("EBDA = %08x\r\n", ebda_addr);

	rdmsrll(IA32_APIC_BASE, msr_base);

	if (msr_base & (1<<11)) {
		puts("APIC enabled");
	}
}



#define ALEN(a) (sizeof(a)/sizeof(a[0]))

int cmain()
{
	char *p = (char*)0xb8000;
	char buffer[16];
	int ver, bidx, extf, func;
	unsigned int apic_svr;
	unsigned char *apboot_addr;
	

	int i, j;
	for (i=0; i<25; i++) {
		for (j=0; j<80; j++) {
			int idx = i*160 + j*2;
			p[idx] = ' ';
		}
	}
	cpuid(1, ver, bidx, extf, func);

	ns16550_init();
	WRITE_STR("hello QO3!\r\n");

	if (func & (1<<9)) {
		WRITE_STR("OK:APIC found\r\n");
		vga_puts("OK:APIC found");
	} else {
		WRITE_STR("FAIL:APIC not found. too old cpu\r\n");
		vga_puts("FAIL:APIC not found. too old cpu");
		while (1)
			hlt();
	}

	dump_info();

	apic_svr = read_local_apic(LOCAL_APIC_SVR);
	write_local_apic(LOCAL_APIC_SVR, apic_svr | LOCAL_APIC_SVR_APIC_ENABLE);
	(void)apic_svr;

	apboot_addr = (unsigned char*)APBOOT_ADDR;
	apboot_addr[0] = 0xff;
	apboot_addr[1] = 0x25;
	*(int*)(apboot_addr+2) = 6;
	*(int*)(apboot_addr+6) = (int)ap_main;

	while(1) {
		int len = serial_gets(buffer, 16);
		size_t i;
		for (i=0; i<ALEN(commands); i++) {
			if (len == commands[i].len) {
				if (memcmp(buffer, commands[i].com, len) == 0) {
					commands[i].command_func();
				}
			}
		}
	}

	while (1);
}
