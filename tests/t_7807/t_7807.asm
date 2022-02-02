	cpu	7807
	page	0
	assume	v:0feh

	mov	eah,a		; 18
	mov	eal,a		; 19
	mov	b,a		; 1a
	mov	c,a		; 1b
	mov	d,a		; 1c
	mov	e,a		; 1d
	mov	h,a		; 1e
	mov	l,a		; 1f

	mov	a,eah		; 08
	mov	a,eal		; 09
	mov	a,b		; 0a
	mov	a,c		; 0b
	mov	a,d		; 0c
	mov	a,e		; 0d
	mov	a,h		; 0e
	mov	a,l		; 0f

	mov	pa,a		; 4d c0
	mov	pb,a		; 4d c1
	mov	pc,a		; 4d c2
	mov	pd,a		; 4d c3
	mov	pf,a		; 4d c5
	mov	mkh,a		; 4d c6
	mov	mkl,a		; 4d c7
	mov	smh,a		; 4d c9
	mov	sml,a		; 4d ca
	mov	eom,a		; 4d cb
	mov	etmm,a		; 4d cc
	mov	tmm,a		; 4d cd
	mov	mm,a		; 4d d0
	mov	mcc,a		; 4d d1
	mov	ma,a		; 4d d2
	mov	mb,a		; 4d d3
	mov	mc,a		; 4d d4
	mov	mf,a		; 4d d7
	mov	txb,a		; 4d d8
	mov	tm0,a		; 4d da
	mov	tm1,a		; 4d db
	mov	wdm,a		; 4d e0
	mov	mt,a		; 4d e1

	mov	a,pa		; 4c c0
	mov	a,pb		; 4c c1
	mov	a,pc		; 4c c2
	mov	a,pd		; 4c c3
	mov	a,pf		; 4c c5
	mov	a,mkh		; 4c c6
	mov	a,mkl		; 4c c7
	mov	a,smh		; 4c c9
	mov	a,eom		; 4c cb
	mov	a,tmm		; 4c cd
	mov	a,rxb		; 4c d9
	mov	a,pt		; 4c ce
	mov	a,wdm		; 4c e0

	mov	v,1234h		; 70 68 34 12
	mov	a,1234h		; 70 69 34 12
	mov	b,1234h		; 70 6a 34 12
	mov	c,1234h		; 70 6b 34 12
	mov	d,1234h		; 70 6c 34 12
	mov	e,1234h		; 70 6d 34 12
	mov	h,1234h		; 70 6e 34 12
	mov	l,1234h		; 70 6f 34 12

	mov	1234h,v		; 70 78 34 12
	mov	1234h,a		; 70 79 34 12
	mov	1234h,b		; 70 7a 34 12
	mov	1234h,c		; 70 7b 34 12
	mov	1234h,d		; 70 7c 34 12
	mov	1234h,e		; 70 7d 34 12
	mov	1234h,h		; 70 7e 34 12
	mov	1234h,l		; 70 7f 34 12

	mvi	v,12h		; 68 12
	mvi	a,12h		; 69 12
	mvi	b,12h		; 6a 12
	mvi	c,12h		; 6b 12
	mvi	d,12h		; 6c 12
	mvi	e,12h		; 6d 12
	mvi	h,12h		; 6e 12
	mvi	l,12h		; 6f 12

	mvi	pa,12h		; 64 00 12
	mvi	pb,12h		; 64 01 12
	mvi	pc,12h		; 64 02 12
	mvi	pd,12h		; 64 03 12
	mvi	pf,12h		; 64 05 12
	mvi	mkh,12h		; 64 06 12
	mvi	mkl,12h		; 64 07 12
	mvi	smh,12h		; 64 81 12
	mvi	eom,12h		; 64 83 12
	mvi	tmm,12h		; 64 85 12

	mviw	0fe34h,12h	; 71 34 12

	mvix	b,12h		; 49 12
	mvix	d,12h		; 4a 12
	mvix	h,12h		; 4b 12 

	staw	0fe34h		; 63 34

	ldaw	0fe34h		; 01 34

	stax	b		; 39
	stax	d		; 3a
	stax	h		; 3b
	stax	d+		; 3c
	stax	h+		; 3d
	stax	d-		; 3e
	stax	h-		; 3f
	stax	d+12h		; bb
	stax	h+a		; bc
	stax	h+b		; bd
	stax	h+ea		; be
	stax	h+12h		; bf

	ldax	b		; 29
	ldax	d		; 2a
	ldax	h		; 2b
	ldax	d+		; 2c
	ldax	h+		; 2d
	ldax	d-		; 2e
	ldax	h-		; 2f
	ldax	d+12h		; ab 12
	ldax	h+a		; ac
	ldax	h+b		; ad
	ldax	h+ea		; ae
	ldax	h+12h		; af 12

	exx			; 48 af

	exa			; 48 ac

	exh			; 48 ae

	exr			; 48 ad

	block	d+		; 10
	block	d-		; 11

	dmov	b,ea		; b5
	dmov	d,ea		; b6
	dmov	h,ea		; b7

	dmov	ea,b		; a5
	dmov	ea,d		; a6
	dmov	ea,h		; a7

	dmov	etm0,ea		; 48 d2
	dmov	etm1,ea		; 48 d3

	dmov	ea,ecnt		; 48 c0
	dmov	ea,ecpt0	; 48 c1
	dmov	ea,ecpt1	; 48 c2

	sbcd	1234h		; 70 1e 34 12

	sded	1234h		; 70 2e 34 12

	shld	1234h		; 70 3e 34 12

	sspd	1234h		; 70 0e 34 12

	steax	d		; 48 92
	steax	h		; 48 93
	steax	d++		; 48 94
	steax	h++		; 48 95
	steax	d+12h		; 48 9b 12
	steax	h+a		; 48 9c
	steax	h+b		; 48 9d
	steax	h+ea		; 48 9e
	steax	h+12h		; 48 9f 12

	lbcd	1234h		; 70 1f 34 12

	lded	1234h		; 70 2f 34 12

	lhld	1234h		; 70 3f 34 12

	lspd	1234h		; 70 0f 34 12

	ldeax	d		; 48 82
	ldeax	h		; 48 83
	ldeax	d++		; 48 84
	ldeax	h++		; 48 85
	ldeax	d+12h		; 48 8b 12
	ldeax	h+a		; 48 8c
	ldeax	h+b		; 48 8d
	ldeax	h+ea		; 48 8e
	ldeax	h+12h		; 48 8f 12

	push	v		; b0
	push	b		; b1
	push	d		; b2
	push	h		; b3
	push	ea		; b4

	pop	v		; a0
	pop	b		; a1
	pop	d		; a2
	pop	h		; a3
	pop	ea		; a4

	lxi	sp,1234h	; 04 34 12
	lxi	b,1234h		; 14 34 12
	lxi	d,1234h		; 24 34 12
	lxi	h,1234h		; 34 34 12
	lxi	ea,1234h	; 44 34 12

	table			; 48 a8

	add	a,v		; 60 c0
	add	a,a		; 60 c1
	add	a,b		; 60 c2
	add	a,c		; 60 c3
	add	a,d		; 60 c4
	add	a,e		; 60 c5
	add	a,h		; 60 c6
	add	a,l		; 60 c7

	add	v,a		; 60 40
	add	a,a		; 60 41
	add	b,a		; 60 42
	add	c,a		; 60 43
	add	d,a		; 60 44
	add	e,a		; 60 45
	add	h,a		; 60 46
	add	l,a		; 60 47

	adc	a,v		; 60 d0
	adc	a,a		; 60 d1
	adc	a,b		; 60 d2
	adc	a,c		; 60 d3
	adc	a,d		; 60 d4
	adc	a,e		; 60 d5
	adc	a,h		; 60 d6
	adc	a,l		; 60 d7

	adc	v,a		; 60 50
	adc	a,a		; 60 51
	adc	b,a		; 60 52
	adc	c,a		; 60 53
	adc	d,a		; 60 54
	adc	e,a		; 60 55
	adc	h,a		; 60 56
	adc	l,a		; 60 57

	addnc	a,v		; 60 a0
	addnc	a,a		; 60 a1
	addnc	a,b		; 60 a2
	addnc	a,c		; 60 a3
	addnc	a,d		; 60 a4
	addnc	a,e		; 60 a5
	addnc	a,h		; 60 a6
	addnc	a,l		; 60 a7

	addnc	v,a		; 60 20
	addnc	a,a		; 60 21
	addnc	b,a		; 60 22
	addnc	c,a		; 60 23
	addnc	d,a		; 60 24
	addnc	e,a		; 60 25
	addnc	h,a		; 60 26
	addnc	l,a		; 60 27

	sub	a,v		; 60 e0
	sub	a,a		; 60 e1
	sub	a,b		; 60 e2
	sub	a,c		; 60 e3
	sub	a,d		; 60 e4
	sub	a,e		; 60 e5
	sub	a,h		; 60 e6
	sub	a,l		; 60 e7

	sub	v,a		; 60 60
	sub	a,a		; 60 61
	sub	b,a		; 60 62
	sub	c,a		; 60 63
	sub	d,a		; 60 64
	sub	e,a		; 60 65
	sub	h,a		; 60 66
	sub	l,a		; 60 67

	sbb	a,v		; 60 f0
	sbb	a,a		; 60 f1
	sbb	a,b		; 60 f2
	sbb	a,c		; 60 f3
	sbb	a,d		; 60 f4
	sbb	a,e		; 60 f5
	sbb	a,h		; 60 f6
	sbb	a,l		; 60 f7

	sbb	v,a		; 60 70
	sbb	a,a		; 60 71
	sbb	b,a		; 60 72
	sbb	c,a		; 60 73
	sbb	d,a		; 60 74
	sbb	e,a		; 60 75
	sbb	h,a		; 60 76
	sbb	l,a		; 60 77

	subnb	a,v		; 60 b0
	subnb	a,a		; 60 b1
	subnb	a,b		; 60 b2
	subnb	a,c		; 60 b3
	subnb	a,d		; 60 b4
	subnb	a,e		; 60 b5
	subnb	a,h		; 60 b6
	subnb	a,l		; 60 b7

	subnb	v,a		; 60 30
	subnb	a,a		; 60 31
	subnb	b,a		; 60 32
	subnb	c,a		; 60 33
	subnb	d,a		; 60 34
	subnb	e,a		; 60 35
	subnb	h,a		; 60 36
	subnb	l,a		; 60 37

	ana	a,v		; 60 88
	ana	a,a		; 60 89
	ana	a,b		; 60 8a
	ana	a,c		; 60 8b
	ana	a,d		; 60 8c
	ana	a,e		; 60 8d
	ana	a,h		; 60 8e
	ana	a,l		; 60 8f

	ana	v,a		; 60 08
	ana	a,a		; 60 09
	ana	b,a		; 60 0a
	ana	c,a		; 60 0b
	ana	d,a		; 60 0c
	ana	e,a		; 60 0d
	ana	h,a		; 60 0e
	ana	l,a		; 60 0f

	ora	a,v		; 60 98
	ora	a,a		; 60 99
	ora	a,b		; 60 9a
	ora	a,c		; 60 9b
	ora	a,d		; 60 9c
	ora	a,e		; 60 9d
	ora	a,h		; 60 9e
	ora	a,l		; 60 9f

	ora	v,a		; 60 18
	ora	a,a		; 60 19
	ora	b,a		; 60 1a
	ora	c,a		; 60 1b
	ora	d,a		; 60 1c
	ora	e,a		; 60 1d
	ora	h,a		; 60 1e
	ora	l,a		; 60 1f

	xra	a,v		; 60 90
	xra	a,a		; 60 91
	xra	a,b		; 60 92
	xra	a,c		; 60 93
	xra	a,d		; 60 94
	xra	a,e		; 60 95
	xra	a,h		; 60 96
	xra	a,l		; 60 97

	xra	v,a		; 60 10
	xra	a,a		; 60 11
	xra	b,a		; 60 12
	xra	c,a		; 60 13
	xra	d,a		; 60 14
	xra	e,a		; 60 15
	xra	h,a		; 60 16
	xra	l,a		; 60 17

	gta	a,v		; 60 a8
	gta	a,a		; 60 a9
	gta	a,b		; 60 aa
	gta	a,c		; 60 ab
	gta	a,d		; 60 ac
	gta	a,e		; 60 ad
	gta	a,h		; 60 ae
	gta	a,l		; 60 af

	gta	v,a		; 60 28
	gta	a,a		; 60 29
	gta	b,a		; 60 2a
	gta	c,a		; 60 2b
	gta	d,a		; 60 2c
	gta	e,a		; 60 2d
	gta	h,a		; 60 2e
	gta	l,a		; 60 2f

	lta	a,v		; 60 b8
	lta	a,a		; 60 b9
	lta	a,b		; 60 ba
	lta	a,c		; 60 bb
	lta	a,d		; 60 bc
	lta	a,e		; 60 bd
	lta	a,h		; 60 be
	lta	a,l		; 60 bf

	lta	v,a		; 60 38
	lta	a,a		; 60 39
	lta	b,a		; 60 3a
	lta	c,a		; 60 3b
	lta	d,a		; 60 3c
	lta	e,a		; 60 3d
	lta	h,a		; 60 3e
	lta	l,a		; 60 3f

	nea	a,v		; 60 e8
	nea	a,a		; 60 e9
	nea	a,b		; 60 ea
	nea	a,c		; 60 eb
	nea	a,d		; 60 ec
	nea	a,e		; 60 ed
	nea	a,h		; 60 ee
	nea	a,l		; 60 ef

	nea	v,a		; 60 68
	nea	a,a		; 60 69
	nea	b,a		; 60 6a
	nea	c,a		; 60 6b
	nea	d,a		; 60 6c
	nea	e,a		; 60 6d
	nea	h,a		; 60 6e
	nea	l,a		; 60 6f

	eqa	a,v		; 60 f8
	eqa	a,a		; 60 f9
	eqa	a,b		; 60 fa
	eqa	a,c		; 60 fb
	eqa	a,d		; 60 fc
	eqa	a,e		; 60 fd
	eqa	a,h		; 60 fe
	eqa	a,l		; 60 ff

	eqa	v,a		; 60 78
	eqa	a,a		; 60 79
	eqa	b,a		; 60 7a
	eqa	c,a		; 60 7b
	eqa	d,a		; 60 7c
	eqa	e,a		; 60 7d
	eqa	h,a		; 60 7e
	eqa	l,a		; 60 7f

	ona	a,v		; 60 c8
	ona	a,a		; 60 c9
	ona	a,b		; 60 ca
	ona	a,c		; 60 cb
	ona	a,d		; 60 cc
	ona	a,e		; 60 cd
	ona	a,h		; 60 ce
	ona	a,l		; 60 cf

	offa	a,v		; 60 d8
	offa	a,a		; 60 d9
	offa	a,b		; 60 da
	offa	a,c		; 60 db
	offa	a,d		; 60 dc
	offa	a,e		; 60 dd
	offa	a,h		; 60 de
	offa	a,l		; 60 df

	addx	b		; 70 c1
	addx	d		; 70 c2
	addx	h		; 70 c3
	addx	d+		; 70 c4
	addx	h+		; 70 c5
	addx	d-		; 70 c6
	addx	h-		; 70 c7

	adcx	b		; 70 d1
	adcx	d		; 70 d2
	adcx	h		; 70 d3
	adcx	d+		; 70 d4
	adcx	h+		; 70 d5
	adcx	d-		; 70 d6
	adcx	h-		; 70 d7

	addncx	b		; 70 a1
	addncx	d		; 70 a2
	addncx	h		; 70 a3
	addncx	d+		; 70 a4
	addncx	h+		; 70 a5
	addncx	d-		; 70 a6
	addncx	h-		; 70 a7

	subx	b		; 70 e1
	subx	d		; 70 e2
	subx	h		; 70 e3
	subx	d+		; 70 e4
	subx	h+		; 70 e5
	subx	d-		; 70 e6
	subx	h-		; 70 e7

	sbbx	b		; 70 f1
	sbbx	d		; 70 f2
	sbbx	h		; 70 f3
	sbbx	d+		; 70 f4
	sbbx	h+		; 70 f5
	sbbx	d-		; 70 f6
	sbbx	h-		; 70 f7

	subnbx	b		; 70 b1
	subnbx	d		; 70 b2
	subnbx	h		; 70 b3
	subnbx	d+		; 70 b4
	subnbx	h+		; 70 b5
	subnbx	d-		; 70 b6
	subnbx	h-		; 70 b7

	anax	b		; 70 89
	anax	d		; 70 8a
	anax	h		; 70 8b
	anax	d+		; 70 8c
	anax	h+		; 70 8d
	anax	d-		; 70 8e
	anax	h-		; 70 8f

	orax	b		; 70 99
	orax	d		; 70 9a
	orax	h		; 70 9b
	orax	d+		; 70 9c
	orax	h+		; 70 9d
	orax	d-		; 70 9e
	orax	h-		; 70 9f

	xrax	b		; 70 91
	xrax	d		; 70 92
	xrax	h		; 70 93
	xrax	d+		; 70 94
	xrax	h+		; 70 95
	xrax	d-		; 70 96
	xrax	h-		; 70 97

	gtax	b		; 70 a9
	gtax	d		; 70 aa
	gtax	h		; 70 ab
	gtax	d+		; 70 ac
	gtax	h+		; 70 ad
	gtax	d-		; 70 ae
	gtax	h-		; 70 af

	ltax	b		; 70 b9
	ltax	d		; 70 ba
	ltax	h		; 70 bb
	ltax	d+		; 70 bc
	ltax	h+		; 70 bd
	ltax	d-		; 70 be
	ltax	h-		; 70 bf

	neax	b		; 70 e9
	neax	d		; 70 ea
	neax	h		; 70 eb
	neax	d+		; 70 ec
	neax	h+		; 70 ed
	neax	d-		; 70 ee
	neax	h-		; 70 ef

	eqax	b		; 70 f9
	eqax	d		; 70 fa
	eqax	h		; 70 fb
	eqax	d+		; 70 fc
	eqax	h+		; 70 fd
	eqax	d-		; 70 fe
	eqax	h-		; 70 ff

	onax	b		; 70 c9
	onax	d		; 70 ca
	onax	h		; 70 cb
	onax	d+		; 70 cc
	onax	h+		; 70 cd
	onax	d-		; 70 ce
	onax	h-		; 70 cf

	offax	b		; 70 d9
	offax	d		; 70 da
	offax	h		; 70 db
	offax	d+		; 70 dc
	offax	h+		; 70 dd
	offax	d-		; 70 de
	offax	h-		; 70 df

	adi	a,12h		; 46 12
	adi	v,12h		; 74 40 12
	adi	>a,12h		; 74 41 12
	adi	b,12h		; 74 42 12
	adi	c,12h		; 74 43 12
	adi	d,12h		; 74 44 12
	adi	e,12h		; 74 45 12
	adi	h,12h		; 74 46 12
	adi	l,12h		; 74 47 12

	adi	pa,12h		; 64 40 12
	adi	pb,12h		; 64 41 12
	adi	pc,12h		; 64 42 12
	adi	pd,12h		; 64 43 12
	adi	pf,12h		; 64 45 12
	adi	mkh,12h		; 64 46 12
	adi	mkl,12h		; 64 47 12
	adi	smh,12h		; 64 c1 12
	adi	eom,12h		; 64 c3 12
	adi	tmm,12h		; 64 c5 12

	aci	a,12h		; 56 12
	aci	v,12h		; 74 50 12
	aci	>a,12h		; 74 51 12
	aci	b,12h		; 74 52 12
	aci	c,12h		; 74 53 12
	aci	d,12h		; 74 54 12
	aci	e,12h		; 74 55 12
	aci	h,12h		; 74 56 12
	aci	l,12h		; 74 57 12

	aci	pa,12h		; 64 50 12
	aci	pb,12h		; 64 51 12
	aci	pc,12h		; 64 52 12
	aci	pd,12h		; 64 53 12
	aci	pf,12h		; 64 55 12
	aci	mkh,12h		; 64 56 12
	aci	mkl,12h		; 64 57 12
	aci	smh,12h		; 64 d1 12
	aci	eom,12h		; 64 d3 12
	aci	tmm,12h		; 64 d5 12

	adinc	a,12h		; 26 12
	adinc	v,12h		; 74 20 12
	adinc	>a,12h		; 74 21 12
	adinc	b,12h		; 74 22 12
	adinc	c,12h		; 74 23 12
	adinc	d,12h		; 74 24 12
	adinc	e,12h		; 74 25 12
	adinc	h,12h		; 74 26 12
	adinc	l,12h		; 74 27 12

	adinc	pa,12h		; 64 20 12
	adinc	pb,12h		; 64 21 12
	adinc	pc,12h		; 64 22 12
	adinc	pd,12h		; 64 23 12
	adinc	pf,12h		; 64 25 12
	adinc	mkh,12h		; 64 26 12
	adinc	mkl,12h		; 64 27 12
	adinc	smh,12h		; 64 a1 12
	adinc	eom,12h		; 64 a3 12
	adinc	tmm,12h		; 64 a5 12

	sui	a,12h		; 66 12
	sui	v,12h		; 74 60 12
	sui	>a,12h		; 74 61 12
	sui	b,12h		; 74 62 12
	sui	c,12h		; 74 63 12
	sui	d,12h		; 74 64 12
	sui	e,12h		; 74 65 12
	sui	h,12h		; 74 66 12
	sui	l,12h		; 74 67 12

	sui	pa,12h		; 64 60 12
	sui	pb,12h		; 64 61 12
	sui	pc,12h		; 64 62 12
	sui	pd,12h		; 64 63 12
	sui	pf,12h		; 64 65 12
	sui	mkh,12h		; 64 66 12
	sui	mkl,12h		; 64 67 12
	sui	smh,12h		; 64 e1 12
	sui	eom,12h		; 64 e3 12
	sui	tmm,12h		; 64 e5 12

	sbi	a,12h		; 76 12
	sbi	v,12h		; 74 70 12
	sbi	>a,12h		; 74 71 12
	sbi	b,12h		; 74 72 12
	sbi	c,12h		; 74 73 12
	sbi	d,12h		; 74 74 12
	sbi	e,12h		; 74 75 12
	sbi	h,12h		; 74 76 12
	sbi	l,12h		; 74 77 12

	sbi	pa,12h		; 64 70 12
	sbi	pb,12h		; 64 71 12
	sbi	pc,12h		; 64 72 12
	sbi	pd,12h		; 64 73 12
	sbi	pf,12h		; 64 75 12
	sbi	mkh,12h		; 64 76 12
	sbi	mkl,12h		; 64 77 12
	sbi	smh,12h		; 64 f1 12
	sbi	eom,12h		; 64 f3 12
	sbi	tmm,12h		; 64 f5 12

	suinb	a,12h		; 36 12
	suinb	v,12h		; 74 30 12
	suinb	>a,12h		; 74 31 12
	suinb	b,12h		; 74 32 12
	suinb	c,12h		; 74 33 12
	suinb	d,12h		; 74 34 12
	suinb	e,12h		; 74 35 12
	suinb	h,12h		; 74 36 12
	suinb	l,12h		; 74 37 12

	suinb	pa,12h		; 64 30 12
	suinb	pb,12h		; 64 31 12
	suinb	pc,12h		; 64 32 12
	suinb	pd,12h		; 64 33 12
	suinb	pf,12h		; 64 35 12
	suinb	mkh,12h		; 64 36 12
	suinb	mkl,12h		; 64 37 12
	suinb	smh,12h		; 64 b1 12
	suinb	eom,12h		; 64 b3 12
	suinb	tmm,12h		; 64 b5 12

	ani	a,12h		; 07 12
	ani	v,12h		; 74 08 12
	ani	>a,12h		; 74 09 12
	ani	b,12h		; 74 0a 12
	ani	c,12h		; 74 0b 12
	ani	d,12h		; 74 0c 12
	ani	e,12h		; 74 0b 12
	ani	h,12h		; 74 0e 12
	ani	l,12h		; 74 0f 12

	ani	pa,12h		; 64 08 12
	ani	pb,12h		; 64 09 12
	ani	pc,12h		; 64 0a 12
	ani	pd,12h		; 64 0b 12
	ani	pf,12h		; 64 0c 12
	ani	mkh,12h		; 64 0d 12
	ani	mkl,12h		; 64 0e 12
	ani	smh,12h		; 64 89 12
	ani	eom,12h		; 64 8b 12
	ani	tmm,12h		; 64 8d 12

	ori	a,12h		; 17 12
	ori	v,12h		; 74 18 12
	ori	>a,12h		; 74 19 12
	ori	b,12h		; 74 1a 12
	ori	c,12h		; 74 1b 12
	ori	d,12h		; 74 1c 12
	ori	e,12h		; 74 1b 12
	ori	h,12h		; 74 1e 12
	ori	l,12h		; 74 1f 12

	ori	pa,12h		; 64 18 12
	ori	pb,12h		; 64 19 12
	ori	pc,12h		; 64 1a 12
	ori	pd,12h		; 64 1b 12
	ori	pf,12h		; 64 1c 12
	ori	mkh,12h		; 64 1d 12
	ori	mkl,12h		; 64 1e 12
	ori	smh,12h		; 64 99 12
	ori	eom,12h		; 64 9b 12
	ori	tmm,12h		; 64 9d 12

	xri	a,12h		; 16 12
	xri	v,12h		; 74 10 12
	xri	>a,12h		; 74 11 12
	xri	b,12h		; 74 12 12
	xri	c,12h		; 74 13 12
	xri	d,12h		; 74 14 12
	xri	e,12h		; 74 15 12
	xri	h,12h		; 74 16 12
	xri	l,12h		; 74 17 12

	xri	pa,12h		; 64 10 12
	xri	pb,12h		; 64 11 12
	xri	pc,12h		; 64 12 12
	xri	pd,12h		; 64 13 12
	xri	pf,12h		; 64 14 12
	xri	mkh,12h		; 64 15 12
	xri	mkl,12h		; 64 16 12
	xri	smh,12h		; 64 91 12
	xri	eom,12h		; 64 93 12
	xri	tmm,12h		; 64 95 12

	gti	a,12h		; 27 12
	gti	v,12h		; 74 28 12
	gti	>a,12h		; 74 29 12
	gti	b,12h		; 74 2a 12
	gti	c,12h		; 74 2b 12
	gti	d,12h		; 74 2c 12
	gti	e,12h		; 74 2d 12
	gti	h,12h		; 74 2e 12
	gti	l,12h		; 74 2f 12

	gti	pa,12h		; 64 28 12
	gti	pb,12h		; 64 29 12
	gti	pc,12h		; 64 2a 12
	gti	pd,12h		; 64 2b 12
	gti	pf,12h		; 64 2c 12
	gti	mkh,12h		; 64 2d 12
	gti	mkl,12h		; 64 2e 12
	gti	smh,12h		; 64 a9 12
	gti	eom,12h		; 64 ab 12
	gti	tmm,12h		; 64 ad 12

	lti	a,12h		; 37 12
	lti	v,12h		; 74 38 12
	lti	>a,12h		; 74 39 12
	lti	b,12h		; 74 3a 12
	lti	c,12h		; 74 3b 12
	lti	d,12h		; 74 3c 12
	lti	e,12h		; 74 3d 12
	lti	h,12h		; 74 3e 12
	lti	l,12h		; 74 3f 12

	lti	pa,12h		; 64 38 12
	lti	pb,12h		; 64 39 12
	lti	pc,12h		; 64 3a 12
	lti	pd,12h		; 64 3b 12
	lti	pf,12h		; 64 3c 12
	lti	mkh,12h		; 64 3d 12
	lti	mkl,12h		; 64 3e 12
	lti	smh,12h		; 64 b9 12
	lti	eom,12h		; 64 bb 12
	lti	tmm,12h		; 64 bd 12

	nei	a,12h		; 67 12
	nei	v,12h		; 74 68 12
	nei	>a,12h		; 74 69 12
	nei	b,12h		; 74 6a 12
	nei	c,12h		; 74 6b 12
	nei	d,12h		; 74 6c 12
	nei	e,12h		; 74 6d 12
	nei	h,12h		; 74 6e 12
	nei	l,12h		; 74 6f 12

	nei	pa,12h		; 64 68 12
	nei	pb,12h		; 64 69 12
	nei	pc,12h		; 64 6a 12
	nei	pd,12h		; 64 6b 12
	nei	pf,12h		; 64 6c 12
	nei	mkh,12h		; 64 6d 12
	nei	mkl,12h		; 64 6e 12
	nei	smh,12h		; 64 e9 12
	nei	eom,12h		; 64 eb 12
	nei	tmm,12h		; 64 ed 12

	eqi	a,12h		; 77 12
	eqi	v,12h		; 74 78 12
	eqi	>a,12h		; 74 79 12
	eqi	b,12h		; 74 7a 12
	eqi	c,12h		; 74 7b 12
	eqi	d,12h		; 74 7c 12
	eqi	e,12h		; 74 7d 12
	eqi	h,12h		; 74 7e 12
	eqi	l,12h		; 74 7f 12

	eqi	pa,12h		; 64 78 12
	eqi	pb,12h		; 64 79 12
	eqi	pc,12h		; 64 7a 12
	eqi	pd,12h		; 64 7b 12
	eqi	pf,12h		; 64 7c 12
	eqi	mkh,12h		; 64 7d 12
	eqi	mkl,12h		; 64 7e 12
	eqi	smh,12h		; 64 f9 12
	eqi	eom,12h		; 64 fb 12
	eqi	tmm,12h		; 64 fd 12

	oni	a,12h		; 47 12
	oni	v,12h		; 74 48 12
	oni	>a,12h		; 74 49 12
	oni	b,12h		; 74 4a 12
	oni	c,12h		; 74 4b 12
	oni	d,12h		; 74 4c 12
	oni	e,12h		; 74 4d 12
	oni	h,12h		; 74 4e 12
	oni	l,12h		; 74 4f 12

	oni	pa,12h		; 64 48 12
	oni	pb,12h		; 64 49 12
	oni	pc,12h		; 64 4a 12
	oni	pd,12h		; 64 4b 12
	oni	pf,12h		; 64 4c 12
	oni	mkh,12h		; 64 4d 12
	oni	mkl,12h		; 64 4e 12
	oni	smh,12h		; 64 c9 12
	oni	eom,12h		; 64 cb 12
	oni	tmm,12h		; 64 cd 12

	offi	a,12h		; 57 12
	offi	v,12h		; 74 58 12
	offi	>a,12h		; 74 59 12
	offi	b,12h		; 74 5a 12
	offi	c,12h		; 74 5b 12
	offi	d,12h		; 74 5c 12
	offi	e,12h		; 74 5d 12
	offi	h,12h		; 74 5e 12
	offi	l,12h		; 74 5f 12

	offi	pa,12h		; 64 58 12
	offi	pb,12h		; 64 59 12
	offi	pc,12h		; 64 5a 12
	offi	pd,12h		; 64 5b 12
	offi	pf,12h		; 64 5c 12
	offi	mkh,12h		; 64 5d 12
	offi	mkl,12h		; 64 5e 12
	offi	smh,12h		; 64 d9 12
	offi	eom,12h		; 64 db 12
	offi	tmm,12h		; 64 dd 12

	addw	0fe12h		; 74 c0 12

	adcw	0fe12h		; 74 d0 12

	addncw	0fe12h		; 74 a0 12

	subw	0fe12h		; 74 e0 12

	sbbw	0fe12h		; 74 f0 12

	subnbw	0fe12h		; 74 b0 12

	anaw	0fe12h		; 74 88 12

	oraw	0fe12h		; 74 98 12

	xraw	0fe12h		; 74 90 12

	gtaw	0fe12h		; 74 a8 12

	ltaw	0fe12h		; 74 b8 12

	neaw	0fe12h		; 74 e8 12

	eqaw	0fe12h		; 74 f8 12

	onaw	0fe12h		; 74 c8 12

	offaw	0fe12h		; 74 d8 12

	aniw	0fe12h,34h	; 05 12 34

	oriw	0fe12h,34h	; 15 12 34

	gtiw	0fe12h,34h	; 25 12 34

	ltiw	0fe12h,34h	; 35 12 34

	neiw	0fe12h,34h	; 65 12 34

	eqiw	0fe12h,34h	; 75 12 34

	oniw	0fe12h,34h	; 45 12 34
	
	offiw	0fe12h,34h	; 55 12 34

	eadd	ea,a		; 70 41
	eadd	ea,b		; 70 42
	eadd	ea,c		; 70 43

	dadd	ea,b		; 74 c5
	dadd	ea,d		; 74 c6
	dadd	ea,h		; 74 c7

	dadc	ea,b		; 74 d5
	dadc	ea,d		; 74 d6
	dadc	ea,h		; 74 d7

	daddnc	ea,b		; 74 a5
	daddnc	ea,d		; 74 a6
	daddnc	ea,h		; 74 a7

	esub	ea,a		; 70 61
	esub	ea,b		; 70 62
	esub	ea,c		; 70 63

	dsub	ea,b		; 74 e5
	dsub	ea,d		; 74 e6
	dsub	ea,h		; 74 e7

	dsbb	ea,b		; 74 f5
	dsbb	ea,d		; 74 f6
	dsbb	ea,h		; 74 f7

	dsubnb	ea,b		; 74 b5
	dsubnb	ea,d		; 74 b6
	dsubnb	ea,h		; 74 b7

	dan	ea,b		; 74 8d
	dan	ea,d		; 74 8e
	dan	ea,h		; 74 8f

	dor	ea,b		; 74 9d
	dor	ea,d		; 74 9e
	dor	ea,h		; 74 9f

	dxr	ea,b		; 74 95
	dxr	ea,d		; 74 96
	dxr	ea,h		; 74 97

	dgt	ea,b		; 74 ad
	dgt	ea,d		; 74 ae
	dgt	ea,h		; 74 af

	dlt	ea,b		; 74 bd
	dlt	ea,d		; 74 be
	dlt	ea,h		; 74 bf

	dne	ea,b		; 74 ed
	dne	ea,d		; 74 ee
	dne	ea,h		; 74 ef

	deq	ea,b		; 74 fd
	deq	ea,d		; 74 fe
	deq	ea,h		; 74 ff

	don	ea,b		; 74 cd
	don	ea,d		; 74 ce
	don	ea,h		; 74 cf

	doff	ea,b		; 74 dd
	doff	ea,d		; 74 de
	doff	ea,h		; 74 df

	mul	a		; 48 2d
	mul	b		; 48 2e
	mul	c		; 48 2f

	div	a		; 48 3d
	div	b		; 48 3e
	div	c		; 48 3f

	inr	a		; 41
	inr	b		; 42
	inr	c		; 43

	inrw	0fe12h		; 20 12

	inx	sp		; 02
	inx	b		; 12
	inx	d		; 22
	inx	h		; 32
	inx	ea		; a8

	dcr	a		; 51
	dcr	b		; 52
	dcr	c		; 53

	dcrw	0fe12h		; 30 12

	dcx	sp		; 03
	dcx	b		; 13
	dcx	d		; 23
	dcx	h		; 33
	dcx	ea		; a9

	daa			; 61

	stc			; 48 2b
	clc			; 48 2a
	cmc			; 48 aa

	nega			; 48 3a

	rld			; 48 38

	rrd			; 48 39

	ral			; 48 35
	rll	a		; 48 35
	rll	b		; 48 36
	rll	c		; 48 37

	rar			; 48 31
	rlr	a		; 48 31
	rlr	b		; 48 32
	rlr	c		; 48 33

	sll	a		; 48 25
	sll	b		; 48 26
	sll	c		; 48 27

	slr	a		; 48 21
	slr	b		; 48 22
	slr	c		; 48 23

	sllc	a		; 48 05
	sllc	b		; 48 06
	sllc	c		; 48 07

	slrc	a		; 48 01
	slrc	b		; 48 02
	slrc	c		; 48 03

	drll	ea		; 48 b4

	drlr	ea		; 48 b0

	dsll	ea		; 48 a4

	dslr	ea		; 48 a0

	jmp	1234h		; 54 34 12

	jb			; 21

	jr	$+13h		; d2

	jre	$+14h		; 4e 12

	jea			; 48 28

	call	1234h		; 40 34 12

	calb			; 48 28

	calf	0812h		; 78 12

	calt	92h		; 89

	softi			; 72

	ret			; b8

	rets			; b9

	reti			; 62

membit	defbit	0fe02h.2
pa0	defbit	pa.0
pb1	defbit	pb.1
pc2	defbit	pc.2
pd3	defbit	pd.3
pf4	defbit	pf.4
mkh5	defbit	mkh.5
mkl6	defbit	mkl.6
eom7	defbit	eom.7
tmm0	defbit	tmm.0
pt1	defbit	pt.1

	mov	cy,0fe02h.2	; 5f 12
	mov	cy,pa.0		; 5f 80
	mov	cy,pb.1		; 5f 89
	mov	cy,pc.2		; 5f 92
	mov	cy,pd.3		; 5f 9b
	mov	cy,pf.4		; 5f ac
	mov	cy,mkh.5	; 5f b5
	mov	cy,mkl.6	; 5f be
	mov	cy,eom.7	; 5f df
	mov	cy,tmm.0	; 5f e8
	mov	cy,pt.1		; 5f f1

	mov	membit,cy	; 5a 12
	mov	pa0,cy		; 5a 80
	mov	pb1,cy		; 5a 89
	mov	pc2,cy		; 5a 92
	mov	pd3,cy		; 5a 9b
	mov	pf4,cy		; 5a ac
	mov	mkh5,cy		; 5a b5
	mov	mkl6,cy		; 5a be
	mov	eom7,cy		; 5a df
	mov	tmm0,cy		; 5a e8
	mov	pt1,cy		; 5a f1

	and	cy,0fe02h.2	; 31 12
	and	cy,pa.0		; 31 80
	and	cy,pb.1		; 31 89
	and	cy,pc.2		; 31 92
	and	cy,pd.3		; 31 9b
	and	cy,pf.4		; 31 ac
	and	cy,mkh.5	; 31 b5
	and	cy,mkl.6	; 31 be
	and	cy,eom.7	; 31 df
	and	cy,tmm.0	; 31 e8
	and	cy,pt.1		; 31 f1

	or	cy,0fe02h.2	; 5c 12
	or	cy,pa.0		; 5c 80
	or	cy,pb.1		; 5c 89
	or	cy,pc.2		; 5c 92
	or	cy,pd.3		; 5c 9b
	or	cy,pf.4		; 5c ac
	or	cy,mkh.5	; 5c b5
	or	cy,mkl.6	; 5c be
	or	cy,eom.7	; 5c df
	or	cy,tmm.0	; 5c e8
	or	cy,pt.1		; 5c f1

	xor	cy,0fe02h.2	; 5e 12
	xor	cy,pa.0		; 5e 80
	xor	cy,pb.1		; 5e 89
	xor	cy,pc.2		; 5e 92
	xor	cy,pd.3		; 5e 9b
	xor	cy,pf.4		; 5e ac
	xor	cy,mkh.5	; 5e b5
	xor	cy,mkl.6	; 5e be
	xor	cy,eom.7	; 5e df
	xor	cy,tmm.0	; 5e e8
	xor	cy,pt.1		; 5e f1

	setb	0fe02h.2	; 58 12
	setb	pa.0		; 58 80
	setb	pb.1		; 58 89
	setb	pc.2		; 58 92
	setb	pd.3		; 58 9b
	setb	pf.4		; 58 ac
	setb	mkh.5		; 58 b5
	setb	mkl.6		; 58 be
	setb	eom.7		; 58 df
	setb	tmm.0		; 58 e8
	setb	pt.1		; 58 f1

	clr	0fe02h.2	; 5b 12
	clr	pa.0		; 5b 80
	clr	pb.1		; 5b 89
	clr	pc.2		; 5b 92
	clr	pd.3		; 5b 9b
	clr	pf.4		; 5b ac
	clr	mkh.5		; 5b b5
	clr	mkl.6		; 5b be
	clr	eom.7		; 5b df
	clr	tmm.0		; 5b e8
	clr	pt.1		; 5b f1

	not	0fe02h.2	; 59 12
	not	pa.0		; 59 80
	not	pb.1		; 59 89
	not	pc.2		; 59 92
	not	pd.3		; 59 9b
	not	pf.4		; 59 ac
	not	mkh.5		; 59 b5
	not	mkl.6		; 59 be
	not	eom.7		; 59 df
	not	tmm.0		; 59 e8
	not	pt.1		; 59 f1

	sk	0fe02h.2	; 5d 12
	sk	pa.0		; 5d 80
	sk	pb.1		; 5d 89
	sk	pc.2		; 5d 92
	sk	pd.3		; 5d 9b
	sk	pf.4		; 5d ac
	sk	mkh.5		; 5d b5
	sk	mkl.6		; 5d be
	sk	eom.7		; 5d df
	sk	tmm.0		; 5d e8
	sk	pt.1		; 5d f1

	skn	0fe02h.2	; 50 12
	skn	pa.0		; 50 80
	skn	pb.1		; 50 89
	skn	pc.2		; 50 92
	skn	pd.3		; 50 9b
	skn	pf.4		; 50 ac
	skn	mkh.5		; 50 b5
	skn	mkl.6		; 50 be
	skn	eom.7		; 50 df
	skn	tmm.0		; 50 e8
	skn	pt.1		; 50 f1

	sk	cy		; 48 0a
	sk	hc		; 48 0b
	sk	z		; 48 0c

	skn	cy		; 48 18
	skn	hc		; 48 1b
	skn	z		; 48 1c

	skit	nmi		; 48 40
	skit	ft0		; 48 41
	skit	ft1		; 48 42
	skit	f1		; 48 43
	skit	f2		; 48 44
	skit	fe0		; 48 45
	skit	fe1		; 48 46
	skit	fein		; 48 47
	skit	fsr		; 48 49
	skit	fst		; 48 4a
	skit	er		; 48 4b
	skit	ov		; 48 4c
	;skit	ife2		; 48 ??
	skit	sb		; 48 54

	sknit	nmi		; 48 60
	sknit	ft0		; 48 61
	sknit	ft1		; 48 62
	sknit	f1		; 48 63
	sknit	f2		; 48 64
	sknit	fe0		; 48 65
	sknit	fe1		; 48 66
	sknit	fein		; 48 67
	sknit	fsr		; 48 69
	sknit	fst		; 48 6a
	sknit	er		; 48 6b
	sknit	ov		; 48 6c
	;sknit	ife2		; 48 ??
	sknit	sb		; 48 74

	nop			; 00

	ei			; aa

	di			; ba

	hlt			; 48 3b
