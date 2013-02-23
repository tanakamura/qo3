%define IA32_EFER 0xc0000080

	; 64GB direct map をつくる
	;
	; 用意するもの
	;   pml4 entry 1個
	;   pdp  entry 64個
	;   pd   entry 512 * 64

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

	; num pdp entry = 64
	; esi = pdp
	mov	ecx, 64
	mov	eax, pdir
	or	eax, 7


	; first pdp table entyry (B = pdir base)
	; P, W, U, WT
	; 0xxx xxxx xxxx 0000 0000 0000 BBBB BBBB
	; BBBB BBBB BBBB BBBB BBBB xxxr rr00 0111

pdp_entry:
	mov	dword [esi], eax
	mov	dword [esi+4], 0
	add	eax, 4096
	add	esi, 8
	dec	ecx
	jne	pdp_entry

	mov	esi, pdir
	; num pd entry = 64 * 512
	mov	ecx, 64*512
	xor	edx, edx
	mov	eax, 7 | (1<<7) ; 1<<7 = 2MB

	; setup direct mapping(0-64GB, 2MB page)
	; 0xxx xxxx xxxx 0000 0000 0000 BBBB BBBB
	; BBBB BBBB BBBr rrrr rrr0 xxx0 1000 0111

pdir_entry:
	mov	dword [esi], eax
	mov	dword [esi+4], edx

	add	eax, (2*1024*1024)
	adc	edx, 0
	add	esi, 8

	dec	ecx
	jne	pdir_entry

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
