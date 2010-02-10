	SECTION .text
	global	_start
	global	generic_int
	extern	cmain
	extern	cAP_main
	extern	ap_stack
	extern	e820_setup
	extern	e820_setup_end
	extern	_edata
	extern	_end

%include "kernel/firstsegment.inc"
%include "kernel/save-regs.inc"

	cpu	X64
	bits	32

%include	"kernel/gdt.inc"

%macro	firstsegment_addr 2
	movzx	%1, word [FIRSTSEGMENT_ADDR32 + %2*2]
	add	%1, FIRSTSEGMENT_ADDR32
%endmacro

_start:
addr100000:
	jmp	start2

	align 4

multiboot_header:
	dd  0x1badb002 ; magic
	dd  0x00010000 ; flags
	dd -0x1baeb002 ; checksum

	dd  multiboot_header ; header_addr
	dd  _start ; load_addr
	dd  _edata ; load_end_addr
	dd  _end ; bss_end_addr
	dd  start2 ; entry

%define STACK_SIZE 16384
%define NUM_MAX_CPU 16

start2:
	mov	esp, stack+STACK_SIZE
	push	dword 0
	popf

	mov	eax, cr4
	or	eax, (1<<9) ; enable SSE
	mov	cr4, eax

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

	mov	ecx, tss64_end-tss64
	mov	esi, tss64
	firstsegment_addr edi, TSS64
	call	memcpy4

	firstsegment_addr eax, INIT_TSS64
	call	eax

	mov	ecx, idt_end-idt
	mov	esi, idt
	firstsegment_addr edi, IDT
	mov	ebx, edi

	call	memcpy4

	firstsegment_addr eax, IDTDESC
	mov	word [eax], 8*256-1
	mov	dword [eax+2], ebx
	lidt	[eax]

	call	init_ns16550
	mov	esi, hello32
	call	puts

%include	"kernel/enable64.inc"

	jmp	40:start64

	bits	64
start64:
	mov	esi, hello64
	call	puts64

	mov	eax, 48
	ltr	ax

	sti

	jmp	cmain


	bits	32

clear_table:
	xor	ecx, ecx
.loop:
	mov	dword [8*ecx + esi], 0;
	add	ecx, 1
	cmp	ecx, 512
	jne	.loop

	ret

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

%include	"kernel/serial32.inc"

puts:
%include	"kernel/boot-puts.inc"

	bits	64
puts64:
%include	"kernel/boot-puts.inc"

%define	lo(x) (((x)-addr100000))
%define	hi(x) (0x10)

%define idt_entry(h) dw lo(h), 40, 0x8e01, hi(h), 0, 0, 0, 0

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

%define	SAVE_REGS_OFFSET 16*16 + 8*16
%macro	SAVE_REGS_NO_ERROR_CODE 0
	sub	rsp, SAVE_REGS_OFFSET+8
	call	save_regs
%endmacro

%macro	RESTORE_REGS_NO_ERROR_CODE 0
	call	restore_regs
	add	rsp, SAVE_REGS_OFFSET+8
%endmacro


%macro	SAVE_REGS_WITH_ERROR_CODE 0
	sub	rsp, SAVE_REGS_OFFSET
	call	save_regs
%endmacro

%macro	RESTORE_REGS_WITH_ERROR_CODE 0
	call	restore_regs
	add	rsp, SAVE_REGS_OFFSET+8
%endmacro




	;  +8 for align to 16
	;
	;  | xxxx    |
	;  +---------+
	;  | xxx     |
	;  | ret     | <- rsp (in save_regs)
	;  +---------+ <- rsp (in handler)
	;  |save area|
	;  |(384byte)|
	;  +---------+ <- alignd to 16
	;  |padding 8|
	;  +---------+
	;  |IA32e    |
	;  |interrupt|
	;  |stack    |
	;  |frame    |
	;  |(40byte) |
	;  +---------+ <- IST (aligned to 16)
	;  |         |

save_regs:
	mov	[rsp+8+0], rax
	mov	[rsp+8+8*1], rbx
	mov	[rsp+8+8*2], rcx
	mov	[rsp+8+8*3], rdx
	mov	[rsp+8+8*4], rsi
	mov	[rsp+8+8*5], rdi
	mov	[rsp+8+8*6], rbp
;	mov	[rsp+8+8*7], 
	mov	[rsp+8+8*8], r8
	mov	[rsp+8+8*9], r9
	mov	[rsp+8+8*10], r10
	mov	[rsp+8+8*11], r11
	mov	[rsp+8+8*12], r12
	mov	[rsp+8+8*13], r13
	mov	[rsp+8+8*14], r14
	mov	[rsp+8+8*15], r15
	movdqa	[rsp+8+128+16*0], xmm0
	movdqa	[rsp+8+128+16*1], xmm1
	movdqa	[rsp+8+128+16*2], xmm2
	movdqa	[rsp+8+128+16*3], xmm3
	movdqa	[rsp+8+128+16*4], xmm4
	movdqa	[rsp+8+128+16*5], xmm5
	movdqa	[rsp+8+128+16*6], xmm6
	movdqa	[rsp+8+128+16*7], xmm7
	movdqa	[rsp+8+128+16*8], xmm8
	movdqa	[rsp+8+128+16*9], xmm9
	movdqa	[rsp+8+128+16*10], xmm10
	movdqa	[rsp+8+128+16*11], xmm11
	movdqa	[rsp+8+128+16*12], xmm12
	movdqa	[rsp+8+128+16*13], xmm13
	movdqa	[rsp+8+128+16*14], xmm14
	movdqa	[rsp+8+128+16*15], xmm15
	ret

restore_regs:
	mov	rax, [rsp+8+0]
	mov	rbx, [rsp+8+8*1]
	mov	rcx, [rsp+8+8*2]
	mov	rdx, [rsp+8+8*3]
	mov	rsi, [rsp+8+8*4]
	mov	rdi, [rsp+8+8*5]
	mov	rbp, [rsp+8+8*6]
;	mov	, [rsp+8*7]
	mov	r8, [rsp+8+8*8]
	mov	r9, [rsp+8+8*9]
	mov	r10, [rsp+8+8*10]
	mov	r11, [rsp+8+8*11]
	mov	r12, [rsp+8+8*12]
	mov	r13, [rsp+8+8*13]
	mov	r14, [rsp+8+8*14]
	mov	r15, [rsp+8+8*15]
	movdqa	xmm0, [rsp+8+128+16*0]
	movdqa	xmm1, [rsp+8+128+16*1]
	movdqa	xmm2, [rsp+8+128+16*2]
	movdqa	xmm3, [rsp+8+128+16*3]
	movdqa	xmm4, [rsp+8+128+16*4]
	movdqa	xmm5, [rsp+8+128+16*5]
	movdqa	xmm6, [rsp+8+128+16*6]
	movdqa	xmm7, [rsp+8+128+16*7]
	movdqa	xmm8, [rsp+8+128+16*8]
	movdqa	xmm9, [rsp+8+128+16*9]
	movdqa	xmm10, [rsp+8+128+16*10]
	movdqa	xmm11, [rsp+8+128+16*11]
	movdqa	xmm12, [rsp+8+128+16*12]
	movdqa	xmm13, [rsp+8+128+16*13]
	movdqa	xmm14, [rsp+8+128+16*14]
	movdqa	xmm15, [rsp+8+128+16*15]
	ret


%macro gen_handler 2
%1:
	extern %2
	SAVE_REGS_NO_ERROR_CODE
	call	%2
	RESTORE_REGS_NO_ERROR_CODE
	iretd
%endmacro

%macro	gen_handler_pass_regs_error_code 2
%1:
	extern %2
	SAVE_REGS_WITH_ERROR_CODE
	mov	rax, [SAVE_REGS_OFFSET + 32 + rsp] ; RSP
	mov	[SAVE_REG_OFF_RSP + rsp], rax
	mov	rdi, [SAVE_REGS_OFFSET + 0 + rsp] ; error code
	mov	rsi, [SAVE_REGS_OFFSET + 8 + rsp] ; RIP
	mov	rdx, [SAVE_REGS_OFFSET + 16 + rsp] ; CS
	mov	rcx, rsp ; saved regs
	mov	r8, [SAVE_REGS_OFFSET + 24 + rsp] ; flags
	call	%2
	RESTORE_REGS_WITH_ERROR_CODE
	iretd
%endmacro

%macro gen_handler_code 3
%1:
	extern %2
	SAVE_REGS_WITH_ERROR_CODE
	mov	rdi, %3
	call	%2
	add	esp, 4
	RESTORE_REGS_WITH_ERROR_CODE
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
	gen_handler_pass_regs_error_code stack_segment_fault, cstack_segment_fault
	gen_handler_pass_regs_error_code general_protection, cgeneral_protection
	gen_handler_pass_regs_error_code page_fault, cpage_fault
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

hello32:
	db	`hello 32bit.\r\n\0`
hello64:
	db	`hello 64bit.\r\n\0`

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


	alignb	4096
pml4:	resb	4096
	alignb	4096
pdp:	resb	4096
	alignb	4096
pdir:	resb	4096
	alignb	4096
pte:	resb	(4096*4)

%define lo32(a) a
%define hi32(a) 0

	SECTION	.rodata

	align	16
tss64:
	dd	0		; reserved
	dd	lo32(int_stack)	; rsp0 (4)
	dd	hi32(int_stack)	; rsp0 (8)
	dd	lo32(int_stack)	; rsp1 (c)
	dd	hi32(int_stack)	; rsp1 (10)
	dd	lo32(int_stack)	; rsp2 (14)
	dd	hi32(int_stack)	; rsp2 (18)
	dd	0		; reserved (1c)
	dd	0		; reserved (20)
	dd	lo32(int_stack)	; ist1(24)
	dd	hi32(int_stack)	; ist1(28)
	dd	lo32(int_stack)	; ist2(2c)
	dd	hi32(int_stack)	; ist2(30)
	dd	lo32(int_stack)	; ist3(34)
	dd	hi32(int_stack)	; ist3(38)
	dd	lo32(int_stack)	; ist4(3c)
	dd	hi32(int_stack)	; ist4(40)
	dd	lo32(int_stack)	; ist5(44)
	dd	hi32(int_stack)	; ist5(48)
	dd	lo32(int_stack)	; ist6(4c)
	dd	hi32(int_stack)	; ist6(50)
	dd	lo32(int_stack)	; ist7(54)
	dd	hi32(int_stack)	; ist7(58)
	dd	0		; reserved(5c)
	dd	0		; reserved(60)
	dd	0		; reserved+iomap base(64)

	align	4
tss64_end: