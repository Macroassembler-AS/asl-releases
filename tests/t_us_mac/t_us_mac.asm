	cpu	z80

a1	equ	0a1h
a2	equ	0a2h
no_arg_1 equ	10

mymac	macro	arg_1,arg_2
	db	arg_1
	db	arg_2
	db	no_arg_1
	endm

	mymac	a1,a2

	irp	arg,a1,a2
	db	arg
	db	no_arg_1
	endm
