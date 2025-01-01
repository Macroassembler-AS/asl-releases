	cpu	kcpsm

	;ei
	;di
	;retie
	;retid

	CONSTANT const1, 02
	NAMEREG s08, treg


	load	treg, #const1
	
	nop
	
	load	s0, #1
	jump	test1
	jump	Z, test1
	jump	NZ, test1
	jump	C, test1
	jump	NC, test1

test1:
	call	test2
	call	Z, test2
	call	NZ, test2
	call	C, test2
	call	NC, test2

test2:
	return
	return	Z
	return	NZ
	return	C
	return	NC

	load	s0, #21
	and	s0, #03
	or	s0, #19
	xor	s0, #71

	load	s1, s0
	and	s1, s0
	or	s1, s0
	xor	s1, s0

	add	s0, #21
	addcy	s0, #03
	sub	s0, #19
	subcy	s0, #71

	add	s1, s0
	addcy	s1, s0
	sub	s1, s0
	subcy	s1, s0

	sr0	s0
	sr1	s0
	srx	s0
	sra	s0
	rr	s0

	sl0	s0
	sl1	s0
	slx	s0
	sla	s0
	rl	s0

	input	s0, #21
	input	s0, (s1)
	output	s0, #21
	output	s0, (s1)

	returni	enable
	returni	disable

	enable	interrupt
	disable	interrupt

myreg	equ	s12
myregr	reg	s12
myregre	reg	myreg

	add	s12,200
	add	myreg,200
	add	myregr,200
	add	myregre,200

	segment	data

	org	0aah
vari:

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	segment	code

	db	128,255,-128,1
	dw	32768,65535,-32768
	dd	100000000
