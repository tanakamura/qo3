	; this object is located at APBOOT_ADDR
	; APBOOT_ADDR is 0xbd00:0000 = 0xbd0000
	;
	cpu	486
	bits	16

%include	"kernel/gdt.inc"

apboot_main16:
	mov	dword [flag], 0xdeadbeef
	jmp	apboot_main16

	lgdt	[gdtdesc]

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

	align	100h
flag:
	db	0xff