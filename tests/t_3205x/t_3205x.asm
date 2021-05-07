;------------------------------------
; C20x subset

	cpu	320c203
	page	0

	abs

	add	#25
	add	#2255
	add	#25,0
	add	#25,1
	add	#2255,1
	add	123
	add	123,3
	add	123,16
	add	*
	add	*,3
	add	*,16
	add	*,0,AR2
	add	*,3,AR2
	add	*,16,AR2

	addc	123
	addc	*
	addc	*,ar2

	adds	123
	adds	*
	adds	*,ar2

	addt	123
	addt	*
	addt	*,ar2

	adrk	#30

	and	123
	and	*
	and	*,ar2
	and	#255
	and	#255, 3
	and	#255,16

	apac

	b	1234h
	b	1234h, *
	b       1234h, *, ar2

	bacc

	banz	1234h
	banz	1234h, *
	banz    1234h, *, ar2

	bcnd	1234h, eq, c

	bit	123, 3
	bit	*, 3
	bit	*, 3, ar2

	bitt	123
	bitt	*
	bitt	*,ar2

	bldd	#10, 123
	bldd	#10, *
	bldd	#10, *, ar2
	bldd	123, #10
	bldd	*, #10
	bldd	*, #10, ar2

	blpd	#1234h, 123
	blpd	#1233h, *, ar2

	cala

	call	1234h
	call	1234h, *
	call	1234h, *, ar2

	cc	1234h, eq, c

	clrc	c
	clrc	cnf
	clrc	intm
	clrc	ovm
	clrc	sxm
	clrc	tc
	clrc	xf

	cmpl

	cmpr	2

	dmov	123
	dmov	*
	dmov	*,ar2

	idle

	in	123, 1234h
	in	*, 1234h
	in	*, 1234h, ar2

	intr	4

	lacc	123
	lacc	123, 16
	lacc	*
	lacc	*, 16
	lacc	*, 0, ar2
	lacc	*, 16, ar2
	lacc	#2
	lacc	#2, 10

	lacl	123
	lacl	*
	lacl	*, ar2
	lacl	#23

	lact	123
	lact	*
	lact	*, ar2

	lar	ar2, 123
	lar	ar2, *
	lar	ar2, *, ar2
	lar	ar2, #10
	lar	ar2, #1000

	ldp	123
	ldp	*
	ldp	*, ar2
	ldp	#40

	lph	123
	lph	*
	lph	*, ar2

	lst	#0, 123
	lst	#0, *
	lst	#0, *, ar2

	lst	#1, 123
	lst	#1, *
	lst	#1, *, ar2

        lt	123
        lt	*
        lt	*, ar2

        lta     123
        lta     *
        lta     *, ar2

        ltd     123
        ltd     *
        ltd     *, ar2

        ltp     123
        ltp     *
        ltp     *, ar2

        lts     123
        lts     *
        lts     *, ar2

	mac	1234h, 123
	mac	1234h, *
	mac	1234h, *, ar2

	macd	1234h, 123
	macd	1234h, *
	macd	1234h, *, ar2

	mar	123
	mar	*
	mar	*, ar2

	mpy	123
	mpy	*
	mpy	*, ar2
	mpy	#300
	mpy	#-300

        mpya    123
        mpya    *
        mpya    *, ar2

        mpys    123
        mpys    *
        mpys    *, ar2

        mpyu    123
        mpyu    *
        mpyu    *, ar2

	neg

	nmi

	nop

	norm	*
	norm	*, ar2

	or 	123
	or 	*
	or 	*,ar2
	or 	#255
	or 	#255, 3
	or 	#255,16

	out	123, 1234h
	out	*, 1234h
	out	*, 1234h, ar2

	pac

	pop

	popd	123
	popd	*
	popd	*, ar2

        pshd    123
        pshd    *
        pshd    *, ar2
	
	push

	ret

	retc	eq

	rol

	ror

	rpt	#30
	rpt	123
	rpt	*
	rpt	*, ar2

	sach	123
	sach	*, 0
	sach	*, 2
	sach	*, 0, ar2
	sach    *, 2, ar2

	sacl	123
	sacl	*, 0
	sacl	*, 2
	sacl	*, 0, ar2
	sacl    *, 2, ar2

	sar	ar3, 123
	sar	ar3, *
	sar	ar3, *, ar2

	sbrk	#10

	setc	c
	setc	cnf
	setc	intm
	setc	ovm
	setc	sxm
	setc	tc
	setc	xf

	sfl

	sfr

	spac

        spl     123
        spl     *
        spl     *, ar2

        sph     123
        sph     *
        sph     *, ar2

	splk	#1234, 123
	splk	#1234, *
	splk	#1234, *, ar2

	spm	2

	sqra	123
	sqra	*
	sqra	*, ar2

        sqrs    123
        sqrs    *
        sqrs    *, ar2

	sst	#0, 123
	sst	#0, *
	sst	#0, *, ar2

	sst	#1, 123
	sst	#1, *
	sst	#1, *, ar2

	sub	#25
	sub	#2255
	sub	#25,0
	sub	#25,1
	sub	#2255,1
	sub	123
	sub	123,3
	sub	123,16
	sub	*
	sub	*,3
	sub	*,16
	sub	*,0,AR2
	sub	*,3,AR2
	sub	*,16,AR2

        subb    123
        subb    *
        subb    *, ar2

        subc    123
        subc    *
        subc    *, ar2

        subs    123
        subs    *
        subs    *, ar2

        subt    123
        subt    *
        subt    *, ar2

        tblr    123
        tblr    *
        tblr    *, ar2

        tblw    123
        tblw    *
        tblw    *, ar2

	trap

	xor 	123
	xor 	*
	xor 	*,ar2
	xor 	#255
	xor 	#255, 3
	xor 	#255,16

	zalr	123
	zalr	*
	zalr	*, ar2

;------------------------------------
; C5x additions

	cpu	320c50

	adcb

	addb

	andb

	apl	#10, 123
	apl	#10, *
	apl	#10, *, ar2
	apl	123
	apl	*
	apl	*, ar2

	bd	1234h
	bd	1234h, *
	bd      1234h, *, ar2

	baccd

	banzd	1234h
	banzd	1234h, *
	banzd   1234h, *, ar2

	bcndd	1234h, eq, c

	bldd	bmar, 123
	bldd	bmar, *
	bldd	bmar, *, ar2
	bldd	123, bmar
	bldd	*, bmar
	bldd	*, bmar, ar2

	bldp	123
	bldp	*
	bldp	*, ar2

	blpd	bmar, 123
	blpd	bmar, *
	blpd	bmar, *, ar2

	bsar	7

	calad

	calld	1234h
	calld	1234h, *
	calld	1234h, *, ar2

	ccd	1234h, eq, c

	cpl	#10, 123
	cpl	#10, *
	cpl	#10, *, ar2
	cpl	123
	cpl	*
	cpl	*, ar2

	crgt

	crlt

	exar

	idle2

	lacb

	lamm	123
	lamm	*
	lamm	*, ar2

	lmmr	123, #1234
	lmmr	*, #1234
	lmmr    *, #1234, ar2

	madd	123
	madd	*
	madd	*, ar2

	mads	123
	mads	*
	mads	*, ar2

	opl	#10, 123
	opl	#10, *
	opl	#10, *, ar2
	opl	123
	opl	*
	opl	*, ar2

	orb

	retd

	retcd	eq

	rete

	reti

	rolb

	rorb

	rptb	123

	rptz	#10

	sacb

        samm    123
        samm    *
        samm    *, ar2

	sath

	satl

	sbb

	sbbb

	sflb

	sfrb

; haven't found encoding of the C5x SHM instruction so far :-(
;	shm

        smmr    123, #1234
        smmr    *, #1234
        smmr    *, #1234, ar2

	xc	2, eq
