#include <stdint.h>
#include <stdio.h>
#include "acpi.h"
#include "kernel/pci.h"
#include "kernel/brk.h"
#include "kernel/qo3-acpi.h"
#include "kernel/wait.h"
#include "kernel/intrinsics.h"
#include "kernel/mmio.h"
#include "kernel/ioapic.h"
#include "kernel/int-assign.h"
#include "kernel/lapic.h"

#define unimplemented(s) puts("acpi unimplemented: " s)

struct acpi_handler {
	ACPI_OSD_HANDLER handler;
	void *ctxt;
};

static struct acpi_handler handler_data;


void
AcpiOsFree(void *mem)
{
	//unimplemented("free");
}

void *
AcpiOsAllocate(ACPI_SIZE size)
{
	return sbrk_align_shift(size, 4);
}


ACPI_STATUS
AcpiOsInitialize(void)
{
	return AE_OK;
}

ACPI_STATUS
AcpiOsTerminate(void)
{
	return AE_OK;
}
    

void
AcpiOsPrintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
AcpiOsVprintf(const char *fmt, va_list args)
{
	vprintf(fmt, args);
}

ACPI_STATUS
AcpiOsCreateLock(ACPI_HANDLE *o)
{
	return AE_OK;
}

void
AcpiOsDeleteLock(ACPI_HANDLE h)
{
    
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock(ACPI_HANDLE h)
{
	return 0;
}

void
AcpiOsReleaseLock(ACPI_HANDLE h, ACPI_CPU_FLAGS f)
{
    
}

ACPI_STATUS
AcpiOsCreateSemaphore(UINT32 max, UINT32 init, ACPI_HANDLE *r)
{
	return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore(ACPI_HANDLE h)
{
	return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore(ACPI_HANDLE h, UINT32 u, UINT16 to)
{
	return AE_OK;
}

ACPI_THREAD_ID
AcpiOsGetThreadId(void) {return 1;}

ACPI_STATUS
AcpiOsSignalSemaphore(ACPI_SEMAPHORE h, UINT32 u)
{
	return AE_OK;
}

ACPI_STATUS
AcpiOsExecute(
	ACPI_EXECUTE_TYPE t,
	ACPI_OSD_EXEC_CALLBACK cb,
	void *ctxt)
{
	unimplemented("execute");
	return AE_OK;
}

ACPI_STATUS
AcpiOsInstallInterruptHandler(
	UINT32 intn,
	ACPI_OSD_HANDLER h,
	void *ctxt)
{
	handler_data.handler = h;
	handler_data.ctxt = ctxt;

	ioapic_set_redirect32(intn,
			      IOAPIC_DESTINATION_ID32(0),
			      IOAPIC_DELIVERY_FIXED|IOAPIC_VECTOR(ACPI_VEC)|
			      IOAPIC_LEVEL_TRIGGER);

	printf("install interrupt: %d\n", intn);
	return AE_OK;
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler(
	UINT32 intn,
	ACPI_OSD_HANDLER h)
{
	unimplemented("remove int");
	return AE_OK;
}

void *
AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS paddr,
		ACPI_SIZE l)
{
	puts("map");
	return (void*)paddr;
}

void
AcpiOsUnmapMemory(void *la, ACPI_SIZE l)
{
    
}

void
AcpiOsDerivePciId(
	ACPI_HANDLE rhandle,
	ACPI_HANDLE chandle,
	ACPI_PCI_ID **pci_id)
{
	unimplemented("deriv pciid");
}


ACPI_STATUS
AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *init,
			 ACPI_STRING *new_val)
{
	unimplemented("override");
	*new_val = NULL;
	return AE_OK;
}

ACPI_STATUS
AcpiOsReadPciConfiguration(ACPI_PCI_ID *id,
			   UINT32 reg,
			   void *val,
			   UINT32 wid)
{
	unsigned int b = id->Bus;
	unsigned int d = id->Device;
	unsigned int f = id->Function;
	switch (wid) {
	case 32:
		*(UINT32*)val = pci_conf_read32_bdf(&pci_root0, b,d,f,reg);
		break;

	case 16:
		*(UINT16*)val = pci_conf_read16_bdf(&pci_root0, b,d,f,reg);
		break;

	case 8:
		*(UINT8*)val = pci_conf_read8_bdf(&pci_root0, b,d,f,reg);
		break;

	case 64:
		unimplemented("64 read pci config");
		break;
	}
	printf("read config %x = %x\n", reg, *(UINT32*)val);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWritePciConfiguration(ACPI_PCI_ID *id,
			    UINT32 reg,
			    ACPI_INTEGER val,
			    UINT32 wid)
{
	unimplemented("write pci config");
	return AE_OK;
}

void
AcpiOsStall(UINT32 usec)
{
	//printf("stall: %d\n", usec);
	wait_usec(usec);
}

void
AcpiOsSleep(ACPI_INTEGER msec)
{
	unimplemented("sleep");
}

ACPI_STATUS
AcpiOsSignal(UINT32 func, void *i)
{
	unimplemented("signal");
	return AE_OK;
}

ACPI_STATUS
AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS a,
		 UINT32 *val,
		 UINT32 wid)
{
	//printf("read memory: %x\n", (int)a);
	switch (wid) {
	case 32:
		*val = mmio_read32(a);
		break;


	case 16:
		*val = mmio_read16(a);
		break;


	case 8:
		*val = mmio_read8(a);
		break;

	default:
		break;
	}

	printf("read memory %lx = %x\n", (long)a, *val);

	return AE_OK;
}

ACPI_STATUS
AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS a,
		  UINT32 val,
		  UINT32 wid)
{
	printf("write memory: %x\n", (int)a);
	switch (wid) {
	case 32:
		mmio_write32(a, val);
		break;


	case 16:
		mmio_write16(a, val);
		break;


	case 8:
		mmio_write8(a, val);
		break;

	default:
		break;
	}

	return AE_OK;
}

ACPI_STATUS
AcpiOsReadPort(ACPI_IO_ADDRESS addr,
	       UINT32 *val,
	       UINT32 wid)
{
	//printf("read port:%x\n", addr);
	switch (wid) {
	case 32:
		*val = inl(addr);
		break;
	case 16:
		*val = inw(addr);
		break;

	case 8:
		*val = inb(addr);
		break;
	}
	return AE_OK;
}

ACPI_STATUS
AcpiOsWritePort(ACPI_IO_ADDRESS addr,
		UINT32 val,
		UINT32 wid)
{
	//printf("write port:%x\n", addr);
	switch (wid) {
	case 32:
		outl(addr, val);
		break;
	case 16:
		outw(addr, val);
		break;
	case 8:
		outb(addr, val);
		break;
	}
	return AE_OK;
}

ACPI_STATUS
AcpiOsTableOverride(ACPI_TABLE_HEADER *exist,
		    ACPI_TABLE_HEADER **n)
{
	*n = NULL;
	return AE_OK;
}

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer(void)
{
	return acpi_table.rsdp;
}

ACPI_STATUS
AcpiOsValidateInterface(char *iface)
{
	unimplemented("validate interface");
	return AE_OK;
}

UINT64
AcpiOsGetTimer(void)
{
	unimplemented("timer");
	return 0;
}


void
cacpi_intr(void)
{
	puts("acpi");
	write_local_apic(LAPIC_EOI, 0);
}
