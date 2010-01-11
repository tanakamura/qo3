MODNAME=QO3
OUT_EXE=QO3
CFLAGS=-mssse3
NEED_MODULE="libc npr acpica net"
SOURCES="boot.s bios.s main.c ns16550.c serial-stdio.c vga-stdio.c intr.c wait.c
hpet.c smp.c apboot.s acpi.c brk.c lapic.c ioapic.c pci.c gma.c event.c
acpi-osqo3.c hda.c irq.c uhci.c bench.c
"
GENERATED="apboot16.o"

POST_APPEND='
kernel/apboot.o: kernel/apboot16.o
kernel/apboot16.o: kernel/apboot16.s
	nasm -f bin -o $@ $<
'