#include "kernel/gma.h"
#include "kernel/pci.h"
#include "kernel/gmaregs.h"
#include <stdio.h>

struct gma_pipe {
	
};

struct gma_dev {
	uintptr_t mmio_base;
	struct pci_device *pci;

	struct gma_pipe pipes[2];
};

#define SETUP_LOCAL					\
	uintptr_t gma_mmio_base = gma.mmio_base;

static struct gma_dev gma;

#define GMA_MR32(R) (mmio_read32(gma_mmio_base + (R)))
#define GMA_MR16(R) (mmio_read16(gma_mmio_base + (R)))
#define GMA_MR8(R) (mmio_read8(gma_mmio_base + (R)))

#define GMA_MW32(R,V) (mmio_write32(gma_mmio_base + (R), V))
#define GMA_MW16(R,V) (mmio_write16(gma_mmio_base + (R), V))
#define GMA_MW8(R,V) (mmio_write8(gma_mmio_base + (R), V))

int
gma_init(struct pci_root *pci, struct gma_init_error *error)
{
	int i;
	int n = pci->num_dev;
	struct pci_device *p = pci->devices;
	uintptr_t gma_mmio_base;
	for (i=0; i<n; i++) {
		if (p[i].vendor_id == 0x8086) {
			int di = p[i].device_id;
			if (di == 0x27a2 ||
			    di == 0x27ae ||
			    di == 0x2772) {
				break;
			}
		}
	}

	if (i == n) {
		error->code = GMA_NOT_FOUND;
		return -1;
	}
	p = pci->devices + i;
	gma_mmio_base = pci_conf_read32(p, 0x10);

	printf("id = %08x, mmio base=%08x, io base=%08x gmaddr=%08x gttaddr=%08x cmd=%08x\n",
	       pci_conf_read32(p, 0x0),
	       (int)gma_mmio_base,
	       pci_conf_read32(p, 0x14),
	       pci_conf_read32(p, 0x18),
	       pci_conf_read32(p, 0x1c),
	       pci_conf_read16(p, 0x04));

	printf("vgacntrl = %x\n", GMA_MR32(VGACNTRL));
	printf("pipeaconf = %x\n", GMA_MR32(PIPEACONF));
	printf("pipebconf = %x\n", GMA_MR32(PIPEBCONF));

	gma.pci = p;
	gma.mmio_base = gma_mmio_base;

	return 0;
}

void
gma_disable_vga(void)
{
	uint32_t v;
	SETUP_LOCAL;
	v = GMA_MR32(VGACNTRL);
	v |= (1<<31);
	GMA_MW32(VGACNTRL, v);

	v = GMA_MR32(PIPEACONF);
	v &= ~(1<<31);
	GMA_MW32(PIPEACONF, v);

	v = GMA_MR32(PIPEBCONF);
	v &= ~(1<<31);
	GMA_MW32(PIPEBCONF, v);
}

void
gma_enable_vga(void)
{
	uint32_t v;
	SETUP_LOCAL;
	v = GMA_MR32(VGACNTRL);
	v &= ~(1<<31);
	GMA_MW32(VGACNTRL, v);

	v = GMA_MR32(PIPEACONF);
	v |= (1<<31);
	GMA_MW32(PIPEACONF, v);

	v = GMA_MR32(PIPEBCONF);
	v |= (1<<31);
	GMA_MW32(PIPEBCONF, v);
}

int
gma_read_edid(char *buffer,
	      int size_buffer,
	      struct gma_read_edid_error *e)
{
    return 0;
}
