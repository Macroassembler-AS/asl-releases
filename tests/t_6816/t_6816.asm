	cpu	68hc16
	page	0
	assume  ek:0

	aba   		;$370b
	abx   		;$374f
	aby   		;$375f
	abz   		;$376f
	ace   		;$3722
	aced  		;$3723
	ade   		;$2778
	adx   		;$37cd
	ady   		;$37dd
	adz   		;$37ed
	aex   		;$374d
	aey   		;$375d
	aez   		;$376d
	asla  		;$3704
	aslb  		;$3714
	asld  		;$27f4
	asle  		;$2774
	aslm  		;$27b6
	asra  		;$370d
	asrb  		;$371d
	asrd  		;$27fd
	asre  		;$277d
	asrm  		;$27ba
	bgnd  		;$37a6
	cba   		;$371b
	clra  		;$3705
	clrb  		;$3715
	clrd  		;$27f5
	clre  		;$2775
	clrm  		;$27b7
	coma  		;$3700
	comb  		;$3710
	comd  		;$27f0
	come  		;$2770
	daa   		;$3721
	deca  		;$3701
	decb  		;$3711
	ediv  		;$3728
	edivs 		;$3729
	emul  		;$3725
	emuls 		;$3726
	fdiv  		;$372b
	fmuls 		;$3727
	idiv  		;$372a
	inca  		;$3703
	incb  		;$3713
	lpstop		;$27f1
	lsla  		;$3704
	lslb  		;$3714
	lsld  		;$27f4
	lsle  		;$2774
	lsra  		;$370f
	lsrb  		;$371f
	lsrd  		;$27ff
	lsre  		;$277f
	mul   		;$3724
	nega  		;$3702
	negb  		;$3712
	negd  		;$27f2
	nege  		;$2772
	nop   		;$274c
	psha  		;$3708
	pshb  		;$3718
	pshmac		;$27b8
	pula  		;$3709
	pulb  		;$3719
	pulmac		;$27b9
	rola  		;$370c
	rolb  		;$371c
	rold  		;$27fc
	role  		;$277c
	rora  		;$370e
	rorb  		;$371e
	rord  		;$27fe
	rore  		;$277e
	rti   		;$2777
	rts   		;$27f7
	sba   		;$370a
	sde   		;$2779
	swi   		;$3720
	sxt   		;$27f8
	tab   		;$3717
	tap   		;$37fd
	tba   		;$3707
	tbek  		;$27fa
	tbsk  		;$379f
	tbxk  		;$379c
	tbyk  		;$379d
	tbzk  		;$379e
	tde   		;$277b
	tdmsk 		;$372f
	tdp   		;$372d
	ted   		;$27fb
	tedm  		;$27b1
	tekb  		;$27bb
	tem   		;$27b2
	tmer  		;$27b4
	tmet  		;$27b5
	tmxed 		;$27b3
	tpa   		;$37fc
	tpd   		;$372c
	tskb  		;$37af
	tsta  		;$3706
	tstb  		;$3716
	tstd  		;$27f6
	tste  		;$2776
	tsx   		;$274f
	tsy   		;$275f
	tsz   		;$276f
	txkb  		;$37ac
	txs   		;$374e
	txy   		;$275c
	txz   		;$276c
	tykb  		;$37ad
	tys   		;$375e
	tyx   		;$274d
	tyz   		;$276d
	tzkb  		;$37ae
	tzs   		;$376e
	tzx   		;$274e
	tzy   		;$275e
	wai   		;$27f3
	xgab  		;$371a
	xgde  		;$277a
	xgdx  		;$37cc
	xgdy  		;$37dc
	xgdz  		;$37ec
	xgex  		;$374c
	xgey  		;$375c
	xgez  		;$376c

targ:	bra	targ
	brn	targ
	bcc	targ
	bcs	targ
	bhs	targ
	blo	targ
	bmi	targ
	bne	targ
	bpl	targ
	bvc	targ
	bvs	targ
	bhi	targ
	bls	targ
	beq	targ
	bge	targ
	bgt	targ
	ble	targ
	blt	targ
	lbra	targ
	lbrn	targ
	lbcc	targ
	lbcs	targ
	lbhs	targ
	lblo	targ
	lbmi	targ
	lbne	targ
	lbpl	targ
	lbvc	targ
	lbvs	targ
	lbhi	targ
	lbls	targ
	lbeq	targ
	lbge	targ
	lbgt	targ
	lble	targ
	lblt	targ
	bsr	targ
	lbev	targ
	lbmv	targ
	lbsr	targ

	adca	$55,x
	adca	$55,y
	adca	$55,z
	adca	#$aa
	adca	$5aa5,x
	adca	$5aa5,y
	adca	$5aa5,z
	adca	$1234
	adca	e,x
	adca	e,y
	adca	e,z

	adcb	$55,x
	adcb	$55,y
	adcb	$55,z
	adcb	#$aa
	adcb	$5aa5,x
	adcb	$5aa5,y
	adcb	$5aa5,z
	adcb	$1234
	adcb	e,x
	adcb	e,y
	adcb	e,z

	adcd	$55,x
	adcd	$55,y
	adcd	$55,z
	adcd	#$aa
	adcd	$5aa5,x
	adcd	$5aa5,y
	adcd	$5aa5,z
	adcd	$1234
	adcd	e,x
	adcd	e,y
	adcd	e,z

	adce	$55,x
	adce	$55,y
	adce	$55,z
	adce	#$aa
	adce	$5aa5,x
	adce	$5aa5,y
	adce	$5aa5,z
	adce	$1234

	adda	$55,x
	adda	$55,y
	adda	$55,z
	adda	#$aa
	adda	$5aa5,x
	adda	$5aa5,y
	adda	$5aa5,z
	adda	$1234
	adda	e,x
	adda	e,y
	adda	e,z

	addb	$55,x
	addb	$55,y
	addb	$55,z
	addb	#$aa
	addb	$5aa5,x
	addb	$5aa5,y
	addb	$5aa5,z
	addb	$1234
	addb	e,x
	addb	e,y
	addb	e,z

	addd	$55,x
	addd	$55,y
	addd	$55,z
	addd	#$55
	addd	#$aa
	addd	$5aa5,x
	addd	$5aa5,y
	addd	$5aa5,z
	addd	$1234
	addd	e,x
	addd	e,y
	addd	e,z

	adde	$55,x
	adde	$55,y
	adde	$55,z
	adde	#$55
	adde	#$aa
	adde	$5aa5,x
	adde	$5aa5,y
	adde	$5aa5,z
	adde	$1234

	anda	$55,x
	anda	$55,y
	anda	$55,z
	anda	#$aa
	anda	$5aa5,x
	anda	$5aa5,y
	anda	$5aa5,z
	anda	$1234
	anda	e,x
	anda	e,y
	anda	e,z

	andb	$55,x
	andb	$55,y
	andb	$55,z
	andb	#$aa
	andb	$5aa5,x
	andb	$5aa5,y
	andb	$5aa5,z
	andb	$1234
	andb	e,x
	andb	e,y
	andb	e,z

	andd	$55,x
	andd	$55,y
	andd	$55,z
	andd	#$aa
	andd	$5aa5,x
	andd	$5aa5,y
	andd	$5aa5,z
	andd	$1234
	andd	e,x
	andd	e,y
	andd	e,z

	ande	$55,x
	ande	$55,y
	ande	$55,z
	ande	#$aa
	ande	$5aa5,x
	ande	$5aa5,y
	ande	$5aa5,z
	ande	$1234

	asl	$55,x
	asl	$55,y
	asl	$55,z
	asl	$5aa5,x
	asl	$5aa5,y
	asl	$5aa5,z
	asl	$1234

	aslw	$5aa5,x
	aslw	$5aa5,y
	aslw	$5aa5,z
	aslw	$1234

	asr	$55,x
	asr	$55,y
	asr	$55,z
	asr	$5aa5,x
	asr	$5aa5,y
	asr	$5aa5,z
	asr	$1234

	asrw	$5aa5,x
	asrw	$5aa5,y
	asrw	$5aa5,z
	asrw	$1234

	bita	$55,x
	bita	$55,y
	bita	$55,z
	bita	#$aa
	bita	$5aa5,x
	bita	$5aa5,y
	bita	$5aa5,z
	bita	$1234
	bita	e,x
	bita	e,y
	bita	e,z

	bitb	$55,x
	bitb	$55,y
	bitb	$55,z
	bitb	#$aa
	bitb	$5aa5,x
	bitb	$5aa5,y
	bitb	$5aa5,z
	bitb	$1234
	bitb	e,x
	bitb	e,y
	bitb	e,z

	clr	$55,x
	clr	$55,y
	clr	$55,z
	clr	$5aa5,x
	clr	$5aa5,y
	clr	$5aa5,z
	clr	$1234

	clrw	$5aa5,x
	clrw	$5aa5,y
	clrw	$5aa5,z
	clrw	$1234

	cmpa	$55,x
	cmpa	$55,y
	cmpa	$55,z
	cmpa	#$aa
	cmpa	$5aa5,x
	cmpa	$5aa5,y
	cmpa	$5aa5,z
	cmpa	$1234
	cmpa	e,x
	cmpa	e,y
	cmpa	e,z

	cmpb	$55,x
	cmpb	$55,y
	cmpb	$55,z
	cmpb	#$aa
	cmpb	$5aa5,x
	cmpb	$5aa5,y
	cmpb	$5aa5,z
	cmpb	$1234
	cmpb	e,x
	cmpb	e,y
	cmpb	e,z

	com	$55,x
	com	$55,y
	com	$55,z
	com	$5aa5,x
	com	$5aa5,y
	com	$5aa5,z
	com	$1234

	comw	$5aa5,x
	comw	$5aa5,y
	comw	$5aa5,z
	comw	$1234

	cpd	$55,x
	cpd	$55,y
	cpd	$55,z
	cpd	#$aa
	cpd	$5aa5,x
	cpd	$5aa5,y
	cpd	$5aa5,z
	cpd	$1234
	cpd	e,x
	cpd	e,y
	cpd	e,z

	cpe	$55,x
	cpe	$55,y
	cpe	$55,z
	cpe	#$aa
	cpe	$5aa5,x
	cpe	$5aa5,y
	cpe	$5aa5,z
	cpe	$1234

	dec	$55,x
	dec	$55,y
	dec	$55,z
	dec	$5aa5,x
	dec	$5aa5,y
	dec	$5aa5,z
	dec	$1234

	decw	$5aa5,x
	decw	$5aa5,y
	decw	$5aa5,z
	decw	$1234

	eora	$55,x
	eora	$55,y
	eora	$55,z
	eora	#$aa
	eora	$5aa5,x
	eora	$5aa5,y
	eora	$5aa5,z
	eora	$1234
	eora	e,x
	eora	e,y
	eora	e,z

	eorb	$55,x
	eorb	$55,y
	eorb	$55,z
	eorb	#$aa
	eorb	$5aa5,x
	eorb	$5aa5,y
	eorb	$5aa5,z
	eorb	$1234
	eorb	e,x
	eorb	e,y
	eorb	e,z

	eord	$55,x
	eord	$55,y
	eord	$55,z
	eord	#$aa
	eord	$5aa5,x
	eord	$5aa5,y
	eord	$5aa5,z
	eord	$1234
	eord	e,x
	eord	e,y
	eord	e,z

	eore	$55,x
	eore	$55,y
	eore	$55,z
	eore	#$aa
	eore	$5aa5,x
	eore	$5aa5,y
	eore	$5aa5,z
	eore	$1234

	inc	$55,x
	inc	$55,y
	inc	$55,z
	inc	$5aa5,x
	inc	$5aa5,y
	inc	$5aa5,z
	inc	$1234

	incw	$5aa5,x
	incw	$5aa5,y
	incw	$5aa5,z
	incw	$1234

	ldaa	$55,x
	ldaa	$55,y
	ldaa	$55,z
	ldaa	#$aa
	ldaa	$5aa5,x
	ldaa	$5aa5,y
	ldaa	$5aa5,z
	ldaa	$1234
	ldaa	e,x
	ldaa	e,y
	ldaa	e,z

	ldab	$55,x
	ldab	$55,y
	ldab	$55,z
	ldab	#$aa
	ldab	$5aa5,x
	ldab	$5aa5,y
	ldab	$5aa5,z
	ldab	$1234
	ldab	e,x
	ldab	e,y
	ldab	e,z

	ldd	$55,x
	ldd	$55,y
	ldd	$55,z
	ldd	#$aa
	ldd	$5aa5,x
	ldd	$5aa5,y
	ldd	$5aa5,z
	ldd	$1234
	ldd	e,x
	ldd	e,y
	ldd	e,z

	lde	$55,x
	lde	$55,y
	lde	$55,z
	lde	#$aa
	lde	$5aa5,x
	lde	$5aa5,y
	lde	$5aa5,z
	lde	$1234

	lsl	$55,x
	lsl	$55,y
	lsl	$55,z
	lsl	$5aa5,x
	lsl	$5aa5,y
	lsl	$5aa5,z
	lsl	$1234

	lslw	$5aa5,x
	lslw	$5aa5,y
	lslw	$5aa5,z
	lslw	$1234

	lsr	$55,x
	lsr	$55,y
	lsr	$55,z
	lsr	$5aa5,x
	lsr	$5aa5,y
	lsr	$5aa5,z
	lsr	$1234

	lsrw	$5aa5,x
	lsrw	$5aa5,y
	lsrw	$5aa5,z
	lsrw	$1234

	neg	$55,x
	neg	$55,y
	neg	$55,z
	neg	$5aa5,x
	neg	$5aa5,y
	neg	$5aa5,z
	neg	$1234

	negw	$5aa5,x
	negw	$5aa5,y
	negw	$5aa5,z
	negw	$1234

	oraa	$55,x
	oraa	$55,y
	oraa	$55,z
	oraa	#$aa
	oraa	$5aa5,x
	oraa	$5aa5,y
	oraa	$5aa5,z
	oraa	$1234
	oraa	e,x
	oraa	e,y
	oraa	e,z

	orab	$55,x
	orab	$55,y
	orab	$55,z
	orab	#$aa
	orab	$5aa5,x
	orab	$5aa5,y
	orab	$5aa5,z
	orab	$1234
	orab	e,x
	orab	e,y
	orab	e,z

	ord	$55,x
	ord	$55,y
	ord	$55,z
	ord	#$aa
	ord	$5aa5,x
	ord	$5aa5,y
	ord	$5aa5,z
	ord	$1234
	ord	e,x
	ord	e,y
	ord	e,z

	ore	$55,x
	ore	$55,y
	ore	$55,z
	ore	#$aa
	ore	$5aa5,x
	ore	$5aa5,y
	ore	$5aa5,z
	ore	$1234

	rol	$55,x
	rol	$55,y
	rol	$55,z
	rol	$5aa5,x
	rol	$5aa5,y
	rol	$5aa5,z
	rol	$1234

	rolw	$5aa5,x
	rolw	$5aa5,y
	rolw	$5aa5,z
	rolw	$1234

	ror	$55,x
	ror	$55,y
	ror	$55,z
	ror	$5aa5,x
	ror	$5aa5,y
	ror	$5aa5,z
	ror	$1234

	rorw	$5aa5,x
	rorw	$5aa5,y
	rorw	$5aa5,z
	rorw	$1234

	sbca	$55,x
	sbca	$55,y
	sbca	$55,z
	sbca	#$aa
	sbca	$5aa5,x
	sbca	$5aa5,y
	sbca	$5aa5,z
	sbca	$1234
	sbca	e,x
	sbca	e,y
	sbca	e,z

	sbcb	$55,x
	sbcb	$55,y
	sbcb	$55,z
	sbcb	#$aa
	sbcb	$5aa5,x
	sbcb	$5aa5,y
	sbcb	$5aa5,z
	sbcb	$1234
	sbcb	e,x
	sbcb	e,y
	sbcb	e,z

	sbcd	$55,x
	sbcd	$55,y
	sbcd	$55,z
	sbcd	#$aa
	sbcd	$5aa5,x
	sbcd	$5aa5,y
	sbcd	$5aa5,z
	sbcd	$1234
	sbcd	e,x
	sbcd	e,y
	sbcd	e,z

	sbce	$55,x
	sbce	$55,y
	sbce	$55,z
	sbce	#$aa
	sbce	$5aa5,x
	sbce	$5aa5,y
	sbce	$5aa5,z
	sbce	$1234

	staa	$55,x
	staa	$55,y
	staa	$55,z
	staa	$5aa5,x
	staa	$5aa5,y
	staa	$5aa5,z
	staa	$1234
	staa	e,x
	staa	e,y
	staa	e,z

	stab	$55,x
	stab	$55,y
	stab	$55,z
	stab	$5aa5,x
	stab	$5aa5,y
	stab	$5aa5,z
	stab	$1234
	stab	e,x
	stab	e,y
	stab	e,z

	std	$55,x
	std	$55,y
	std	$55,z
	std	$5aa5,x
	std	$5aa5,y
	std	$5aa5,z
	std	$1234
	std	e,x
	std	e,y
	std	e,z

	ste	$55,x
	ste	$55,y
	ste	$55,z
	ste	$5aa5,x
	ste	$5aa5,y
	ste	$5aa5,z
	ste	$1234

	suba	$55,x
	suba	$55,y
	suba	$55,z
	suba	#$aa
	suba	$5aa5,x
	suba	$5aa5,y
	suba	$5aa5,z
	suba	$1234
	suba	e,x
	suba	e,y
	suba	e,z

	subb	$55,x
	subb	$55,y
	subb	$55,z
	subb	#$aa
	subb	$5aa5,x
	subb	$5aa5,y
	subb	$5aa5,z
	subb	$1234
	subb	e,x
	subb	e,y
	subb	e,z

	subd	$55,x
	subd	$55,y
	subd	$55,z
	subd	#$aa
	subd	$5aa5,x
	subd	$5aa5,y
	subd	$5aa5,z
	subd	$1234
	subd	e,x
	subd	e,y
	subd	e,z

	sube	$55,x
	sube	$55,y
	sube	$55,z
	sube	#$aa
	sube	$5aa5,x
	sube	$5aa5,y
	sube	$5aa5,z
	sube	$1234

	tst	$55,x
	tst	$55,y
	tst	$55,z
	tst	$5aa5,x
	tst	$5aa5,y
	tst	$5aa5,z
	tst	$1234

	tstw	$5aa5,x
	tstw	$5aa5,y
	tstw	$5aa5,z
	tstw	$1234

	cps	$55,x
	cps	$55,y
	cps	$55,z
	cps	#$aa
	cps	$5aa5,x
	cps	$5aa5,y
	cps	$5aa5,z
	cps	$1234

	cpx	$55,x
	cpx	$55,y
	cpx	$55,z
	cpx	#$aa
	cpx	$5aa5,x
	cpx	$5aa5,y
	cpx	$5aa5,z
	cpx	$1234

	cpy	$55,x
	cpy	$55,y
	cpy	$55,z
	cpy	#$aa
	cpy	$5aa5,x
	cpy	$5aa5,y
	cpy	$5aa5,z
	cpy	$1234

	cpz	$55,x
	cpz	$55,y
	cpz	$55,z
	cpz	#$aa
	cpz	$5aa5,x
	cpz	$5aa5,y
	cpz	$5aa5,z
	cpz	$1234

	lds	$55,x
	lds	$55,y
	lds	$55,z
	lds	#$aa
	lds	$5aa5,x
	lds	$5aa5,y
	lds	$5aa5,z
	lds	$1234

	ldx	$55,x
	ldx	$55,y
	ldx	$55,z
	ldx	#$aa
	ldx	$5aa5,x
	ldx	$5aa5,y
	ldx	$5aa5,z
	ldx	$1234

	ldy	$55,x
	ldy	$55,y
	ldy	$55,z
	ldy	#$aa
	ldy	$5aa5,x
	ldy	$5aa5,y
	ldy	$5aa5,z
	ldy	$1234

	ldz	$55,x
	ldz	$55,y
	ldz	$55,z
	ldz	#$aa
	ldz	$5aa5,x
	ldz	$5aa5,y
	ldz	$5aa5,z
	ldz	$1234

	sts	$55,x
	sts	$55,y
	sts	$55,z
	sts	$5aa5,x
	sts	$5aa5,y
	sts	$5aa5,z
	sts	$1234

	stx	$55,x
	stx	$55,y
	stx	$55,z
	stx	$5aa5,x
	stx	$5aa5,y
	stx	$5aa5,z
	stx	$1234

	sty	$55,x
	sty	$55,y
	sty	$55,z
	sty	$5aa5,x
	sty	$5aa5,y
	sty	$5aa5,z
	sty	$1234

	stz	$55,x
	stz	$55,y
	stz	$55,z
	stz	$5aa5,x
	stz	$5aa5,y
	stz	$5aa5,z
	stz	$1234

	jmp	$12345
	jmp	$23456,x
	jmp	$23456,y
	jmp	$23456,z

	jsr	$12345
	jsr	$23456,x
	jsr	$23456,y
	jsr	$23456,z

	ais	#-2
	ais	#1000

	aix	#-2
	aix	#1000

	aiy	#-2
	aiy	#1000

	aiz	#-2
	aiz	#1000

	andp	#$feff
	orp	#$0100

	lded	$a55a
	ldhi	$a55a
	sted	$5aa5

	mac	3,5		; 7B 35
	rmac	5,3		; FB 53
	mac	-8,7		; 7B 87
	rmac	1,-1		; FB 1F
	expect	1320
	mac	8,5
	endexpect
	expect	1315
	mac	5,-9
	endexpect

	pshm	d,x,z,ccr
	pulm	e,y,k

	bclr    $55,x,#2	; 1708 02 55
	bclr	$55,y,#2	; 1718 02 55
	bclr	$55,z,#2	; 1728 02 55
	bclr    $5aa5,x,#2	; 08 02 5AA5
	bclr	$5aa5,y,#2	; 18 02 5AA5
	bclr	$5aa5,z,#2	; 28 02 5AA5
	bclr	$1234,#2	; 38 02 1234

	bset    $55,x,#2	; 1709 02 55
	bset	$55,y,#2	; 1719 02 55
	bset	$55,z,#2	; 1729 02 55
	bset    $5aa5,x,#2	; 09 02 5AA5
	bset	$5aa5,y,#2	; 19 02 5AA5
	bset	$5aa5,z,#2	; 29 02 5AA5
	bset	$1234,#2	; 39 02 1234

	bclrw   $55,x,#2	; 2708 0055 0002
	bclrw	$55,y,#2	; 2718 0055 0002
	bclrw	$55,z,#2	; 2728 0055 0002
	bclrw   $5aa5,x,#2	; 2708 5AA5 0002
	bclrw	$5aa5,y,#2	; 2718 5AA5 0002
	bclrw	$5aa5,z,#2	; 2728 5AA5 0002
	bclrw	$1234,#2	; 2738 1234 0002

	bsetw   $55,x,#2	; 2709 0055 0002
	bsetw	$55,y,#2	; 2719 0055 0002
	bsetw	$55,z,#2	; 2729 0055 0002
	bsetw   $5aa5,x,#2	; 2709 5AA5 0002
	bsetw	$5aa5,y,#2	; 2719 5AA5 0002
	bsetw	$5aa5,z,#2	; 2729 5AA5 0002
	bsetw	$1234,#2	; 2739 1234 0002

	movb	$1234,$5678	; 37FE 1234 5678
	movb	$1234,5,x	; 32 05 1234
	movb	-5,x,$5678	; 30 FB 5678

	movw	$1234,$5678	; 37FF 1234 5678
	movw	$1234,5,x	; 33 05 1234
	movw	-5,x,$5678	; 31 FB 5678

	brclr	$55,x,#2,*+$30
	brclr	$55,y,#2,*+$30
	brclr	$55,z,#2,*+$30
	brclr	$55,x,#2,*+$300
	brclr	$55,y,#2,*+$300
	brclr	$55,z,#2,*+$300
	brclr	$5aa5,x,#2,*+$300
	brclr	$5aa5,y,#2,*+$300
	brclr	$5aa5,z,#2,*+$300
	brclr	$1234,#2,*+$300

	brset	$55,x,#2,*+$30
	brset	$55,y,#2,*+$30
	brset	$55,z,#2,*+$30
	brset	$55,x,#2,*+$300
	brset	$55,y,#2,*+$300
	brset	$55,z,#2,*+$300
	brset	$5aa5,x,#2,*+$300
	brset	$5aa5,y,#2,*+$300
	brset	$5aa5,z,#2,*+$300
	brset	$1234,#2,*+$300

	clc
	andp	#$feff
	cli
	andp	#$ff1f
	clv
	andp	#$fdff
	des
	ais	#-1
	dex
	aix	#-1
	dey
	aiy	#-1
	ins
	ais	#1
	inx
	aix	#1
	iny
	aiy	#1
	pshx
	pshm	x
	pshy
	pshm	y
	pulx
	pulm	x
	puly
	pulm	y
	sec
	orp	#$0100
	sei
	orp	#$00e0
	sev
	orp	#$0200
