	cpu	atmega8:codesegsize=0

	; Do not assume padding to be on by default
	; for anything but 68K!

	padding	on

	nop
	nop
	nop
	data	1,2,3
	data	"blablabla"

	lds	r25, 0x0060
	andi	r25, 0x7F ; 127
	lds	r24, 0x0061
	sbrc	r24, 5
	ori	r25, 0x80 ; 128
	sts	0x0060, r25
	ret

	jmp	next
	nop
	nop
next	nop

	rjmp	next2
	nop
	nop
next2	nop
