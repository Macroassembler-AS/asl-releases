        cpu     melps7751
	page	0


	ldt	#0
	assume	pg:0,dt:0,dpr:0

accmode macro   size
	 switch	size
	  case   8
	   sep    #$20
	   assume m:1
	  case	16
	   clp    #$20
	   assume m:0
	  elsecase
	   fatal  "accmode: invalid operand size: $\{SIZE}"
	 endcase
	endm

idxmode macro	size
	 switch	size
	  case   8
	   sep    #$10
	   assume x:1
	  case	16
	   clp    #$10
	   assume x:0
	  elsecase
	   fatal  "idxmode: invalid operand size: $\{SIZE}"
	 endcase
	endm

	xab
	wit
	tyx
	tyb
	tya
	txy
	txs
	txb
	txa
	tsx
	tsb
	tsa
	tdb
	tda
	tby
	tbx
	tbs
	tbd
	tay
	tax
	tas
	tad
	stp
	sem
	sei
	sec
	rts
	rtl
	rti
	ply
	plx
	plt
	plp
	pld
	plb
	pla
	phy
	phx
	pht
	php
	phg
	phd
	phb
	pha
	nop
	iny
	inx
	dey
	dex
	clv
	clm
	cli
	clc

	rla	#7

targ:
	bcc	targ
	bcs	targ
	beq	targ
	bmi	targ
	bne	targ
	bpl	targ
	bvc	targ
	bvs	targ
	bra	targ
	per	targ
	org	*+$100
	bra	targ
	per	targ
	per	#-30

	accmode	8
	idxmode	8

	adc	#20
	and	a,#-1
	cmp	b,#-$a

targ2:
	bbc	#$80,$20,targ2
	bbs	#$40,$2000,targ2
	clb	#$20,$20
	seb	#$10,$2000

	cpx	#$aa
	cpy     #$aa
	ldx	#$bb
	ldy	#$bb

	ldm	#$aa,$20
	ldm	#$aa,$2000
	ldm	#$aa,$20,x
	ldm	#$aa,$2000,x

	accmode	16
	idxmode	16

	eor	#%0000111111110000
	lda	a,#'AB'
	ora	b,#3000

	sbc	$20
	sta	a,$2000
	adc	b,$200000

	and	$30,x
	cmp	a,$3000,x
	eor	b,$300000,x

	lda	$40,y
	ora	a,$4000,y
;	sbc	b,$400000,y

	sta	($50)
;	adc	a,($5000)
;	and	b,($500000)

	cmpl	($60)
;	eorl	a,($6000)
;	ldal	b,($600000)

	ora	($70,x)
;	sbc	($7000,x)
;	sta	($700000,x)

	adc	($80),y
;	and	($8000),y
;	cmp	($800000),y

	eorl	($80),y
;	ldal	($8000),y
;	oral	($800000),y

	sbc	b,3,s
	sta	a,(5,s),y

	asl
	dec	a
	inc	b
	lsr	$20
	rol	$2000
	ror	$20,x
	asl	$2000,x

targ3:
	bbs	#$80,$20,targ3
	bbc	#$40,$2000,targ3
	clb	#$20,$20
	seb	#$10,$2000

	cpx	#$aa
	cpy     #$aa
	ldx	#$bb
	ldy	#$bb
	cpx	$20
	cpy	$20
	ldx	$20
	ldy	$20
	stx	$20
	sty	$20
	cpx	$2000
	cpy	$2000
	ldx	$2000
	ldy	$2000
	stx	$2000
	sty	$2000
	ldx	$30,y
	ldy	$30,x
	stx	$30,y
	sty	$30,x
	ldx	$3000,y
	ldy	$3000,x

        mpy     #55
        accmode 8
        mpy     #55
        accmode 16
        mpy     $12
        mpy     $12,x
        mpy     ($12)
        mpy     ($12,x)
        mpy     ($12),y
        mpyl    ($12)
        mpyl    ($12),y
        mpy     $1234
        mpy     $1234,x
        mpy     $1234,y
        mpy     $123456
        mpy     $123456,x
        mpy     $12,s
        mpy     ($12,s),y

        div     #55
        accmode 8
        div     #55
        accmode 16
        div     $12
        div     $12,x
        div     ($12)
        div     ($12,x)
        div     ($12),y
        divl    ($12)
        divl    ($12),y
        div     $1234
        div     $1234,x
        div     $1234,y
        div     $123456
        div     $123456,x
        div     $12,s
        div     ($12,s),y

        jmp     $20
	jmp	$2000
	jmp	$200000
	jmp	($3000)
	jmpl	($3000)
	jmp	($4000,x)
	jsr	$20
	jsr	$2000
	jsr	$200000
	jsr	($4000,x)

	ldm	#$aa,$20
	ldm	#$aa,$2000
	ldm	#$aa,$20,x
	ldm	#$aa,$2000,x

	mvn	$200000,$300000
	mvp	$200000,$300000

;- - - - - - - - - - - - - - - - - - - - - - - - - - - -

	cpu     65816

	cop	#0
	jml     $2000
	jsl     $4000
	brl     *
	rep     #$55
	tsb     $33
	trb     $66
        tsb     $9999
        trb     $cccc
        bit     $00
	bit     $11,x
        bit     $2222
        bit     $3333,x
        bit     #$44
        stz     $55
        stz     $6666
        stz     $77,x
        stz     $8888,x
        cld
        sed
	tcs
	tsc
	phk
	tcd
	tdc
	phb
	plb
	wai
	xba
	xce

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        cpu     melps7750

        asr
        asr     a
        asr     b
        asr     $aa
        asr     $1234
        asr     $aa,x
        asr     $1234,x

        extz
        extz    a
        extz    b
        exts
        exts    a
        exts    b

        mpys     #55
        accmode 8
        mpys     #55
        accmode 16
        mpys     $12
        mpys     $12,x
        mpys     ($12)
        mpys     ($12,x)
        mpys     ($12),y
        mpysl    ($12)
        mpysl    ($12),y
        mpys     $1234
        mpys     $1234,x
        mpys     $1234,y
        mpys     $123456
        mpys     $123456,x
        mpys     $12,s
        mpys     ($12,s),y

        divs     #55
        accmode 8
        divs     #55
        accmode 16
        divs     $12
        divs     $12,x
        divs     ($12)
        divs     ($12,x)
        divs     ($12),y
        divsl    ($12)
        divsl    ($12),y
        divs     $1234
        divs     $1234,x
        divs     $1234,y
        divs     $123456
        divs     $123456,x
        divs     $12,s
        divs     ($12,s),y

	; plain break (one byte)
	brk
	; break with extra NOP to compensate PC increment by 2 if not corrected by break handler
	brk	$ea

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
