	cpu	pdp-11/94
	supmode	on
	page	0

data	equ	0x100

	if 0
	; Similar to MACRO-11, only the form %n to address registers
        ; is built in (we however do not allow arbitrary arithmetic
        ; expressions for n, just 0..7).  There are default built-in
        ; aliases to refer to %n as Rn, %6 as SP and %7 as PC.  This
        ; may be turned off via the reg directive (without a name in
        ; the label column), but similar to MACRO-11, turning the default
        ; aliases off is discouraged:

	reg	off
r0	reg	%0
r1	reg	%1
r2	reg	%2
r3	reg	%3
r4	reg	%4
r5	reg	%5
r6	reg	%6
r7	reg	%7
sp	reg	%6
pc	reg	%7

	endif

	; register aliases

src	reg	r2
dest	equ	r5

	; test addressing modes:

	; -> register direct (0n)

	adc	%4
	adc	sp
	adc	pc
	adc	src
	adc	dest

	; -> register deferred (1n)

	adc	@r4
	adc	(r4)
	adc	@sp
	adc	(sp)
	adc	@pc
	adc	(pc)
	adc	@src
	adc	(src)
	adc	@dest
	adc	(dest)
	if	mompass > 1
	expect	1010
	endif
	adc	(undefined)
	if	mompass > 1
	endexpect
	endif

	; -> autoincrement (2n)

	adc	(%4)+
	adc	( sp )+
	adc	(pc)+
	adc	(src)+
	adc	(dest)+
	expect	1110
	; without defined register, gets interpreted as addition with missing second operand
	adc	(undefined)+
	endexpect

	; -> autoincrement deferred (3n)

	adc	@( r4 )+
	adc	@ (sp)+
	adc	@ ( pc )+
	adc	@(src)+
	adc	@(dest)+
	expect	1110
	; without defined register, gets interpreted as addition with missing second operand
	adc	@(undefined)+
	endexpect

	; -> autodecrement (4n)

	adc	-(%4)
	adc	-( sp )
	adc	-(pc)
	adc	-(src)
	adc	-(dest)
	if	mompass > 1
	expect	1010
	endif
	; without defined register, gets interpreted as negation
	adc	-(undefined)
	if	mompass > 1
	endexpect
	endif

	; -> autodecrement deferred (5n)

	adc	@-( r4 )
	adc	@ -(sp)
	adc	@ -( pc )
	adc	@-(src)
	adc	@-(dest)
	if	mompass > 1
	expect	1010
	endif
	; without defined register, gets interpreted as negation
	adc	@-(undefined)
	if	mompass > 1
	endexpect
	endif

	; -> index

	adc	10(%4)
	adc	-20(sp)
	adc	30(pc)
	adc	-40(src)
	adc	50(dest)
	if	mompass > 1
	expect	1010
	endif
	; register may be forward defined, so error occurs in pass 2
	adc	-60(undefined)
	if	mompass > 1
	endexpect
	endif

	; -> index deferred

	adc	@10(r4)
	adc	@-20(sp)
	adc	@30(pc)
	adc	@-40(src)
	adc	@50(dest)
	if	mompass > 1
	expect	1010
	endif
	; register may be forward defined, so error occurs in pass 2
	adc	@-60(undefined)
	if	mompass > 1
	endexpect
	endif

	; @(Rn) was wrongly 'optimized' to @Rn up to Build 242:

	adc	@(r4)

	; -> immediate, which is actually (PC)+

	mov	#1234,r3
	movb	#12,r3
	expect	1320
	movb	#300,r3
	endexpect

	; -> absolute, which is actually @(PC)+

	mov	@#1234,r3

	; "plain label" is PC-relative

	adc	data
	adc	@data

	; now we iterate though the instructions:

	adc	r1
	adc	@r2
	adc	(r3)+
	adc	@(r4)+
	adc	-(r5)
	adc	@-(r6)
	adc	0123456(r1)
	adc	@0123456(r2)
	expect	1350
	adc	#0123456
	endexpect
	adc	@#0123456
	adc	0123456
	adc	@0123456

	adcb	r1
	adcb	@r2
	adcb	(r3)+
	adcb	@(r4)+
	adcb	-(r5)
	adcb	@-(r6)
	adcb	0123456(r1)
	adcb	@0123456(r2)
	expect	1350
	adcb	#0123456
	endexpect
	adcb	@#0123456
	adcb	0123456
	adcb	@0123456

	add	%1,%2
	add	@r2,@r3
	add	(r3)+,(r4)+
	add	@(r4)+,@(r5)+
	add	-(r5),-(r6)
	add	@-(r6),@-(r1)
	add	0123456(r1),0134567(r2)
	add	@0123456(r2),@0134567(r3)
	expect	1350
	add	#0123456,#0134567
	endexpect
	add	#0123456,r4
	add	@#0123456,@#0134567
	add	0123456,0134567
	add	@0123456,@0134567

	ash	r1,r6
	ash	@r2,r6
	ash	(r3)+,r6
	ash	@(r4)+,r6
	ash	-(r5),r6
	ash	@-(r6),r6
	ash	0123456(r1),r6
	ash	@0123456(r2),r6
	ash	#0123456,r6
	ash	@#0123456,r6
	ash	0123456,r6
	ash	@0123456,r6
	expect	1350
	ash	r1,@r6
	endexpect

	ashc	r1,r4
	ashc	@r2,r4
	ashc	(r3)+,r4
	ashc	@(r4)+,r4
	ashc	-(r5),r4
	ashc	@-(r6),r4
	ashc	0123456(r1),r4
	ashc	@0123456(r2),r4
	ashc	#0123456,r4
	ashc	@#0123456,r4
	ashc	0123456,r4
	ashc	@0123456,r4
	expect	1350
	ashc	r1,@r4
	endexpect

	asl	%1
	asl	@r2
	asl	(r3)+
	asl	@(r4)+
	asl	-(r5)
	asl	@-(r6)
	asl	0123456(r1)
	asl	@0123456(r2)
	expect	1350
	asl	#0123456
	endexpect
	asl	@#0123456
	asl	0123456
	asl	@0123456

	aslb	r1
	aslb	@r2
	aslb	(r3)+
	aslb	@(r4)+
	aslb	-(r5)
	aslb	@-(r6)
	aslb	0123456(r1)
	aslb	@0123456(r2)
	expect	1350
	aslb	#0123456
	endexpect
	aslb	@#0123456
	aslb	0123456
	aslb	@0123456

	asr	r1
	asr	@r2
	asr	(r3)+
	asr	@(r4)+
	asr	-(r5)
	asr	@-(r6)
	asr	0123456(r1)
	asr	@0123456(r2)
	expect	1350
	asr	#0123456
	endexpect
	asr	@#0123456
	asr	0123456
	asr	@0123456

	asrb	r1
	asrb	@r2
	asrb	(r3)+
	asrb	@(r4)+
	asrb	-(r5)
	asrb	@-(r6)
	asrb	0123456(r1)
	asrb	@0123456(r2)
	expect	1350
	asrb	#0123456
	endexpect
	asrb	@#0123456
	asrb	0123456
	asrb	@0123456

	bcc	*+20

	bcs	*-30

	beq	*+40

	bge	*-50

	bgt	*+60

	bhi	*-70

	bhis	*+80

	bic	r1,r2
	bic	@r2,@r3
	bic	(r3)+,(r4)+
	bic	@(r4)+,@(r5)+
	bic	-(r5),-(r6)
	bic	@-(r6),@-(r1)
	bic	0123456(r1),0134567(r2)
	bic	@0123456(r2),@0134567(r3)
	expect	1350
	bic	#0123456,#0134567
	endexpect
	bic	#0123456,r4
	bic	@#0123456,@#0134567
	bic	0123456,0134567
	bic	@0123456,@0134567

	bicb	r1,r2
	bicb	@r2,@r3
	bicb	(r3)+,(r4)+
	bicb	@(r4)+,@(r5)+
	bicb	-(r5),-(r6)
	bicb	@-(r6),@-(r1)
	bicb	0123456(r1),0134567(r2)
	bicb	@0123456(r2),@0134567(r3)
	expect	1350
	bicb	#0123,#0134
	endexpect
	bicb	#0123,r4
	bicb	@#0123456,@#0134567
	bicb	0123456,0134567
	bicb	@0123456,@0134567

	bis	r1,r2
	bis	@r2,@r3
	bis	(r3)+,(r4)+
	bis	@(r4)+,@(r5)+
	bis	-(r5),-(r6)
	bis	@-(r6),@-(r1)
	bis	0123456(r1),0134567(r2)
	bis	@0123456(r2),@0134567(r3)
	expect	1350
	bis	#0123456,#0134567
	endexpect
	bis	#0123456,r4
	bis	@#0123456,@#0134567
	bis	0123456,0134567
	bis	@0123456,@0134567

	bisb	%1,%2
	bisb	@r2,@r3
	bisb	(r3)+,(r4)+
	bisb	@(r4)+,@(r5)+
	bisb	-(r5),-(r6)
	bisb	@-(r6),@-(r1)
	bisb	0123456(r1),0134567(r2)
	bisb	@0123456(r2),@0134567(r3)
	expect	1350
	bisb	#0123,#0134
	endexpect
	bisb	#0123,r4
	bisb	@#0123456,@#0134567
	bisb	0123456,0134567
	bisb	@0123456,@0134567

	bit	r1,r2
	bit	@r2,@r3
	bit	(r3)+,(r4)+
	bit	@(r4)+,@(r5)+
	bit	-(r5),-(r6)
	bit	@-(r6),@-(r1)
	bit	0123456(r1),0134567(r2)
	bit	@0123456(r2),@0134567(r3)
	bit	#0123456,#0134567
	bit	#0123456,r4
	bit	@#0123456,@#0134567
	bit	0123456,0134567
	bit	@0123456,@0134567

	bitb	r1,r2
	bitb	@r2,@r3
	bitb	(r3)+,(r4)+
	bitb	@(r4)+,@(r5)+
	bitb	-(r5),-(r6)
	bitb	@-(r6),@-(r1)
	bitb	0123456(r1),0134567(r2)
	bitb	@0123456(r2),@0134567(r3)
	bitb	#0123,#0134
	bitb	#0123,r4
	bitb	@#0123456,@#0134567
	bitb	0123456,0134567
	bitb	@0123456,@0134567

	ble	*-90

	blo	*+100

	blos	*-110

	blt	*+120

	bmi	*-130

	bne	*+140

	bpl	*-150

	bpt

	br	*-170
	expect	1375,1375
	br	*+77		; odd displacement not OK
	br	*-79
	endexpect
	br	*+256		; just OK
	br	*-254
	expect	1370,1370
	br	*+258
	br	*-256
	endexpect

	bvc	*+180

	bvs	*-190

	c	7
	expect	1320
	c	20
	endexpect

	; CALL is JSR with register 6 (SP)

	call	r1
	call	@r2
	call	(r3)+
	call	@(r4)+
	call	-(r5)
	call	@-(r6)
	call	0123456(r1)
	call	@0123456(r2)
	call	#0123456
	call	@#0123456
	call	0123456
	call	@0123456

	clr	r1
	clr	@r2
	clr	(r3)+
	clr	@(r4)+
	clr	-(r5)
	clr	@-(r6)
	clr	0123456(r1)
	clr	@0123456(r2)
	expect	1350
	clr	#0123456
	endexpect
	clr	@#0123456
	clr	0123456
	clr	@0123456

	clrb	r1
	clrb	@r2
	clrb	(r3)+
	clrb	@(r4)+
	clrb	-(r5)
	clrb	@-(r6)
	clrb	0123456(r1)
	clrb	@0123456(r2)
	expect	1350
	clrb	#0123456
	endexpect
	clrb	@#0123456
	clrb	0123456
	clrb	@0123456

	ccc
	clc
	cln
	clv
	clz

	cmp	r1,r2
	cmp	@r2,@r3
	cmp	(r3)+,(r4)+
	cmp	@(r4)+,@(r5)+
	cmp	-(r5),-(r6)
	cmp	@-(r6),@-(r1)
	cmp	0123456(r1),0134567(r2)
	cmp	@0123456(r2),@0134567(r3)
	cmp	#0123456,#0134567	; immediate allowed for both args!
	cmp	#0123456,r4
	cmp	@#0123456,@#0134567
	cmp	0123456,0134567
	cmp	@0123456,@0134567

	cmpb	r1,r2
	cmpb	@r2,@r3
	cmpb	(r3)+,(r4)+
	cmpb	@(r4)+,@(r5)+
	cmpb	-(r5),-(r6)
	cmpb	@-(r6),@-(r1)
	cmpb	0123456(r1),0134567(r2)
	cmpb	@0123456(r2),@0134567(r3)
	cmpb	#0123,#0134		; immediate allowed for both args!
	cmpb	#0123,r4
	cmpb	@#0123456,@#0134567
	cmpb	0123456,0134567
	cmpb	@0123456,@0134567

	com	r1
	com	@r2
	com	(r3)+
	com	@(r4)+
	com	-(r5)
	com	@-(r6)
	com	0123456(r1)
	com	@0123456(r2)
	expect	1350
	com	#0123456
	endexpect
	com	@#0123456
	com	0123456
	com	@0123456

	comb	r1
	comb	@r2
	comb	(r3)+
	comb	@(r4)+
	comb	-(r5)
	comb	@-(r6)
	comb	0123456(r1)
	comb	@0123456(r2)
	expect	1350
	comb	#0123456
	endexpect
	comb	@#0123456
	comb	0123456
	comb	@0123456

	csm	r1
	csm	@r2
	csm	(r3)+
	csm	@(r4)+
	csm	-(r5)
	csm	@-(r6)
	csm	0123456(r1)
	csm	@0123456(r2)
	csm	#0123456
	csm	@#0123456
	csm	0123456
	csm	@0123456

	dec	r1
	dec	@r2
	dec	(r3)+
	dec	@(r4)+
	dec	-(r5)
	dec	@-(r6)
	dec	0123456(r1)
	dec	@0123456(r2)
	expect	1350
	dec	#0123456
	endexpect
	dec	@#0123456
	dec	0123456
	dec	@0123456

	decb	r1
	decb	@r2
	decb	(r3)+
	decb	@(r4)+
	decb	-(r5)
	decb	@-(r6)
	decb	0123456(r1)
	decb	@0123456(r2)
	expect	1350
	decb	#0123456
	endexpect
	decb	@#0123456
	decb	0123456
	decb	@0123456

	div	r1,r4
	div	@r2,r4
	div	(r3)+,r4
	div	@(r4)+,r4
	div	-(r5),r4
	div	@-(r6),r4
	div	0123456(r1),r4
	div	@0123456(r2),r4
	div	#0123456,r4
	div	@#0123456,r4
	div	0123456,r4
	div	@0123456,r4
	expect	1350
	div	r1,@r4
	endexpect

	emt
	emt	20
	expect	1320
	emt	256
	endexpect

	halt

	inc	r1
	inc	@r2
	inc	(r3)+
	inc	@(r4)+
	inc	-(r5)
	inc	@-(r6)
	inc	0123456(r1)
	inc	@0123456(r2)
	expect	1350
	inc	#0123456
	endexpect
	inc	@#0123456
	inc	0123456
	inc	@0123456

	incb	r1
	incb	@r2
	incb	(r3)+
	incb	@(r4)+
	incb	-(r5)
	incb	@-(r6)
	incb	0123456(r1)
	incb	@0123456(r2)
	expect	1350
	incb	#0123456
	endexpect
	incb	@#0123456
	incb	0123456
	incb	@0123456

	iot

	expect	1350
	jmp	r1
	endexpect
	jmp	@r2
	jmp	(r3)+
	jmp	@(r4)+
	jmp	-(r5)
	jmp	@-(r6)
	jmp	0123456(r1)
	jmp	@0123456(r2)
	jmp	#0123456
	jmp	@#0123456
	jmp	0123456
	jmp	@0123456

	jsr	r4,r1
	jsr	r4,@r2
	jsr	r4,(r3)+
	jsr	r4,@(r4)+
	jsr	r4,-(r5)
	jsr	r4,@-(r6)
	jsr	r4,0123456(r1)
	jsr	r4,@0123456(r2)
	jsr	r4,#0123456
	jsr	r4,@#0123456
	jsr	r4,0123456
	jsr	r4,@0123456
	expect	1350
	jsr	@r4,r1
	endexpect

	mark	4
	expect	1320
	mark	64
	endexpect

	mfpd	r1
	mfpd	@r2
	mfpd	(r3)+
	mfpd	@(r4)+
	mfpd	-(r5)
	mfpd	@-(r6)
	mfpd	0123456(r1)
	mfpd	@0123456(r2)
	mfpd	#0123456
	mfpd	@#0123456
	mfpd	0123456
	mfpd	@0123456

	mfpi	r1
	mfpi	@r2
	mfpi	(r3)+
	mfpi	@(r4)+
	mfpi	-(r5)
	mfpi	@-(r6)
	mfpi	0123456(r1)
	mfpi	@0123456(r2)
	mfpi	#0123456
	mfpi	@#0123456
	mfpi	0123456
	mfpi	@0123456

	mfps	r1
	mfps	@r2
	mfps	(r3)+
	mfps	@(r4)+
	mfps	-(r5)
	mfps	@-(r6)
	mfps	0123456(r1)
	mfps	@0123456(r2)
	expect	1350
	mfps	#0123456
	endexpect
	mfps	@#0123456
	mfps	0123456
	mfps	@0123456

	mfpt

	mov	r1,r2
	mov	@r2,@r3
	mov	(r3)+,(r4)+
	mov	@(r4)+,@(r5)+
	mov	-(r5),-(r6)
	mov	@-(r6),@-(r1)
	mov	0123456(r1),0134567(r2)
	mov	@0123456(r2),@0134567(r3)
	expect	1350
	mov	#0123456,#0134567
	endexpect
	mov	#0123456,r4
	mov	@#0123456,@#0134567
	mov	0123456,0134567
	mov	@0123456,@0134567

	movb	r1,r2
	movb	@r2,@r3
	movb	(r3)+,(r4)+
	movb	@(r4)+,@(r5)+
	movb	-(r5),-(r6)
	movb	@-(r6),@-(r1)
	movb	0123456(r1),0134567(r2)
	movb	@0123456(r2),@0134567(r3)
	expect	1350
	movb	#0123,#0134
	endexpect
	movb	#0123,r4
	movb	@#0123456,@#0134567
	movb	0123456,0134567
	movb	@0123456,@0134567

	mtpd	r1
	mtpd	@r2
	mtpd	(r3)+
	mtpd	@(r4)+
	mtpd	-(r5)
	mtpd	@-(r6)
	mtpd	0123456(r1)
	mtpd	@0123456(r2)
	expect	1350
	mtpd	#0123456
	endexpect
	mtpd	@#0123456
	mtpd	0123456
	mtpd	@0123456

	mtpi	r1
	mtpi	@r2
	mtpi	(r3)+
	mtpi	@(r4)+
	mtpi	-(r5)
	mtpi	@-(r6)
	mtpi	0123456(r1)
	mtpi	@0123456(r2)
	expect	1350
	mtpi	#0123456
	endexpect
	mtpi	@#0123456
	mtpi	0123456
	mtpi	@0123456

	mtps	r1
	mtps	@r2
	mtps	(r3)+
	mtps	@(r4)+
	mtps	-(r5)
	mtps	@-(r6)
	mtps	0123456(r1)
	mtps	@0123456(r2)
	expect	1320
	mtps	#0123456
	endexpect
	mtps	#0123
	mtps	@#0123456
	mtps	0123456
	mtps	@0123456

	mul	r1,r4
	mul	@r2,r4
	mul	(r3)+,r4
	mul	@(r4)+,r4
	mul	-(r5),r4
	mul	@-(r6),r4
	mul	0123456(r1),r4
	mul	@0123456(r2),r4
	mul	#0123456,r4
	mul	@#0123456,r4
	mul	0123456,r4
	mul	@0123456,r4
	expect	1350
	mul	r1,@r4
	endexpect

	neg	r1
	neg	@r2
	neg	(r3)+
	neg	@(r4)+
	neg	-(r5)
	neg	@-(r6)
	neg	0123456(r1)
	neg	@0123456(r2)
	expect	1350
	neg	#0123456
	endexpect
	neg	@#0123456
	neg	0123456
	neg	@0123456

	negb	r1
	negb	@r2
	negb	(r3)+
	negb	@(r4)+
	negb	-(r5)
	negb	@-(r6)
	negb	0123456(r1)
	negb	@0123456(r2)
	expect	1350
	negb	#0123456
	endexpect
	negb	@#0123456
	negb	0123456
	negb	@0123456

	nop

	reset

	rol	r1
	rol	@r2
	rol	(r3)+
	rol	@(r4)+
	rol	-(r5)
	rol	@-(r6)
	rol	0123456(r1)
	rol	@0123456(r2)
	expect	1350
	rol	#0123456
	endexpect
	rol	@#0123456
	rol	0123456
	rol	@0123456

	rolb	r1
	rolb	@r2
	rolb	(r3)+
	rolb	@(r4)+
	rolb	-(r5)
	rolb	@-(r6)
	rolb	0123456(r1)
	rolb	@0123456(r2)
	expect	1350
	rolb	#0123456
	endexpect
	rolb	@#0123456
	rolb	0123456
	rolb	@0123456

	ror	r1
	ror	@r2
	ror	(r3)+
	ror	@(r4)+
	ror	-(r5)
	ror	@-(r6)
	ror	0123456(r1)
	ror	@0123456(r2)
	expect	1350
	ror	#0123456
	endexpect
	ror	@#0123456
	ror	0123456
	ror	@0123456

	rorb	r1
	rorb	@r2
	rorb	(r3)+
	rorb	@(r4)+
	rorb	-(r5)
	rorb	@-(r6)
	rorb	0123456(r1)
	rorb	@0123456(r2)
	expect	1350
	rorb	#0123456
	endexpect
	rorb	@#0123456
	rorb	0123456
	rorb	@0123456

	rti

	rts	r4
	expect	1350
	rts	(r4)
	endexpect

	rtt

	sbc	r1
	sbc	@r2
	sbc	(r3)+
	sbc	@(r4)+
	sbc	-(r5)
	sbc	@-(r6)
	sbc	0123456(r1)
	sbc	@0123456(r2)
	expect	1350
	sbc	#0123456
	endexpect
	sbc	@#0123456
	sbc	0123456
	sbc	@0123456

	sbcb	r1
	sbcb	@r2
	sbcb	(r3)+
	sbcb	@(r4)+
	sbcb	-(r5)
	sbcb	@-(r6)
	sbcb	0123456(r1)
	sbcb	@0123456(r2)
	expect	1350
	sbcb	#0123456
	endexpect
	sbcb	@#0123456
	sbcb	0123456
	sbcb	@0123456

	s	7
	expect	1320
	s	20
	endexpect

	scc
	sec
	sen
	sev
	sez

	expect	1350,1350
	sob	(r4),*		; counter must be register
	sob	#0123456,*
	endexpect
	sob	r4,*
	expect	1375
	sob	r4,*+1		; no odd displacement
	endexpect
	sob	r4,*+2		; just allowed (distance 0 words)
	expect	1370
	sob	r4,*+4		; not allowed (would be distance +1 word)
	endexpect
	sob	r4,*-124	; just allowed (distance -63 words)
	expect	1370
	sob	r4,*-126	; not allowed (would be distance -64 words)
	endexpect

	spl	2
	expect	1320
	spl	8
	endexpect

	sub	r1,r2
	sub	@r2,@r3
	sub	(r3)+,(r4)+
	sub	@(r4)+,@(r5)+
	sub	-(r5),-(r6)
	sub	@-(r6),@-(r1)
	sub	0123456(r1),0134567(r2)
	sub	@0123456(r2),@0134567(r3)
	expect	1350
	sub	#0123456,#0134567
	endexpect
	sub	#0123456,r4
	sub	@#0123456,@#0134567
	sub	0123456,0134567
	sub	@0123456,@0134567

	swab	r1
	swab	@r2
	swab	(r3)+
	swab	@(r4)+
	swab	-(r5)
	swab	@-(r6)
	swab	0123456(r1)
	swab	@0123456(r2)
	expect	1350
	swab	#0123456
	endexpect
	swab	@#0123456
	swab	0123456
	swab	@0123456

	sxt	r1
	sxt	@r2
	sxt	(r3)+
	sxt	@(r4)+
	sxt	-(r5)
	sxt	@-(r6)
	sxt	0123456(r1)
	sxt	@0123456(r2)
	expect	1350
	sxt	#0123456
	endexpect
	sxt	@#0123456
	sxt	0123456
	sxt	@0123456

	trap
	trap	20
	expect	1320
	trap	256
	endexpect

	tst	r1
	tst	@r2
	tst	(r3)+
	tst	@(r4)+
	tst	-(r5)
	tst	@-(r6)
	tst	0123456(r1)
	tst	@0123456(r2)
	expect	1350
	tst	#0123456
	endexpect
	tst	@#0123456
	tst	0123456
	tst	@0123456

	tstb	r1
	tstb	@r2
	tstb	(r3)+
	tstb	@(r4)+
	tstb	-(r5)
	tstb	@-(r6)
	tstb	0123456(r1)
	tstb	@0123456(r2)
	expect	1350
	tstb	#0123456
	endexpect
	tstb	@#0123456
	tstb	0123456
	tstb	@0123456

	tstset	r1
	tstset	@r2
	tstset	(r3)+
	tstset	@(r4)+
	tstset	-(r5)
	tstset	@-(r6)
	tstset	0123456(r1)
	tstset	@0123456(r2)
	expect	1350
	tstset	#0123456
	endexpect
	tstset	@#0123456
	tstset	0123456
	tstset	@0123456

	wait

	expect	1350
	wrtlck	r1
	endexpect
	wrtlck	@r2
	wrtlck	(r3)+
	wrtlck	@(r4)+
	wrtlck	-(r5)
	wrtlck	@-(r6)
	wrtlck	0123456(r1)
	wrtlck	@0123456(r2)
	expect	1350
	wrtlck	#0123456
	endexpect
	wrtlck	@#0123456
	wrtlck	0123456
	wrtlck	@0123456

	xor	r4,r1
	xor	r4,@r2
	xor	r4,(r3)+
	xor	r4,@(r4)+
	xor	r4,-(r5)
	xor	r4,@-(r6)
	xor	r4,0123456(r1)
	xor	r4,@0123456(r2)
        expect  1350
	xor	r4,#0123456
        endexpect
	xor	r4,@#0123456
	xor	r4,0123456
	xor	r4,@0123456
	expect	1350
	xor	@r4,r1
	endexpect

	; -------------------------------
	; FIS instruction set on selected models,
        ; with proper option installed:

	cpu	pdp-11/40
	fis	off
	expect	1500
	fadd	r4
	endexpect

	fis	on
	fadd	r4
	fsub	r4
	fmul	r4
	fdiv	r4

	; -------------------------------
	; similar to FP11 extension:

	cpu	pdp-11/45
	fp11	on

fsrc	equ	ac2
fdest	reg	ac5

	; test aliases & invalid operands once:

	absf	ac2
	absf	fsrc
	absf	ac5
	absf	fdest
	expect	1149
	absf	r5		; no CPU registers for FP11
	endexpect
	if	mompass>1
	expect	1010
	endif
	absf	ac6
	if	mompass>1
	endexpect
	endif
	absf	@r2
	absf	(r3)+
	absf	@(r4)+
	absf	-(r5)
	absf	@-(r6)
	absf	0123456(r1)
	absf	@0123456(r2)
	expect	1350
	absf	#0123456
	endexpect
	absf	@#0123456
	absf	0123456
	absf	@0123456

	absd	ac2
	absd	ac5
	absd	@r2
	absd	(r3)+
	absd	@(r4)+
	absd	-(r5)
	absd	@-(r6)
	absd	0123456(r1)
	absd	@0123456(r2)
	expect	1350
	absd	#0123456
	endexpect
	absd	@#0123456
	absd	0123456
	absd	@0123456

	addf	ac4,ac2
	addf	@r2,ac2
	addf	(r3)+,ac2
	addf	@(r4)+,ac2
	addf	-(r5),ac2
	addf	@-(r6),ac2
	addf	0123456(r1),ac2
	addf	@0123456(r2),ac2
	addf	#1.5,ac2
	addf	@#0123456,ac2
	addf	0123456,ac2
	addf	@0123456,ac2

	addd	ac4,ac2
	addd	@r2,ac2
	addd	(r3)+,ac2
	addd	@(r4)+,ac2
	addd	-(r5),ac2
	addd	@-(r6),ac2
	addd	0123456(r1),ac2
	addd	@0123456(r2),ac2
	addd	#1.5,ac2
	addd	@#0123456,ac2
	addd	0123456,ac2
	addd	@0123456,ac2

	cfcc

	clrf	ac2
	clrf	ac5
	clrf	@r2
	clrf	(r3)+
	clrf	@(r4)+
	clrf	-(r5)
	clrf	@-(r6)
	clrf	0123456(r1)
	clrf	@0123456(r2)
	expect	1350
	clrf	#0123456
	endexpect
	clrf	@#0123456
	clrf	0123456
	clrf	@0123456

	clrd	ac2
	clrd	ac5
	clrd	@r2
	clrd	(r3)+
	clrd	@(r4)+
	clrd	-(r5)
	clrd	@-(r6)
	clrd	0123456(r1)
	clrd	@0123456(r2)
	expect	1350
	clrd	#0123456
	endexpect
	clrd	@#0123456
	clrd	0123456
	clrd	@0123456

	cmpf	ac4,ac2
	cmpf	@r2,ac2
	cmpf	(r3)+,ac2
	cmpf	@(r4)+,ac2
	cmpf	-(r5),ac2
	cmpf	@-(r6),ac2
	cmpf	0123456(r1),ac2
	cmpf	@0123456(r2),ac2
	cmpf	#1.5,ac2
	cmpf	@#0123456,ac2
	cmpf	0123456,ac2
	cmpf	@0123456,ac2

	cmpd	ac4,ac2
	cmpd	@r2,ac2
	cmpd	(r3)+,ac2
	cmpd	@(r4)+,ac2
	cmpd	-(r5),ac2
	cmpd	@-(r6),ac2
	cmpd	0123456(r1),ac2
	cmpd	@0123456(r2),ac2
	cmpd	#1.5,ac2
	cmpd	@#0123456,ac2
	cmpd	0123456,ac2
	cmpd	@0123456,ac2

	divf	ac4,ac2
	divf	@r2,ac2
	divf	(r3)+,ac2
	divf	@(r4)+,ac2
	divf	-(r5),ac2
	divf	@-(r6),ac2
	divf	0123456(r1),ac2
	divf	@0123456(r2),ac2
	divf	#1.5,ac2
	divf	@#0123456,ac2
	divf	0123456,ac2
	divf	@0123456,ac2

	divd	ac4,ac2
	divd	@r2,ac2
	divd	(r3)+,ac2
	divd	@(r4)+,ac2
	divd	-(r5),ac2
	divd	@-(r6),ac2
	divd	0123456(r1),ac2
	divd	@0123456(r2),ac2
	divd	#1.5,ac2
	divd	@#0123456,ac2
	divd	0123456,ac2
	divd	@0123456,ac2

	ldcfd	ac4,ac2			; load F32 from memory into F64 AC
	ldcfd	@r2,ac2
	ldcfd	(r3)+,ac2
	ldcfd	@(r4)+,ac2
	ldcfd	-(r5),ac2
	ldcfd	@-(r6),ac2
	ldcfd	0123456(r1),ac2
	ldcfd	@0123456(r2),ac2
	ldcfd	#1.5,ac2
	ldcfd	@#0123456,ac2
	ldcfd	0123456,ac2
	ldcfd	@0123456,ac2

	ldcdf	ac4,ac2			; load F64 from memory into F32 AC
	ldcdf	@r2,ac2
	ldcdf	(r3)+,ac2
	ldcdf	@(r4)+,ac2
	ldcdf	-(r5),ac2
	ldcdf	@-(r6),ac2
	ldcdf	0123456(r1),ac2
	ldcdf	@0123456(r2),ac2
	ldcdf	#1.5,ac2
	ldcdf	@#0123456,ac2
	ldcdf	0123456,ac2
	ldcdf	@0123456,ac2

	ldcif	r4,ac2			; load I16 from memory into F32 AC
	expect	1134
	ldcif	ac4,ac2
	endexpect
	ldcif	@r2,ac2
	ldcif	(r3)+,ac2
	ldcif	@(r4)+,ac2
	ldcif	-(r5),ac2
	ldcif	@-(r6),ac2
	ldcif	0123456(r1),ac2
	ldcif	@0123456(r2),ac2
	expect	1133
	ldcif	#1.5,ac2
	endexpect
	expect	1320
	ldcif	#150000,ac2
	endexpect
	ldcif	#1500,ac2
	ldcif	@#0123456,ac2
	ldcif	0123456,ac2
	ldcif	@0123456,ac2

	ldcid	r4,ac2			; load I16 from memory into F64 AC
	expect	1134
	ldcid	ac4,ac2
	endexpect
	ldcid	@r2,ac2
	ldcid	(r3)+,ac2
	ldcid	@(r4)+,ac2
	ldcid	-(r5),ac2
	ldcid	@-(r6),ac2
	ldcid	0123456(r1),ac2
	ldcid	@0123456(r2),ac2
	expect	1133
	ldcid	#1.5,ac2
	endexpect
	expect	1320
	ldcid	#15000000,ac2
	endexpect
	ldcid	#1500,ac2
	ldcid	@#0123456,ac2
	ldcid	0123456,ac2
	ldcid	@0123456,ac2

	ldclf	r4,ac2			; load I32 from memory into F32 AC
	expect	1134
	ldclf	ac4,ac2
	endexpect
	ldclf	@r2,ac2
	ldclf	(r3)+,ac2
	ldclf	@(r4)+,ac2
	ldclf	-(r5),ac2
	ldclf	@-(r6),ac2
	ldclf	0123456(r1),ac2
	ldclf	@0123456(r2),ac2
	expect	1133
	ldclf	#1.5,ac2
	endexpect
	ldclf	#150000,ac2
	ldclf	#1500,ac2
	ldclf	@#0123456,ac2
	ldclf	0123456,ac2
	ldclf	@0123456,ac2

	ldcld	r4,ac2			; load I32 from memory into F64 AC
	expect	1134
	ldcld	ac4,ac2
	endexpect
	ldcld	@r2,ac2
	ldcld	(r3)+,ac2
	ldcld	@(r4)+,ac2
	ldcld	-(r5),ac2
	ldcld	@-(r6),ac2
	ldcld	0123456(r1),ac2
	ldcld	@0123456(r2),ac2
	expect	1133
	ldcld	#1.5,ac2
	endexpect
	ldcld	#15000000,ac2
	ldcld	#1500,ac2
	ldcld	@#0123456,ac2
	ldcld	0123456,ac2
	ldcld	@0123456,ac2

	ldexp	r4,ac2
	expect	1134
	ldexp	ac4,ac2
	endexpect
	ldexp	@r2,ac2
	ldexp	(r3)+,ac2
	ldexp	@(r4)+,ac2
	ldexp	-(r5),ac2
	ldexp	@-(r6),ac2
	ldexp	0123456(r1),ac2
	ldexp	@0123456(r2),ac2
	expect	1133
	ldexp	#1.5,ac2
	endexpect
	expect	1320
	ldexp	#15000000,ac2
	endexpect
	ldexp	#1500,ac2
	ldexp	@#0123456,ac2
	ldexp	0123456,ac2
	ldexp	@0123456,ac2

	ldf	ac4,ac2
	ldf	@r2,ac2
	ldf	(r3)+,ac2
	ldf	@(r4)+,ac2
	ldf	-(r5),ac2
	ldf	@-(r6),ac2
	ldf	0123456(r1),ac2
	ldf	@0123456(r2),ac2
	ldf	#1.5,ac2
	ldf	@#0123456,ac2
	ldf	0123456,ac2
	ldf	@0123456,ac2

	ldd	ac4,ac2
	ldd	@r2,ac2
	ldd	(r3)+,ac2
	ldd	@(r4)+,ac2
	ldd	-(r5),ac2
	ldd	@-(r6),ac2
	ldd	0123456(r1),ac2
	ldd	@0123456(r2),ac2
	ldd	#1.5,ac2
	ldd	@#0123456,ac2
	ldd	0123456,ac2
	ldd	@0123456,ac2

	ldfps	r1
	ldfps	@r2
	ldfps	(r3)+
	ldfps	@(r4)+
	ldfps	-(r5)
	ldfps	@-(r6)
	ldfps	0123456(r1)
	ldfps	@0123456(r2)
	ldfps	#0123456
	ldfps	@#0123456
	ldfps	0123456
	ldfps	@0123456

	modf	ac4,ac2
	modf	@r2,ac2
	modf	(r3)+,ac2
	modf	@(r4)+,ac2
	modf	-(r5),ac2
	modf	@-(r6),ac2
	modf	0123456(r1),ac2
	modf	@0123456(r2),ac2
	modf	#1.5,ac2
	modf	@#0123456,ac2
	modf	0123456,ac2
	modf	@0123456,ac2

	modd	ac4,ac2
	modd	@r2,ac2
	modd	(r3)+,ac2
	modd	@(r4)+,ac2
	modd	-(r5),ac2
	modd	@-(r6),ac2
	modd	0123456(r1),ac2
	modd	@0123456(r2),ac2
	modd	#1.5,ac2
	modd	@#0123456,ac2
	modd	0123456,ac2
	modd	@0123456,ac2

	mulf	ac4,ac2
	mulf	@r2,ac2
	mulf	(r3)+,ac2
	mulf	@(r4)+,ac2
	mulf	-(r5),ac2
	mulf	@-(r6),ac2
	mulf	0123456(r1),ac2
	mulf	@0123456(r2),ac2
	mulf	#1.5,ac2
	mulf	@#0123456,ac2
	mulf	0123456,ac2
	mulf	@0123456,ac2

	muld	ac4,ac2
	muld	@r2,ac2
	muld	(r3)+,ac2
	muld	@(r4)+,ac2
	muld	-(r5),ac2
	muld	@-(r6),ac2
	muld	0123456(r1),ac2
	muld	@0123456(r2),ac2
	muld	#1.5,ac2
	muld	@#0123456,ac2
	muld	0123456,ac2
	muld	@0123456,ac2

	negf	ac2
	negf	ac5
	negf	@r2
	negf	(r3)+
	negf	@(r4)+
	negf	-(r5)
	negf	@-(r6)
	negf	0123456(r1)
	negf	@0123456(r2)
	expect	1350
	negf	#0123456
	endexpect
	negf	@#0123456
	negf	0123456
	negf	@0123456

	negd	ac2
	negd	ac5
	negd	@r2
	negd	(r3)+
	negd	@(r4)+
	negd	-(r5)
	negd	@-(r6)
	negd	0123456(r1)
	negd	@0123456(r2)
	expect	1350
	negd	#0123456
	endexpect
	negd	@#0123456
	negd	0123456
	negd	@0123456

	setd
	setf
	seti
	setl

	stf	ac2,ac4
	stf	ac2,@r2
	stf	ac2,(r3)+
	stf	ac2,@(r4)+
	stf	ac2,-(r5)
	stf	ac2,@-(r6)
	stf	ac2,0123456(r1)
	stf	ac2,@0123456(r2)
	expect	1350
	stf	ac2,#1.5
	endexpect
	stf	ac2,@#0123456
	stf	ac2,0123456
	stf	ac2,@0123456

	std	ac2,ac4
	std	ac2,@r2
	std	ac2,(r3)+
	std	ac2,@(r4)+
	std	ac2,-(r5)
	std	ac2,@-(r6)
	std	ac2,0123456(r1)
	std	ac2,@0123456(r2)
	expect	1350
	std	ac2,#1.5
	endexpect
	std	ac2,@#0123456
	std	ac2,0123456
	std	ac2,@0123456

	stcfi	ac2,r4			; store F32 AC to I16 in memory
	expect	1134
	stcfi	ac2,ac4
	endexpect
	stcfi	ac2,@r2
	stcfi	ac2,(r3)+
	stcfi	ac2,@(r4)+
	stcfi	ac2,-(r5)
	stcfi	ac2,@-(r6)
	stcfi	ac2,0123456(r1)
	stcfi	ac2,@0123456(r2)
	expect	1350
	stcfi	ac2,#1500
	endexpect
	stcfi	ac2,@#0123456
	stcfi	ac2,0123456
	stcfi	ac2,@0123456

	stcdi	ac2,r4			; store F64 AC to I16 in memory
	expect	1134
	stcdi	ac2,ac4
	endexpect
	stcdi	ac2,@r2
	stcdi	ac2,(r3)+
	stcdi	ac2,@(r4)+
	stcdi	ac2,-(r5)
	stcdi	ac2,@-(r6)
	stcdi	ac2,0123456(r1)
	stcdi	ac2,@0123456(r2)
	expect	1350
	stcdi	ac2,#1500
	endexpect
	stcdi	ac2,@#0123456
	stcdi	ac2,0123456
	stcdi	ac2,@0123456

	stcfl	ac2,r4			; store F32 AC to I32 in memory
	expect	1134
	stcfl	ac2,ac4
	endexpect
	stcfl	ac2,@r2
	stcfl	ac2,(r3)+
	stcfl	ac2,@(r4)+
	stcfl	ac2,-(r5)
	stcfl	ac2,@-(r6)
	stcfl	ac2,0123456(r1)
	stcfl	ac2,@0123456(r2)
	expect	1350
	stcfl	ac2,#1500
	endexpect
	stcfl	ac2,@#0123456
	stcfl	ac2,0123456
	stcfl	ac2,@0123456

	stcdl	ac2,r4			; store F64 AC to I32 in memory
	expect	1134
	stcdl	ac2,ac4
	endexpect
	stcdl	ac2,@r2
	stcdl	ac2,(r3)+
	stcdl	ac2,@(r4)+
	stcdl	ac2,-(r5)
	stcdl	ac2,@-(r6)
	stcdl	ac2,0123456(r1)
	stcdl	ac2,@0123456(r2)
	expect	1350
	stcdl	ac2,#1500
	endexpect
	stcdl	ac2,@#0123456
	stcdl	ac2,0123456
	stcdl	ac2,@0123456

	stexp	ac2,r4
	expect	1134
	stexp	ac2,ac4
	endexpect
	stexp	ac2,@r2
	stexp	ac2,(r3)+
	stexp	ac2,@(r4)+
	stexp	ac2,-(r5)
	stexp	ac2,@-(r6)
	stexp	ac2,0123456(r1)
	stexp	ac2,@0123456(r2)
	expect	1350
	stexp	ac2,#1500
	endexpect
	stexp	ac2,@#0123456
	stexp	ac2,0123456
	stexp	ac2,@0123456

	stfps	r1
	stfps	@r2
	stfps	(r3)+
	stfps	@(r4)+
	stfps	-(r5)
	stfps	@-(r6)
	stfps	0123456(r1)
	stfps	@0123456(r2)
	expect	1350
	stfps	#0123456
	endexpect
	stfps	@#0123456
	stfps	0123456
	stfps	@0123456

	stst	r1
	stst	@r2
	stst	(r3)+
	stst	@(r4)+
	stst	-(r5)
	stst	@-(r6)
	stst	0123456(r1)
	stst	@0123456(r2)
	expect	1350
	stst	#0123456
	endexpect
	stst	@#0123456
	stst	0123456
	stst	@0123456

	subf	ac4,ac2
	subf	@r2,ac2
	subf	(r3)+,ac2
	subf	@(r4)+,ac2
	subf	-(r5),ac2
	subf	@-(r6),ac2
	subf	0123456(r1),ac2
	subf	@0123456(r2),ac2
	subf	#1.5,ac2
	subf	@#0123456,ac2
	subf	0123456,ac2
	subf	@0123456,ac2

	subd	ac4,ac2
	subd	@r2,ac2
	subd	(r3)+,ac2
	subd	@(r4)+,ac2
	subd	-(r5),ac2
	subd	@-(r6),ac2
	subd	0123456(r1),ac2
	subd	@0123456(r2),ac2
	subd	#1.5,ac2
	subd	@#0123456,ac2
	subd	0123456,ac2
	subd	@0123456,ac2

	tstf	ac2
	tstf	ac5
	tstf	@r2
	tstf	(r3)+
	tstf	@(r4)+
	tstf	-(r5)
	tstf	@-(r6)
	tstf	0123456(r1)
	tstf	@0123456(r2)
	tstf	#1.5
	tstf	@#0123456
	tstf	0123456
	tstf	@0123456

	tstd	ac2
	tstd	ac5
	tstd	@r2
	tstd	(r3)+
	tstd	@(r4)+
	tstd	-(r5)
	tstd	@-(r6)
	tstd	0123456(r1)
	tstd	@0123456(r2)
	tstd	#1.5
	tstd	@#0123456
	tstd	0123456
	tstd	@0123456

	flt2	0.5		; 4000 0000 (hex) 040000 000000 (oct)
	flt2	1.5		; 40C0 0000 (hex) 040300 000000 (oct)
	flt2	12		; 4240 0000 (hex) 041100 000000 (oct)
	expect	1320
	flt2	1e39
	endexpect

	flt4	0.5		; 4000 0000 0000 0000 (hex) 040000 000000 000000 000000 (oct)
	flt4	-0.5		; C000 0000 0000 0000 (hex) 140000 000000 000000 000000 (oct)
	flt4	1.5		; 40C0 0000 0000 0000 (hex) 040300 000000 000000 000000 (oct)
	flt4	-1.5		; C0C0 0000 0000 0000 (hex) 140300 000000 000000 000000 (oct)
	flt4	12		; 4240 0000 0000 0000 (hex) 041100 000000 000000 000000 (oct)
	flt4	-12		; C240 0000 0000 0000 (hex) 141100 000000 000000 000000 (oct)
	;flt4	0.1		; 3ECC CCCC CCCC CCD0 (hex) 037314 146314 146314 146320 (oct)
	;flt4	1.0/3		; 3FAA AAAA AAAA AAA8 (hex) 037652 125252 125252 125250 (oct)
	expect	1320
	flt4	1e39
	endexpect

	; --------------------
	; CIS extensions

	cpu	pdp-11/03
	cis	on

	addn
	addni	0123456,0123462,0123466 

	addp
	addpi	0123456,0123462,0123466

	ashn
	ashni	0123456,0123462,-3

	ashp
	ashpi	0123456,0123462,3

	cmpc
	cmpci	0123456,0123462,' '

	cmpn
	cmpni	0123456,0123462

	cmpp
	cmppi	0123456,0123462

	cvtln
	cvtlni	0123456,0123462

	cvtlp
	cvtlpi	0123456,0123462

	cvtnl
	cvtnli	0123456,0123462

	cvtpl
	cvtpli	0123456,0123462

	cvtnp
	cvtnpi	0123456,0123462

	cvtpn
	cvtpni	0123456,0123462

	divp
	divpi	0123456,0123462,0123466

	locc
	locci	0123456,0123462,'a'

	l2dr	(r6)+
	l3dr	(r6)+

	matc
	matci	0123456,0123462

	movc
	movci	0123456,0123462,' '

	movrc
	movrci	0123456,0123462,' '

	movtc
	movtci	0123456,0123462,' ',0123466

	mulp
	mulpi	0123456,0123462,0123466

	scanc
	scanci	0123456,0123462

	skpc
	skpci	0123456,' '

	spanc
	spanci	0123456,0123462

	subn
	subni	0123456,0123462,0123466 

	subp
	subpi	0123456,0123462,0123466

	; 68K inherited a lot from PDP-11, so padding is on by
        ; default:

	byte	1,2,3
	nop			; padding byte is inserted before machine instruction

	word	1,2,3

	padding	off		; no padding is inserted...
	byte	1,2,3
	expect	180
	nop			; ...resulting in a misalignment warning
	endexpect

	; PDP-11 is the only target that implements multi
	; character constants in little endian mode, so this
	; results in text in memory that is not byte swapped:

	word	'Th','e ','qu','ic','k ','br','ow','n ','fo'
	word	'x ','ju','mp','s ','ov','er',' t','he',' l'
	word	'az','y ','do','g.'
