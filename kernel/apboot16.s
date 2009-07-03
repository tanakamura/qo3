	; this object is located at APBOOT_ADDR
	; APBOOT_ADDR is 0x4000:0000 = 0x40000
	;
	cpu	486
	bits	16

%include	"kernel/gdt.inc"

apboot_main16:
	mov	ax, 0x4000
	mov	ds, ax
	mov	dword [flag], 0xdeadbeef

	mov	eax, cr0
	or	eax, 1 ; PE
	mov	cr0, eax

	; maybe.. not required.. this program can't run on 486
	jmp	flush
flush:	

	lgdt	[gdtdesc]

	mov	ax, 8
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax

	jmp	dword 16:(start3+0x40000)

	bits	32
start3:
	mov	eax, 0x100000 + 0x100
	jmp	eax ; ap_start32

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
	dd	(gdt+0x40000)

	align	100h
start_addr:
	dd	0
flag:
	dd	0xff
