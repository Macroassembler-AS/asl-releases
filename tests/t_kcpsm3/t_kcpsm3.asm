	cpu	kcpsm3

	CONSTANT const1, 02
	NAMEREG s8, treg


	load	treg, const1
	
	nop
	
	load	s0, 1
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

	load	s3, 21
	and	sC, 03
	or	s4, 19
	xor	s7, 71

	load	s1, s5
	and	s1, s5
	or	s1, s5
	xor	s1, s5

	add	s9, 21
	addcy	s9, 03
	sub	s9, 19
	subcy	s9, 71

	add	s1, s4
	addcy	s1, s4
	sub	s1, s4
	subcy	s1, s4

	sr0	sC
	sr1	sC
	srx	sC
	sra	sC
	rr	sC

	sl0	sC
	sl1	sC
	slx	sC
	sla	sC
	rl	sC

	input	s6, 21
	input	sB, (s1)
	output	sE, 21
	output	sF, (s1)

	returni	enable
	returni	disable

	enable	interrupt
	disable	interrupt

        ; new KCPSM3 instructions

	compare	s1,s7
	compare	s6,-3

	fetch	s2,vari
	fetch	s9,(s4)

	store	s3,vari
	store	s6,(sE)

	test	s8,0aah
	test	sC,s3

; register aliases

myreg	equ	sc
myregr	reg	sc
myregre	reg	myreg

	add	sc,200
	add	myreg,200
	add	myregr,200
	add	myregre,200

	segment	data

	org	01ah
vari:

	segment	code

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	dn	8 dup(1,2,3)
	db	4 dup(1,2,3)
	dw	4 dup(1,2,3)
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
