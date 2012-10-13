	SECTION .rodata
	global	addr_symbol_table
addr_symbol_table:
	incbin	"kernel/addr-table"