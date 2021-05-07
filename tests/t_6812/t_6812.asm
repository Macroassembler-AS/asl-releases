	cpu	68hc12
	page	0

	aba

	abx

	aby

	adca	#45
        adca	$45
        adca	$1234
        adca	>$45
        adca	,y
        adca	5,x
        adca	-5,sp
        adca	*,pc
        adca	*+10,pc
        adca	*+17,pc
        adca	a,y
        adca	b,sp
        adca	d,pc
        adca	55,y
        adca	-55,sp
        adca	<5,y
        adca	<-5,sp
        adca	*+18,pc
        adca	*+258,pc
        adca	555,y
        adca	-555,sp
	adca	>55,y
        adca	>-55,sp
        adca	*+259,pc
        adca	[d,sp]
        adca	[d,x]
	adca	[-5,y]
        adca	[500,y]
        adca	4,+y
        adca	4,y+
        adca	4,-y
        adca	4,y-

        adcb	#55
        adcb	$55
        adcb	,x
        adcb	$1234

        adda	#55
        adda	$55
        adda	,x
        adda	$1234

        addb	#55
        addb	$55
        addb	,x
        addb	$1234

        addd	#55
        addd	$55
        addd	,x
        addd	$1234

        anda	#55
        anda	$55
        anda	,x
        anda	$1234

        andb	#55
        andb	$55
        andb	,x
        andb	$1234

        andcc	#$fe

        asl	$55
        asl	,x
        asl	$1234

        asla

        aslb

        asld

        asr	$55
        asr	,x
        asr	$1234

        asra

        asrb

        bcc	*

        bclr	$20 #$40
	bclr	$20,#$40
	bclr	$1234 #$40
	bclr	$1234,#$40
	bclr	,x $40
	bclr	$20,y,$40
        bclr	*,pc,$40

        bcs	*

        beq	*

        bge	*

        bgnd

        bgt	*

        bhi	*

        bhs	*

        bita	#$55
        bita	$55
        bita	,x
        bita	$1234

        bitb	#$55
        bitb	$55
        bitb	,x
        bitb	$1234

        ble	*

        blo	*

        bls	*

        blt	*

        bmi	*

        bne	*

        bpl	*

        bra	*

        brclr	$20 #$40 *
        brclr	$2000,#$40,*
        brclr	,x,#$40,*
        brclr	*,pc,#$40,*

        brn	*

        brset	$20 #$40 *
        brset	$2000,#$40,*
        brset	,x,#$40,*
        brset	*,pc,#$40,*

        bset	$20 #$40
	bset	$20,#$40
	bset	$1234 #$40
	bset	$1234,#$40
	bset	,x $40
	bset	$20,y,$40

        bsr	*

        bvc	*

        bvs	*

        call	$2000,5
        call	5,y,6
        call	200,y,7
        call	20000,y,8
        call	[d,y]
        call	[20,y]

        cba

        clc
        andcc	#$fe

        cli
        andcc	#$ef

        clr	$55
        clr	,x
        clr	$1234

        clra

        clrb

        clv
        andcc	#$fd

        cmpa	#55
        cmpa	$55
        cmpa	,x
        cmpa	$1234

        cmpb	#55
        cmpb	$55
        cmpb	,x
        cmpb	$1234

        com	$55
        com	,x
        com	$1234

        coma

        comb

        cpd	#55
        cpd	$55
        cpd	,x
        cpd	$1234

        cps	#55
        cps	$55
        cps	,x
        cps	$1234

        cpx	#55
        cpx	$55
        cpx	,x
        cpx	$1234

        cpy	#55
        cpy	$55
        cpy	,x
        cpy	$1234

        daa

        dbeq	x,*

        dbne	x,*

        dec	$55
        dec	,x
        dec	$1234

        deca

        decb

        des
        leas	-1,sp

        dex

        dey

        ediv

        edivs

        emacs	$1234

        emaxd	,x

        emaxm	,x

        emind	,x

        eminm	,x

        emul

        emuls

        eora	#55
        eora	$55
        eora	,x
        eora	$1234

        eorb	#55
        eorb	$55
        eorb	,x
        eorb	$1234

        etbl	5,y

        exg	ccr,sp

        fdiv

        ibeq	a,*

        ibne	a,*

        idiv

        idivs

        inc	$55
        inc	,x
        inc	$1234

        inca

        incb

        ins
        leas	1,sp

        inx

        iny

	jmp	$2000
	jmp	$20
        jmp	[d,x]

	jsr	$2000
	jsr	$20
        jsr	[d,x]

        lbcc	*

        lbcs	*

        lbeq	*

        lbge	*

        lbgt	*

        lbhi	*

        lbhs	*

        lble	*

        lblo	*

        lbls	*

        lblt	*

        lbmi	*

        lbne	*

        lbpl	*

        lbra	*

        lbrn	*

        lbvc	*

        lbvs	*

        ldaa	#55
        ldaa	$55
        ldaa	,x
        ldaa	$1234

        ldab	#55
        ldab	$55
        ldab	,x
        ldab	$1234

        ldd	#55
        ldd	$55
        ldd	,x
        ldd	$1234

        lds	#55
        lds	$55
        lds	,x
        lds	$1234

        ldx	#55
        ldx	$55
        ldx	,x
        ldx	$1234

        ldy	#55
        ldy	$55
        ldy	,x
        ldy	$1234

        leas	2000,sp

        leax	,y

        leay	,x

        lsl	$55
        lsl	,x
        lsl	$1234

        lsla

        lslb

        lsld

        lsr	$55
        lsr	,x
        lsr	$1234

        lsra

        lsrb

        lsrd

        maxa	,x

        maxm	,x

        mem

        mina	,x

        minm	,x

        movb	#$55 $1234
        movb	#$55,$1234
        movb	#$55 2,y
        movb	#$55,2,y
        movb	#$55,next_a,pc
next_a: movb	$1234 $3456
	movb	$1234,$3456
        movb	$1234 2,y
        movb	$1234,2,y
        movb	$1234,next_b,pc
next_b: movb	2,y $1234
	movb	2,y,$1234
        movb	next_c,pc,$1234
next_c: movb	2,y 2,y
	movb	2,y,2,y
        movb	next_d,pc next_d,pc
next_d:
        movw	#$55 $1234
        movw	#$55,$1234
        movw	#$55 2,y
        movw	#$55,2,y
        movw	#$55,next_e,pc
next_e: movw	$1234 $3456
	movw	$1234,$3456
        movw	$1234 2,y
        movw	$1234,2,y
        movw	$1234,next_f,pc
next_f: movw	2,y $1234
	movw	2,y,$1234
        movw	next_g,pc,$1234
next_g: movw	2,y 2,y
	movw	2,y,2,y
        movw	next_h,pc next_h,pc
next_h:

	mul

        neg	$55
        neg	,x
        neg	$1234

        nega

        negb

        nop

        oraa	#55
        oraa	$55
        oraa	,x
        oraa	$1234

        orab	#55
        orab	$55
        orab	,x
        orab	$1234

        orcc    #$10

        psha

        pshb

        pshc

        pshd

        pshx

        pshy

        pula

        pulb

        pulc

        puld

        pulx

        puly

        rev

        revw

        rol	$55
        rol	,x
        rol	$1234

        rola

        rolb

        ror	$55
        ror	,x
        ror	$1234

        rora

        rorb

        rtc

        rti

        rts

        sba

        sbca	#55
        sbca	$55
        sbca	,x
        sbca	$1234

        sbcb	#55
        sbcb	$55
        sbcb	,x
        sbcb	$1234

        sec
        orcc	#$01

        sei
        orcc	#$10

        sev
        orcc	#$02

	sex	a,d

        staa	$55
        staa	,x
        staa	$1234

        stab	$55
        stab	,x
        stab	$1234

        std	$55
        std	,x
        std	$1234

        sts	$55
        sts	,x
        sts	$1234

        stx	$55
        stx	,x
        stx	$1234

        sty	$55
        sty	,x
        sty	$1234

        stop

        suba	#55
        suba	$55
        suba	,x
        suba	$1234

        subb	#55
        subb	$55
        subb	,x
        subb	$1234

        subd	#55
        subd	$55
        subd	,x
        subd	$1234

        swi

        tab

        tap
        tfr	a,ccr

        tba

        tbeq	d,*

        tbl	a,x

        tbne	d,*

        tfr	a,b
        tfr	x,y

        tpa
        tfr	ccr,a

        trap	#$42

        tst	$55
        tst	,x
        tst	$1234

        tsta

        tstb

	tsx
        tfr	sp,x

        tsy
        tfr	sp,y

        txs
        tfr	x,sp

        tys
        tfr	y,sp

        wai

        wav

        xgdx
        exg	d,x

        xgdy
        exg	d,y

	cpu	68hc12x

	aslx
	asly
	asrx
	asry
	clrx
	clry
	comx
	comy
	decx
	decy
	incx
	incy
	lslx
	lsly
	lsrx
	lsry
	negx
	negy
	pshcw
	pulcw
	rolx
	roly
	rorx
	rory
	tstx
	tsty

        aslw    $55
        aslw    ,x
        aslw    ,y
        aslw    ,sp
        aslw    *,pc
        aslw    10,x
        aslw    10,y
        aslw    10,sp
        aslw    *+10,pc
        aslw    $1234

        asrw    $55
        asrw    ,x
        asrw    ,y
        asrw    ,sp
        asrw    *,pc
        asrw    10,x
        asrw    10,y
        asrw    10,sp
        asrw    *+10,pc
        asrw    $1234

        clrw    $55
        clrw    ,x
        clrw    ,y
        clrw    ,sp
        clrw    *,pc
        clrw    10,x
        clrw    10,y
        clrw    10,sp
        clrw    *+10,pc
        clrw    $1234

        comw    $55
        comw    ,x
        comw    ,y
        comw    ,sp
        comw    *,pc
        comw    10,x
        comw    10,y
        comw    10,sp
        comw    *+10,pc
        comw    $1234

        decw    $55
        decw    ,x
        decw    ,y
        decw    ,sp
        decw    *,pc
        decw    10,x
        decw    10,y
        decw    10,sp
        decw    *+10,pc
        decw    $1234

        incw    $55
        incw    ,x
        incw    ,y
        incw    ,sp
        incw    *,pc
        incw    10,x
        incw    10,y
        incw    10,sp
        incw    *+10,pc
        incw    $1234

        lslw    $55
        lslw    ,x
        lslw    ,y
        lslw    ,sp
        lslw    *,pc
        lslw    10,x
        lslw    10,y
        lslw    10,sp
        lslw    *+10,pc
        lslw    $1234

        lsrw    $55
        lsrw    ,x
        lsrw    ,y
        lsrw    ,sp
        lsrw    *,pc
        lsrw    10,x
        lsrw    10,y
        lsrw    10,sp
        lsrw    *+10,pc
        lsrw    $1234

        negw    $55
        negw    ,x
        negw    ,y
        negw    ,sp
        negw    *,pc
        negw    10,x
        negw    10,y
        negw    10,sp
        negw    *+10,pc
        negw    $1234

        rolw    $55
        rolw    ,x
        rolw    ,y
        rolw    ,sp
        rolw    *,pc
        rolw    10,x
        rolw    10,y
        rolw    10,sp
        rolw    *+10,pc
        rolw    $1234

        rorw    $55
        rorw    ,x
        rorw    ,y
        rorw    ,sp
        rorw    *,pc
        rorw    10,x
        rorw    10,y
        rorw    10,sp
        rorw    *+10,pc
        rorw    $1234

        tstw    $55
        tstw    ,x
        tstw    ,y
        tstw    ,sp
        tstw    *,pc
        tstw    10,x
        tstw    10,y
        tstw    10,sp
        tstw    *+10,pc
        tstw    $1234

	addx	#$1234
        addx    $55
        addx    ,x
        addx    ,y
        addx    ,sp
        addx    *,pc
        addx    10,x
        addx    10,y
        addx    10,sp
        addx    *+10,pc
        addx    $1234

	addy	#$1234
        addy    $55
        addy    ,x
        addy    ,y
        addy    ,sp
        addy    *,pc
        addy    10,x
        addy    10,y
        addy    10,sp
        addy    *+10,pc
        addy    $1234

	aded	#$1234
        aded    $55
        aded    ,x
        aded    ,y
        aded    ,sp
        aded    *,pc
        aded    10,x
        aded    10,y
        aded    10,sp
        aded    *+10,pc
        aded    $1234

	adex	#$1234
        adex    $55
        adex    ,x
        adex    ,y
        adex    ,sp
        adex    *,pc
        adex    10,x
        adex    10,y
        adex    10,sp
        adex    *+10,pc
        adex    $1234

	adey	#$1234
        adey    $55
        adey    ,x
        adey    ,y
        adey    ,sp
        adey    *,pc
        adey    10,x
        adey    10,y
        adey    10,sp
        adey    *+10,pc
        adey    $1234

	andx	#$1234
        andx    $55
        andx    ,x
        andx    ,y
        andx    ,sp
        andx    *,pc
        andx    10,x
        andx    10,y
        andx    10,sp
        andx    *+10,pc
        andx    $1234

	andy	#$1234
        andy    $55
        andy    ,x
        andy    ,y
        andy    ,sp
        andy    *,pc
        andy    10,x
        andy    10,y
        andy    10,sp
        andy    *+10,pc
        andy    $1234

	bitx	#$1234
        bitx    $55
        bitx    ,x
        bitx    ,y
        bitx    ,sp
        bitx    *,pc
        bitx    10,x
        bitx    10,y
        bitx    10,sp
        bitx    *+10,pc
        bitx    $1234

	bity	#$1234
        bity    $55
        bity    ,x
        bity    ,y
        bity    ,sp
        bity    *,pc
        bity    10,x
        bity    10,y
        bity    10,sp
        bity    *+10,pc
        bity    $1234

	cped	#$1234
        cped    $55
        cped    ,x
        cped    ,y
        cped    ,sp
        cped    *,pc
        cped    10,x
        cped    10,y
        cped    10,sp
        cped    *+10,pc
        cped    $1234

	cpes	#$1234
        cpes    $55
        cpes    ,x
        cpes    ,y
        cpes    ,sp
        cpes    *,pc
        cpes    10,x
        cpes    10,y
        cpes    10,sp
        cpes    *+10,pc
        cpes    $1234

	cpex	#$1234
        cpex    $55
        cpex    ,x
        cpex    ,y
        cpex    ,sp
        cpex    *,pc
        cpex    10,x
        cpex    10,y
        cpex    10,sp
        cpex    *+10,pc
        cpex    $1234

	cpey	#$1234
        cpey    $55
        cpey    ,y
        cpey    ,y
        cpey    ,sp
        cpey    *,pc
        cpey    10,y
        cpey    10,y
        cpey    10,sp
        cpey    *+10,pc
        cpey    $1234

	eorx	#$1234
        eorx    $55
        eorx    ,x
        eorx    ,y
        eorx    ,sp
        eorx    *,pc
        eorx    10,x
        eorx    10,y
        eorx    10,sp
        eorx    *+10,pc
        eorx    $1234

	eory	#$1234
        eory    $55
        eory    ,y
        eory    ,y
        eory    ,sp
        eory    *,pc
        eory    10,y
        eory    10,y
        eory    10,sp
        eory    *+10,pc
        eory    $1234

	orx	#$1234
        orx     $55
        orx     ,x
        orx     ,y
        orx     ,sp
        orx     *,pc
        orx     10,x
        orx     10,y
        orx     10,sp
        orx     *+10,pc
        orx     $1234

	ory	#$1234
        ory     $55
        ory     ,y
        ory     ,y
        ory     ,sp
        ory      *,pc
        ory     10,y
        ory     10,y
        ory     10,sp
        ory     *+10,pc
        ory     $1234

        sbed    #$1234
        sbed    $55
        sbed    ,x
        sbed    ,y
        sbed    ,sp
        sbed    *,pc
        sbed    10,x
        sbed    10,y
        sbed    10,sp
        sbed    *+10,pc
        sbed    $1234

        sbex    #$1234
        sbex    $55
        sbex    ,x
        sbex    ,y
        sbex    ,sp
        sbex    *,pc
        sbex    10,x
        sbex    10,y
        sbex    10,sp
        sbex    *+10,pc
        sbex    $1234

        sbey    #$1234
        sbey    $55
        sbey    ,x
        sbey    ,y
        sbey    ,sp
        sbey     *,pc
        sbey    10,y
        sbey    10,y
        sbey    10,sp
        sbey    *+10,pc
        sbey    $1234

        subx    #$1234
        subx    $55
        subx    ,x
        subx    ,y
        subx    ,sp
        subx    *,pc
        subx    10,x
        subx    10,y
        subx    10,sp
        subx    *+10,pc
        subx    $1234

        suby    #$1234
        suby    $55
        suby    ,x
        suby    ,y
        suby    ,sp
        suby     *,pc
        suby    10,y
        suby    10,y
        suby    10,sp
        suby    *+10,pc
        suby    $1234


        gldaa   $55
        gldaa   ,x
        gldaa   ,y
        gldaa   ,sp
        gldaa    *,pc
        gldaa   10,y
        gldaa   10,y
        gldaa   10,sp
        gldaa   *+10,pc
        gldaa   $1234

        gldab   $55
        gldab   ,x
        gldab   ,y
        gldab   ,sp
        gldab    *,pc
        gldab   10,y
        gldab   10,y
        gldab   10,sp
        gldab   *+10,pc
        gldab   $1234

        gldd    $55
        gldd    ,x
        gldd    ,y
        gldd    ,sp
        gldd    *,pc
        gldd    10,y
        gldd    10,y
        gldd    10,sp
        gldd    *+10,pc
        gldd    $1234

        glds    $55
        glds    ,x
        glds    ,y
        glds    ,sp
        glds    *,pc
        glds    10,y
        glds    10,y
        glds    10,sp
        glds    *+10,pc
        glds    $1234

        gldx    $55
        gldx    ,x
        gldx    ,y
        gldx    ,sp
        gldx    *,pc
        gldx    10,y
        gldx    10,y
        gldx    10,sp
        gldx    *+10,pc
        gldx    $1234

        gldy    $55
        gldy    ,x
        gldy    ,y
        gldy    ,sp
        gldy    *,pc
        gldy    10,y
        gldy    10,y
        gldy    10,sp
        gldy    *+10,pc
        gldy    $1234

        gstaa   $55
        gstaa   ,x
        gstaa   ,y
        gstaa   ,sp
        gstaa    *,pc
        gstaa   10,y
        gstaa   10,y
        gstaa   10,sp
        gstaa   *+10,pc
        gstaa   $1234

        gstab   $55
        gstab   ,x
        gstab   ,y
        gstab   ,sp
        gstab    *,pc
        gstab   10,y
        gstab   10,y
        gstab   10,sp
        gstab   *+10,pc
        gstab   $1234

        gstd    $55
        gstd    ,x
        gstd    ,y
        gstd    ,sp
        gstd    *,pc
        gstd    10,y
        gstd    10,y
        gstd    10,sp
        gstd    *+10,pc
        gstd    $1234

        gsts    $55
        gsts    ,x
        gsts    ,y
        gsts    ,sp
        gsts    *,pc
        gsts    10,y
        gsts    10,y
        gsts    10,sp
        gsts    *+10,pc
        gsts    $1234

        gstx    $55
        gstx    ,x
        gstx    ,y
        gstx    ,sp
        gstx    *,pc
        gstx    10,y
        gstx    10,y
        gstx    10,sp
        gstx    *+10,pc
        gstx    $1234

        gsty    $55
        gsty    ,x
        gsty    ,y
        gsty    ,sp
        gsty    *,pc
        gsty    10,y
        gsty    10,y
        gsty    10,sp
        gsty    *+10,pc
        gsty    $1234

        btas    $55,#$45
        btas    ,x,#$45
        btas    ,y,#$45
        btas    ,sp,#$45
        btas    *,pc,#$45
        btas    10,y,#$45
        btas    10,y,#$45
        btas    10,sp,#$45
        btas    *+10,pc,#$45
        btas    $1234,#$45

	ldaa	$10
	ldaa	$1010
	assume	direct:$10
	ldaa	$10
	ldaa	$1010

	irp	reg,a,b,ccr,ccrl,d,x,y,sp,ccrh,xh,yh,sph
	tfr	a,reg
	endm

	irp	reg,a,b,ccr,ccrl,d,x,y,sp,xl,yl,spl
	tfr	b,reg
	endm

	irp	reg,a,b,ccr,ccrl,d,x,y,sp
	tfr	ccr,reg
	tfr	ccrl,reg
	endm

        irp     reg,a,b,ccr,ccrl,d,x,y,sp,ccrw
        tfr     d,reg
        endm

        irp     reg,a,b,ccr,ccrl,d,x,y,sp,ccrw
        tfr     x,reg
        endm

        irp     reg,a,b,ccr,ccrl,d,x,y,sp,ccrw
        tfr     y,reg
        endm

        irp     reg,a,b,ccr,ccrl,d,x,y,sp,ccrw
        tfr     sp,reg
        endm

	irp	reg,a,b,ccr,ccrl
	tfr	xl,reg
	endm

	irp	reg,a,b,ccr,ccrl
	tfr	yl,reg
	endm

	irp	reg,a,b,ccr,ccrl
	tfr	spl,reg
	endm

	tfr	xh,a
	tfr	yh,a
	tfr	sph,a
	tfr	ccrh,a

	irp	reg,ccrw,d,x,y,sp
	tfr	ccrw,reg
	endm

	movb	#$56,$1234
	movb	#$56,,y
	movb	#$56,64,y
	movb	#$56,1000,y
	movb	#$56,[d,sp]
	movb	#$56,[500,y]

        movb    $2356,$1234
        movb    $2356,,y
        movb    $2356,64,y
        movb    $2356,1000,y
        movb    $2356,[d,sp]
        movb    $2356,[500,y]

        movb    0,y,$1234
        movb    0,y,,y
        movb    0,y,64,y
        movb    0,y,1000,y
        movb    0,y,[d,sp]
        movb    0,y,[500,y]

        movb    100,y,$1234
        movb    100,y,,y
        movb    100,y,64,y
        movb    100,y,1000,y
        movb    100,y,[d,sp]
        movb    100,y,[500,y]

        movb    1000,y,$1234
        movb    1000,y,,y
        movb    1000,y,64,y
        movb    1000,y,10000,y
        movb    1000,y,[d,sp]
        movb    1000,y,[500,y]

        movb    [d,sp],$1234
        movb    [d,sp],,y
        movb    [d,sp],64,y
        movb    [d,sp],10000,y
        movb    [d,sp],[d,sp]
        movb    [d,sp],[500,y]

        movb    [500,y],$1234
        movb    [500,y],,y
        movb    [500,y],64,y
        movb    [500,y],10000,y
        movb    [500,y],[d,sp]
        movb    [500,y],[500,y]


	movw	#$a56,$1234
	movw	#$a56,,y
	movw	#$a56,64,y
	movw	#$a56,1000,y
	movw	#$a56,[d,sp]
	movw	#$a56,[500,y]

        movw    $2356,$1234
        movw    $2356,,y
        movw    $2356,64,y
        movw    $2356,1000,y
        movw    $2356,[d,sp]
        movw    $2356,[500,y]

        movw    0,y,$1234
        movw    0,y,,y
        movw    0,y,64,y
        movw    0,y,1000,y
        movw    0,y,[d,sp]
        movw    0,y,[500,y]

        movw    100,y,$1234
        movw    100,y,,y
        movw    100,y,64,y
        movw    100,y,1000,y
        movw    100,y,[d,sp]
        movw    100,y,[500,y]

        movw    1000,y,$1234
        movw    1000,y,,y
        movw    1000,y,64,y
        movw    1000,y,10000,y
        movw    1000,y,[d,sp]
        movw    1000,y,[500,y]

        movw    [d,sp],$1234
        movw    [d,sp],,y
        movw    [d,sp],64,y
        movw    [d,sp],10000,y
        movw    [d,sp],[d,sp]
        movw    [d,sp],[500,y]

        movw    [500,y],$1234
        movw    [500,y],,y
        movw    [500,y],64,y
        movw    [500,y],10000,y
        movw    [500,y],[d,sp]
        movw    [500,y],[500,y]
