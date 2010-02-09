MODNAME=QO3
OUT_EXE=QO3-64
EXTRA_TARGET=QO3
CFLAGS=-mssse3
NEED_MODULE="libc npr acpica net"
SOURCES="boot.s bios.s main.c ns16550.c serial-stdio.c vga-stdio.c intr.c wait.c
hpet.c smp.c code16.s acpi.c brk.c lapic.c ioapic.c pci.c gma.c event.c
acpi-osqo3.c hda.c irq.c uhci.c bench.c addr-table.s
"
GENERATED="apboot16.o e820-setup.o boot.o firstsegment-const.h"

tat=${mod_dir}/tmp-addr-table 
at=${mod_dir}/addr-table

POST_APPEND="
${mod_dir}/code16.o: ${mod_dir}/apboot16.o ${mod_dir}/e820-setup.o
${mod_dir}/boot.o: ${mod_dir}/serial32.inc ${mod_dir}/enable64.inc

${mod_dir}/apboot16.o: ${mod_dir}/apboot16.s
	nasm -f bin -o \$@ \$<

${mod_dir}/e820-setup.o: ${mod_dir}/e820-setup.s ${mod_dir}/firstsegment.inc
	nasm -f bin -o \$@ \$<

${mod_dir}/firstsegment-const.h: ${mod_dir}/firstsegment.inc
	sed s/%/#/g < \$< > \$@

${mod_dir}/addr-table.o: ${at}

${at}:
	touch \$@

${mod_dir}/QO3: ${mod_dir}/QO3-64
	\$(NM) \$< | perl ${mod_dir}/tools/addrtable.pl > ${tat}
	+if ! cmp ${tat} ${at}; then mv ${tat} ${at}; rm \$<; make \$<; fi
	\$(OBJCOPY) -S -O binary \$< \$@
"
