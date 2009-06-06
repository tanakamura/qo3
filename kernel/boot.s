	SECTION .text
	global	_start
	global	generic_int
	extern	cmain
	extern	cinterrupt_main

	cpu	486

%define SEGDESC_S	(1<<12)
%define SEGDESC_P	(1<<15)
%define SEGDESC_AVL	(1<<20)
%define SEGDESC_DB	(1<<22)
%define SEGDESC_G	(1<<23)

%define SEGDESC_EXECREAD (0xa<<8)
%define SEGDESC_READWRITE (0x2<<8)

%define SEGDESC_DPL_SHIFT 13
%define SEGDESC_LIMIT_SHIFT 16
%define SEGDESC_BASE_SHIFTL 0
%define SEGDESC_BASE_SHIFTH 24

_start:
addr100000:	
	jmp	start2

	align 4

	dd 0x1badb002 ; magic
	dd 0	      ; flags
	dd -0x1badb002 ; checksum


start2:
	mov	esp, stack+4096
	mov	eax, 0xb80000

	push	dword 0
	popf

	lgdt	[gdtdesc]
	mov	eax, 8

	mov	ds, eax
	mov	es, eax
	mov	fs, eax
	mov	gs, eax
	mov	ss, eax

	jmp	16:start3
start3:
	mov	eax, 0x11

	out	0x20, al
	out	0xA0, al

	mov	eax, 0x20
	out	0x21, al
	mov	eax, 1<<2
	out	0x21, al
	mov	eax, 0x01
	out	0x21, al

	mov	eax, 0x28
	out	0xA1, al
	mov	eax, 2
	out	0xA1, al
	mov	eax, 0x01
	out	0xA1, al

	mov	eax, 0xff
	out	0x21, al
	out	0xa1, al

	lidt	[idtdesc]

	sti

	call	cmain


%define	lo(x) (((x)-addr100000))
%define	hi(x) (0x10)

%define idt_entry(h) dw lo(h), 16, 0x8e00, hi(h)

	align	16
idt:
	idt_entry(div_error) ; 0
	idt_entry(unknown_exception) ; 1 reserved
	idt_entry(nmi) ; 2
	idt_entry(breakpoint) ; 3
	idt_entry(overflow) ; 4 into instr
	idt_entry(bound) ; 5 bound instr
	idt_entry(invalid_opcode) ; 6
	idt_entry(unknown_exception) ; 7 device not available
	idt_entry(unknown_exception) ; 8 double fault
	idt_entry(unknown_exception) ; 9 coprocessor segment overrun(?)
	idt_entry(unknown_exception) ; 10 invalid tss
	idt_entry(segment_not_present) ; 11 
	idt_entry(stack_segment_fault) ; 12
	idt_entry(general_protection) ; 13
	idt_entry(page_fault) ; 14
	idt_entry(unknown_exception) ; 15 reserved
	idt_entry(fp_error) ; 16
	idt_entry(alignment_check) ; 17
	idt_entry(machine_check) ; 18
	idt_entry(simd_float) ; 19

%rep (12)
	idt_entry(unknown_exception) ; 20 - 31 reserved
%endrep

%rep (32)
	idt_entry(unknown_exception) ; 32 - 64(??) irq
%endrep

	idt_entry(generic_int) ; 32 user defined

%macro gen_handler 2


%1:
	extern %2
	pushad
	call	%2
	popad
	iret
%endmacro

	gen_handler div_error, cdiv_error
	gen_handler unknown_exception, cunknown_exception
	gen_handler invalid_opcode, cinvalid_opcode


nmi:
overflow:
bound:
breakpoint:
segment_not_present:
stack_segment_fault:
general_protection:
page_fault:
fp_error:
alignment_check:
machine_check:
simd_float:
generic_int:
	jmp	generic_int

	SECTION .bss
stack:	resb	4096

%define	SEGDESC_RW32 (SEGDESC_P|SEGDESC_DB|SEGDESC_G|SEGDESC_S|SEGDESC_READWRITE)
%define	SEGDESC_EXEC (SEGDESC_P|SEGDESC_DB|SEGDESC_G|SEGDESC_S|SEGDESC_EXECREAD)
	

	SECTION	.rodata
	align	16
gdt:
	dw	0, 0
	dd	0

	; read write (8)
	dw	0xffff, 0
	dd	(0xf<<SEGDESC_LIMIT_SHIFT) | (SEGDESC_RW32) | (0<<SEGDESC_DPL_SHIFT)

	; exec read (16)
	dw	0xffff, 0
	dd	(0xf<<SEGDESC_LIMIT_SHIFT) | (SEGDESC_EXEC) | (0<<SEGDESC_DPL_SHIFT)

gdtdesc:
	dw	8*3-1
	dd	gdt

idtdesc:
	dw	8*33-1
	dd	idt

