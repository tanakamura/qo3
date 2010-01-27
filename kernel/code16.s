	section	.data
	global	apboot_main16
	global	apboot_main16_end
apboot_main16:
	incbin	"kernel/apboot16.o"
apboot_main16_end:

	global	e820_setup
	global	e820_setup_end
e820_setup:
	incbin	"kernel/e820-setup.o"
	align	4
e820_setup_end:	
