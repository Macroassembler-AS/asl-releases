	cpu	z80

var	set	30
	ld	a,(var)
	pushv	mystack,var

var	set	40
	ld	a,(var)

	popv	MyStack,var	; var is now 30 again
	ld	a,(var)
