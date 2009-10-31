#include "kernel/hda.h"
#include "kernel/pci.h"
#include "kernel/pci-id.h"
#include "kernel/wait.h"
#include <stdio.h>


#define GCTL 0x08
#define WAKEEN 0x0c
#define STATESTS 0x0e

#define INTCTL 0x20
#define INTCTL_GIE (1<<31)
#define INTCTL_CIE (1<<30)

#define CORBSIZE_8 0x0
#define CORBSIZE_64 0x1
#define CORBSIZE_1024 0x2

#define CORBLBASE 0x40          /* 4byte */
#define CORBUBASE 0x44          /* 4byte */
#define CORBRP 0x4a             /* 2byte */
#define CORBRP_RESET (1<<15)
#define CORBWP 0x48             /* 2byte */
#define CORBSIZE 0x4e           /* 1byte */


#define RIRBSIZE_16 0x0
#define RIRBSIZE_128 0x1
#define RIRBSIZE_2048 0x2

#define RIRBLBASE 0x50          /* 4byte */
#define RIRBUBASE 0x54          /* 4byte */
#define RIRBWP 0x58             /* 2byte */
#define RIRBWP_RESET (1<<15)
#define RIRBSIZE 0x5e           /* 1byte */


#define CORBCTL 0x4c
#define CORBCTL_RUN (1<<1)
#define CORBCTL_CMEIE (1<<0)

#define RIRBCTL 0x5c
#define RIRBCTL_RIRBOIC (1<<2)
#define RIRBCTL_RIRBDMAEN (1<<1)
#define RIRBCTL_RINTCTL (1<<0)

static unsigned char corb[1024] __attribute__((aligned(1024)));
static unsigned char rirb[2048] __attribute__((aligned(1024)));

struct hda_driver {
	uintptr_t hdbar;
	int num_oss;
	int num_iss;
	int num_bss;
	int num_sdo;
};

static struct hda_driver hda;

int
hda_init(struct pci_root *pci, struct hda_init_error *err)
{
	struct pci_device *p = pci->devices;
	int n = pci->num_dev, i;
	uintptr_t base;
	uint16_t gcap;
	uint32_t c;

	for (i=0; i<n; i++) {
		if (p[i].bcc == PCI_CLS_MULTIMEDIA &&
		    p[i].scc == PCI_SUBCLS_HD_AUDIO) {
			break;
		}
	}

	if (i == n) {
		err->code = HDA_INIT_NOT_FOUND;
		return -1;
	}

	p = pci->devices + i;
	base = pci_conf_read32(p, 0x10) & ~((1<<13)-1); /* align to 16k */

	hda.hdbar = base;

	gcap = mmio_read16(base + 0);

	/* stop corb, rirb */
	mmio_write8(base + CORBCTL, 0);
	mmio_write8(base + RIRBCTL, 0);	

	hda.num_oss = gcap>>12;
	hda.num_iss = (gcap>>8)&0xf;
	hda.num_bss = (gcap>>3)&0x1f;
	hda.num_sdo = 1<<((gcap>>1)&0x3);

	n = hda.num_oss + hda.num_iss + hda.num_bss + hda.num_sdo;

	/* stop all streams */
	for (i=0; i<n; i++) {
		mmio_write32(base + 0x80 + 0x20*i, 0);
	}

	/* clear all interrupts */
	mmio_write16(base + STATESTS, (1<<14)-1);

	/* assert reset */
	c = mmio_read32(base + GCTL);
	mmio_write32(base + GCTL, c&~1);
	while (1) {
		wait_usec(100);
		c = mmio_read32(base + GCTL);
		if ((c & 1) == 0) {
			break;
		}
	}

	/* deassert reset */
	c = mmio_read32(base + GCTL);
	mmio_write32(base + GCTL, c|1);
	wait_usec(100);

	while (1) {
		c = mmio_read32(base + GCTL);
		wait_usec(250);
		if (c & 1) {
			break;
		}
	}

	c = mmio_read32(base + GCTL);

	mmio_write16(base + WAKEEN, 0);

        mmio_write32(base + CORBLBASE, (uint32_t)corb);
        mmio_write32(base + CORBUBASE, 0);
        mmio_write32(base + CORBSIZE, CORBSIZE_1024);
        mmio_write32(base + CORBRP, CORBRP_RESET);
        mmio_write32(base + CORBWP, 0);
        mmio_write32(base + RIRBWP, RIRBWP_RESET);

        mmio_write32(base + RIRBLBASE, (uint32_t)rirb);
        mmio_write32(base + RIRBUBASE, 0);
        mmio_write32(base + RIRBSIZE, RIRBSIZE_2048);

	return 0;
}


void
hda_dump(void)
{
	uintptr_t base = hda.hdbar;
	printf("base = %08x\n", (int)base);
	printf("noss=%d, niss=%d, nbss=%d, nsdo=%d\n",
	       hda.num_oss, hda.num_iss, hda.num_bss, hda.num_sdo);
	printf("gcap = %08x\n", mmio_read16(base + 0));
	printf("outpay = %08x\n", mmio_read16(base + 4));
	printf("inpay = %08x\n", mmio_read16(base + 6));
	printf("gctl = %08x\n", mmio_read32(base + 8));
	printf("statests = %08x\n", mmio_read16(base + 0x0e));
}
