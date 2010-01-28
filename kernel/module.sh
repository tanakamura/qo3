MODNAME=QO3
OUT_EXE=QO3
CFLAGS=-mssse3
NEED_MODULE="libc npr acpica net"
SOURCES="boot.s bios.s main.c ns16550.c serial-stdio.c vga-stdio.c intr.c wait.c
hpet.c smp.c code16.s acpi.c brk.c lapic.c ioapic.c pci.c gma.c event.c
acpi-osqo3.c hda.c irq.c uhci.c bench.c
"
GENERATED="apboot16.o"

POST_APPEND='
kernel/code16.o: kernel/apboot16.o kernel/e820-setup.o

kernel/apboot16.o: kernel/apboot16.s
	nasm -f bin -o $@ $<

kernel/e820-setup.o: kernel/e820-setup.s kernel/firstsegment.inc
	nasm -f bin -o $@ $<

kernel/firstsegment-const.h: kernel/firstsegment.inc
	sed s/%/#/g < $< > $@
'
