	cpu	87c70
	include	stddef87.inc
	page	0

	ei
	di
	swi
	nop
	ret
	reti
	retn

targ:	jrs	t,targ
	jrs	f,targ

	jr	t,targ
	jr	f,targ
	jr	eq,targ
	jr	z,targ
	jr	ne,targ
	jr	nz,targ
	jr	cs,targ
	jr	lt,targ
	jr	cc,targ
	jr	ge,targ
	jr	le,targ
	jr	gt,targ

	daa	a
	daa	b
	das	a
	das	c
	shlc	a
	shlc	d
	shrc	a
	shrc	e
	rolc	a
	rolc	h
	rorc	a
	rorc	l
	swap	a
	swap	b

	add	a,c
	addc	l,a
	sub	wa,de
	subb	a,(12h)
	and	a,(hl)
	or	a,(de)
	xor	a,(hl+)
	cmp	a,(-hl)
	add	a,(hl+10)
	addc	a,(hl-125)
	sub	a,(hl+c)
	subb	a,(c+hl)
	and	a,(pc+a)
	or	a,(a+pc)
	cmp	(12h),(hl)
	add	(de),(hl)
	addc	(hl+5),(hl)
	sub	(hl+c),(hl)
	subb	(pc+a),(hl)
	and	a,55h
	or	c,55h
	xor	hl,5678h
	cmp     (12h),55h
	add	(hl),55h
	addc	(hl+),55h
	sub	(-hl),55h
	subb	(hl-32),55h
	and	(hl+c),55h
	or	(pc+a),55h

	mcmp	(12h),55h
	mcmp	(de),55h
	mcmp	(hl+),55h
	mcmp	(-hl),55h
	mcmp	(hl+3),55h
	mcmp	(c+hl),55h
	mcmp	(pc+a),55h


	inc	c
	inc	bc
	inc	(12h)
	inc	(hl)
	inc	(de)
	inc	(hl+)
	inc	(-hl)
	inc	(hl+4)
	inc	(hl+c)
	inc	(a+pc)
	dec	e
	dec	hl
	dec	(12h)
	dec	(hl)
	dec	(de)
	dec	(hl+)
	dec	(-hl)
	dec	(hl-7)
	dec	(hl+c)
	dec	(pc+a)

	mul	w,a
	mul	c,b
	mul	d,e
	mul	l,h

targ2:
	jr	targ2

	div	wa,c
	div	de,c
	div	hl,c

	rold	a,(12h)
	rold	a,(hl)
	rold	a,(hl+)
	rold	a,(-hl)
	rold	a,(hl+16)
	rold	a,(c+hl)
	rord	a,(12h)
	rord	a,(hl)
	rord	a,(hl+)
	rord	a,(-hl)
	rord	a,(hl-16)
	rord	a,(hl+c)

	xch	a,d
	xch	d,w
	xch	hl,de
	xch	bc,wa
	xch	b,(12h)
	xch	(12h),c
	xch	d,(hl)
	xch	(de),e
	expect	140
	 xch	 l,(hl+)
	endexpect
	expect	140
	 xch	 (hl+),h
	endexpect
	xch	a,(-hl)
	xch	(-hl),w
	xch	b,(hl+5)
	xch	(hl-3),c
	xch	d,(hl+c)
	xch	(c+hl),e
	xch	h,(pc+a)
	xch	(a+pc),l

	clr	b
	clr	de
	clr	(12h)
	clr	(hl)
	clr	(de)
	clr	(hl+)
	clr	(-hl)
	clr	(hl+5)

	ldw	hl,1234h
	ldw	(12h),1234h
	ldw	(hl),1234h

	ld	a,l
	ld	d,a
	ld	w,h
	ld	a,(12h)
	ld	c,(12h)
	ld	a,(hl)
	ld	d,(hl)
	ld	c,(de)
	ld	b,(-hl)
	expect	140
	 ld	 h,(hl+)
	endexpect
	ld	c,(hl-122)
	ld	w,(hl+c)
	ld      d,(a+pc)
	ld	h,20

	ld	hl,bc
	ld	de,(12h)
	ld	bc,(de)
	ld	wa,(hl+1)
	ld	hl,(hl+c)
	ld	de,(pc+a)
	ld	bc,1234

	ld	(12h),a
	ld	(12h),c
	ld	(12h),de
	ld	(12h),(23h)
	ld	(12h),(de)
	ld	(12h),(hl-42)
	ld	(12h),(hl+c)
	ld	(12h),(pc+a)
	ld	(12h),23h

	ld	(hl),a
	ld	(de),d
	ld	(hl+),c
	ld	(-hl),e
	ld      (hl+4),h
	ld	(hl),de
	ld	(hl+5),bc
	ld	(hl),(12h)
	ld	(hl),(de)
	ld	(hl),(hl+1)
	ld	(hl),(hl+c)
	ld	(hl),(pc+a)
	ld	(hl),23h
	ld	(de),23h
	ld	(hl+),23h
	ld	(-hl),23h
	ld	(hl-77),23h

	ld	sp,1234h
	ld	sp,de
	ld	hl,sp
	ld	rbs,7

	jp	2000h
	call	1234h
	call	0ff54h
	jp	hl
	call	de
	jp	(hl)
	jp	(de)
	jp	(12h)
	call	(12h)
	jp	(hl+5)
	call	(hl-100)
	jp	(hl+c)
	call	(a+pc)
	jp	(c+hl)
	call	(pc+a)

	callv	3

	callp	76h
	callp	0ff12h

	push	psw
	push	de
	pop	wa
	pop	psw

	ld	cf,a.5
	ld	cf,(12h).4
	ld	cf,(hl).3
	ld	cf,(hl+).2
	ld	cf,(-hl).1
	ld	cf,(hl+3).0
	ld	cf,(hl+c).7
	ld	cf,(pc+a).3
	ld	cf,(de).c

	ld	a.5,cf
	ld	(12h).4,cf
	ld	(hl).3,cf
	ld	(hl+).2,cf
	ld	(-hl).1,cf
	ld	(hl+3).0,cf
	ld	(hl+c).7,cf
	ld	(pc+a).3,cf
	ld	(de).c,cf

	xor	cf,d.3
	xor	cf,(12h).4
	xor	cf,(de).5
	xor	cf,(hl+).6
	xor	cf,(-hl).7
	xor	cf,(hl+3).0
	xor	cf,(hl+c).1
	xor	cf,(pc+a).2

	clr	cf
	clr	d.3
	clr	(12h).4
	clr	(de).5
	clr	(hl+).6
	clr	(-hl).7
	clr	(hl+3).0
	clr	(hl+c).1
	clr	(pc+a).2
	clr	(de).c

	set	cf
	set	d.3
	set	(12h).4
	set	(de).5
	set	(hl+).6
	set	(-hl).7
	set	(hl+3).0
	set	(hl+c).1
	set	(pc+a).2
	set	(de).c

	cpl	cf
	cpl	d.3
	cpl	(12h).4
	cpl	(de).5
	cpl	(hl+).6
	cpl	(-hl).7
	cpl	(hl+3).0
	cpl	(hl+c).1
	cpl	(pc+a).2
	cpl	(de).c

	test	d.3
	test	(12h).4
	test	(de).5
	test	(hl+).6
	test	(-hl).7
	test	(hl+3).0
	test	(hl+c).1
	test	(pc+a).2
	test	(de).c

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
