	cpu	mb89190
	page	0

;----------------------------------------------------------------------------
; Transfer

	mov	23h,a
	mov	@ix+34h,a
	mov	1234h,a
	mov	@ep,a
	mov	r0,a
	mov	r1,a
	mov	r2,a
	mov	r3,a
	mov	r4,a
	mov	r5,a
	mov	r6,a
	mov	r7,a
	mov	a,#12h
	mov	a,23h
	mov	a,@ix+34h
	mov	a,1234h
	mov	a,@a
	mov	a,@ep
	mov	a,r0
	mov	a,r1
	mov	a,r2
	mov	a,r3
	mov	a,r4
	mov	a,r5
	mov	a,r6
	mov	a,r7
	mov	23h,#12h
	mov	@ix+34h,#12h
	mov	@ep,#12h
	mov	r0,#12h
	mov	r1,#12h
	mov	r2,#12h
	mov	r3,#12h
	mov	r4,#12h
	mov	r5,#12h
	mov	r6,#12h
	mov	r7,#12h
	movw	12h,a
	movw	@ix+34h,a
	movw	1234h,a
	movw	@ep,a
	movw	ep,a
	movw	a,#2345h
	movw	a,12h
	movw	a,@ix+34h
	movw	a,1234h
	movw	a,@a
	movw	a,@ep
	movw	a,ep
	movw	ep,#2345h
	movw	ix,a
	movw	a,ix
	movw	sp,a
	movw	a,sp
	mov     @a,t
	movw	@a,t
	movw	ix,#2345h
	movw	a,ps
	movw	ps,a
	movw	sp,#2345h
	swap
	setb	12h:6
	clrb	12h:5
	xch	a,t
	xchw	a,t
	xchw	a,ep
	xchw	a,ix
	xchw	a,sp
	movw	a,pc


;----------------------------------------------------------------------------
; Arithmetik

	addc	a,r0
	addc	a,r1
	addc	a,r2
	addc	a,r3
	addc	a,r4
	addc	a,r5
	addc	a,r6
	addc	a,r7
	addc	a,#12h
	addc	a,23h
	addc	a,@ix+34h
	addc	a,@ep
        addcw	a
	addc	a
	subc	a,r0
	subc	a,r1
	subc	a,r2
	subc	a,r3
	subc	a,r4
	subc	a,r5
	subc	a,r6
	subc	a,r7
	subc	a,#12h
	subc	a,23h
	subc	a,@ix+34h
	subc	a,@ep
	subcw	a
	subc	a
	inc	r0
	inc	r1
	inc	r2
	inc	r3
	inc	r4
	inc	r5
	inc	r6
	inc	r7
	incw	ep
	incw	ix
	incw	a
	dec	r0
	dec	r1
	dec	r2
	dec	r3
	dec	r4
	dec	r5
	dec	r6
	dec	r7
	decw	ep
	decw	ix
	decw	a
	mulu	a
	divu	a
	andw	a
	orw	a
	xorw	a
	cmp	a
	rorc	a
	rolc	a
	cmp	a,#12h
	cmp	a,23h
	cmp	a,@ix+34h
	cmp	a,@ep
	cmp	a,r0
	cmp	a,r1
	cmp	a,r2
	cmp	a,r3
	cmp	a,r4
	cmp	a,r5
	cmp	a,r6
	cmp	a,r7
	daa
	das
	xor	a
	xor	a,#12h
	xor	a,23h
	xor	a,@ix+34h
	xor	a,@ep
	xor	a,r0
	xor	a,r1
	xor	a,r2
	xor	a,r3
	xor	a,r4
	xor	a,r5
	xor	a,r6
	xor	a,r7
	and	a
	and	a,#12h
	and	a,23h
	and	a,@ix+34h
	and	a,@ep
	and	a,r0
	and	a,r1
	and	a,r2
	and	a,r3
	and	a,r4
	and	a,r5
	and	a,r6
	and	a,r7
	or	a
	or	a,#12h
	or	a,23h
	or	a,@ix+34h
	or	a,@ep
	or	a,r0
	or	a,r1
	or	a,r2
	or	a,r3
	or	a,r4
	or	a,r5
	or	a,r6
	or	a,r7
	cmp	23h,#12h
	cmp	@ep,#12h
	cmp	@ix+34h,#12h
	cmp	r0,#12h
	cmp	r1,#12h
	cmp	r2,#12h
	cmp	r3,#12h
	cmp	r4,#12h
	cmp	r5,#12h
	cmp	r6,#12h
	cmp	r7,#12h
	incw	sp
	decw	sp

;----------------------------------------------------------------------------
; Spruenge

	bz	$
	beq	$
	bnz	$
	bne	$
	bc	$
	blo	$
	bnc	$
	bhs	$
	bn	$
	bp	$
	blt	$
	bge	$
	bbc	23h:5,$
	bbs	23h:4,$
	jmp	@a
	jmp	2345h
	callv	#4
	call	2345h
	xchw	a,pc
	ret
	reti

;----------------------------------------------------------------------------
; Sonstiges

	pushw	a
	popw	a
	pushw	ix
	popw	ix
	nop
	clrc
	setc
	clri
	seti

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
