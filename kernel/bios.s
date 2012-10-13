	SECTION .text
	global	bios_system_reset
	bits	64

bios_system_reset: ; fixme in 64
	jmp	bios_system_reset

	lgdt	[gdtdesc]
	mov	ax, 0x10
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax

	;jmp	0x08:do_reset

do_reset:
	use16

	mov	eax, cr0
	and	eax, ~1
	mov	cr0, eax

	jmp	do_reset2
do_reset2:
	mov	di, 0x0472
	mov	[di], ax
	jmp	0xffff:0x0

gdt:
	dw	0, 0
	db	0, 0, 0, 0

	; 0x08 : real code
	dw	0xffff, 0
	db	0x0, 0x9a, 0x0, 0x0

	; 0x10 : real data
	dw	0xffff, 0
	db	0x0, 0x92, 0x0, 0x0

gdtdesc:
	dw	0x27
	dd	gdt