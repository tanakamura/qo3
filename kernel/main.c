#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "kernel/fatal.h"
#include "ns16550.h"
#include "intrinsics.h"
#include "vga.h"
#include "msr.h"
#include "lapic.h"
#include "pci.h"
#include "ioapic.h"
#include "ich7.h"
#include "intr.h"
#include "int-assign.h"
#include "hpet.h"
#include "wait.h"
#include "smp.h"
#include "qo3-acpi.h"
#include "gma.h"
#include "event.h"
#include "acpi.h"
#include "brk.h"
#include "bios.h"
#include "hda.h"
#include "kernel/net/r8169.h"
#include "kernel/uhci.h"
#include "kernel/net/tcpip.h"
#include "kernel/bench.h"
#include "kernel/segment16.h"
#include "kernel/self-info.h"
#include "kernel/page.h"

#define SIZEOF_STR(p) (sizeof(p) - 1)
#define WRITE_STR(p) ns16550_write_text_poll(p, SIZEOF_STR(p))

static struct r8169_dev r8169_dev;
static struct uhci_dev uhci_dev;
static struct tcpip_link_state tcpip_link;

static int
serial_gets(char *buffer, int buflen)
{
	int i;
	char c;
	for (i=0; i<buflen; i++) {
		event_bits_t ready = 0;

		ns16550_read(&c, 1, &ready, (1<<0));
		wait_event_any(&ready, (1<<0));

		ns16550_write_poll(&c, 1);

		if (c == '\r') {
			ns16550_write_poll("\n", 1);
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
extern unsigned char apboot_main16_end[];

static void
boot_ap(void)
{
	unsigned int addr = APBOOT_ADDR_4K;
	unsigned char *apboot_addr = (unsigned char*)APBOOT_ADDR;

	memcpy(apboot_addr, apboot_main16, apboot_main16_end-apboot_main16);
	sfence();

	/* send INIT 
	 *	0    0	  0    c    4	 5    0	   0
	 * 0b0000 0000 0000 1100 0100 0101 0000 0000 */
	write_local_apic(LAPIC_ICR0, 0x000c4500);
	/* wait 10msec */
	wait_msec(10);
	/* send startup 
	 *	0    0	  0    c    4	 6    addr 
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
	uint32_t cnf;
	int sec;
	uint32_t ticks;
	event_bits_t ready = 0;

	/* enable replace legacy root */
	cnf = HPET_READ(HPET_GEN_CONF);
	cnf |= HPET_LEG_RT_CNF;
	HPET_WRITE(HPET_GEN_CONF, cnf);

	printf("sec?> ");
	sec = read_int();

	ticks = hpet_msec_to_tick(sec * 1000);
	hpet_oneshot(ticks, 0, &ready, 1);

	wait_event_any(&ready, 1);

	puts("ok");
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

static void
do_mwait(void)
{
	int c = 0;
	monitor(&c, 0, 0);
	mwait(3<<4|2, 0);
}

static void
do_async_get(void)
{
	char buffer[4];
	event_bits_t ready = 0;

	ns16550_read(buffer, 2, &ready, (1<<0));

	wait_event_any(&ready, (1<<0));
	buffer[2] = '\0';
	printf("ok: %s\n", buffer);
}

static void
do_disablevga(void)
{
	gma_disable_vga();
}


static void
do_enablevga(void)
{
	gma_enable_vga();
}

static void
do_acpi_sleep(void)
{
	int n;
	ACPI_STATUS r;

	printf("S?> ");
	n = read_int();

	r = AcpiEnterSleepStatePrep(n);
	if (ACPI_FAILURE(r)) {
		printf("sleep prep: %s\n", AcpiFormatException(r));
	}

	r = AcpiEnterSleepState(n);
	if (ACPI_FAILURE(r)) {
		printf("sleep: %s\n", AcpiFormatException(r));
	}
}

static void
do_reset(void)
{
	/*
	ACPI_STATUS r = AcpiReset();
	if (ACPI_FAILURE(r)) {
		printf("reset: %s\n", AcpiFormatException(r));
	}
	*/
	bios_system_reset();
}

static void
do_hda_dump(void)
{
	hda_dump();
}

static void
do_lspci(void)
{
	lspci(&pci_root0);
}

static void
do_lspci_tree(void)
{
	lspci_tree(&pci_root0);
}

static void
do_r8169_dump(void)
{
	r8169_dump(&r8169_dev);
}

static unsigned char buffer[4096] __attribute__((aligned(4096)));
static unsigned char buffer_many[2][4096] __attribute__((aligned(4096)));

static void
do_r8169_tx(void)
{
	int i;

	for (i=0; i<10; i++) {
		event_bits_t done = 0;
		r8169_tx_packet(&r8169_dev,
				buffer, 28+14, 0,
				&done, 1);
		wait_event_any(&done, 1);
	}
}

static void
do_r8169_rx(void)
{
	uint32_t flags;
	event_bits_t done = 0;
	int i,j,len;

	event_bits_t bits = 1<<0;

	r8169_rx_packet(&r8169_dev, buffer, 1024, &flags,
			&done, bits);

	wait_event_any(&done, (1<<0));

	len = flags & ((1<<14)-1);
	for (i=0; i<(len+15)/16; i++) {
		printf("%04x: ", i*16);
		for (j=0; j<16; j++) {
			printf("%02x ", buffer[i*16+j]);
		}
		puts("");
	}
}

static void
do_tcpip_dump(void)
{
	tcpip_dump(&tcpip_link);
}

static void
do_tcpip_rs(void)
{
	int len = tcpip_build_rs(buffer, &tcpip_link), i, j;
	uint32_t flags;
	event_bits_t done = 0;
	struct tcpip_parse_result parse;

	R8169_RX_CLEAR_COMPLETE(flags);
	r8169_rx_packet(&r8169_dev, buffer_many[0], 1024, &flags,
			&done, 2);
	r8169_tx_packet(&r8169_dev,
			buffer, len, 0,
			&done, 1);

	while (1) {
		ns16550_read(buffer_many[1], 1, &done, 4);
		wait_event_any(&done, 2|4);

		if (done&4) {
			if (buffer_many[1][0] == 'q') {
				return;
			}

			__sync_and_and_fetch(&done, ~4);
		}

		if (done & 2) {
			break;
		}
	}

	len = flags & ((1<<14)-1);

	for (i=0; i<(len+15)/16; i++) {
		printf("%04x: ", i*16);
		for (j=0; j<16; j++) {
			printf("%02x ", buffer_many[0][i*16+j]);
		}
		puts("");
	}

	if (len > 14) {
		tcpip_parse_packet(&tcpip_link, buffer_many[0]+14, len-14, &parse);
		printf("%d\n", (int)parse.code);
	}

	wait_event_all(&done, 1);
}


static void
do_bench(void)
{
	run_bench(&r8169_dev);
}

static void
do_hello_ap(void)
{
	int a;

	printf("apic id?> ");
	a = read_int();
	post_command_to_ap(a, AP_COMMAND_HELLO);
}

void
do_list_ap(void)
{
	int i;
	for (i=0; i<NUM_MAX_CPU; i++) {
		if (smp_table[i].flags & PROCESSOR_ENABLED) {
			printf("hello %d!\n", i);
		}
	}
}

#define NAME_LEN(name) name, sizeof(name)-1
static void do_help(void);

static struct command commands[] = {
	{NAME_LEN("bench"), do_bench},
	{NAME_LEN("reset"), do_reset},
	{NAME_LEN("rdmsr"), show_msr},
	{NAME_LEN("cpuid"), show_cpuid},
	{NAME_LEN("show_mtrr"), show_mtrr},
	{NAME_LEN("boot_ap"), boot_ap},
	{NAME_LEN("hello_ap"), do_hello_ap},
	{NAME_LEN("list_ap"), do_list_ap},
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
	{NAME_LEN("help"), do_help},
	{NAME_LEN("mwait"), do_mwait},
	{NAME_LEN("async_get"), do_async_get},
	{NAME_LEN("disablevga"), do_disablevga},
	{NAME_LEN("enablevga"), do_enablevga},
	{NAME_LEN("acpi_sleep"), do_acpi_sleep},
	{NAME_LEN("hda_dump"), do_hda_dump},
	{NAME_LEN("lspci"), do_lspci},
	{NAME_LEN("lspci_tree"), do_lspci_tree},
	{NAME_LEN("r8169_dump"), do_r8169_dump},
	{NAME_LEN("r8169_tx"), do_r8169_tx},
	{NAME_LEN("r8169_rx"), do_r8169_rx},

	{NAME_LEN("tcpip_dump"), do_tcpip_dump},
	{NAME_LEN("tcpip_rs"), do_tcpip_rs}
};

#define ALEN(a) (sizeof(a)/sizeof(a[0]))

static void
do_help(void)
{
	unsigned int i;
	for (i=0; i<ALEN(commands); i++) {
		puts(commands[i].com);
	}
}

__attribute__((noinline))
static void
dump_info(void)
{
	unsigned int ioapic_ver, bsp_apic_id, ebda_addr, lvt;
	unsigned long long msr_base;
	unsigned int hpet_period;
	short *e820_table_info;

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

	e820_table_info = (short*)get_segment16_addr(E820_TABLE_INFO);
	if (e820_table_info[0]) {
		int n = e820_table_info[1], i;
		uint32_t *e820_table = (uint32_t*)get_segment16_addr(E820_TABLE);

		printf("size of e820 table: %d\n", n);

		for (i=0; i<n; i++) {
			/*  0: base addr low
			 *  4: base addr high
			 *  8: length low
			 * 12: length high
			 * 16: type
			 */
			printf("%04d: addr: %08x%08x, length:%08x%08x, type=%x\n",
			       i,
			       e820_table[5*i+1], e820_table[5*i+0],
			       e820_table[5*i+3], e820_table[5*i+2],
			       e820_table[5*i+4]);
		}
	} else {
		puts("no smap");
	}
}

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
	struct pci_init_error pci_error;
	struct gma_init_error gma_error;
	struct hda_init_error hda_err;
	struct r8169_init_error r8169_err;
	struct uhci_init_error uhci_err;

	int i, j;
	for (i=0; i<25; i++) {
		for (j=0; j<80; j++) {
			int idx = i*160 + j*2;
			p[idx] = ' ';
		}
	}
	cpuid(1, ver, bidx, extf, func);

	init_kernel_address_space();

	brk_init();

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

	acpi_start();

	printf("IO APIC @ %08x\n", (int)apic_info.ioapic_addr);
	printf("num processor = %d\n", (int)apic_info.num_processor);

	r = pci_init(&pci_root0, &pci_error);
	if (r < 0) {
		puts("pci init error");
		pci_print_init_error(&pci_error);
	}

	lspci(&pci_root0);
	lspci_tree(&pci_root0);

	r = hda_init(&pci_root0, &hda_err);
	if (r < 0) {
		puts("hda init error");
	}

	printf("link = %p, 8169 = %p\n", &tcpip_link, &r8169_dev);


	r = r8169_init(&pci_root0, &r8169_dev, &r8169_err, 0);
	if (r < 0) {
		puts("r8169 init error");
		r8169_print_init_error(&r8169_err);
	}

	printf("link = %p, 8169 = %p\n", &tcpip_link, &r8169_dev);

	/*
	r = gma_init(&gma_error);
	if (r < 0) {
		puts("gma init error");
		vga_puts("gma init error");
	}
	*/
	(void)gma_error;

	/*
	r = uhci_init(&pci_root0, &uhci_dev, &uhci_err, 0);
	if (r < 0) {
		puts("uhci init error");
	}
	*/
	(void)uhci_err;
	(void)uhci_dev;

	ns16550_init_intr();

	tcpip_init(&tcpip_link, r8169_dev.mac);

	while(1) {
		int len;
		size_t i;

		printf("> ");
		len = serial_gets(buffer, 16);
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

	((ap_command_t*)ap_command_mwait_line[apic_id])[0] = 0;
	smp_table[apic_id].flags |= PROCESSOR_ENABLED;
	sfence();

	while (1) {
		ap_thread(apic_id);
	}
}

