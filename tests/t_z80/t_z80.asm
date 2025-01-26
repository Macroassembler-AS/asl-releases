	cpu	z80
	page	0

	nop			; 00
	ld	bc,1234h	; 01 34 12
	ld	(bc),a		; 02
	inc	bc		; 03
	inc	b		; 04
	dec	b		; 05
	ld	b,12h		; 06 12
	rlca			; 07
	ex	af,af'		; 08
	exa			; 08
	add	hl,bc		; 09
	ld	a,(bc)		; 0a
	dec	bc		; 0b
	inc	c		; 0c
	dec	c		; 0d
	ld	c,12h		; 0e 12
	rrca			; 0f
	djnz	$		; 10 fe
	ld	de,1234h	; 11 34 12
	ld	(de),a		; 12
	inc	de		; 13
	inc	d		; 14
	dec	d		; 15
	ld	d,12h		; 16 12
	rla			; 17
	jr	$		; 18 fe
	add	hl,de		; 19
	ld	a,(de)		; 1a
	dec	de		; 1b
	inc	e		; 1c
	dec	e		; 1d
	ld	e,12h		; 1e 12
	rra			; 1f
	jr	nz,$		; 20 fe
	ld	hl,12h		; 21 12
	ld	(1234h),hl	; 22 34 12
	inc	hl		; 23
	inc	h		; 24
	dec	h		; 25
	ld	h,12h		; 26 12
	daa			; 27
	jr	z,$		; 28 fe
	add	hl,hl		; 29
	ld	hl,(1234h)	; 2a 34 12
	dec	hl		; 2b
	inc	l		; 2c
	dec	l		; 2d
	ld	l,12h		; 2e 12
	cpl			; 2f
	jr	nc,$		; 30 fe
	ld	sp,1234h	; 31 34 12
	ld	(1234h),a	; 32 34 12
	inc	sp		; 33
	inc	(hl)		; 34
	dec	(hl)		; 35
	ld	(hl),12h	; 36 12
	scf			; 37
	jr	c,$		; 38 12
	add	hl,sp		; 39
	ld	a,(1234h)	; 3a 34 12
	dec	sp		; 3b
	inc	a		; 3c
	dec	a		; 3d
	ld	a,12h		; 3e 12
	ccf			; 3f
	ld	b,b		; 40
	ld	b,c		; 41
	ld	b,d		; 42
	ld	b,e		; 43
	ld	b,h		; 44
	ld	b,l		; 45
	ld	b,(hl)		; 46
	ld	b,a		; 47
	ld	c,b		; 48
	ld	c,c		; 49
	ld	c,d		; 4a
	ld	c,e		; 4b
	ld	c,h		; 4c
	ld	c,l		; 4d
	ld	c,(hl)		; 4e
	ld	c,a		; 4f
	ld	d,b		; 50
	ld	d,c		; 51
	ld	d,d		; 52
	ld	d,e		; 53
	ld	d,h		; 54
	ld	d,l		; 55
	ld	d,(hl)		; 56
	ld	d,a		; 57
	ld	e,b		; 58
	ld	e,c		; 59
	ld	e,d		; 5a
	ld	e,e		; 5b
	ld	e,h		; 5c
	ld	e,l		; 5d
	ld	e,(hl)		; 5e
	ld	e,a		; 5f
	ld	h,b		; 60
	ld	h,c		; 61
	ld	h,d		; 62
	ld	h,e		; 63
	ld	h,h		; 64
	ld	h,l		; 65
	ld	h,(hl)		; 66
	ld	h,a		; 67
	ld	l,b		; 68
	ld	l,c		; 69
	ld	l,d		; 6a
	ld	l,e		; 6b
	ld	l,h		; 6c
	ld	l,l		; 6d
	ld	l,(hl)		; 6e
	ld	l,a		; 6f
	ld	(hl),b		; 70
	ld	(hl),c		; 71
	ld	(hl),d		; 72
	ld	(hl),e		; 73
	ld	(hl),h		; 74
	ld	(hl),l		; 75
	halt			; 76
	ld	(hl),a		; 77
	ld	a,b		; 78
	ld	a,c		; 79
	ld	a,d		; 7a
	ld	a,e		; 7b
	ld	a,h		; 7c
	ld	a,l		; 7d
	ld	a,(hl)		; 7e
	ld	a,a		; 7f
	add	a,b		; 80
	add	a,c		; 81
	add	a,d		; 82
	add	a,e		; 83
	add	a,h		; 84
	add	a,l		; 85
	add	a,(hl)		; 86
	add	a,a		; 87
	adc	a,b		; 88
	adc	a,c		; 89
	adc	a,d		; 8a
	adc	a,e		; 8b
	adc	a,h		; 8c
	adc	a,l		; 8d
	adc	a,(hl)		; 8e
	adc	a,a		; 8f
	sub	b		; 90
	sub	c		; 91
	sub	d		; 92
	sub	e		; 93
	sub	h		; 94
	sub	l		; 95
	sub	(hl)		; 96
	sub	a		; 97
	sbc	a,b		; 98
	sbc	a,c		; 99
	sbc	a,d		; 9a
	sbc	a,e		; 9b
	sbc	a,h		; 9c
	sbc	a,l		; 9d
	sbc	a,(hl)		; 9e
	sbc	a,a		; 9f
	and	b		; a0
	and	c		; a1
	and	d		; a2
	and	e		; a3
	and	h		; a4
	and	l		; a5
	and	(hl)		; a6
	and	a		; a7
	xor	b		; a8
	xor	c		; a9
	xor	d		; aa
	xor	e		; ab
	xor	h		; ac
	xor	l		; ad
	xor	(hl)		; ae
	xor	a		; af
	or	b		; b0
	or	c		; b1
	or	d		; b2
	or	e		; b3
	or	h		; b4
	or	l		; b5
	or	(hl)		; b6
	or	a		; b7
	cp	b		; b8
	cp	c		; b9
	cp	d		; ba
	cp	e		; bb
	cp	h		; bc
	cp	l		; bd
	cp	(hl)		; be
	cp	a		; bf
	ret	nz		; c0
	pop	bc		; c1
	jp	nz,1234h	; c2 34 12
	jp	1234h		; c3 34 12
-	jp	-		; c3 xx xx
	call	nz,1234h	; c4 34 12
	push	bc		; c5
	add	a,12h		; c6 12
	rst	00h		; c7
	ret	z		; c8
	ret			; c9
	jp	z,1234h		; ca 34 12
	;BITS			; cb
	call	z,1234h		; cc 34 12
	call	1234h		; cd 34 12
	adc	a,12h		; ce 12
	rst	08h		; cf
	ret	nc		; d0
	pop	de		; d1
	jp	nc,1234h	; d2 34 12
	out	(12h),a		; d3 12
	call	nc,1234h	; d4 34 12
	push	de		; d5
	sub	12h		; d6 12
	rst	10h		; d7
	ret	c		; d8
	exx			; d9
	jp	c,1234h		; da 34 12
	in	a,(12h)		; db 12
	call	c,1234h		; dc 34 12	
	;IX			; dd
	sbc	a,12h		; de 12
	rst	18h		; df
	ret	po		; e0
	pop	hl		; e1
	jp	po,1234h	; e2 34 12
	ex	(sp),hl		; e3
	call	po,1234h	; e4 34 12
	push	hl		; e5
	and	12h		; e6 12
	rst	20h		; e7
	ret	pe		; e8
	jp	( hl )		; e9
	jp	pe,1234h	; ea 34 12
	ex	de,hl		; eb
	exd			; eb
	call	pe,1234h	; ec 34 12
	;EXTD			; ed
	xor	12h		; ee 12
	rst	28h		; ef
	ret	p		; f0
	pop	af		; f1
	jp	p,1234h		; f2 34 12
	di			; f3
	call	p,1234h		; f4 34 12
	push	af		; f5
	or	12h		; f6 12
	rst	30h		; f7
	ret	m		; f8
	ld	sp,hl		; f9
	jp	m,1234h		; fa 34 12
	ei			; fb
	call	m,1234h		; fc 34 12
	;IY			; fd
	cp	12h		; fe 12
	rst	38h		; ff

	in	b,(c)		; ed 40
	out	(c),b		; ed 41
	sbc	hl,bc		; ed 42
	ld	(1234h),bc	; ed 43 34 12
	neg			; ed 44
	retn			; ed 45
	im	0		; ed 46
	ld	i,a		; ed 47
	in	c,(c)		; ed 48
	out	(c),c		; ed 49
	adc	hl,bc		; ed 4a
	ld	bc,(1234h)	; ed 4b 34 12
	;neg			; ed 4c
	reti			; ed 4d
	;im	0/1		; ed 4e
	ld	r,a		; ed 4f
	in	d,(c)		; ed 50
	out	(c),d		; ed 51
	sbc	hl,de		; ed 52
	ld	(1234h),de	; ed 53 34 12
	;neg			; ed 54
	;retn			; ed 55
	im	1		; ed 56
	ld	a,i		; ed 57
	in	e,(c)		; ed 58
	out	(c),e		; ed 59
	adc	hl,de		; ed 5a
	ld	de,(1234h)	; ed 5b 34 12
	;neg			; ed 5c
	;retn			; ed 5d
	im	2		; ed 5e
	ld	a,r		; ed 5f
	in	h,(c)		; ed 60
	out	(c),h		; ed 61
	sbc	hl,hl		; ed 62
	;ld	(1234h),hl	; ed 63 34 12
	;neg			; ed 64
	retn			; ed 65
	im	0		; ed 66
	rrd			; ed 67
	in	l,(c)		; ed 68
	out	(c),l		; ed 69
	adc	hl,hl		; ed 6a
	;ld	hl,(1234h)	; ed 6b 34 12
	;neg			; ed 6c
	;retn			; ed 6d
	;im	0/1		; ed 6e
	rld			; ed 6f
	;in	(c)		; ed 70
	;out	(c),0		; ed 71
	sbc	hl,sp		; ed 72
	ld	(1234h),sp	; ed 73 34 12
	;neg			; ed 74
	;retn			; ed 75
	;im	1		; ed 76
	in	a,(c)		; ed 78
	out	(c),a		; ed 79
	adc	hl,sp		; ed 7a
	ld	sp,(1234h)	; ed 7b 34 12
	;neg			; ed 7c
	;retn			; ed 7d
	;im	2		; ed 7e
	ldi			; ed a0
	cpi			; ed a1
	ini			; ed a2
	outi			; ed a3
	ldd			; ed a8
	cpd			; ed a9
	ind			; ed aa
	outd			; ed ab
	ldir			; ed b0
	cpir			; ed b1
	inir			; ed b2
	otir			; ed b3
	lddr			; ed b8
	cpdr			; ed b9
	indr			; ed ba
	otdr			; ed bb

	rlc	b		; cb 00
	rlc	c		; cb 01
	rlc	d		; cb 02
	rlc	e		; cb 03
	rlc	h		; cb 04
	rlc	l		; cb 05
	rlc	(hl)		; cb 06
	rlc	a		; cb 07
	rrc	b		; cb 08
	rrc	c		; cb 09
	rrc	d		; cb 0a
	rrc	e		; cb 0b
	rrc	h		; cb 0c
	rrc	l		; cb 0d
	rrc	(hl)		; cb 0e
	rrc	a		; cb 0f
	rl	b		; cb 10
	rl	c		; cb 11
	rl	d		; cb 12
	rl	e		; cb 13
	rl	h		; cb 14
	rl	l		; cb 15
	rl	(hl)		; cb 16
	rl	a		; cb 17
	rr	b		; cb 18
	rr	c		; cb 19
	rr	d		; cb 1a
	rr	e		; cb 1b
	rr	h		; cb 1c
	rr	l		; cb 1d
	rr	(hl)		; cb 1e
	rr	a		; cb 1f
	sla	b		; cb 20
	sla	c		; cb 21
	sla	d		; cb 22
	sla	e		; cb 23
	sla	h		; cb 24
	sla	l		; cb 25
	sla	(hl)		; cb 26
	sla	a		; cb 27
	sra	b		; cb 28
	sra	c		; cb 29
	sra	d		; cb 2a
	sra	e		; cb 2b
	sra	h		; cb 2c
	sra	l		; cb 2d
	sra	(hl)		; cb 2e
	sra	a		; cb 2f
	;sll	b		; cb 30
	;sll	c		; cb 31
	;sll	d		; cb 32
	;sll	e		; cb 33
	;sll	h		; cb 34
	;sll	l		; cb 35
	;sll	(hl)		; cb 36
	;sll	a		; cb 37
	srl	b		; cb 38
	srl	c		; cb 39
	srl	d		; cb 3a
	srl	e		; cb 3b
	srl	h		; cb 3c
	srl	l		; cb 3d
	srl	(hl)		; cb 3e
	srl	a		; cb 3f
	bit	0,b		; cb 40
	bit	0,c		; cb 41
	bit	0,d		; cb 42
	bit	0,e		; cb 43
	bit	0,h		; cb 44
	bit	0,l		; cb 45
	bit	0,(hl)		; cb 46
	bit	0,a		; cb 47
	bit	1,b		; cb 48
	bit	1,c		; cb 49
	bit	1,d		; cb 4a
	bit	1,e		; cb 4b
	bit	1,h		; cb 4c
	bit	1,l		; cb 4d
	bit	1,(hl)		; cb 4e
	bit	1,a		; cb 4f
	bit	2,b		; cb 50
	bit	2,c		; cb 51
	bit	2,d		; cb 52
	bit	2,e		; cb 53
	bit	2,h		; cb 54
	bit	2,l		; cb 55
	bit	2,(hl)		; cb 56
	bit	2,a		; cb 57
	bit	3,b		; cb 58
	bit	3,c		; cb 59
	bit	3,d		; cb 5a
	bit	3,e		; cb 5b
	bit	3,h		; cb 5c
	bit	3,l		; cb 5d
	bit	3,(hl)		; cb 5e
	bit	3,a		; cb 5f
	bit	4,b		; cb 60
	bit	4,c		; cb 61
	bit	4,d		; cb 62
	bit	4,e		; cb 63
	bit	4,h		; cb 64
	bit	4,l		; cb 65
	bit	4,(hl)		; cb 66
	bit	4,a		; cb 67
	bit	5,b		; cb 68
	bit	5,c		; cb 69
	bit	5,d		; cb 6a
	bit	5,e		; cb 6b
	bit	5,h		; cb 6c
	bit	5,l		; cb 6d
	bit	5,(hl)		; cb 6e
	bit	5,a		; cb 6f
	bit	6,b		; cb 70
	bit	6,c		; cb 71
	bit	6,d		; cb 72
	bit	6,e		; cb 73
	bit	6,h		; cb 74
	bit	6,l		; cb 75
	bit	6,(hl)		; cb 76
	bit	6,a		; cb 77
	bit	7,b		; cb 78
	bit	7,c		; cb 79
	bit	7,d		; cb 7a
	bit	7,e		; cb 7b
	bit	7,h		; cb 7c
	bit	7,l		; cb 7d
	bit	7,(hl)		; cb 7e
	bit	7,a		; cb 7f
	res	0,b		; cb 80
	res	0,c		; cb 81
	res	0,d		; cb 82
	res	0,e		; cb 83
	res	0,h		; cb 84
	res	0,l		; cb 85
	res	0,(hl)		; cb 86
	res	0,a		; cb 87
	res	1,b		; cb 88
	res	1,c		; cb 89
	res	1,d		; cb 8a
	res	1,e		; cb 8b
	res	1,h		; cb 8c
	res	1,l		; cb 8d
	res	1,(hl)		; cb 8e
	res	1,a		; cb 8f
	res	2,b		; cb 90
	res	2,c		; cb 91
	res	2,d		; cb 92
	res	2,e		; cb 93
	res	2,h		; cb 94
	res	2,l		; cb 95
	res	2,(hl)		; cb 96
	res	2,a		; cb 97
	res	3,b		; cb 98
	res	3,c		; cb 99
	res	3,d		; cb 9a
	res	3,e		; cb 9b
	res	3,h		; cb 9c
	res	3,l		; cb 9d
	res	3,(hl)		; cb 9e
	res	3,a		; cb 9f
	res	4,b		; cb a0
	res	4,c		; cb a1
	res	4,d		; cb a2
	res	4,e		; cb a3
	res	4,h		; cb a4
	res	4,l		; cb a5
	res	4,(hl)		; cb a6
	res	4,a		; cb a7
	res	5,b		; cb a8
	res	5,c		; cb a9
	res	5,d		; cb aa
	res	5,e		; cb ab
	res	5,h		; cb ac
	res	5,l		; cb ad
	res	5,(hl)		; cb ae
	res	5,a		; cb af
	res	6,b		; cb b0
	res	6,c		; cb b1
	res	6,d		; cb b2
	res	6,e		; cb b3
	res	6,h		; cb b4
	res	6,l		; cb b5
	res	6,(hl)		; cb b6
	res	6,a		; cb b7
	res	7,b		; cb b8
	res	7,c		; cb b9
	res	7,d		; cb ba
	res	7,e		; cb bb
	res	7,h		; cb bc
	res	7,l		; cb bd
	res	7,(hl)		; cb be
	res	7,a		; cb bf
	set	0,b		; cb 80
	set	0,c		; cb c1
	set	0,d		; cb c2
	set	0,e		; cb c3
	set	0,h		; cb c4
	set	0,l		; cb c5
	set	0,(hl)		; cb c6
	set	0,a		; cb c7
	set	1,b		; cb c8
	set	1,c		; cb c9
	set	1,d		; cb ca
	set	1,e		; cb cb
	set	1,h		; cb cc
	set	1,l		; cb cd
	set	1,(hl)		; cb ce
	set	1,a		; cb cf
	set	2,b		; cb d0
	set	2,c		; cb d1
	set	2,d		; cb d2
	set	2,e		; cb d3
	set	2,h		; cb d4
	set	2,l		; cb d5
	set	2,(hl)		; cb d6
	set	2,a		; cb d7
	set	3,b		; cb d8
	set	3,c		; cb d9
	set	3,d		; cb da
	set	3,e		; cb db
	set	3,h		; cb dc
	set	3,l		; cb dd
	set	3,(hl)		; cb de
	set	3,a		; cb df
	set	4,b		; cb e0
	set	4,c		; cb e1
	set	4,d		; cb e2
	set	4,e		; cb e3
	set	4,h		; cb e4
	set	4,l		; cb e5
	set	4,(hl)		; cb e6
	set	4,a		; cb e7
	set	5,b		; cb e8
	set	5,c		; cb e9
	set	5,d		; cb ea
	set	5,e		; cb eb
	set	5,h		; cb ec
	set	5,l		; cb ed
	set	5,(hl)		; cb ee
	set	5,a		; cb ef
	set	6,b		; cb f0
	set	6,c		; cb f1
	set	6,d		; cb f2
	set	6,e		; cb f3
	set	6,h		; cb f4
	set	6,l		; cb f5
	set	6,(hl)		; cb f6
	set	6,a		; cb f7
	set	7,b		; cb f8
	set	7,c		; cb f9
	set	7,d		; cb fa
	set	7,e		; cb fb
	set	7,h		; cb fc
	set	7,l		; cb fd
	set	7,(hl)		; cb fe
	set	7,a		; cb ff

	add	ix,bc		; dd 09
	add	ix,de		; dd 19
	ld	ix,1234h	; dd 21 34 12
	ld	(1234h),ix	; dd 22 34 12
	inc	ix		; dd 23
	;inc	ixh		; dd 24
	;dec	ixh		; dd 25
	;ld	ixh,12h		; dd 26 12
	add	ix,ix		; dd 29
	ld	ix,(1234h)	; dd 2a 34 12
	dec	ix		; dd 2b
	;inc	ixl		; dd 2c
	;dec	ixl		; dd 2d
	;ld	ixl,12h		; dd 2e 12
	inc	(ix+12h)	; dd 34 12
	dec	(ix+12h)	; dd 35 12
	ld	(ix+12h),34h	; dd 36 12 34
	add	ix,sp		; dd 39
	;ld	b,ixh		; dd 44
	;ld	b,ixl		; dd 45
	ld	b,(ix+12h)	; dd 46 12
	;ld	c,ixh		; dd 4c
	;ld	c,ixl		; dd 4d
	ld	c,(ix+12h)	; dd 4e 12
	;ld	d,ixh		; dd 54
	;ld	d,ixl		; dd 55
	ld	d,(ix+12h)	; dd 56 12
	;ld	e,ixh		; dd 5c
	;ld	e,ixl		; dd 5d
	ld	e,(ix+12h)	; dd 5e 12
	;ld	ixh,b		; dd 60
	;ld	ixh,c		; dd 61
	;ld	ixh,d		; dd 62
	;ld	ixh,e		; dd 63
	;ld	ixh,ixh		; dd 64
	;ld	ixh,ixl		; dd 65
	ld	h,(ix+12h)	; dd 66 12
	;ld	ixh,a		; dd 67
	;ld	ixl,b		; dd 68
	;ld	ixl,c		; dd 69
	;ld	ixl,d		; dd 6a
	;ld	ixl,e		; dd 6b
	;ld	ixl,ixh		; dd 6c
	;ld	ixl,ixl		; dd 6d
	ld	l,(ix+12h)	; dd 6e 12
	;ld	ixl,a		; dd 6f
	ld	(ix+12h),b	; dd 70 12
	ld	(ix+12h),c	; dd 71 12
	ld	(ix+12h),d	; dd 72 12
	ld	(ix+12h),e	; dd 73 12
	ld	(ix+12h),h	; dd 74 12
	ld	(ix+12h),l	; dd 75 12
	ld	(ix+12h),a	; dd 77 12
	;ld	a,ixh		; dd 7c
	;ld	a,ixl		; dd 7d
	ld	a,(ix+12h)	; dd 7e 12
	;add	a,ixh		; dd 84
	;add	a,ixl		; dd 85
	add	a,(ix+12h)	; dd 86 12
	;adc	a,ixh		; dd 8c
	;adc	a,ixl		; dd 8d
	adc	a,(ix+12h)	; dd 8e 12
	;sub	a,ixh		; dd 94
	;sub	a,ixl		; dd 95
	sub	a,(ix+12h)	; dd 96 12
	;sbc	a,ixh		; dd 9c
	;sbc	a,ixl		; dd 9d
	sbc	a,(ix+12h)	; dd 9e 12
	;and	a,ixh		; dd a4
	;and	a,ixl		; dd a5
	and	a,(ix+12h)	; dd a6 12
	;xor	a,ixh		; dd ac
	;xor	a,ixl		; dd ad
	xor	a,(ix+12h)	; dd ae 12
	;or	ixh		; dd b4
	;or	ixl		; dd b5
	or	(ix+12h)	; dd b6 12
	;cp	ixh		; dd bc
	;cp	ixl		; dd bd
	cp	(ix+12h)	; dd be 12
	pop	ix		; dd e1
	ex	(sp),ix		; dd e3
	push	ix		; dd e5
	jp	(ix)		; dd e9
	ld	sp,ix		; dd f9

	;rlc	(ix+12h),b	; dd cb 12 00
	;rlc	(ix+12h),c	; dd cb 12 01
	;rlc	(ix+12h),d	; dd cb 12 02
	;rlc	(ix+12h),e	; dd cb 12 03
	;rlc	(ix+12h),h	; dd cb 12 04
	;rlc	(ix+12h),l	; dd cb 12 05
	rlc	(ix+12h)	; dd cb 12 06
	;rlc	(ix+12h),a	; dd cb 12 07
	;rrc	(ix+12h),b	; dd cb 12 08
	;rrc	(ix+12h),c	; dd cb 12 09
	;rrc	(ix+12h),d	; dd cb 12 0a
	;rrc	(ix+12h),e	; dd cb 12 0b
	;rrc	(ix+12h),h	; dd cb 12 0c
	;rrc	(ix+12h),l	; dd cb 12 0d
	rrc	(ix+12h)	; dd cb 12 0e
	;rrc	(ix+12h),a	; dd cb 12 0f
	;rl	(ix+12h),b	; dd cb 12 10
	;rl	(ix+12h),c	; dd cb 12 11
	;rl	(ix+12h),d	; dd cb 12 12
	;rl	(ix+12h),e	; dd cb 12 13
	;rl	(ix+12h),h	; dd cb 12 14
	;rl	(ix+12h),l	; dd cb 12 15
	rl	(ix+12h)	; dd cb 12 16
	;rl	(ix+12h),a	; dd cb 12 17
	;rr	(ix+12h),b	; dd cb 12 18
	;rr	(ix+12h),c	; dd cb 12 19
	;rr	(ix+12h),d	; dd cb 12 1a
	;rr	(ix+12h),e	; dd cb 12 1b
	;rr	(ix+12h),h	; dd cb 12 1c
	;rr	(ix+12h),l	; dd cb 12 1d
	rr	(ix+12h)	; dd cb 12 1e
	;rr	(ix+12h),a	; dd cb 12 1f
	;sla	(ix+12h),b	; dd cb 12 20
	;sla	(ix+12h),c	; dd cb 12 21
	;sla	(ix+12h),d	; dd cb 12 22
	;sla	(ix+12h),e	; dd cb 12 23
	;sla	(ix+12h),h	; dd cb 12 24
	;sla	(ix+12h),l	; dd cb 12 25
	sla	(ix+12h)	; dd cb 12 26
	;sla	(ix+12h),a	; dd cb 12 27
	;sra	(ix+12h),b	; dd cb 12 28
	;sra	(ix+12h),c	; dd cb 12 29
	;sra	(ix+12h),d	; dd cb 12 2a
	;sra	(ix+12h),e	; dd cb 12 2b
	;sra	(ix+12h),h	; dd cb 12 2c
	;sra	(ix+12h),l	; dd cb 12 2d
	sra	(ix+12h)	; dd cb 12 2e
	;sra	(ix+12h),a	; dd cb 12 2f
	;sll	(ix+12h),b	; dd cb 12 30
	;sll	(ix+12h),c	; dd cb 12 31
	;sll	(ix+12h),d	; dd cb 12 32
	;sll	(ix+12h),e	; dd cb 12 33
	;sll	(ix+12h),h	; dd cb 12 34
	;sll	(ix+12h),l	; dd cb 12 35
	;sll	(ix+12h)	; dd cb 12 36
	;sll	(ix+12h),a	; dd cb 12 37
	;srl	(ix+12h),b	; dd cb 12 38
	;srl	(ix+12h),c	; dd cb 12 39
	;srl	(ix+12h),d	; dd cb 12 3a
	;srl	(ix+12h),e	; dd cb 12 3b
	;srl	(ix+12h),h	; dd cb 12 3c
	;srl	(ix+12h),l	; dd cb 12 3d
	srl	(ix+12h)	; dd cb 12 3e
	;srl	(ix+12h),a	; dd cb 12 3f
	;bit	0,(ix+12h)	; dd cb 12 40
	;bit	0,(ix+12h)	; dd cb 12 41
	;bit	0,(ix+12h)	; dd cb 12 42
	;bit	0,(ix+12h)	; dd cb 12 43
	;bit	0,(ix+12h)	; dd cb 12 44
	;bit	0,(ix+12h)	; dd cb 12 45
	bit	0,(ix+12h)	; dd cb 12 46
	;bit	0,(ix+12h)	; dd cb 12 47
	;bit	1,(ix+12h)	; dd cb 12 48
	;bit	1,(ix+12h)	; dd cb 12 49
	;bit	1,(ix+12h)	; dd cb 12 4a
	;bit	1,(ix+12h)	; dd cb 12 4b
	;bit	1,(ix+12h)	; dd cb 12 4c
	;bit	1,(ix+12h)	; dd cb 12 4d
	bit	1,(ix+12h)	; dd cb 12 4e
	;bit	1,(ix+12h)	; dd cb 12 4f
	;bit	2,(ix+12h)	; dd cb 12 50
	;bit	2,(ix+12h)	; dd cb 12 51
	;bit	2,(ix+12h)	; dd cb 12 52
	;bit	2,(ix+12h)	; dd cb 12 53
	;bit	2,(ix+12h)	; dd cb 12 54
	;bit	2,(ix+12h)	; dd cb 12 55
	bit	2,(ix+12h)	; dd cb 12 56
	;bit	2,(ix+12h)	; dd cb 12 57
	;bit	3,(ix+12h)	; dd cb 12 58
	;bit	3,(ix+12h)	; dd cb 12 59
	;bit	3,(ix+12h)	; dd cb 12 5a
	;bit	3,(ix+12h)	; dd cb 12 5b
	;bit	3,(ix+12h)	; dd cb 12 5c
	;bit	3,(ix+12h)	; dd cb 12 5d
	bit	3,(ix+12h)	; dd cb 12 5e
	;bit	3,(ix+12h)	; dd cb 12 5f
	;bit	4,(ix+12h)	; dd cb 12 60
	;bit	4,(ix+12h)	; dd cb 12 61
	;bit	4,(ix+12h)	; dd cb 12 62
	;bit	4,(ix+12h)	; dd cb 12 63
	;bit	4,(ix+12h)	; dd cb 12 64
	;bit	4,(ix+12h)	; dd cb 12 65
	bit	4,(ix+12h)	; dd cb 12 66
	;bit	4,(ix+12h)	; dd cb 12 67
	;bit	5,(ix+12h)	; dd cb 12 68
	;bit	5,(ix+12h)	; dd cb 12 69
	;bit	5,(ix+12h)	; dd cb 12 6a
	;bit	5,(ix+12h)	; dd cb 12 6b
	;bit	5,(ix+12h)	; dd cb 12 6c
	;bit	5,(ix+12h)	; dd cb 12 6d
	bit	5,(ix+12h)	; dd cb 12 6e
	;bit	5,(ix+12h)	; dd cb 12 6f
	;bit	6,(ix+12h)	; dd cb 12 70
	;bit	6,(ix+12h)	; dd cb 12 71
	;bit	6,(ix+12h)	; dd cb 12 72
	;bit	6,(ix+12h)	; dd cb 12 73
	;bit	6,(ix+12h)	; dd cb 12 74
	;bit	6,(ix+12h)	; dd cb 12 75
	bit	6,(ix+12h)	; dd cb 12 76
	;bit	6,(ix+12h)	; dd cb 12 77
	;bit	7,(ix+12h)	; dd cb 12 78
	;bit	7,(ix+12h)	; dd cb 12 79
	;bit	7,(ix+12h)	; dd cb 12 7a
	;bit	7,(ix+12h)	; dd cb 12 7b
	;bit	7,(ix+12h)	; dd cb 12 7c
	;bit	7,(ix+12h)	; dd cb 12 7d
	bit	7,(ix+12h)	; dd cb 12 7e
	;bit	7,(ix+12h)	; dd cb 12 7f
	;res	0,(ix+12h),b	; dd cb 12 80
	;res	0,(ix+12h),c	; dd cb 12 81
	;res	0,(ix+12h),d	; dd cb 12 82
	;res	0,(ix+12h),e	; dd cb 12 83
	;res	0,(ix+12h),h	; dd cb 12 84
	;res	0,(ix+12h),l	; dd cb 12 85
	res	0,(ix+12h)  	; dd cb 12 86
	;res	0,(ix+12h),a	; dd cb 12 87
	;res	1,(ix+12h),b	; dd cb 12 88
	;res	1,(ix+12h),c	; dd cb 12 89
	;res	1,(ix+12h),d	; dd cb 12 8a
	;res	1,(ix+12h),e	; dd cb 12 8b
	;res	1,(ix+12h),h	; dd cb 12 8c
	;res	1,(ix+12h),l	; dd cb 12 8d
	res	1,(ix+12h)  	; dd cb 12 8e
	;res	1,(ix+12h),a	; dd cb 12 8f
	;res	2,(ix+12h),b	; dd cb 12 90
	;res	2,(ix+12h),c	; dd cb 12 91
	;res	2,(ix+12h),d	; dd cb 12 92
	;res	2,(ix+12h),e	; dd cb 12 93
	;res	2,(ix+12h),h	; dd cb 12 94
	;res	2,(ix+12h),l	; dd cb 12 95
	res	2,(ix+12h)  	; dd cb 12 96
	;res	2,(ix+12h),a	; dd cb 12 97
	;res	3,(ix+12h),b	; dd cb 12 98
	;res	3,(ix+12h),c	; dd cb 12 99
	;res	3,(ix+12h),d	; dd cb 12 9a
	;res	3,(ix+12h),e	; dd cb 12 9b
	;res	3,(ix+12h),h	; dd cb 12 9c
	;res	3,(ix+12h),l	; dd cb 12 9d
	res	3,(ix+12h)  	; dd cb 12 9e
	;res	3,(ix+12h),a	; dd cb 12 9f
	;res	4,(ix+12h),b	; dd cb 12 a0
	;res	4,(ix+12h),c	; dd cb 12 a1
	;res	4,(ix+12h),d	; dd cb 12 a2
	;res	4,(ix+12h),e	; dd cb 12 a3
	;res	4,(ix+12h),h	; dd cb 12 a4
	;res	4,(ix+12h),l	; dd cb 12 a5
	res	4,(ix+12h)  	; dd cb 12 a6
	;res	4,(ix+12h),a	; dd cb 12 a7
	;res	5,(ix+12h),b	; dd cb 12 a8
	;res	5,(ix+12h),c	; dd cb 12 a9
	;res	5,(ix+12h),d	; dd cb 12 aa
	;res	5,(ix+12h),e	; dd cb 12 ab
	;res	5,(ix+12h),h	; dd cb 12 ac
	;res	5,(ix+12h),l	; dd cb 12 ad
	res	5,(ix+12h)  	; dd cb 12 ae
	;res	5,(ix+12h),a	; dd cb 12 af
	;res	6,(ix+12h),b	; dd cb 12 b0
	;res	6,(ix+12h),c	; dd cb 12 b1
	;res	6,(ix+12h),d	; dd cb 12 b2
	;res	6,(ix+12h),e	; dd cb 12 b3
	;res	6,(ix+12h),h	; dd cb 12 b4
	;res	6,(ix+12h),l	; dd cb 12 b5
	res	6,(ix+12h)  	; dd cb 12 b6
	;res	6,(ix+12h),a	; dd cb 12 b7
	;res	7,(ix+12h),b	; dd cb 12 b8
	;res	7,(ix+12h),c	; dd cb 12 b9
	;res	7,(ix+12h),d	; dd cb 12 ba
	;res	7,(ix+12h),e	; dd cb 12 bb
	;res	7,(ix+12h),h	; dd cb 12 bc
	;res	7,(ix+12h),l	; dd cb 12 bd
	res	7,(ix+12h)  	; dd cb 12 be
	;res	7,(ix+12h),a	; dd cb 12 bf
	;set	0,(ix+12h),b	; dd cb 12 c0
	;set	0,(ix+12h),c	; dd cb 12 c1
	;set	0,(ix+12h),d	; dd cb 12 c2
	;set	0,(ix+12h),e	; dd cb 12 c3
	;set	0,(ix+12h),h	; dd cb 12 c4
	;set	0,(ix+12h),l	; dd cb 12 c5
	set	0,(ix+12h)  	; dd cb 12 c6
	;set	0,(ix+12h),a	; dd cb 12 c7
	;set	1,(ix+12h),b	; dd cb 12 c8
	;set	1,(ix+12h),c	; dd cb 12 c9
	;set	1,(ix+12h),d	; dd cb 12 ca
	;set	1,(ix+12h),e	; dd cb 12 cb
	;set	1,(ix+12h),h	; dd cb 12 cc
	;set	1,(ix+12h),l	; dd cb 12 cd
	set	1,(ix+12h)  	; dd cb 12 ce
	;set	1,(ix+12h),a	; dd cb 12 cf
	;set	2,(ix+12h),b	; dd cb 12 d0
	;set	2,(ix+12h),c	; dd cb 12 d1
	;set	2,(ix+12h),d	; dd cb 12 d2
	;set	2,(ix+12h),e	; dd cb 12 d3
	;set	2,(ix+12h),h	; dd cb 12 d4
	;set	2,(ix+12h),l	; dd cb 12 d5
	set	2,(ix+12h)  	; dd cb 12 d6
	;set	2,(ix+12h),a	; dd cb 12 d7
	;set	3,(ix+12h),b	; dd cb 12 d8
	;set	3,(ix+12h),c	; dd cb 12 d9
	;set	3,(ix+12h),d	; dd cb 12 da
	;set	3,(ix+12h),e	; dd cb 12 db
	;set	3,(ix+12h),h	; dd cb 12 dc
	;set	3,(ix+12h),l	; dd cb 12 dd
	set	3,(ix+12h)  	; dd cb 12 de
	;set	3,(ix+12h),a	; dd cb 12 df
	;set	4,(ix+12h),b	; dd cb 12 e0
	;set	4,(ix+12h),c	; dd cb 12 e1
	;set	4,(ix+12h),d	; dd cb 12 e2
	;set	4,(ix+12h),e	; dd cb 12 e3
	;set	4,(ix+12h),h	; dd cb 12 e4
	;set	4,(ix+12h),l	; dd cb 12 e5
	set	4,(ix+12h)  	; dd cb 12 e6
	;set	4,(ix+12h),a	; dd cb 12 e7
	;set	5,(ix+12h),b	; dd cb 12 e8
	;set	5,(ix+12h),c	; dd cb 12 e9
	;set	5,(ix+12h),d	; dd cb 12 ea
	;set	5,(ix+12h),e	; dd cb 12 eb
	;set	5,(ix+12h),h	; dd cb 12 ec
	;set	5,(ix+12h),l	; dd cb 12 ed
	set	5,(ix+12h)  	; dd cb 12 ee
	;set	5,(ix+12h),a	; dd cb 12 ef
	;set	6,(ix+12h),b	; dd cb 12 f0
	;set	6,(ix+12h),c	; dd cb 12 f1
	;set	6,(ix+12h),d	; dd cb 12 f2
	;set	6,(ix+12h),e	; dd cb 12 f3
	;set	6,(ix+12h),h	; dd cb 12 f4
	;set	6,(ix+12h),l	; dd cb 12 f5
	set	6,(ix+12h)  	; dd cb 12 f6
	;set	6,(ix+12h),a	; dd cb 12 f7
	;set	7,(ix+12h),b	; dd cb 12 f8
	;set	7,(ix+12h),c	; dd cb 12 f9
	;set	7,(ix+12h),d	; dd cb 12 fa
	;set	7,(ix+12h),e	; dd cb 12 fb
	;set	7,(ix+12h),h	; dd cb 12 fc
	;set	7,(ix+12h),l	; dd cb 12 fd
	set	7,(ix+12h)  	; dd cb 12 fe
	;set	7,(ix+12h),a	; dd cb 12 ff

	add	iy,bc		; fd 09
	add	iy,de		; fd 19
	ld	iy,1234h	; fd 21 34 12
	ld	(1234h),iy	; fd 22 34 12
	inc	iy		; fd 23
	;inc	iyh		; fd 24
	;dec	iyh		; fd 25
	;ld	iyh,12h		; fd 26 12
	add	iy,iy		; fd 29
	ld	iy,(1234h)	; fd 2a 34 12
	dec	iy		; fd 2b
	;inc	iyl		; fd 2c
	;dec	iyl		; fd 2d
	;ld	iyl,12h		; fd 2e 12
	inc	(iy+12h)	; fd 34 12
	dec	(iy+12h)	; fd 35 12
	ld	(iy+12h),34h	; fd 36 12 34
	add	iy,sp		; fd 39
	;ld	b,iyh		; fd 44
	;ld	b,iyl		; fd 45
	ld	b,(iy+12h)	; fd 46 12
	;ld	c,iyh		; fd 4c
	;ld	c,iyl		; fd 4d
	ld	c,(iy+12h)	; fd 4e 12
	;ld	d,iyh		; fd 54
	;ld	d,iyl		; fd 55
	ld	d,(iy+12h)	; fd 56 12
	;ld	e,iyh		; fd 5c
	;ld	e,iyl		; fd 5d
	ld	e,(iy+12h)	; fd 5e 12
	;ld	iyh,b		; fd 60
	;ld	iyh,c		; fd 61
	;ld	iyh,d		; fd 62
	;ld	iyh,e		; fd 63
	;ld	iyh,iyh		; fd 64
	;ld	iyh,iyl		; fd 65
	ld	h,(iy+12h)	; fd 66 12
	;ld	iyh,a		; fd 67
	;ld	iyl,b		; fd 68
	;ld	iyl,c		; fd 69
	;ld	iyl,d		; fd 6a
	;ld	iyl,e		; fd 6b
	;ld	iyl,iyh		; fd 6c
	;ld	iyl,iyl		; fd 6d
	ld	l,(iy+12h)	; fd 6e 12
	;ld	iyl,a		; fd 6f
	ld	(iy+12h),b	; fd 70 12
	ld	(iy+12h),c	; fd 71 12
	ld	(iy+12h),d	; fd 72 12
	ld	(iy+12h),e	; fd 73 12
	ld	(iy+12h),h	; fd 74 12
	ld	(iy+12h),l	; fd 75 12
	ld	(iy+12h),a	; fd 77 12
	;ld	a,iyh		; fd 7c
	;ld	a,iyl		; fd 7d
	ld	a,(iy+12h)	; fd 7e 12
	;add	a,iyh		; fd 84
	;add	a,iyl		; fd 85
	add	a,(iy+12h)	; fd 86 12
	;adc	a,iyh		; fd 8c
	;adc	a,iyl		; fd 8d
	adc	a,(iy+12h)	; fd 8e 12
	;sub	a,iyh		; fd 94
	;sub	a,iyl		; fd 95
	sub	a,(iy+12h)	; fd 96 12
	;sbc	a,iyh		; fd 9c
	;sbc	a,iyl		; fd 9d
	sbc	a,(iy+12h)	; fd 9e 12
	;and	a,iyh		; fd a4
	;and	a,iyl		; fd a5
	and	a,(iy+12h)	; fd a6 12
	;xor	a,iyh		; fd ac
	;xor	a,iyl		; fd ad
	xor	a,(iy+12h)	; fd ae 12
	;or	iyh		; fd b4
	;or	iyl		; fd b5
	or	(iy+12h)	; fd b6 12
	;cp	iyh		; fd bc
	;cp	iyl		; fd bd
	cp	(iy+12h)	; fd be 12
	pop	iy		; fd e1
	ex	(sp),iy		; fd e3
	push	iy		; fd e5
	jp	(iy)		; fd e9
	ld	sp,iy		; fd f9

	;rlc	(iy+12h),b	; fd cb 12 00
	;rlc	(iy+12h),c	; fd cb 12 01
	;rlc	(iy+12h),d	; fd cb 12 02
	;rlc	(iy+12h),e	; fd cb 12 03
	;rlc	(iy+12h),h	; fd cb 12 04
	;rlc	(iy+12h),l	; fd cb 12 05
	rlc	(iy+12h)	; fd cb 12 06
	;rlc	(iy+12h),a	; fd cb 12 07
	;rrc	(iy+12h),b	; fd cb 12 08
	;rrc	(iy+12h),c	; fd cb 12 09
	;rrc	(iy+12h),d	; fd cb 12 0a
	;rrc	(iy+12h),e	; fd cb 12 0b
	;rrc	(iy+12h),h	; fd cb 12 0c
	;rrc	(iy+12h),l	; fd cb 12 0d
	rrc	(iy+12h)	; fd cb 12 0e
	;rrc	(iy+12h),a	; fd cb 12 0f
	;rl	(iy+12h),b	; fd cb 12 10
	;rl	(iy+12h),c	; fd cb 12 11
	;rl	(iy+12h),d	; fd cb 12 12
	;rl	(iy+12h),e	; fd cb 12 13
	;rl	(iy+12h),h	; fd cb 12 14
	;rl	(iy+12h),l	; fd cb 12 15
	rl	(iy+12h)	; fd cb 12 16
	;rl	(iy+12h),a	; fd cb 12 17
	;rr	(iy+12h),b	; fd cb 12 18
	;rr	(iy+12h),c	; fd cb 12 19
	;rr	(iy+12h),d	; fd cb 12 1a
	;rr	(iy+12h),e	; fd cb 12 1b
	;rr	(iy+12h),h	; fd cb 12 1c
	;rr	(iy+12h),l	; fd cb 12 1d
	rr	(iy+12h)	; fd cb 12 1e
	;rr	(iy+12h),a	; fd cb 12 1f
	;sla	(iy+12h),b	; fd cb 12 20
	;sla	(iy+12h),c	; fd cb 12 21
	;sla	(iy+12h),d	; fd cb 12 22
	;sla	(iy+12h),e	; fd cb 12 23
	;sla	(iy+12h),h	; fd cb 12 24
	;sla	(iy+12h),l	; fd cb 12 25
	sla	(iy+12h)	; fd cb 12 26
	;sla	(iy+12h),a	; fd cb 12 27
	;sra	(iy+12h),b	; fd cb 12 28
	;sra	(iy+12h),c	; fd cb 12 29
	;sra	(iy+12h),d	; fd cb 12 2a
	;sra	(iy+12h),e	; fd cb 12 2b
	;sra	(iy+12h),h	; fd cb 12 2c
	;sra	(iy+12h),l	; fd cb 12 2d
	sra	(iy+12h)	; fd cb 12 2e
	;sra	(iy+12h),a	; fd cb 12 2f
	;sll	(iy+12h),b	; fd cb 12 30
	;sll	(iy+12h),c	; fd cb 12 31
	;sll	(iy+12h),d	; fd cb 12 32
	;sll	(iy+12h),e	; fd cb 12 33
	;sll	(iy+12h),h	; fd cb 12 34
	;sll	(iy+12h),l	; fd cb 12 35
	;sll	(iy+12h)	; fd cb 12 36
	;sll	(iy+12h),a	; fd cb 12 37
	;srl	(iy+12h),b	; fd cb 12 38
	;srl	(iy+12h),c	; fd cb 12 39
	;srl	(iy+12h),d	; fd cb 12 3a
	;srl	(iy+12h),e	; fd cb 12 3b
	;srl	(iy+12h),h	; fd cb 12 3c
	;srl	(iy+12h),l	; fd cb 12 3d
	srl	(iy+12h)	; fd cb 12 3e
	;srl	(iy+12h),a	; fd cb 12 3f
	;bit	0,(iy+12h)	; fd cb 12 40
	;bit	0,(iy+12h)	; fd cb 12 41
	;bit	0,(iy+12h)	; fd cb 12 42
	;bit	0,(iy+12h)	; fd cb 12 43
	;bit	0,(iy+12h)	; fd cb 12 44
	;bit	0,(iy+12h)	; fd cb 12 45
	bit	0,(iy+12h)	; fd cb 12 46
	;bit	0,(iy+12h)	; fd cb 12 47
	;bit	1,(iy+12h)	; fd cb 12 48
	;bit	1,(iy+12h)	; fd cb 12 49
	;bit	1,(iy+12h)	; fd cb 12 4a
	;bit	1,(iy+12h)	; fd cb 12 4b
	;bit	1,(iy+12h)	; fd cb 12 4c
	;bit	1,(iy+12h)	; fd cb 12 4d
	bit	1,(iy+12h)	; fd cb 12 4e
	;bit	1,(iy+12h)	; fd cb 12 4f
	;bit	2,(iy+12h)	; fd cb 12 50
	;bit	2,(iy+12h)	; fd cb 12 51
	;bit	2,(iy+12h)	; fd cb 12 52
	;bit	2,(iy+12h)	; fd cb 12 53
	;bit	2,(iy+12h)	; fd cb 12 54
	;bit	2,(iy+12h)	; fd cb 12 55
	bit	2,(iy+12h)	; fd cb 12 56
	;bit	2,(iy+12h)	; fd cb 12 57
	;bit	3,(iy+12h)	; fd cb 12 58
	;bit	3,(iy+12h)	; fd cb 12 59
	;bit	3,(iy+12h)	; fd cb 12 5a
	;bit	3,(iy+12h)	; fd cb 12 5b
	;bit	3,(iy+12h)	; fd cb 12 5c
	;bit	3,(iy+12h)	; fd cb 12 5d
	bit	3,(iy+12h)	; fd cb 12 5e
	;bit	3,(iy+12h)	; fd cb 12 5f
	;bit	4,(iy+12h)	; fd cb 12 60
	;bit	4,(iy+12h)	; fd cb 12 61
	;bit	4,(iy+12h)	; fd cb 12 62
	;bit	4,(iy+12h)	; fd cb 12 63
	;bit	4,(iy+12h)	; fd cb 12 64
	;bit	4,(iy+12h)	; fd cb 12 65
	bit	4,(iy+12h)	; fd cb 12 66
	;bit	4,(iy+12h)	; fd cb 12 67
	;bit	5,(iy+12h)	; fd cb 12 68
	;bit	5,(iy+12h)	; fd cb 12 69
	;bit	5,(iy+12h)	; fd cb 12 6a
	;bit	5,(iy+12h)	; fd cb 12 6b
	;bit	5,(iy+12h)	; fd cb 12 6c
	;bit	5,(iy+12h)	; fd cb 12 6d
	bit	5,(iy+12h)	; fd cb 12 6e
	;bit	5,(iy+12h)	; fd cb 12 6f
	;bit	6,(iy+12h)	; fd cb 12 70
	;bit	6,(iy+12h)	; fd cb 12 71
	;bit	6,(iy+12h)	; fd cb 12 72
	;bit	6,(iy+12h)	; fd cb 12 73
	;bit	6,(iy+12h)	; fd cb 12 74
	;bit	6,(iy+12h)	; fd cb 12 75
	bit	6,(iy+12h)	; fd cb 12 76
	;bit	6,(iy+12h)	; fd cb 12 77
	;bit	7,(iy+12h)	; fd cb 12 78
	;bit	7,(iy+12h)	; fd cb 12 79
	;bit	7,(iy+12h)	; fd cb 12 7a
	;bit	7,(iy+12h)	; fd cb 12 7b
	;bit	7,(iy+12h)	; fd cb 12 7c
	;bit	7,(iy+12h)	; fd cb 12 7d
	bit	7,(iy+12h)	; fd cb 12 7e
	;bit	7,(iy+12h)	; fd cb 12 7f
	;res	0,(iy+12h),b	; fd cb 12 80
	;res	0,(iy+12h),c	; fd cb 12 81
	;res	0,(iy+12h),d	; fd cb 12 82
	;res	0,(iy+12h),e	; fd cb 12 83
	;res	0,(iy+12h),h	; fd cb 12 84
	;res	0,(iy+12h),l	; fd cb 12 85
	res	0,(iy+12h)  	; fd cb 12 86
	;res	0,(iy+12h),a	; fd cb 12 87
	;res	1,(iy+12h),b	; fd cb 12 88
	;res	1,(iy+12h),c	; fd cb 12 89
	;res	1,(iy+12h),d	; fd cb 12 8a
	;res	1,(iy+12h),e	; fd cb 12 8b
	;res	1,(iy+12h),h	; fd cb 12 8c
	;res	1,(iy+12h),l	; fd cb 12 8d
	res	1,(iy+12h)  	; fd cb 12 8e
	;res	1,(iy+12h),a	; fd cb 12 8f
	;res	2,(iy+12h),b	; fd cb 12 90
	;res	2,(iy+12h),c	; fd cb 12 91
	;res	2,(iy+12h),d	; fd cb 12 92
	;res	2,(iy+12h),e	; fd cb 12 93
	;res	2,(iy+12h),h	; fd cb 12 94
	;res	2,(iy+12h),l	; fd cb 12 95
	res	2,(iy+12h)  	; fd cb 12 96
	;res	2,(iy+12h),a	; fd cb 12 97
	;res	3,(iy+12h),b	; fd cb 12 98
	;res	3,(iy+12h),c	; fd cb 12 99
	;res	3,(iy+12h),d	; fd cb 12 9a
	;res	3,(iy+12h),e	; fd cb 12 9b
	;res	3,(iy+12h),h	; fd cb 12 9c
	;res	3,(iy+12h),l	; fd cb 12 9d
	res	3,(iy+12h)  	; fd cb 12 9e
	;res	3,(iy+12h),a	; fd cb 12 9f
	;res	4,(iy+12h),b	; fd cb 12 a0
	;res	4,(iy+12h),c	; fd cb 12 a1
	;res	4,(iy+12h),d	; fd cb 12 a2
	;res	4,(iy+12h),e	; fd cb 12 a3
	;res	4,(iy+12h),h	; fd cb 12 a4
	;res	4,(iy+12h),l	; fd cb 12 a5
	res	4,(iy+12h)  	; fd cb 12 a6
	;res	4,(iy+12h),a	; fd cb 12 a7
	;res	5,(iy+12h),b	; fd cb 12 a8
	;res	5,(iy+12h),c	; fd cb 12 a9
	;res	5,(iy+12h),d	; fd cb 12 aa
	;res	5,(iy+12h),e	; fd cb 12 ab
	;res	5,(iy+12h),h	; fd cb 12 ac
	;res	5,(iy+12h),l	; fd cb 12 ad
	res	5,(iy+12h)  	; fd cb 12 ae
	;res	5,(iy+12h),a	; fd cb 12 af
	;res	6,(iy+12h),b	; fd cb 12 b0
	;res	6,(iy+12h),c	; fd cb 12 b1
	;res	6,(iy+12h),d	; fd cb 12 b2
	;res	6,(iy+12h),e	; fd cb 12 b3
	;res	6,(iy+12h),h	; fd cb 12 b4
	;res	6,(iy+12h),l	; fd cb 12 b5
	res	6,(iy+12h)  	; fd cb 12 b6
	;res	6,(iy+12h),a	; fd cb 12 b7
	;res	7,(iy+12h),b	; fd cb 12 b8
	;res	7,(iy+12h),c	; fd cb 12 b9
	;res	7,(iy+12h),d	; fd cb 12 ba
	;res	7,(iy+12h),e	; fd cb 12 bb
	;res	7,(iy+12h),h	; fd cb 12 bc
	;res	7,(iy+12h),l	; fd cb 12 bd
	res	7,(iy+12h)  	; fd cb 12 be
	;res	7,(iy+12h),a	; fd cb 12 bf
	;set	0,(iy+12h),b	; fd cb 12 c0
	;set	0,(iy+12h),c	; fd cb 12 c1
	;set	0,(iy+12h),d	; fd cb 12 c2
	;set	0,(iy+12h),e	; fd cb 12 c3
	;set	0,(iy+12h),h	; fd cb 12 c4
	;set	0,(iy+12h),l	; fd cb 12 c5
	set	0,(iy+12h)  	; fd cb 12 c6
	;set	0,(iy+12h),a	; fd cb 12 c7
	;set	1,(iy+12h),b	; fd cb 12 c8
	;set	1,(iy+12h),c	; fd cb 12 c9
	;set	1,(iy+12h),d	; fd cb 12 ca
	;set	1,(iy+12h),e	; fd cb 12 cb
	;set	1,(iy+12h),h	; fd cb 12 cc
	;set	1,(iy+12h),l	; fd cb 12 cd
	set	1,(iy+12h)  	; fd cb 12 ce
	;set	1,(iy+12h),a	; fd cb 12 cf
	;set	2,(iy+12h),b	; fd cb 12 d0
	;set	2,(iy+12h),c	; fd cb 12 d1
	;set	2,(iy+12h),d	; fd cb 12 d2
	;set	2,(iy+12h),e	; fd cb 12 d3
	;set	2,(iy+12h),h	; fd cb 12 d4
	;set	2,(iy+12h),l	; fd cb 12 d5
	set	2,(iy+12h)  	; fd cb 12 d6
	;set	2,(iy+12h),a	; fd cb 12 d7
	;set	3,(iy+12h),b	; fd cb 12 d8
	;set	3,(iy+12h),c	; fd cb 12 d9
	;set	3,(iy+12h),d	; fd cb 12 da
	;set	3,(iy+12h),e	; fd cb 12 db
	;set	3,(iy+12h),h	; fd cb 12 dc
	;set	3,(iy+12h),l	; fd cb 12 dd
	set	3,(iy+12h)  	; fd cb 12 de
	;set	3,(iy+12h),a	; fd cb 12 df
	;set	4,(iy+12h),b	; fd cb 12 e0
	;set	4,(iy+12h),c	; fd cb 12 e1
	;set	4,(iy+12h),d	; fd cb 12 e2
	;set	4,(iy+12h),e	; fd cb 12 e3
	;set	4,(iy+12h),h	; fd cb 12 e4
	;set	4,(iy+12h),l	; fd cb 12 e5
	set	4,(iy+12h)  	; fd cb 12 e6
	;set	4,(iy+12h),a	; fd cb 12 e7
	;set	5,(iy+12h),b	; fd cb 12 e8
	;set	5,(iy+12h),c	; fd cb 12 e9
	;set	5,(iy+12h),d	; fd cb 12 ea
	;set	5,(iy+12h),e	; fd cb 12 eb
	;set	5,(iy+12h),h	; fd cb 12 ec
	;set	5,(iy+12h),l	; fd cb 12 ed
	set	5,(iy+12h)  	; fd cb 12 ee
	;set	5,(iy+12h),a	; fd cb 12 ef
	;set	6,(iy+12h),b	; fd cb 12 f0
	;set	6,(iy+12h),c	; fd cb 12 f1
	;set	6,(iy+12h),d	; fd cb 12 f2
	;set	6,(iy+12h),e	; fd cb 12 f3
	;set	6,(iy+12h),h	; fd cb 12 f4
	;set	6,(iy+12h),l	; fd cb 12 f5
	set	6,(iy+12h)  	; fd cb 12 f6
	;set	6,(iy+12h),a	; fd cb 12 f7
	;set	7,(iy+12h),b	; fd cb 12 f8
	;set	7,(iy+12h),c	; fd cb 12 f9
	;set	7,(iy+12h),d	; fd cb 12 fa
	;set	7,(iy+12h),e	; fd cb 12 fb
	;set	7,(iy+12h),h	; fd cb 12 fc
	;set	7,(iy+12h),l	; fd cb 12 fd
	set	7,(iy+12h)  	; fd cb 12 fe
	;set	7,(iy+12h),a	; fd cb 12 ff

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
