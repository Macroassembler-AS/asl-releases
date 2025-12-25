	cpu	8051:bitsegsize=1

	segment	bitdata

	; dispose constants

	d1	1,0,1,3 dup(1,0,1,0,1,0,1)

	; reserve space

	d1	8 dup (?)

	; A nibble occupies four bits
	; Note the default endianess is little-endian,
	; though some regard the 8051 a big-endian processor,
	; due to the operands of LCALL and MOV DPTR,#...:

	dn	5,10		; 1 0 1 0, 0 1 0 1

	; A byte occupies eight bits

	db	170		; hex AA -> 01010101 in little endian
	bigendian on
	db	170		;    -> 10101010 in big endian

	; D1 not allowed in other address spaces

	segment	code

	expect	1960
	d1	1,1,0,0,1,1,0,0
	endexpect

	segment	data

	expect	1960
	d1	1,1,0,0,1,1,0,0
	endexpect
	