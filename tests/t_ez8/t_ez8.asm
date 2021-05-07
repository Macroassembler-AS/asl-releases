	cpu	eZ8

	ccf
	rcf
	scf

	di
	ei

	halt
	stop

	wdt

	iret
	ret

	nop

	atm

	and	r7,r13
	and	r5,vari
	and	vari,r4

	and	r3,@r14
	and	r2,@ivari
	and	vari,@r6

	and	r3,#5
	and	vari,#77
	and	@r9,#35h
	and	@ivari,#10011b

	add	>2,>5		; disallow work register addressing
	adc	r5,#4
	sub	@r0,#20
	sbc	r7,vari
	or	vari,@r5
	tcm	r0,@r8
	tm	@ivari,#00001000b
	cp	vari,#20
	xor	r5,#255

	inc	r5
	inc	@r12
	inc	vari

	dec	r6
	dec	vari
	dec	@r5
	dec	@ivari
	decw	rr6
	decw	vari
	decw	@r5
	decw	@ivari
	decw	@r5

test1:	jr	test1
	jr	f,test1
	jr	uge,test1

	djnz	r5,test1

	call	test1
	call	@iwvari
	call	@rr10

	jp	test1
	jp	c,test1
	jp	@iwvari
	jp	@rr6


	ld	r3,r4
	ld	r5,vari
	ld	r6,@r7
	ld	r8,@ivari
	ld	r9,vari(r10)
	ld	r11,#45

	ld	vari,r12
	ld	vari,vari
	ld	vari,@r13
	ld	vari,@ivari
	ld	vari,#67

	ld	@r14,r15
	ld	@r0,vari
	ld	@r1,#89

	ld	@ivari,r2
	ld	@ivari,vari
	ld	@ivari,#01

	ld	vari(r3),r4


	ldc	r5,@rr6
	ldc	@rr8,r7
	lde	r9,@rr10
	lde	@rr12,r11

	ldci	@r13,@rr14
	ldci	@rr0,@r15
	ldei	@r1,@rr2
	ldei	@rr4,@r3


	srp	#0

;-------------------------------------
; new eZ8 instrs

	brk

	adcx	634h,0b12h
	adcx	r4,0b12h
	adcx	46ch,#03h

        addx    634h,0b12h
        addx    r4,0b12h
        addx    46ch,#03h

        andx	93ah,142h
	andx	0d7ah,#0f0h

	cpx	0ab3h,911h
	cpx	26ch,#2ah

	orx	93ah,142h
	orx	0d7ah,#01100000b

	sbcx	346h,129h
	sbcx	0c6ch,#03h

	subx	234h,912h
	subx	56ch,#03h

	tcmx	0dd4h,420h
	tcmx	0b52h,#02h

	tmx	789h,246h
	tmx	13h,#02h

	xorx	93ah,142h
	xorx	0d7ah,#01100110b

	srl	r6
	srl	12h
	srl	@0c6h

	cpc	r3,r11
	cpc	r15,@r10
	cpc	34h,12h
	cpc	4bh,@r3
	cpc	6ch,#2ah
	cpc	@0d4h,#0ffh

	cpcx	0ab3h,911h
	cpcx	26ch,#2ah

	popx	345h
	pushx	0fcah

	trap	#34h

	bswap	27
	bswap	r5

	mult	rr4
	mult	220

	ldc	@r2,@rr6

	ldx	r3,876h
	ldx	0ee3h,876h
	ldx	@r4,564h
	ldx	34h,@56h
	ldx	@12h,@.RR(9)
	ldx	r4,21h(rr2)
	ldx	72h(rr14),r0
	ldx	345h,r6
	ldx	347h,@r6
	ldx	@rr10,r1
	ldx	@.RR(13h),@0b4h
	ldx	351h,456h
	ldx	364h,#35h

	ldwx	351h,456h

	lea	r11,15h(r3)
	lea	rr12,79h(rr8)

	bit	0,4,r7
	bclr	4,r7
	bit	1,2,r7
	bset	2,r7

	btj	0,5,r7,next
	btjz	5,r7,next
	btj	1,5,@r7,next
	btjnz	5,@r7,next
	halt
next:	ld	r0,@r2

	segment	data

	org	0aah
vari	db	?
ivari	db	?
iwvari	dw	?
