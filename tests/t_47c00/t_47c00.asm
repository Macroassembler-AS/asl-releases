	cpu	470ac00
	include	stddef47.inc

	page 	0


	segment	data

nvar1	db	?
nvar2	db	?
	align	4
bvar	db	2 dup (?)


	segment	io

port1	db	?
port2	db	?
port3	port	7

	segment	code

targ:	db	?

	ret
	nop
	reti

	inc	@hl
	dec	@hl
	inc	a
	dec	a
	inc	l
	dec	l

	and	a,@hl
	and	@hl,#3
	and	a,#5
	or	a,@hl
	or	@hl,#3
	or	a,#5
	xor	a,@hl

	ld	a,@hl
	ld	a,123
	ld	hl,bvar
	ld	a,#4
	ld	h,#-3
	ld	l,#7
	ld	hl,#0a5h
	ld	dmb,#2
	ld	dmb,@hl

	ldl	a,@dc
	ldh	a,@dc+

	st	a,@hl
	st	a,@hl+
	st	a,@hl-
	st	a,123
	st	#3,@hl+
	st	#5,nvar1
	st      dmb,@hl

	mov	h,a
	mov	l,a
	mov	a,dmb
	mov	dmb,a
	mov	a,spw13
	mov	stk13,a

	xch	a,@hl
	xch	nvar2,a
	xch	hl,bvar
	xch	a,l
	xch	h,a
	xch	eir,a

	in	%port1,a
	in	%15h,a
	in	%port2,@hl
	in	%1ah,@hl

	out	a,%port1
	out	@hl,%port2
	out	#-3,%port2

	outb	@hl

	cmpr	a,@hl
	cmpr	a,nvar2
	cmpr	nvar2,#3
	cmpr	a,#4
	cmpr	h,#5
	cmpr	l,#6

	add	a,@hl
	add	@hl,#4
	add	nvar2,#5
	add	a,#6
	add	h,#7
	add	l,#7

	addc	a,@hl
	subrc	a,@hl

	subr	a,#7
	subr	@hl,#0ah

	rolc	a
	rolc	a,3
	rorc	a
	rorc	a,2

	clr	@l
	set	@l
	test	@l

	test	cf
	testp	cf

	testp	zf

;	clr	gf
;	set	gf
;	testp	gf

;	clr	dmb
;	set	dmb
;	test	dmb
;	testp	dmb

	clr	dmb0
	set	dmb0
	test	dmb0
	testp	dmb0

	clr	dmb1
	set	dmb1
	test	dmb1
	testp	dmb1

	clr	stk13
	set	stk13

	clr	il,8h

	test	a,2

	clr	@hl,1
	set	@hl,3
	test	@hl,2

	clr	%5,1
	set	%6,3
	test	%7,2
	testp	%8,0

	clr	nvar2,1
	set	nvar2,3
	test	nvar2,2
	testp	nvar2,0

	bss	($&3fc0h)+20h
	bs	123h
	bsl	0123h
	bsl	1123h
	bsl	2123h
	bsl     3123h

	calls	002eh

	call	123h

	eiclr	il,3
	diclr	il,5

	b	($&3fc0h)+20h
	b	123h
	b	0123h
	b	1123h
	b	2123h
	b	3123h

	bz	targ
	bnz	targ
	bc	targ
	bnc	targ
	be	a,@hl,targ
	be	a,nvar2,targ
	be	a,#3,targ
	be      h,#4,targ
	be	l,#5,targ
	be	nvar1,#6,targ
	bne	a,@hl,targ
	bne	a,nvar2,targ
	bne	a,#3,targ
	bne     h,#4,targ
	bne	l,#5,targ
	bne	nvar1,#6,targ
	bge	a,@hl,targ
	bge	a,nvar2,targ
	bge	a,#3,targ
	bge     h,#4,targ
	bge	l,#5,targ
	bge	nvar1,#6,targ
	bgt	a,@hl,targ
	bgt	a,nvar2,targ
	bgt	a,#3,targ
	bgt     h,#4,targ
	bgt	l,#5,targ
	bgt	nvar1,#6,targ
	ble	a,@hl,targ
	ble	a,nvar2,targ
	ble	a,#3,targ
	ble     h,#4,targ
	ble	l,#5,targ
	ble	nvar1,#6,targ
	blt	a,@hl,targ
	blt	a,nvar2,targ
	blt	a,#3,targ
	blt     h,#4,targ
	blt	l,#5,targ
	blt	nvar1,#6,targ

	callss	0
	callss	5

	callz	targ
	callnz	targ
	callc	targ
	callnc	targ

	retz
	retnz
	retc
	retnc
	retiz
	retinz
	retic
	retinc

	shl	a,2
	shl     h,2
	shl	l,2
	shl	@hl,2
	shl	nvar1,2
	shr	a,2
	shr     h,2
	shr	l,2
	shr	@hl,2
	shr	nvar1,2

	ei
	di

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
