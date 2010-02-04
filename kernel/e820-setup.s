	; this program is loaded to 0:0(from boot.s)

	cpu	486
	section	.text

%include "kernel/firstsegment.inc"
%include "kernel/gdt.inc"

func_table:
	dw	setup_e820_32
	dw	gdtdesc
	dw	gdt
	dw	tss64
	dw	e820_table
	dw	e820_table_info
	dw	idt
	dw	idtdesc

	bits	32
	; esp is saved to sp_save
	; esp is set to stack_end
	; jmp to cx
switch_to_real:	
	mov	eax, 24
	mov	ds, eax
	mov	es, eax
	mov	fs, eax
	mov	gs, eax
	mov	ss, eax

	jmp	32:switch_to_real16

	bits	16
switch_to_real16:
	lidt	[idtdesc16]

	mov	eax, cr0
	and	eax, ~1
	mov	cr0, eax

	jmp	FIRSTSEGMENT_SEG:switch_to_real_real

switch_to_real_real:
	mov	[sp_save], esp
	mov	esp, stack_end
	jmp	cx

	bits	32
setup_e820_32:
	mov	cx, e820_setup
	jmp	switch_to_real

	bits	16
e820_setup:
	mov	ax, FIRSTSEGMENT_SEG

	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax

	xor	ax, ax
	mov	[e820_table_info + 2], ax

	mov	di, e820_table
	mov	cx, E820_TABLE_SIZE
	mov	[cur_dest_addr], di
	mov	[table_remain], cx
	xor	ebx, ebx

next_entry:
	movzx	ecx, word [table_remain]
	mov	di, [cur_dest_addr]
	mov	edx, 0x534d4150

	mov	ax, 20
	sub	[table_remain], ax
	add	[cur_dest_addr], ax

	mov	eax, 0xe820

	int	0x15

	; ecx : buffer size
	; ebx : continuation

	mov	word [e820_table_info], 0
	jc	nosmap

	cmp	eax, 0x534d4150
	jne	nosmap

	mov	word [e820_table_info], 1
	inc	word [e820_table_info+2]

	cmp	ebx, 0
	jnz	next_entry

nosmap:
switch_to_proto:
	lgdt	[gdtdesc]
	mov	esp, [sp_save]

	; switch to protect mode
	mov	eax, cr0,
	or	eax, 1
	mov	cr0, eax

	jmp	flush
flush:
	mov	ax, 8
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax

	jmp	dword 16:(return_to_esp32 + FIRSTSEGMENT_ADDR32)

	bits	32
return_to_esp32:
	ret

	section	.data
idtdesc16:
	dw	1024
	dd	0

	align	8
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
	dd	FIRSTSEGMENT_ADDR32 + gdt

	align	4
magic:
	dd	0xaa55aa55

	section	.bss
	
	align	2
e820_table:
	resb	E820_TABLE_SIZE

	; e820 table info structure
	; 0: 1=presented, 0=none (2byte)
	; 2: number of entry
e820_table_info:
	resb	4

tss64:
	resb	128

cur_dest_addr:
	resb	2
table_remain:
	resb	2
sp_save:
	resb	4

	alignb	16
stack:
	resb	2048
stack_end:

idtdesc:
	resb	8

	alignb	16
idt:
	