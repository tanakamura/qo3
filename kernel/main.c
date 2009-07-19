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
#include "intr.h"
#include "hpet.h"
#include "wait.h"
#include "smp.h"
#include "acpi.h"

#define SIZEOF_STR(p) (sizeof(p) - 1)
#define WRITE_STR(p) ns16550_write_text(p, SIZEOF_STR(p))

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

static int
read_int(void)
{
	char digits[16];
	int val;
	int len = serial_gets(digits, 15);
	digits[len] = '\0';
	val = atoi(digits);
	return val;
}

static void
show_cpuid(void)
{
	int val, a, b, c, d;

	printf("eax?> ");
	val = read_int();

	cpuid(val, a, b, c, d);

	printf("a=%08x b=%08x c=%08x d=%08x\n", a, b, c, d);
}

static void
show_msr(void)
{
	int val, a, d;
	printf("ecx?> ");
	val = read_int();

	rdmsr(val, a, d);

	printf("a=%08x d=%08x\n", a, d);
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

		printf("%d: %016llx %016llx\n", i, base, mask);
		printf("%d: base=%08x\n", i, ((unsigned int)base)&(0-(1<<12)));
		printf("%d: mask=%08x\n", i, ((unsigned int)mask)&(0-(1<<12)));
		printf("%d: v=%d\n", i, (((unsigned int)mask)&(1<<11))>>11);
		printf("%d: type=%08x\n", i, ((unsigned int)base)&0xff);
	}
}

extern unsigned char apboot_main16[];

static void
boot_ap(void)
{
	unsigned int addr = APBOOT_ADDR_4K;
	unsigned char *apboot_addr = (unsigned char*)APBOOT_ADDR;

	memcpy(apboot_addr, apboot_main16, 512);
	sfence();

	/* send INIT 
	 *      0    0    0    c    4    5    0    0
	 * 0b0000 0000 0000 1100 0100 0101 0000 0000 */
	write_local_apic(LAPIC_ICR0, 0x000c4500);
	/* wait 10msec */
	wait_msec(10);
	/* send startup 
	 *      0    0    0    c    4    6    addr 
	 * 0b0000 0000 0000 1100 0100 0110 xxxx xxxx */
	write_local_apic(LAPIC_ICR0, 0x000c4600 | addr);
	/* wait 200usec */
	wait_usec(200);
	/* sipi */
	write_local_apic(LAPIC_ICR0, 0x000c4600 | addr);
	/* wait 200usec */
	wait_usec(200);


	if (have_too_many_cpus) {
		puts("have many cpus");
	}
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

static void
do_ltimer(void)
{
	int val;
	write_local_apic(LAPIC_DIVIDE, LAPIC_DIVIDE_1);
	printf("value?> ");
	val = read_int();
	write_local_apic(LAPIC_INITIAL_COUNT, val);
	write_local_apic(LAPIC_CURRENT_COUNT, val);

	LAPIC_SET_LVT_TIMER(LAPIC_TIMER_MODE_PERIODIC, LAPIC_UNMASK);
}

static void
do_rtimer(void)
{
	int val = read_local_apic(LAPIC_CURRENT_COUNT);
	printf("%d\n", val);
}

static void dump_info(void);

static void
do_sti(void)
{
	__asm__ __volatile__ ("sti");
}

static void
do_waitsec(void)
{
	int val;
	printf("sec?> ");
	val = read_int();
	wait_sec(val);
	puts("ok");
}

static void
do_hpetint(void)
{
	int val;
	printf("sec?> ");
	val = read_int();
}

static void
do_acpiapic(void)
{
	uintptr_t apic = find_acpi_description_entry(ACPI_SIG('A','P','I','C'));
	int len;
	int off;

	if (!apic) {
		puts("no APIC entry");
		return;
	}
	len = ACPI_R32(apic, 4);

	len -= 44;		/* offset to APIC structure */
	off = 44;

	while (len > 0) {
		enum acpi_apic_type_code code = ACPI_R8(apic, off);
		int str_len = ACPI_R8(apic, off+1);

		switch (code) {
		case ACPI_PROCESSOR_LOCAL_APIC:
			puts("local apic");
			printf(" id = %d\n", ACPI_R8(apic, off + 3));
			printf(" flags = %08x\n", ACPI_R32(apic, off + 4));
			break;

		case ACPI_IO_APIC:
			puts("io apic");
			printf(" id = %d\n", ACPI_R8(apic, off + 2));
			printf(" addr = %08x\n", ACPI_R32(apic, off + 4));
			printf(" global system int base = %08x\n", ACPI_R32(apic, off + 8));
			break;

		case ACPI_INTERRUPT_SOURCE_OVERRIDE:
			puts("source override");
			printf(" bus = %d (maybe 0)\n", ACPI_R8(apic, off+2));
			printf(" source = %d\n", ACPI_R8(apic, off+3));
			printf(" global source int = %d\n", ACPI_R32(apic, off+4));
			printf(" flags = %d\n", ACPI_R16(apic, off+8));
			break;

		case ACPI_NMI_SOURCE:
			puts("nmi source");
			printf(" flags = %x\n", ACPI_R16(apic, off+2));
			printf(" global system int = %d\n", ACPI_R32(apic, off+4));
			break;

		case ACPI_LOCAL_APIC_NMI_STRUCTURE:
			puts("nmi structure");
			printf(" id = %d\n", ACPI_R8(apic, off+2));
			printf(" flags = %x\n", ACPI_R16(apic, off+3));
			printf(" lint = %d\n", ACPI_R8(apic, off+5));
			break;
			
		case ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE:
			puts("lapic address over");
			break;

		case ACPI_IO_SAPIC:
			puts("io sapic");
			break;

		case ACPI_LOCAL_SAPIC:
			puts("local sapic");
			break;

		case ACPI_PLATFORM_INTERRUPT_SOURCES:
			puts("platform int source");
			break;

		default:
			puts("unknown apic desctiption");
			break;
		}

		off += str_len;
		len -= str_len;
	}
}

static void
do_dispsdt(void)
{
	int i;
	uintptr_t addr;
	int n;

	addr = acpi_table.rsdt;
	if (addr) {
		n = acpi_table.rsdt_nentry;
		for (i=0; i<n; i++) {
			uintptr_t e = ACPI_R32(addr, 36+i*4);
			char buf[5];
			buf[0] = ((char*)e)[0];
			buf[1] = ((char*)e)[1];
			buf[2] = ((char*)e)[2];
			buf[3] = ((char*)e)[3];
			buf[4] = '\0';

			printf("%s @ %08x\n", buf, (int)e);
		}
	}
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
	{NAME_LEN("ltimer"), do_ltimer},
	{NAME_LEN("rtimer"), do_rtimer},
	{NAME_LEN("dump"), dump_info},
	{NAME_LEN("sti"), do_sti},
	{NAME_LEN("waitsec"), do_waitsec},
	{NAME_LEN("dispsdt"), do_dispsdt},
	{NAME_LEN("acpiapic"), do_acpiapic},
	{NAME_LEN("hpetint"), do_hpetint},
};

static void
dump_info(void)
{
	unsigned int ioapic_ver, bsp_apic_id, ebda_addr, lvt;
	unsigned long long msr_base;
	unsigned int hpet_period;

	/* find apic ver */
	ich7_write(IOAPIC_INDEX,IOAPIC_VER);
	ioapic_ver = ich7_read(IOAPIC_DATA);
	printf("IOAPIC ver. = %08x\n", ioapic_ver);

	bsp_apic_id = read_local_apic(LAPIC_ID);
	printf("BSP APIC ID. = %08x\n", bsp_apic_id);

	ebda_addr = *(uint16_t*)0x40e;

	printf("EBDA = %08x\n", ebda_addr);

	rdmsrll(IA32_APIC_BASE, msr_base);

	if (msr_base & (1<<11)) {
		puts("APIC enabled");
	}

	lvt = read_local_apic(LAPIC_LVT_TIMER);
	printf("LVT TIMER = %08x\n", lvt);
	lvt = read_local_apic(LAPIC_LVT_THERMAL);
	printf("LVT THERMAL = %08x\n", lvt);
	lvt = read_local_apic(LAPIC_LVT_PMC);
	printf("LVT PMC = %08x\n", lvt);
	lvt = read_local_apic(LAPIC_LVT_LINT0);
	printf("LVT LINT0 = %08x\n", lvt);
	lvt = read_local_apic(LAPIC_LVT_LINT1);
	printf("LVT LINT1 = %08x\n", lvt);
	lvt = read_local_apic(LAPIC_LVT_ERROR);
	printf("LVT ERROR = %08x\n", lvt);

	hpet_period = ICH7_HPET_READ(HPET_GCAP_HI);
	printf("hpet period = %d\n", hpet_period);
	printf("hpet freq .=. %d[kHz]\n", 1000000000/(hpet_period/1000));
}


#define ALEN(a) (sizeof(a)/sizeof(a[0]))

void
cmain()
{
	char *p = (char*)0xb8000;
	char buffer[16];
	int ver, bidx, extf, func, r;
	unsigned int apic_svr;
	struct build_acpi_table_error build_acpi_table_error;
	enum apic_setup_error_code setup_apic_error;
	enum hpet_setup_error_code setup_hpet_error;

	int i, j;
	for (i=0; i<25; i++) {
		for (j=0; j<80; j++) {
			int idx = i*160 + j*2;
			p[idx] = ' ';
		}
	}
	cpuid(1, ver, bidx, extf, func);

	ns16550_init();
	WRITE_STR("hello QO3!\n");

	if (func & (1<<9)) {
		WRITE_STR("OK:APIC found\n");
		vga_puts("OK:APIC found");
	} else {
		WRITE_STR("FAIL:APIC not found. too old cpu\n");
		vga_puts("FAIL:APIC not found. too old cpu");
		while (1) hlt();
	}

	dump_info();

	apic_svr = read_local_apic(LAPIC_SVR);
	/* enable apic */
	write_local_apic(LAPIC_SVR, apic_svr | LAPIC_SVR_APIC_ENABLE);
	LAPIC_SET_LVT_ERROR(LAPIC_UNMASK);

	r = build_acpi_table(&build_acpi_table_error);
	if (r < 0) {
		print_build_acpi_table_error(&build_acpi_table_error);
		vga_puts("FAIL:ACPI not found");
		while (1) hlt();
	}
	printf("ACPI RSDP @ %08x\n", (int)acpi_table.rsdp);
	printf("ACPI XSDT @ %08x\n", (int)acpi_table.xsdt);
	printf("ACPI RSDT @ %08x\n", (int)acpi_table.rsdt);

	setup_apic_error = apic_setup();
	if (setup_apic_error != APIC_SETUP_OK) {
		puts("setup apic error");
		vga_puts("FAIL:ACPI not found");
		while (1) hlt();
	}

	printf("IO APIC @ %08x\n", (int)apic_info.ioapic_addr);
	printf("num processor = %d\n", (int)apic_info.num_processor);

	setup_hpet_error = hpet_setup();
	if (setup_hpet_error != HPET_SETUP_OK) {
		puts("Setup hpet error. Assumes @ 0xfed00000");
		hpet_base_addr = ICH7_HPET_ADDR_BASE;
	} else {
		printf("hpet @ %08x\n", (int)hpet_base_addr);
		if (hpet_flags & HPET_FLAG_4K) {
			puts("hpet 4k protected");
		} else if (hpet_flags & HPET_FLAG_64K) {
			puts("hpet 64k protected");
		}
	}

	while(1) {
		int len = serial_gets(buffer, 16);
		size_t i;
		for (i=0; i<ALEN(commands); i++) {
			if (len == commands[i].len) {
				if (memcmp(buffer, commands[i].com, len) == 0) {
					commands[i].command_func();
					break;
				}
			}
		}

		if (i == ALEN(commands)) {
			puts("unknown command");
		}
	}

	while (1);
}

void
cAP_main(void)
{
	unsigned int apic_id, apic_svr;
	/* enable apic */
	apic_svr = read_local_apic(LAPIC_SVR);
	write_local_apic(LAPIC_SVR, apic_svr | LAPIC_SVR_APIC_ENABLE);

	apic_id = read_local_apic(LAPIC_ID);
	apic_id >>= 24U;

	smp_table[apic_id].flags |= PROCESSOR_ENABLED;
	sfence();

	while (1) {
		hlt();
	}
}

