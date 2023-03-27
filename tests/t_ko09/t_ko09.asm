	cpu	052001
	page	0

	leax	$12,x		; 08 24 12
	leay	$12,x		; 09 24 12
	leau	$12,x		; 0a 24 12
	leas	$12,x		; 0b 24 12
	pshs	a,x		; 0c 12
	pshu	a,x		; 0d 12
	puls	a,x		; 0e 12
	pulu	a,x		; 0f 12
	lda	#$12		; 10 12
	ldb	#$12		; 11 12
	lda	$12,x		; 12 24 12
	ldb	$12,x		; 13 24 12
	adda	#$12		; 14 12
	addb	#$12		; 15 12
	adda	$12,x		; 16 24 12
	addb	$12,x		; 17 24 12
	adca	#$12		; 18 12
	adcb	#$12		; 19 12
	adca	$12,x		; 1a 24 12
	adcb	$12,x		; 1b 24 12
	suba	#$12		; 1c 12
	subb	#$12		; 1d 12
	suba	$12,x		; 1e 24 12
	subb	$12,x		; 1f 24 12
	sbca	#$12		; 20 12
	sbcb	#$12		; 21 12
	sbca	$12,x		; 22 24 12
	sbcb	$12,x		; 23 24 12
	anda	#$12		; 24 12
	andb	#$12		; 25 12
	anda	$12,x		; 26 24 12
	andb	$12,x		; 27 24 12
	bita	#$12		; 28 12
	bitb	#$12		; 29 12
	bita	$12,x		; 2a 24 12
	bitb	$12,x		; 2b 24 12
	eora	#$12		; 2c 12
	eorb	#$12		; 2d 12
	eora	$12,x		; 2e 24 12
	eorb	$12,x		; 2f 24 12
	ora	#$12		; 30 12
	orb	#$12		; 31 12
	ora	$12,x		; 32 24 12
	orb	$12,x		; 33 24 12
	cmpa	#$12		; 34 12
	cmpb	#$12		; 35 12
	cmpa	$12,x		; 36 24 12
	cmpb	$12,x		; 37 24 12
	setlines #$12		; 38 12
	setlines $12,x		; 39 24 12
	sta	$12,x		; 3a 24 12
	stb	$12,x		; 3b 24 12
	andcc	#$12		; 3c 12
	orcc	#$12		; 3d 12
	exg	x,y		; 3e 23
	tfr	a,b		; 3f 89
	ldd	#$1234		; 40 12 34
	ldd	$12,x		; 41 24 12
	ldx	#$1234		; 42 12 34
	ldx	$12,x		; 43 24 12
	ldy	#$1234		; 44 12 34
	ldy	$12,x		; 45 24 12
	ldu	#$1234		; 46 12 34
	ldu	$12,x		; 47 24 12
	lds	#$1234		; 48 12 34
	lds	$12,x		; 49 24 12
	cmpd	#$1234		; 4a 12 34
	cmpd	$12,x		; 4b 24 12
	cmpx	#$1234		; 4c 12 34
	cmpx	$12,x		; 4d 24 12
	cmpy	#$1234		; 4e 12 34
	cmpy	$12,x		; 4f 24 12
	cmpu	#$1234		; 50 12 34
	cmpu	$12,x		; 51 24 12
	cmps	#$1234		; 52 12 34
	cmps	$12,x		; 53 24 12
	addd	#$1234		; 54 12 34
	addd	$12,x		; 55 24 12
	subd	#$1234		; 56 12 34
	subd	$12,x		; 57 24 12
	std	$12,x		; 58 24 12
	stx	$12,x		; 59 24 12
	sty	$12,x		; 5a 24 12
	stu	$12,x		; 5b 24 12
	sts	$12,x		; 5c 24 12
				; 5d
				; 5e
				; 5f
	bra	*+$12		; 60 10
	bhi	*+$12		; 61 10
	bcc	*+$12		; 62 10
	bne	*+$12		; 63 10
	bvc	*+$12		; 64 10
	bpl	*+$12		; 65 10
	bge	*+$12		; 66 10
	bgt	*+$12		; 67 10
	lbra	*+$1234		; 68 12 31
	lbhi	*+$1234		; 69 12 31
	lbcc	*+$1234		; 6a 12 31
	lbne	*+$1234		; 6b 12 31
	lbvc	*+$1234		; 6c 12 31
	lbpl	*+$1234		; 6d 12 31
	lbge	*+$1234		; 6e 12 31
	lbgt	*+$1234		; 6f 12 31
	brn	*+$12		; 70 10
	bls	*+$12		; 71 10
	bcs	*+$12		; 72 10
	beq	*+$12		; 73 10
	bvs	*+$12		; 74 10
	bmi	*+$12		; 75 10
	blt	*+$12		; 76 10
	ble	*+$12		; 77 10
	lbrn	*+$1234		; 78 12 31
	lbls	*+$1234		; 79 12 31
	lbcs	*+$1234		; 7a 12 31
	lbeq	*+$1234		; 7b 12 31
	lbvs	*+$1234		; 7c 12 31
	lbmi	*+$1234		; 7d 12 31
	lblt	*+$1234		; 7e 12 31
	lble	*+$1234		; 7f 12 31
	clra			; 80
	clrb			; 81
	clr	$12,x		; 82 24 12
	coma			; 83
	comb			; 84
	com	$12,x		; 85 24 12
	nega			; 86
	negb			; 87
	neg	$12,x		; 88 24 12
	inca			; 89
	inca			; 8a
	inc	$12,x		; 8b 24 12
	deca			; 8c
	decb			; 8d
	dec	$12,x		; 8e 24 12
	rts			; 8f
	tsta			; 90
	tstb			; 91
	tst	$12,x		; 92 24 12
	lsra			; 93
	lsrb			; 94
	lsr	$12,x		; 95 24 12
	rora			; 96
	rorb			; 97
	ror	$12,x		; 98 24 12
	asra			; 99
	asrb			; 9a
	asr	$12,x		; 9b 24 12
	asla			; 9c
	aslb			; 9d
	asl	$12,x		; 9e 24 12
	rti			; 9f
	rola			; a0
	rolb			; a1
	rol	$12,x		; a2 24 12
	lsrw	$12,x		; a3 24 12
	rorw	$12,x		; a4 24 12
	asrw	$12,x		; a5 24 12
	aslw	$12,x		; a6 24 12
	rolw	$12,x		; a7 24 12
	jmp	$12,x		; a8 24 12
	jsr	$12,x		; a9 24 12
	bsr	*+$12		; aa 10
	lbsr	*+$1234		; ab 12 31
	dec	b,jnz,*+$12	; ac 10
	dec	x,jnz,*+$12	; ad 10
	nop			; ae
				; af
	abx			; b0
	daa			; b1
	sex			; b2
	mul			; b3
	lmul			; b4
	div	x,b		; b5
	bmove	y,x,u		; b6
	move	y,x,u		; b7
	lsrd	#$12		; b8 12
	lsrd	$12,x		; b9 24 12
	rord	#$12		; ba 12
	rord	$12,x		; bb 24 12
	asrd	#$12		; bc 12
	asrd	$12,x		; bd 24 12
	asld	#$12		; be 12
	asld	$12,x		; bf 24 12
	rold	#$12		; c0 12
	rold	$12,x		; c1 24 12
	clrd			; c2
	clrw	$12,x		; c3 24 12
	negd			; c4
	negw	$12,x		; c5 24 12
	incd			; c6
	incw	$12,x		; c7 24 12
	decd			; c8
	decw	$12,x		; c9 24 12
	tstd			; ca
	tstw	$12,x		; cb 24 12
	absa			; cc
	absb			; cd
	absd			; ce
	bseta	x,u		; cf
	bsetd	x,u		; d0

	; iterate through addressing modes

	lda	$1234		; 12 07 12 34
	lda	[$1234]		; 12 0f 12 34
	lda	>$1234		; 12 07 12 34
	lda	[>$1234]	; 12 0f 12 34
	lda	>$12		; 12 07 00 12
	lda	[>$12]		; 12 0f 00 12
	lda	,x+		; 12 20
	lda	[,x+]		; 12 28
	lda	,y+		; 12 30
	lda	[,y+]		; 12 38
	lda	,u+		; 12 50
	lda	[,u+]		; 12 58
	lda	,s+		; 12 60
	lda	[,s+]		; 12 68
	lda	,pc+		; 12 70
	lda	[,pc+]		; 12 78
	lda	,x++		; 12 21
	lda	[,x++]		; 12 29
	lda	,y++		; 12 31
	lda	[,y++]		; 12 39
	lda	,u++		; 12 51
	lda	[,u++]		; 12 59
	lda	,s++		; 12 61
	lda	[,s++]		; 12 69
	lda	,pc++		; 12 71
	lda	[,pc++]		; 12 79
	lda	,-x		; 12 22
	lda	[,-x]		; 12 2a
	lda	,-y		; 12 32
	lda	[,-y]		; 12 3a
	lda	,-u		; 12 52
	lda	[,-u]		; 12 5a
	lda	,-s		; 12 62
	lda	[,-s]		; 12 6a
	lda	,-pc		; 12 72
	lda	[,-pc]		; 12 7a
	lda	,--x		; 12 23
	lda	[,--x]		; 12 2b
	lda	,--y		; 12 33
	lda	[,--y]		; 12 3b
	lda	,--u		; 12 53
	lda	[,--u]		; 12 5b
	lda	,--s		; 12 63
	lda	[,--s]		; 12 6b
	lda	,--pc		; 12 73
	lda	[,--pc]		; 12 7b
	lda	$12,x		; 12 24 12
	lda	<$12,x		; 12 24 12
	lda	[$12,x]		; 12 2c 12
	lda	[<$12,x]	; 12 2c 12
	lda	$12,y		; 12 34 12
	lda	<$12,y		; 12 34 12
	lda	[$12,y]		; 12 3c 12
	lda	[<$12,y]	; 12 3c 12
	lda	$12,u		; 12 54 12
	lda	<$12,u		; 12 54 12
	lda	[$12,u]		; 12 5c 12
	lda	[<$12,u]	; 12 5c 12
	lda	$12,s		; 12 64 12
	lda	<$12,s		; 12 64 12
	lda	[$12,s]		; 12 6c 12
	lda	[<$12,s]	; 12 6c 12
	lda	*+$15,pc	; 12 74 12
	lda	<*+$15,pc	; 12 74 12
	lda	[*+$15,pc]	; 12 7c 12
	lda	[<*+$15,pc]	; 12 7c 12
	lda	-$12,x		; 12 24 ee
	lda	<-$12,x		; 12 24 ee
	lda	[-$12,x]	; 12 2c ee
	lda	[<-$12,x]	; 12 2c ee
	lda	-$12,y		; 12 34 ee
	lda	<-$12,y		; 12 34 ee
	lda	[-$12,y]	; 12 3c ee
	lda	[<-$12,y]	; 12 3c ee
	lda	-$12,u		; 12 54 ee
	lda	<-$12,u		; 12 54 ee
	lda	[-$12,u]	; 12 5c ee
	lda	[<-$12,u]	; 12 5c ee
	lda	-$12,s		; 12 64 ee
	lda	<-$12,s		; 12 64 ee
	lda	[-$12,s]	; 12 6c ee
	lda	[<-$12,s]	; 12 6c ee
	lda	*-$0f,pc	; 12 74 ee
	lda	<*-$0f,pc	; 12 74 ee
	lda	[*-$0f,pc]	; 12 7c ee
	lda	[<*-$0f,pc]	; 12 7c ee
	lda	>$12,x		; 12 25 00 12
	lda	>$1234,x	; 12 25 12 34
	lda	$1234,x		; 12 25 12 34
	lda	[>$12,x]	; 12 2d 00 12
	lda	[>$1234,x]	; 12 2d 12 34
	lda	[$1234,x]	; 12 2d 12 34
	lda	>$12,y		; 12 35 00 12
	lda	>$1234,y	; 12 35 12 34
	lda	$1234,y		; 12 35 12 34
	lda	[>$12,y]	; 12 3d 00 12
	lda	[>$1234,y]	; 12 3d 12 34
	lda	[$1234,y]	; 12 3d 12 34
	lda	>$12,u		; 12 55 00 12
	lda	>$1234,u	; 12 55 12 34
	lda	$1234,u		; 12 55 12 34
	lda	[>$12,u]	; 12 5d 00 12
	lda	[>$1234,u]	; 12 5d 12 34
	lda	[$1234,u]	; 12 5d 12 34
	lda	>$12,s		; 12 65 00 12
	lda	>$1234,s	; 12 65 12 34
	lda	$1234,s		; 12 65 12 34
	lda	[>$12,s]	; 12 6d 00 12
	lda	[>$1234,s]	; 12 6d 12 34
	lda	[$1234,s]	; 12 6d 12 34
	lda	>*+$16,pc	; 12 75 00 12
	lda	>*+$1230,pc	; 12 75 12 34
	lda	*+$1230,pc	; 12 75 12 34
	lda	[>*+$16,pc]	; 12 7d 00 12
	lda	[>*+$1230,pc]	; 12 7d 12 34
	lda	[*+$1230,pc]	; 12 7d 12 34
	lda	>-$12,x		; 12 25 ff ee
	lda	>-$1234,x	; 12 25 ed cc
	lda	-$1234,x	; 12 25 ed cc
	lda	[>-$12,x]	; 12 2d ff ee
	lda	[>-$1234,x]	; 12 2d ed cc
	lda	[-$1234,x]	; 12 2d ed cc
	lda	>-$12,y		; 12 35 ff ee
	lda	>-$1234,y	; 12 35 ed cc
	lda	-$1234,y	; 12 35 ed cc
	lda	[>-$12,y]	; 12 3d ff ee
	lda	[>-$1234,y]	; 12 3d ed cc
	lda	[-$1234,y]	; 12 3d ed cc
	lda	>-$12,u		; 12 55 ff ee
	lda	>-$1234,u	; 12 55 ed cc
	lda	-$1234,u	; 12 55 ed cc
	lda	[>-$12,u]	; 12 5d ff ee
	lda	[>-$1234,u]	; 12 5d ed cc
	lda	[-$1234,u]	; 12 5d ed cc
	lda	>-$12,s		; 12 65 ff ee
	lda	>-$1234,s	; 12 65 ed cc
	lda	-$1234,s	; 12 65 ed cc
	lda	[>-$12,s]	; 12 6d ff ee
	lda	[>-$1234,s]	; 12 6d ed cc
	lda	[-$1234,s]	; 12 6d ed cc
	lda	>*-$0e,pc	; 12 75 ff ee
	lda	>*-$230,pc	; 12 75 fd cc
	lda	*-$230,pc	; 12 75 fd cc
	lda	[>*-$0e,pc]	; 12 7d ff ee
	lda	[>*-$230,pc]	; 12 7d fd cc
	lda	[*-$230,pc]	; 12 7d fd cc
	lda	$12		; 12 c4 12
	lda	<$12		; 12 c4 12
	lda	[$12]		; 12 cc 12
	lda	[<$12]		; 12 cc 12
	lda	a,x		; 12 a0
	lda	[a,x]		; 12 a8
	lda	a,y		; 12 b0
	lda	[a,y]		; 12 b8
	lda	a,u		; 12 d0
	lda	[a,u]		; 12 d8
	lda	a,s		; 12 e0
	lda	[a,s]		; 12 e8
	lda	a,pc		; 12 f0
	lda	[a,pc]		; 12 f8
	lda	b,x		; 12 a1
	lda	[b,x]		; 12 a9
	lda	b,y		; 12 b1
	lda	[b,y]		; 12 b9
	lda	b,u		; 12 d1
	lda	[b,u]		; 12 d9
	lda	b,s		; 12 e1
	lda	[b,s]		; 12 e9
	lda	b,pc		; 12 f1
	lda	[b,pc]		; 12 f9
	lda	d,x		; 12 a7
	lda	[d,x]		; 12 af
	lda	d,y		; 12 b7
	lda	[d,y]		; 12 bf
	lda	d,u		; 12 d7
	lda	[d,u]		; 12 df
	lda	d,s		; 12 e7
	lda	[d,s]		; 12 ef
	lda	d,pc		; 12 f7
	lda	[d,pc]		; 12 ff
