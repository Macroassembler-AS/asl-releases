	cpu	8051:bitsegsize=1

	segment	bitdata

	; dispose constants
	; In packed little endian representation,
	; this results in 0xad, 0x56, 0xab

	d1	1,0,1,3 dup(1,0,1,0,1,0,1)

	; reserve space

	d1	8 dup (?)

	; A nibble occupies four bits
	; Note the default endianess is little-endian,
	; though some regard the 8051 a big-endian processor,
	; due to the operands of LCALL and MOV DPTR,#...:
	; In packed little endian representation,
        ; this results in 0xa5, 0x21, 0x13, 0x32, 0x21, 0x*3
	; Since the number of nibbles is odd, the upper nibble
	; of the last byte is used by the next instruction:

	dn	5,10,3 dup(1,2,3)

	; A byte occupies eight bits. Since the previous instruction
	; did not use a multiple of eight bits, this data is split
	; over two bytes in the object file: 0xa*, 0x*a

	db	170		; hex AA -> 01010101 in little endian

	; Now things get tricky: the previous instruction used half a byte
	; of object code in little-endian.  Since little-endian, the bits
	; 0..3 are occupied, and bits 4..7 are free for the next
	; instruction.  However, since we switched to big endian, they
	; are filled with the upper of the value.  So, the result is
	; 0x6*, 0x9*

	bigendian on
	db	69h		; -> 01101001 in big endian

	; This fills the bits 0..3 in the partial byte from the previous
	; instruction.  Finally, we're on a byte boundary again:

	dn	7		; -> 0111 in big endian

	; D1 not allowed in other address spaces

	segment	code

	expect	1960
	d1	1,1,0,0,1,1,0,0
	endexpect

	segment	data

	expect	1960
	d1	1,1,0,0,1,1,0,0
	endexpect
	