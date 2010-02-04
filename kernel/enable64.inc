%define IA32_EFER 0xc0000080


init_pml4:
	; ** pml4 **
	mov	esi, pml4
	mov	cr3, esi
	call	clear_table
	; r = reserved
	; x = free

	; first pml4 entyry (B = pdp base)
	; 0xxx xxxx xxxx 0000 0000 0000 BBBB BBBB
	; BBBB BBBB BBBB BBBB BBBB xxxr rr00 0111
	mov	eax, pdp
	or	eax, 7
	mov	dword [esi], eax
	mov	dword [esi+4], 0

	
	; ** pdp **
	mov	esi, pdp
	call	clear_table

	; first pdp table entyry (B = pdir base)
	; P, W, U, WT
	; 0xxx xxxx xxxx 0000 0000 0000 BBBB BBBB
	; BBBB BBBB BBBB BBBB BBBB xxxr rr00 0111
	mov	eax, pdir
	or	eax, 7
	mov	dword [esi], eax
	mov	dword [esi+4], 0

	; ** pdir **
	mov	esi, pdir
	call	clear_table

	; setup straight mapping(0x000000-0x3fffff)
	; use 4MB for kernel text, bss, data(&stack), rodata

	; first pde (B = 0x000000-0x1fffff)
	; P, W, U, WT, PS
	; 0xxx xxxx xxxx 0000 0000 0000 BBBB BBBB
	; BBBB BBBB BBBr rrrr rrr0 xxx0 1000 0111
	mov	dword [esi], 0x0 | 0x87
	mov	dword [esi+4], 0

	; first pde (B = 0x200000-0x3fffff)
	; P, W, U, WT, PS
	; 0xxx xxxx xxxx 0000 0000 0000 BBBB BBBB
	; BBBB BBBB BBBB BBBB BBBB 1000 0000 1111
	mov	dword [esi+8], 0x200000 | 0x87
	mov	dword [esi+12], 0


	; enable PAE
	mov	eax, cr4
	or	eax, (1<<5)
	mov	cr4, eax

	; set LME
	mov	ecx, IA32_EFER
	rdmsr
	or	eax, (1<<8)
	wrmsr
	
	; enable PG! Go IA32e mode!
	mov	eax, cr0
	or	eax, (1<<31)
	mov	cr0, eax
