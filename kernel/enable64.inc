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

	mov	dword [esi], eax ; 0-1GB
	mov	dword [esi+4], 0

	add	eax, 4096
	mov	dword [esi+8], eax ; 1-2GB
	mov	dword [esi+12], 0
	add	eax, 4096
	mov	dword [esi+16], eax ; 2-3GB
	mov	dword [esi+20], 0
	add	eax, 4096
	mov	dword [esi+24], eax ; 3-4GB
	mov	dword [esi+28], 0

	; ** pdir **
	mov	esi, pdir

	; number of startup page is
	; ALIGN_UP(offset to _end, 2M) / 2M
	; 2M = shift 21

	; ALIGN UP
	mov	ecx, _end
	add	ecx, (2 * 1024 * 1024) - 1
	and	ecx, ~((2 * 1024 * 1024) - 1)

	; ecx = ecx / 2M
	shr	ecx, 21

	; ecx = number of page
	mov	eax, 0x87

	; setup direct mapping(0-4GB)
	; P, W, U, WT, PS
	; 0xxx xxxx xxxx 0000 0000 0000 BBBB BBBB
	; BBBB BBBB BBBr rrrr rrr0 xxx0 1000 0111
.setup_pte:
	mov	dword [esi], eax
	mov	dword [esi+4], 0
	add	esi, 8
	add	eax, 0x200000
	sub	ecx, 1
	jnz	.setup_pte

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
