#include "kernel/pci.h"
#include "kernel/qo3-acpi.h"
#include "kernel/mmio.h"
#include <stdio.h>
#include <string.h>
#include "kernel/brk.h"
#include <acpi.h>
#include "kernel/pci-regs.h"
#include "kernel/pci-id.h"
#include "kernel/brk.h"
#include "kernel/bios.h"

struct pci_root pci_root0;

#if 1
static int
acpi_handle_to_bridge(struct pci_root *pci,
		      ACPI_HANDLE obj)
{
	ACPI_BUFFER buf;
	ACPI_HANDLE parent;
	ACPI_STATUS st;
	char *p;
	int node, i, n, *tree;
	unsigned int dev, fn;
	ACPI_OBJECT adr;
	ACPI_DEVICE_INFO *devinfo;

	if (obj == NULL) {
		goto fail;
	}

	st = AcpiGetObjectInfo(obj, &devinfo);

	p = devinfo->HardwareId.String;

	if (st == AE_OK &&
	    ((strcmp(p,"PNP0A03") == 0) ||
	     (strcmp(p,"PNP0A08") == 0)) )
	{
		/* root bridge */
		return 0;
	}

	st = AcpiGetParent(obj, &parent);
	if (ACPI_FAILURE(st)) {
		goto fail;
	}

	tree = pci->tree;
	node = acpi_handle_to_bridge(pci, parent);
	if (node < 0)
		return -1;
	n = tree[node + PCITREE_OFFSET_NUM_CHILD];

	buf.Pointer = &adr;
	buf.Length = sizeof(adr);
	st = AcpiEvaluateObject(obj, "_ADR", NULL, &buf);
	if (ACPI_FAILURE(st)) {
		goto fail;
	}

	dev = (((int)adr.Integer.Value)>>16) & 0xffff;
	fn = ((int)adr.Integer.Value)&0xffff;
	for (i=0; i<n; i++) {
		int off = tree[node + PCITREE_SIZEOF_NODE + i];
		struct pci_bridge *b = &pci->bridges[tree[off + PCITREE_OFFSET_BRIDGEID]];
		struct pci_device *d = &pci->devices[b->devid];

		if (BDF_DEV(d->busdevfn) == dev &&
		    BDF_FN(d->busdevfn) == fn) {
			return off;
		}
	}

	return -1;

fail:
	printf("ACPI find bridge failure.\n");
	bios_system_reset();
	return 0;
}

static ACPI_STATUS
assign_irq(ACPI_HANDLE obj,
	   UINT32 nest,
	   void *context,
	   void **ret)
{
	ACPI_BUFFER buf;
	ACPI_STATUS st;
	ACPI_PCI_ROUTING_TABLE *tbl, *tbl_start;
	int pcitree_node;
	struct pci_bridge *bridge;
	struct pci_root *pci = &pci_root0;
	int num_routing_table;
	unsigned int prev_addr;
	int tbl_idx;
	struct pci_irq_table *table;

	buf.Length = ACPI_ALLOCATE_BUFFER;
	buf.Pointer = NULL;

	st = AcpiGetIrqRoutingTable(obj, &buf);
	if (ACPI_FAILURE(st)) {
		/* This is not a pci bridge */
		return AE_OK;
	}

	tbl = tbl_start = (ACPI_PCI_ROUTING_TABLE*)buf.Pointer;
	if (tbl_start->Length <= 0) {
		return AE_OK;
	}

	pcitree_node = acpi_handle_to_bridge(pci, obj);
	if (pcitree_node < 0) {
		/* This does not exist in tree. */
		goto done;
	}

	bridge = &pci->bridges[pci->tree[pcitree_node + PCITREE_OFFSET_BRIDGEID]];

	num_routing_table = 1;
	prev_addr = tbl_start->Address;

	while (tbl->Length > 0) {
		if (tbl->Address != prev_addr) {
			num_routing_table ++;
			prev_addr = tbl->Address;
		}
		tbl = (ACPI_PCI_ROUTING_TABLE*)(((char*)tbl) + tbl->Length);
	}

	table = SBRK_TANATIVE(struct pci_irq_table, num_routing_table);

	prev_addr = tbl_start->Address;
	tbl_idx = 0;
	table[0].addr = tbl_start->Address;
	table[0].irq[0] = -1;
	table[0].irq[1] = -1;
	table[0].irq[2] = -1;
	table[0].irq[3] = -1;

	tbl = tbl_start;
	while (tbl->Length > 0) {
		if (tbl->Address != prev_addr) {
			tbl_idx++;
			prev_addr = tbl->Address;
			table[tbl_idx].addr = tbl->Address;
			table[tbl_idx].irq[0] = -1;
			table[tbl_idx].irq[1] = -1;
			table[tbl_idx].irq[2] = -1;
			table[tbl_idx].irq[3] = -1;
		}

		if (tbl->Pin < 4) {
			table[tbl_idx].irq[tbl->Pin] = tbl->SourceIndex;
		} else {
			/* ? */
		}

		tbl = (ACPI_PCI_ROUTING_TABLE*)(((char*)tbl) + tbl->Length);
	}

	bridge->num_irq_table = num_routing_table;
	bridge->irq_table = table;

done:
	AcpiOsFree(buf.Pointer);

	return AE_OK;
}

static int
build_tree(int *tree,
	   struct pci_bridge *bridges,
	   int *bridge_id,
	   struct pci_device *devs, int num_dev,
	   unsigned int cur_bus, int offset)
{
	int num_child = 0;
	int child_offset = 0;
	int i;
	int ret = 0;

	for (i=0; i<num_dev; i++) {
		if (devs[i].bcc == PCI_CLS_BRIDGE &&
		    devs[i].scc == PCI_SUBCLS_PCIPCI_BRIDGE)
		{
			if (BDF_BUS(devs[i].busdevfn) == cur_bus) {
				num_child++;
			}
		}
	}

	child_offset = offset + num_child;

	for (i=0; i<num_dev; i++) {
		if (devs[i].bcc == PCI_CLS_BRIDGE &&
		    devs[i].scc == PCI_SUBCLS_PCIPCI_BRIDGE)
		{
			/* This bridge is child of cur_bus */
			if (BDF_BUS(devs[i].busdevfn) == cur_bus) {
				int pbus = pci_conf_read32(&devs[i], PCI_PRIMARY_BUS);
				int bus = (pbus>>8)&0xff;
				int num_child;

				tree[offset] = child_offset;

				tree[child_offset+PCITREE_OFFSET_BRIDGEID] = *bridge_id;
				bridges[*bridge_id].devid = i;
				bridges[*bridge_id].busno = bus;

				(*bridge_id)++;

				num_child = build_tree(tree,
						       bridges, bridge_id,
						       devs, num_dev, bus,
						       child_offset + PCITREE_SIZEOF_NODE);

				tree[child_offset+PCITREE_OFFSET_NUM_CHILD] = num_child;

				child_offset += PCITREE_SIZEOF_NODE;
				offset++;

				ret++;
			}
		}
	}

	return ret;
}

/*
 * 36 header
 * 8  reserved(?)
 * 44 allocation
 * 
 * 16byte:
 *   0-7 address
 *   8-9 pci_segment
 *   10 start_bus
 *   11 end_bus
 *   12-15 reserved
 */

int
pci_init(struct pci_root *pci, struct pci_init_error *error)
{
	uintptr_t mcfg = find_acpi_description_entry(ACPI_SIG('M','C','F','G'));
	unsigned int len;
	unsigned int num_entry;
	uintptr_t entry;
	uintptr_t address;
	uintptr_t start, end, bus;
	int num_dev, i, cur_dev, num_bridge, *tree, host_bridge, num_child;
	void *assign_ret;
	struct pci_device *devs;
	struct pci_bridge *bridges;
	int bridge_id;

	if (!mcfg) {
		error->code = PCI_MCFG_NOT_FOUND;
		return -1;
	}

	len = ACPI_R32(mcfg, 4);
	len -= 44;
	num_entry = len/16;

	if (num_entry != 1) {
		error->code = PCI_TOO_MANY_MCFG;
		return -1;
	}

	entry = mcfg + 44;
	address = ACPI_R64ADDR(entry, 0);
	if (address & ~0xf8000000) {
		error->code = PCI_MCFG_INVALID_BASE;
		return -1;
	}

	pci->mcfg_addr = address;

	start = ACPI_R8(entry, 10);
	end = ACPI_R8(entry, 11);

	pci->nr_bus = end-start;

	start <<= 20;
	end <<= 20;

	num_dev = 0;

	for (bus=start; bus<end; bus+=(1<<20)) {
		uintptr_t dev;
		for (dev=0; dev<(32<<15); dev += (1<<15)) {
			uintptr_t mmio_addr = address | bus | dev;
			uint32_t id = mmio_read32(mmio_addr);
			uint32_t header;
			if (id == 0xffffffff || id == 0) {
				continue;
			}
			num_dev++;

			header = mmio_read32(mmio_addr + 0x0E);
			if (header & 0x80) {
				uintptr_t fn;
				for (fn = (1<<12); fn<(8<<12); fn+=(1<<12)) {
					mmio_addr = address | bus | dev | fn;
					id = mmio_read32(mmio_addr);
					if (id == 0xffffffff || id == 0) {
						continue;
					}

					num_dev++;
				}
			}
		}
	}

	pci->devices = SBRK_TA4(struct pci_device, num_dev);
	pci->num_dev = num_dev;
	cur_dev = 0;

	for (bus=start; bus<end; bus+=(1<<20)) {
		uintptr_t dev;
		for (dev=0; dev<(32<<15); dev += (1<<15)) {
			uintptr_t mmio_addr = address | bus | dev;
			uint32_t id = mmio_read32(mmio_addr);
			uint32_t header;
			uint8_t bcc,scc,pi;

			if (id == 0xffffffff || id == 0) {
				continue;
			}

			bcc = mmio_read8(mmio_addr + 0x0b);
			scc = mmio_read8(mmio_addr + 0x0a);
			pi = mmio_read8(mmio_addr + 0x09);

			pci->devices[cur_dev].busdevfn = bus|dev|0;
			pci->devices[cur_dev].conf_addr = mmio_addr;
			pci->devices[cur_dev].vendor_id = id&0xffff;
			pci->devices[cur_dev].device_id = id>>16U;
			pci->devices[cur_dev].bcc = bcc;
			pci->devices[cur_dev].scc = scc;
			pci->devices[cur_dev].pi = pi;

			cur_dev++;

			header = mmio_read32(mmio_addr + 0x0E);
			if (header & 0x80) {
				uintptr_t fn;
				for (fn = (1<<12); fn<(8<<12); fn+=(1<<12)) {
					mmio_addr = address | bus | dev | fn;
					id = mmio_read32(mmio_addr);

					if (id == 0xffffffff || id == 0) {
						continue;
					}
					bcc = mmio_read8(mmio_addr + 0x0b);
					scc = mmio_read8(mmio_addr + 0x0a);
					pi = mmio_read8(mmio_addr + 0x09);

					pci->devices[cur_dev].busdevfn = bus|dev|fn;
					pci->devices[cur_dev].conf_addr = mmio_addr;
					pci->devices[cur_dev].vendor_id = id&0xffff;
					pci->devices[cur_dev].device_id = id>>16U;
					pci->devices[cur_dev].bcc = bcc;
					pci->devices[cur_dev].scc = scc;
					pci->devices[cur_dev].pi = pi;

					cur_dev++;
				}
			}
		}
	}

	devs = pci->devices;

	/* build tree */
	num_bridge = 0;
	host_bridge = 0;

	/* count number of bridge */
	for (i=0; i<num_dev; i++) {
		if (devs[i].bcc == PCI_CLS_BRIDGE) {
			if (devs[i].scc == PCI_SUBCLS_PCIPCI_BRIDGE) {
				num_bridge ++;
			} else if (devs[i].scc == PCI_SUBCLS_HOST_BRIDGE) {
				num_bridge ++;
				host_bridge = i;
			}
		}
	}

	bridges = SBRK_TA8(struct pci_bridge, num_bridge);
	tree = SBRK_TA(int, num_bridge*PCITREE_SIZEOF_NODE + (num_bridge-1)*1);

	bridges[0].devid = host_bridge;
	bridges[0].busno = 0;
	tree[0] = 0;
	bridge_id = 1;		/* 0 for root */

	/*lspci(pci);*/

	num_child = build_tree(tree, bridges, &bridge_id, devs, num_dev, 0, 2);
	tree[1] = num_child;

	pci->tree = tree;
	pci->bridges = bridges;
	pci->num_bridge = num_bridge;

	AcpiWalkNamespace(ACPI_TYPE_ANY,
			  ACPI_ROOT_OBJECT,
			  ACPI_UINT32_MAX,
			  assign_irq,
			  NULL,
			  &assign_ret);

	if (num_dev != cur_dev) {
		return -1;
	}

	return 0;
}
#else

static inline uint32_t
conf32(int bus, int dev, int fn, int reg)
{
	outl(0xcf8,
	     0x80000000 |
	     (bus<<16) |
	     (dev<<(8+3)) |
	     (fn<<8) |
	     (reg & 0xfc));

	return inl(0xcfc);
}
static inline uint16_t
conf16(int bus, int dev, int fn, int reg)
{
	outl(0xcf8,
	     0x80000000 |
	     (bus<<16) |
	     (dev<<(8+3)) |
	     (fn<<8) |
	     (reg & 0xfc));

	return inw(0xcfc + (reg&2));
}
static inline uint8_t
conf8(int bus, int dev, int fn, int reg)
{
	outl(0xcf8,
	     0x80000000 |
	     (bus<<16) |
	     (dev<<(8+3)) |
	     (fn<<8) |
	     (reg & 0xfc));

	return inb(0xcfc + (reg&3));
}

int
pci_init(struct pci_root *pci, struct pci_init_error *error)
{
	uintptr_t bus, dev;
	unsigned int num_dev = 0, cur_dev;

	for (bus=0; bus<pci->nr_bus; bus++) {
		for (dev=0; dev<32; dev++) {
			uint32_t id, header;
			id = conf32(bus, dev, 0, 0);

			if (id == 0xffffffff || id == 0) {
				continue;
			}
			num_dev++;

			header = conf32(bus, dev, 0, 0x0e);
			if (header & 0x80) {
				uintptr_t fn;
				for (fn = 1; fn<8; fn++) {
					id = conf32(bus, dev, fn, 0);
					if (id == 0xffffffff || id == 0) {
						continue;
					}
					num_dev++;
				}
			}
		}
	}
	pci->devices = SBRK_TANATIVE(struct pci_device, num_dev);
	pci->num_dev = num_dev;
	cur_dev = 0;

	for (bus=0; bus<pci->nr_bus; bus++) {
		uintptr_t dev;
		for (dev=0; dev<32; dev++) {
			uint32_t id = conf32(bus, dev, 0, 0);
			uint32_t header;
			uint8_t bcc,scc,pi;

			if (id == 0xffffffff || id == 0) {
				continue;
			}

			bcc = conf8(bus, dev, 0, 0x0b);
			scc = conf8(bus, dev, 0, 0x0a);
			pi = conf8(bus, dev, 0, 0x09);
			/*
			for (int i=0; i<16; i++) {
				printf("%08x: ", i*16);
				for (int j=0; j<16; j++) {
					printf("%02x ", conf8(bus, dev, 0, j+i*16));
				}
				puts("");
			}
			printf("%04x:%04x %x %x %x\n", (int)bus, (int)dev, bcc, scc, pi);
			*/

			pci->devices[cur_dev].busdevfn = (bus<<20)|(dev<<15)|(0<<12);
			pci->devices[cur_dev].vendor_id = id&0xffff;
			pci->devices[cur_dev].device_id = id>>16U;
			pci->devices[cur_dev].bcc = bcc;
			pci->devices[cur_dev].scc = scc;
			pci->devices[cur_dev].pi = pi;

			cur_dev++;

			header = conf32(bus, dev, 0, 0x0e);
			if (header & 0x80) {
				uintptr_t fn;
				for (fn = 1; fn<8; fn++) {
					id = conf32(bus, dev, fn, 0);
					if (id == 0xffffffff || id == 0) {
						continue;
					}
					bcc = conf8(bus, dev, fn, 0x0b);
					scc = conf8(bus, dev, fn, 0x0a);
					pi = conf8(bus, dev, fn, 0x09);

					pci->devices[cur_dev].busdevfn = (bus<<20)|(dev<<15)|(fn<<12);
					pci->devices[cur_dev].vendor_id = id&0xffff;
					pci->devices[cur_dev].device_id = id>>16U;
					pci->devices[cur_dev].bcc = bcc;
					pci->devices[cur_dev].scc = scc;
					pci->devices[cur_dev].pi = pi;

					cur_dev++;
				}
			}
		}
	}

	return 0;
}
#endif

const struct pci_irq_table *
find_pci_irq_table(struct pci_root *root,
		   struct pci_device *dev)
{
	int bus = BDF_BUS(dev->busdevfn);
	int i, j, n, m;
	int d, f;

	d = BDF_DEV(dev->busdevfn);
	f = BDF_FN(dev->busdevfn);

	n = root->num_bridge;
	for (i=0; i<n; i++) {
		struct pci_bridge *b = &root->bridges[i];
		if (b->busno == bus) {
			m = b->num_irq_table;
			for (j=0; j<m; j++) {
				int tdev = (b->irq_table[j].addr>>16)&0xffff;
				int tfn = (b->irq_table[j].addr)&0xffff;

				if ((tdev == d) &&
				    ((tfn == 0xffff) ||
				     (tfn == f))) {
					return &b->irq_table[j];
				}
			}
		}
	}

	return NULL;
}

void
lspci(struct pci_root *pci)
{
	int i, n;
	struct pci_device *p = pci->devices;

	n = pci->num_dev;
	for (i=0; i<n; i++) {
		printf("%04x:%02x.%02x %04x:%04x\n",
		       (int)BDF_BUS(p[i].busdevfn),
		       (int)BDF_DEV(p[i].busdevfn),
		       (int)BDF_FN(p[i].busdevfn),
		       p[i].vendor_id, p[i].device_id);
	}
}



static void
lspci_tree0(struct pci_root *pci,
	    int off)
{
	int i, n;
	struct pci_bridge *b = &pci->bridges[pci->tree[off+PCITREE_OFFSET_BRIDGEID]];
	struct pci_device *node = &pci->devices[b->devid];
	(void)node;

	printf("%04lx:%02lx.%02lx %d\n",
	       BDF_BUS(node->busdevfn),
	       BDF_DEV(node->busdevfn),
	       BDF_FN(node->busdevfn),
	       b->busno);

	for (i=0; i<b->num_irq_table; i++) {
		printf("  %08x: %d %d %d %d\n",
		       b->irq_table[i].addr,
		       b->irq_table[i].irq[0],
		       b->irq_table[i].irq[1],
		       b->irq_table[i].irq[2],
		       b->irq_table[i].irq[3]);
	}

	n = pci->tree[off+PCITREE_OFFSET_NUM_CHILD];
	for (i=0; i<n; i++) {
		lspci_tree0(pci,
			    pci->tree[off + PCITREE_SIZEOF_NODE + i]);
	}
}


void
lspci_tree(struct pci_root *pci) {
	lspci_tree0(pci, 0);
}

void
pci_print_init_error(struct pci_init_error *error)
{
	switch (error->code) {
	case PCI_MCFG_NOT_FOUND:
		puts("mcfg not found");
		break;

	case PCI_TOO_MANY_MCFG:
		puts("too many mcfg");
		break;

	case PCI_MCFG_INVALID_BASE:
		puts("invalid base");
		break;
	}
}
