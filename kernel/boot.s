	SECTION .text
	global	_start
	global	generic_int
	extern	cmain
	extern	cAP_main
	extern	cinterrupt_main
	extern	ap_stack
	extern	e820_setup
	extern	e820_setup_end

%include "kernel/firstsegment.inc"

	cpu	P4
	bits	32

%include	"kernel/gdt.inc"

_start:
addr100000:
	jmp	start2

	align 4

	dd 0x1badb002 ; magic
	dd 0	      ; flags
	dd -0x1badb002 ; checksum

%define STACK_SIZE 8192
%define NUM_MAX_CPU 16

start2:
	mov	esp, stack+STACK_SIZE
	push	dword 0
	popf

setup_segment16:
	; copy program to SETUP_START
	mov	ecx, e820_setup_end
	sub	ecx, e820_setup ; size of program
	mov	esi, e820_setup
	mov	edi, FIRSTSEGMENT_ADDR32

	; esi:src = e820_setup (code16.s)
	; edi:dst = FIRSTSEGMENT
	; ecx:size = (e820_setup_end - e820_setup)
	call	memcpy4

	movzx	eax, word [FIRSTSEGMENT_ADDR32 + GDTDESC*2]
	lgdt	[eax + FIRSTSEGMENT_ADDR32]

	mov	eax, 8

	mov	ds, eax
	mov	es, eax
	mov	fs, eax
	mov	gs, eax
	mov	ss, eax

	jmp	16:start3
start3:

	call	run_setup_e820
	xor	eax, eax

	out	0x20, al
	out	0xA0, al

	; disable i8259A
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

	; esi:src = idt
	; edi:dst = FIRSTSEGMENT:idt
	; ecx:size = sizeof idt
	mov	ecx, idt_end-idt
	mov	esi, idt
	movzx	edi, word [FIRSTSEGMENT_ADDR32 + IDT*2]
	add	edi, FIRSTSEGMENT_ADDR32
	mov	ebx, edi

	call	memcpy4

	movzx	eax, word [FIRSTSEGMENT_ADDR32 + IDTDESC*2]
	add	eax, FIRSTSEGMENT_ADDR32
	mov	word [eax], 8*256-1
	mov	dword [eax+2], ebx
	lidt	[eax]
	sti

	jmp	cmain

	; AP routine
	align	0x100
ap_start32:
	push	dword 0
	popf

	mov	eax, dword [0xfee00000 + 0x0020] ; APIC_ID
	shr	eax, 24
	cmp	eax, NUM_MAX_CPU
	jg	too_many_cpu

	imul	eax, STACK_SIZE
	add	eax, ap_stack
	mov	esp, eax

	movzx	eax, word [FIRSTSEGMENT_ADDR32 + GDTDESC*2]
	lgdt	[eax + FIRSTSEGMENT_ADDR32]
	mov	eax, 8

	mov	ds, eax
	mov	es, eax
	mov	fs, eax
	mov	gs, eax
	mov	ss, eax

	jmp	16:ap_start2
ap_start2:
	movzx	eax, word [FIRSTSEGMENT_ADDR32 + IDTDESC*2]
	add	eax, FIRSTSEGMENT_ADDR32
	lidt	[eax]
	sti
	call	cAP_main


run_setup_e820:
	movzx	ecx, word [FIRSTSEGMENT_ADDR32 + E820_SETUP]
	add	ecx, FIRSTSEGMENT_ADDR32
	jmp	ecx

memcpy4:
	; clobbered eax, edx
	; src esi
	; dest edi
	; num ecx
	mov	edx, 0
memcpy4_loop:	
	mov	eax, dword [esi + edx]
	mov	dword [edi + edx], eax
	add	edx, 4
	cmp	edx, ecx
	jnz	memcpy4_loop

	ret

too_many_cpu:
	mov	eax, 1
	mov	[have_too_many_cpus], eax
	sfence
too_many_cpu_hlt:
	hlt
	jmp	too_many_cpu_hlt

%define	lo(x) (((x)-addr100000))
%define	hi(x) (0x10)

%define idt_entry(h) dw lo(h), 16, 0x8e00, hi(h)

	align	16
idt:
	idt_entry(div_error) ; 0
	idt_entry(unknown_exception1) ; 1 reserved
	idt_entry(nmi) ; 2
	idt_entry(breakpoint) ; 3
	idt_entry(overflow) ; 4 into instr
	idt_entry(bound) ; 5 bound instr
	idt_entry(invalid_opcode) ; 6
	idt_entry(unknown_exception7) ; 7 device not available
	idt_entry(unknown_exception8) ; 8 double fault
	idt_entry(unknown_exception9) ; 9 coprocessor segment overrun(?)
	idt_entry(unknown_exception10) ; 10 invalid tss
	idt_entry(segment_not_present) ; 11 
	idt_entry(stack_segment_fault) ; 12
	idt_entry(general_protection) ; 13
	idt_entry(page_fault) ; 14
	idt_entry(unknown_exception15) ; 15 reserved
	idt_entry(fp_error) ; 16
	idt_entry(alignment_check) ; 17
	idt_entry(machine_check) ; 18
	idt_entry(simd_float) ; 19

%assign	vec 20
%rep (44)
	idt_entry(unknown_exception %+ vec) ; 20 - 63 (irq?)
%assign vec vec+1
%endrep

	idt_entry(lapic_timer) ; 64 lapic timer
	idt_entry(lapic_error) ; 65 lapic error
	idt_entry(hpet0_intr)  ; 66 hpet comparator0
	idt_entry(ns16550_intr)	; 67 com0
	idt_entry(acpi_intr)	; 68 acpi

%assign vec 16
%rep (8)
	idt_entry(pci_irq %+ vec) ; 69-76 pci irq
%assign vec vec+1
%endrep

%macro gen_handler 2
%1:
	extern %2
	pushad
	fxsave	[xsave_buffer]
	call	%2
	popad
	fxrstor	[xsave_buffer]
	iretd
%endmacro

%macro gen_handler_code 3
%1:
	extern %2
	pushad
	fxsave	[xsave_buffer]
	push	%3
	call	%2
	add	esp, 4
	popad
	fxrstor	[xsave_buffer]
	iretd
%endmacro


	gen_handler div_error, cdiv_error
	gen_handler invalid_opcode, cinvalid_opcode
	gen_handler lapic_timer, clapic_timer
	gen_handler lapic_error, clapic_error
	gen_handler hpet0_intr, chpet0_intr
	gen_handler ns16550_intr, cns16550_intr
	gen_handler acpi_intr, cacpi_intr

	gen_handler_code unknown_exception1, cunknown_exception, 1
	gen_handler_code unknown_exception7, cunknown_exception, 7
	gen_handler_code unknown_exception8, cunknown_exception, 8
	gen_handler_code unknown_exception9, cunknown_exception, 9
	gen_handler_code unknown_exception10, cunknown_exception, 10
	gen_handler_code unknown_exception15, cunknown_exception, 15

%assign	vec 20
%rep (44)
	gen_handler_code unknown_exception %+ vec, cunknown_exception, vec
%assign vec vec+1
%endrep

	gen_handler nmi, cnmi
	gen_handler overflow, coverflow
	gen_handler bound, cbound
	gen_handler breakpoint, cbreakpoint
	gen_handler segment_not_present, csegment_not_present
	gen_handler stack_segment_fault, cstack_segment_fault
	gen_handler page_fault, cpage_fault
	gen_handler fp_error, cfp_error
	gen_handler alignment_check, calignment_check
	gen_handler machine_check, cmachine_check
	gen_handler simd_float, csimd_float

%assign	vec 16
%rep (8)
	gen_handler_code pci_irq %+ vec, pci_irq, vec
%assign vec vec+1
%endrep

	align	16
idt_end:
	



general_protection:
	extern	cgeneral_protection
	pushad
	fxsave	[xsave_buffer]
	call	cgeneral_protection
	popad
	fxrstor	[xsave_buffer]
	add	esp, 4 ; errcode
	iretd



gdt:
	; limit 16
	; base 16
	;
	; base 8(0-7)
	; type 4(8-11)
	; 1    1(12)
	; dpl  2(13-14)
	; p    1(15)
	;
	; limit 4(16-19)
	; avl   1(20)
	; l     1(21)
	; d     1(22)
	; g     1(23)
	; base  8(24-31)

	dw	0, 0
	dd	0

	; read write (8)
	dw	0xffff, 0
	dd	(0xf<<SEGDESC_LIMIT_SHIFT) | (SEGDESC_RW32) | (0<<SEGDESC_DPL_SHIFT)

	; exec read (16)
	dw	0xffff, 0
	dd	(0xf<<SEGDESC_LIMIT_SHIFT) | (SEGDESC_EXEC) | (0<<SEGDESC_DPL_SHIFT)

	; 24 : real data
	dw	0xffff, FIRSTSEGMENT_ADDR32
	dd	(0xf<<SEGDESC_LIMIT_SHIFT) | (SEGDESC_RW16) | (0<<SEGDESC_DPL_SHIFT)

	; 32 : real code
	dw	0xffff, FIRSTSEGMENT_ADDR32
	dd	(0xf<<SEGDESC_LIMIT_SHIFT) | (SEGDESC_EXEC16) | (0<<SEGDESC_DPL_SHIFT)

%define	BASE_0_16(a) ((a)&0xffff)
%define	BASE_16_23(a) (((a)>>23)&0xff)
%define	BASE_24_31(a) (((a)>>24)&0xff)
%define	BASE_32_64(a) (((a)>>32)&0xffffffff)

	; 40,48 : tss64
	times 8	db 0

gdtdesc:
	dw	(8*(NUM_GDT_ENTRY)-1)
	dd	gdt


	SECTION .bss

	align	16
xsave_buffer:
	resb	512

stack:
	resb	STACK_SIZE

int_stack:
	resb	STACK_SIZE


	global	have_too_many_cpus
have_too_many_cpus:
	resb	4

	SECTION	.rodata
	align	16


	align	8
%if 0
tss64:
	dd	0		; reserved
	dq	int_stack	; rsp0 (4)
	dq	int_stack	; rsp1 (c)
	dq	int_stack	; rsp2 (14)
	dd	0		; reserved (1c)
	dd	0		; reserved (20)
	dq	int_stack	; ist1(24)
	dq	0		; ist2(2c)
	dq	0		; ist3(34)
	dq	0		; ist4(3c)
	dq	0		; ist5(44)
	dq	0		; ist6(4c)
	dq	0		; ist7(54)
	dd	0		; reserved(5c)
	dd	0		; reserved(60)
	dd	0		; reserved+iomap base(64)
%endif