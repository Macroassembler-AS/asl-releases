	cpu	z86c03

myreg5	reg	r5

	ccf
	rcf
	scf

	di
	ei

	halt
	stop

	wdh
	wdt

	iret
	ret

	nop

	and	r7,r13
	and	myreg5,vari
	and	vari,r4

	and	r3,@r14
	and	r2,@vari
	and	vari,@r6

	and	r3,#5
	and	vari,#77
	and	@r9,#35h
	and	@vari,#10011b

	add	>2,>5		; disallow register addressing
	adc	myreg5,#4
	sub	@r0,#20
	sbc	r7,vari
	or	vari,@myreg5
	tcm	r0,@r8
	tm	@vari,#00001000b
	cp	vari,#20
	xor	myreg5,#255

	inc	myreg5
	inc	@r12
	inc	vari

	dec	r6
	dec	vari
	dec	@myreg5
	dec	@vari
	decw	rr6
	decw	vari
	decw	@myreg5
	decw	@vari
	decw	@myreg5

test1:	jr	test1
	jr	f,test1
	jr	uge,test1

	djnz	myreg5,test1

	call	test1
	call	@vari
	call	@rr10

	jp	test1
	jp	c,test1
	jp	@vari
	jp	@rr6


	ld	r3,r4
	ld	myreg5,vari
	ld	r6,@r7
	ld	r8,@vari
	ld	r9,vari(r10)
	ld	r11,#45

	ld	vari,r12
	ld	vari,vari
	ld	vari,@r13
	ld	vari,@vari
	ld	vari,#67

	ld	@r14,r15
	ld	@r0,vari
	ld	@r1,#89

	ld	@vari,r2
	ld	@vari,vari
	ld	@vari,#01

	ld	vari(r3),r4


	ldc	myreg5,@rr6
	ldc	@rr8,r7
	lde	r9,@rr10
	lde	@rr12,r11

	ldci	@r13,@rr14
	ldci	@rr0,@r15
	ldei	@r1,@rr2
	ldei	@rr4,@r3


	srp	#0

	; register aliases

myrege		equ	r7
myrrege		equ	rr12
myregr		reg	r7
myrregr		reg	rr12
myregre		reg	myrege
myrregre	reg	myrrege

	add	r7,#15
	add	myrege,#15
	add	myregr,#15
	add	myregre,#15

	jp	@rr12
	jp	@myrrege
	jp	@myrregr
	jp	@myrregre

	segment	data

	org	0aah
vari:

	segment	code

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
