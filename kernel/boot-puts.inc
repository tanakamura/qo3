	; esi : str
	; eax, edx : clobbered
.puts_poll:
	mov	dx, PORT+REG_LSR
	in	al, dx
	test	al, LSR_THR
	jz	.puts_poll

	mov	al, [esi]
	add	esi, 1
	cmp	al, 0
	je	.exit_puts

	mov	dx, PORT + REG_THR
	out	dx, al

	jmp	puts

.exit_puts:
	ret
