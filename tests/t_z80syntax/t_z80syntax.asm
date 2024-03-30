	cpu	8080
	page 	0
	z80syntax	on

	mov	a,a
	ld	a,a

	mov	a,b
	ld	a,b

	mov	a,c
	ld	a,c

	mov	a,d
	ld	a,d

	mov	a,e
	ld	a,e

	mov	a,h
	ld	a,h

	mov	a,l
	ld	a,l

	mov	a,m
	ld	a,(hl)

	ldax	b
	ld	a,(bc)

	ldax	d
	ld	a,(de)

	lda	1234h
	ld	a,(1234h)

	mov	b,a
	ld	b,a

	mov	b,b
	ld	b,b

	mov	b,c
	ld	b,c

	mov	b,d
	ld	b,d

	mov	b,e
	ld	b,e

	mov	b,h
	ld	b,h

	mov	b,l
	ld	b,l

	mov	b,m
	ld	b,(hl)

	mov	c,a
	ld	c,a

	mov	c,b
	ld	c,b

	mov	c,c
	ld	c,c

	mov	c,d
	ld	c,d

	mov	c,e
	ld	c,e

	mov	c,h
	ld	c,h

	mov	c,l
	ld	c,l

	mov	c,m
	ld	c,(hl)

	mov	d,a
	ld	d,a

	mov	d,b
	ld	d,b

	mov	d,c
	ld	d,c

	mov	d,d
	ld	d,d

	mov	d,e
	ld	d,e

	mov	d,h
	ld	d,h

	mov	d,l
	ld	d,l

	mov	d,m
	ld	d,(hl)

	mov	e,a
	ld	e,a

	mov	e,b
	ld	e,b

	mov	e,c
	ld	e,c

	mov	e,d
	ld	e,d

	mov	e,e
	ld	e,e

	mov	e,h
	ld	e,h

	mov	e,l
	ld	e,l

	mov	e,m
	ld	e,(hl)

	mov	h,a
	ld	h,a

	mov	h,b
	ld	h,b

	mov	h,c
	ld	h,c

	mov	h,d
	ld	h,d

	mov	h,e
	ld	h,e

	mov	h,h
	ld	h,h

	mov	h,l
	ld	h,l

	mov	h,m
	ld	h,(hl)

	mov	l,a
	ld	l,a

	mov	l,b
	ld	l,b

	mov	l,c
	ld	l,c

	mov	l,d
	ld	l,d

	mov	l,e
	ld	l,e

	mov	l,h
	ld	l,h

	mov	l,l
	ld	l,l

	mov	l,m
	ld	l,(hl)

	mov	m,a
	ld	(hl),a

	mov	m,b
	ld	(hl),b

	mov	m,c
	ld	(hl),c

	mov	m,d
	ld	(hl),d

	mov	m,e
	ld	(hl),e

	mov	m,h
	ld	(hl),h

	mov	m,l
	ld	(hl),l

	mvi	a,12h
	ld	a,12h

	mvi	b,12h
	ld	b,12h

	mvi	c,12h
	ld	c,12h

	mvi	d,12h
	ld	d,12h

	mvi	e,12h
	ld	e,12h

	mvi	h,12h
	ld	h,12h

	mvi	l,12h
	ld	l,12h

	mvi	m,12h
	ld	(hl),12h

	stax	b
	ld	(bc),a

	stax	d
	ld	(de),a

	sta	1234h
	ld	(1234h),a

	lxi	b,1234h
	ld	bc,1234h

	lxi	d,1234h
	ld	de,1234h

	lxi	h,1234h
	ld	hl,1234h

	lxi	sp,1234h
	ld	sp,1234h

	lhld	1234h
	ld	hl,(1234h)

	shld	1234h
	ld	(1234h),hl

	sphl
	ld	sp,hl

	xchg
	ex	de,hl
	ex	hl,de

	xthl
	ex	(sp),hl
	ex	hl,(sp)

	add	a
	add	a,a

	add	b
	add	a,b

	add	c
	add	a,c

	add	d
	add	a,d

	add	e
	add	a,e

	add	h
	add	a,h

	add	l
	add	a,l

	add	m
	add	a,(hl)

	adi	12h
	add	a,12h

	adc	a
	adc	a,a

	adc	b
	adc	a,b

	adc	c
	adc	a,c

	adc	d
	adc	a,d

	adc	e
	adc	a,e

	adc	h
	adc	a,h

	adc	l
	adc	a,l

	adc	m
	adc	a,(hl)

	aci	12h
	adc	a,12h

	sub	a
	sub	a,a

	sub	b
	sub	a,b

	sub	c
	sub	a,c

	sub	d
	sub	a,d

	sub	e
	sub	a,e

	sub	h
	sub	a,h

	sub	l
	sub	a,l

	sub	m
	sub	(hl)
	sub	a,(hl)

	sui	12h
	sub	12h
	sub	a,12h

	sbb	a
	sbc	a
	sbc	a,a

	sbb	b
	sbc	b
	sbc	a,b

	sbb	c
	sbc	c
	sbc	a,c

	sbb	d
	sbc	d
	sbc	a,d

	sbb	e
	sbc	e	
	sbc	a,e

	sbb	h
	sbc	h
	sbc	a,h

	sbb	l
	sbc	l
	sbc	a,l

	sbb	m
	sbc	(hl)
	sbc	a,(hl)

	sbi	12h
	sbc	12h
	sbc	a,12h

	dad	b
	add	hl,bc

	dad	d
	add	hl,de

	dad	h
	add	hl,hl

	dad	sp
	add	hl,sp

	inr	a
	inc	a

	inr	b
	inc	b

	inr	c
	inc	c

	inr	d
	inc	d

	inr	e
	inc	e

	inr	h
	inc	h

	inr	l
	inc	l

	inr	m
	inc	(hl)

	dcr	a
	dec	a

	dcr	b
	dec	b

	dcr	c
	dec	c

	dcr	d
	dec	d

	dcr	e
	dec	e

	dcr	h
	dec	h

	dcr	l
	dec	l

	dcr	m
	dec	(hl)

	inx	b
	inc	bc

	inx	d
	inc	de

	inx	h
	inc	hl

	inx	sp
	inc	sp

	dcx	b
	dec	bc

	dcx	d
	dec	de

	dcx	h
	dec	hl

	dcx	sp
	dec	sp

	cma
	cpl

	stc
	scf

	cmc
	ccf

	rlc
	rlca

	rrc
	rrca

	ral
	rla

	rar
	rra

	ana	a
	and	a
	and	a,a

	ana	b
	and	b
	and	a,b

	ana	c
	and	c
	and	a,c

	ana	d
	and	d
	and	a,d

	ana	e
	and	e
	and	a,e

	ana	h
	and	h
	and	a,h

	ana	l
	and	l
	and	a,l

	ana	m
	and	(hl)
	and	a,(hl)

	ani	12h
	and	12h
	and	a,12h

	xra	a
	xor	a
	xor	a,a

	xra	b
	xor	b
	xor	a,b

	xra	c
	xor	c
	xor	a,c

	xra	d
	xor	d
	xor	a,d

	xra	e
	xor	e
	xor	a,e

	xra	h
	xor	h
	xor	a,h

	xra	l
	xor	l
	xor	a,l

	xra	m
	xor	(hl)
	xor	a,(hl)

	xri	12h
	xor	12h
	xor	a,12h

	ora	a
	or	a
	or	a,a

	ora	b
	or	b
	or	a,b

	ora	c
	or	c
	or	a,c

	ora	d
	or	d
	or	a,d

	ora	e
	or	e
	or	a,e

	ora	h
	or	h
	or	a,h

	ora	l
	or	l
	or	a,l

	ora	m
	or	(hl)
	or	a,(hl)

	ori	12h
	or	12h
	or	a,12h

	cmp	a
	cp	a
	cp	a,a

	cmp	b
	cp	b
	cp	a,b

	cmp	c
	cp	c
	cp	a,c

	cmp	d
	cp	d
	cp	a,d

	cmp	e
	cp	e
	cp	a,e

	cmp	h
	cp	h
	cp	a,h

	cmp	l
	cp	l
	cp	a,l

	cmp	m
	cp	(hl)
	cp	a,(hl)

	cpi	12h
	;cp	12h	; leave this out since CP <addr> is call-on-positive <addr> on 8080
	cp	a,12h

	jmp	1234h
	;jp	1234h	; leave this out since JP <addr> is jump-on-positive <addr> on 8080
	j	1234h

	jnz	1234h
	jp	nz,1234h
	j	nz,1234h

	jz	1234h
	jp	z,1234h
	j	z,1234h

	jnc	1234h
	jp	nc,1234h
	j	nc,1234h

	jc	1234h
	jp	c,1234h
	j	c,1234h

	jpo	1234h
	jp	po,1234h
	j	po,1234h

	jpe	1234h
	jp	pe,1234h
	j	pe,1234h

	jp	1234h
	jp	p,1234h
	j	p,1234h

	jm	1234h
	jp	m,1234h
	j	m,1234h

	pchl
	jp	(hl)
	j	(hl)

	call	1234h

	cnz	1234h
	call	nz,1234h

	cz	1234h
	call	z,1234h

	cnc	1234h
	call	nc,1234h

	cc	1234h
	call	c,1234h

	cpo	1234h
	call	po,1234h

	cpe	1234h
	call	pe,1234h

	cp	1234h
	call	p,1234h

	cm	1234h
	call	m,1234h

	ret

	rnz
	ret	nz

	rz
	ret	z

	rnc
	ret	nc

	rc
	ret	c

	rpo
	ret	po

	rpe
	ret	pe

	rp
	ret	p

	rm
	ret	m

	rst	0
	rst	00h

	rst	1
	rst	08h

	rst	2
	rst	10h

	rst	3
	rst	18h

	rst	4
	rst	20h

	rst	5
	rst	28h

	rst	6
	rst	30h

	rst	7
	rst	38h

	push	b
	push	bc

	push	d
	push	de

	push	h
	push	hl

	push	psw
	push	af

	pop	b
	pop	bc

	pop	d
	pop	de

	pop	h
	pop	hl

	pop	psw
	pop	af

	in	56h
	in	a,(56h)

	out	56h
	out	(56h),a

	hlt
	halt

	cpu	8085
	z80syntax	on

	rim
	ld	a,im
	
	sim
	ld	im,a

	cpu	8085undoc
	z80syntax	on

	lhlx
	ld	hl,(de)

	shlx
	ld	(de),hl

	dsub
	sub	hl,bc

	arhl
	sra	hl

	rdel
	rlc	de

	jx5	1234h
	jp	x5,1234h

	jnx5	1234h
	jp	nx5,1234h

	rstv
	rst	v

	ldhi	12h
	add	de,hl,12h

	ldsi	12h
	add	de,sp,12h
