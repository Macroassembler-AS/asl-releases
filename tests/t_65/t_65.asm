	cpu	melps740
	page	0

	adc	#$12
	adc	$12
	adc	$12,x
	adc	$12,y
	adc	$1234
	adc	$1234,x
	adc	$1234,y
	adc	($12,x)
	adc	($12),y

	and	#$12
	and	$12
	and	$12,x
	and	$12,y
	and	$1234
	and	$1234,x
	and	$1234,y
	and	($12,x)
	and	($12),y

	asl
	asl	a
	asl	$12
	asl	$12,x
	asl	$1234
	asl	$1234,x

	bbc	3,a,*
	bbc	4,$12,*

	bbs	5,a,*
	bbs	6,$12,*+1

	bcc	*+2
	bcs	*+3
	beq	*+4

	bit	$12
	bit	$1234

	bmi	*+5
	bne	*+6
	bpl	*+7
	bra	*+8

	brk

	bvc	*+9
	bvs	*+10
	
	clb	2,a
	clb	3,$12
	
	clc
	cld
	cli
	clt
	clv

	cmp	#$12
	cmp	$12
	cmp	$12,x
	cmp	$12,y
	cmp	$1234
	cmp	$1234,x
	cmp	$1234,y
	cmp	($12,x)
	cmp	($12),y

	com	$12

	cpx	#$12
	cpx	$12
	cpx	$1234

	cpy	#$12
	cpy	$12
	cpy	$1234

	dec
	dec	a
	dec	$12
	dec	$12,x
	dec	$1234
	dec	$1234,x

	dex
	dey

	div	$12,x

	eor	#$12
	eor	$12
	eor	$12,x
	eor	$12,y
	eor	$1234
	eor	$1234,x
	eor	$1234,y
	eor	($12,x)
	eor	($12),y

	fst

	inc
	inc	a
	inc	$12
	inc	$12,x
	inc	$1234
	inc	$1234,x

	inx
	iny
	
	jmp	$1234
	jmp	($1234)
	jmp	($12)

	assume	sp:$12
	jsr	$1234
	jsr	\$1234
	jsr	($12)

	lda	#$12
	lda	$12
	lda	$12,x
	lda	$12,y
	lda	$1234
	lda	$1234,x
	lda	$1234,y
	lda	($12,x)
	lda	($12),y
	
	ldm	#$12,$34

	ldx	#$12
	ldx	$12
	ldx	$12,y
	ldx	$1234
	ldx	$1234,y

	ldy	#$12
	ldy	$12
	ldy	$12,x
	ldy	$1234
	ldy	$1234,x

	lsr
	lsr	a
	lsr	$12
	lsr	$12,x
	lsr	$1234
	lsr	$1234,x

	mul	$12,x

	nop

	ora	#$12
	ora	$12
	ora	$12,x
	ora	$12,y
	ora	$1234
	ora	$1234,x
	ora	$1234,y
	ora	($12,x)
	ora	($12),y

	pha
	php
	pla
	plp

	rol
	rol	a
	rol	$12
	rol	$12,x
	rol	$1234
	rol	$1234,x

	ror
	ror	a
	ror	$12
	ror	$12,x
	ror	$1234
	ror	$1234,x
	
	rrf	$12

	rti
	rts

	sbc	#$12
	sbc	$12
	sbc	$12,x
	sbc	$12,y
	sbc	$1234
	sbc	$1234,x
	sbc	$1234,y
	sbc	($12,x)
	sbc	($12),y
	
	seb	2,a
	seb	3,$12
	
	sec
	sed
	sei
	set

	slw

	sta	$12
	sta	$12,x
	sta	$12,y
	sta	$1234
	sta	$1234,x
	sta	$1234,y
	sta	($12,x)
	sta	($12),y

	stp

	stx	$12
	stx	$12,y
	stx	$1234	

	sty	$12
	sty	$12,x
	sty	$1234	

	tax
	tay
	
	tst	$12

	tsx
	txa
	txs
	tya

	wit

        cpu     65c02

        dec     a
        inc     a
        plx
        ply
        phx
        phy

        tsb     $12
        trb     $12
        tsb     $1234
        trb     $1234

        stz     $12
        stz     $1234
        stz     $12,x
        stz     $1234,x

        bit     $12,x
        bit     $1234,x
        bit     #$12

        lda     ($12)
        sta     ($12)
        adc     ($12)
        sbc     ($12)
        and     ($12)
        ora     ($12)
        eor     ($12)
        cmp     ($12)

        jmp     ($12,x)

        bbr2    $12,*
        bbs4    $12,*

        rmb3    $12
        smb5    $12

	byt	0
	db	255
	byt	-128
	db	'a'
	byt	'ab'	; treated as "ab" due to length
	db	"a"
	byt	"ab"

	adr	0
	dw	65535
	adr	-32768
	dw	'a'
	adr	'ab'
	dw	"a"
	adr	"ab"

	ddb	'a'	; big endian storage
	ddb	'ab'
	ddb	"a"
	ddb	"ab"

	ds	10
