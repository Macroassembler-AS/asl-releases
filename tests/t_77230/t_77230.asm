	cpu	77230
	page	0

	; Test Case 1 (nur LDI) und DestReg-Kodierungen

	ldi	rp,123456h
	ldi	psw0,123456h
	ldi	psw1,123456h
	ldi	svr,123456h
	ldi	sr,123456h
	ldi	lc,123456h
	ldi	stk,123456h
	ldi	lkr0,123456h
	ldi	klr1,123456h
	ldi	tre,123456h
	ldi	tr,123456h
	ldi	ar,123456h
	ldi	so,123456h
	ldi	dr,123456h
	ldi	drs,123456h
	ldi	wr0,123456h
	ldi	wr1,123456h
	ldi	wr2,123456h
	ldi	wr3,123456h
	ldi	wr4,123456h
	ldi	wr5,123456h
	ldi	wr6,123456h
	ldi	wr7,123456h
	ldi	ram0,123456h
	ldi	ram1,123456h
	ldi	bp0,123456h
	ldi	bp1,123456h
	ldi	ix0,123456h
	ldi	ix1,123456h
	ldi	k,123456h
	ldi	l,123456h

	; Sprünge wahlweise mit oder ohne MOV, Test SrcReg-Kodierungen
	jmp	target
	call	target 		mov 	wr0,rp
	ret
	ret			mov 	wr0,psw0
        mov	wr0,psw1	jnzrp	target
	jz0	target		mov	wr0,svr
	mov	wr0,sr		jnz0	target
	jz1	target		mov	wr0,lc
	mov	wr0,stx		jnz1	target
	jc0	target		mov	wr0,m
	mov	wr0,ml		jnc0	target
	jc1	target		mov	wr0,rom
	mov	wr0,tr		jnc1	target
	js0	target		mov	wr0,ar
	mov	wr0,si		jns0	target
	js1	target		mov	wr0,dr
	mov	wr0,drs		jns1	target
	jv0	target		mov	wr0,wr0
	mov	wr0,wr1		jnv0	target
	jv1	target		mov	wr0,wr2
	mov	wr0,wr3		jnv1	target
	jev0	target		mov	wr0,wr4
	mov	wr0,wr5		jev1	target
	jnfsi	target		mov	wr0,wr6
	mov	wr0,wr7		jneso	target
	jip0	target		mov	wr0,ram0
	mov	wr0,ram1	jip1	target
	jnzix0	target		mov	wr0,bp0
	mov	wr0,bp1		jnzix1	target
	jnzbp0	target		mov	wr0,ix0
	mov	wr0,ix1		jnzbp1	target
	jrdy	target		mov	wr0,k
	mov	wr0,l		jrqm	target
target:

	; ALU

	nop
	inc	wr1
	dec	wr2
	abs	wr3
	not	wr4
	neg	wr5
	shlc	wr6
	shrc	wr7
	rol	wr1
	ror	wr2
	shlm	wr3
	shrm	wr4
	shram	wr5
	clr	wr6
	norm	wr7
	cvt	wr1
	add	wr1,ib
	sub	wr1,m
	addc	wr1,ram0
	subc	wr1,ram1
	cmp	wr1,ib
	and	wr1,m
	or	wr1,ram0
	xor	wr1,ram1
	addf	wr1,ib
	subf	wr1,m

	; Mx-Felder

	spcbp0  spcbi1
	spcix0	spcix1
	spcbi0	spcbp1

	; DPx-Felder

	incbp0	clrix1
	decbp0	decix1
	clrbp0	incix1
	stix0	stix1
	incix0	clrbp1
	decix0	decbp1
	clrix0	incbp1

	; EA-Feld

	incar
	decar

	; FC-Feld

	xchpsw

	; RP-Feld
	
	incrp
	decrp
	incbrp

	; L-Feld

	declc

	; BASE-Felder

	mcnbp0 0	mcnbp1 7
	mcnbp0 1	mcnbp1 6
	mcnbp0 2	mcnbp1 5
	mcnbp0 3	mcnbp1 4
	mcnbp0 4	mcnbp1 3
	mcnbp0 5	mcnbp1 2
	mcnbp0 6	mcnbp1 1
	mcnbp0 7	mcnbp1 0

	; RPC-Feld

	bitrp	0
	bitrp   1
        bitrp   2
        bitrp   3
        bitrp   4
        bitrp   5
        bitrp   6
        bitrp   7
        bitrp   8
        bitrp   9

	; Ports nur im Paket

	clrp2	clrp3
	setp2	clrp3
	clrp2	setp3
	setp2	setp3

	; Interrupts nur mit Ports

	clrp2   clrp3		clrbm
	clrp2   clrp3		setbm
	clrp2   clrp3	di
	clrp2   clrp3	ei
	clrp2   clrp3	ei	clrbm
	clrp2   clrp3	ei	setbm

	; RW-Feld

	rd
	wr

	; WT-Feld

	wrbord
	wrbl24
	wrbl23
	wrbel8
	wrbl8e
	wrbxch
	wrbbrv

	; NF-Feld

	trnorm
	rdnorm
	fltfix
	fixma

	; WI-Feld

	bwrl24
	bwrord

	; FIS-Feld

	spcpsw0
	spcpsw1
	clrpsw0
	clrpsw1
	clrpsw

	; FD-Feld

	spie
	iesp

	; SHV-Feld

	setsvl 7
	setsvr 17

	; RPS-Feld

	spcra 123

	; NAL-Feld

	jblk $+5

	; Data Storage (packed by default)

	dw 12345678h
	dw 4294967295
	dw -10000
	dw -2147483648
	dw 'A'
	dw 'AB'
	dw 'ABC'
	dw 'ABCD'
	dw "a"
	dw "ab"
	dw "abc"
	dw "abcd"
	dw "abcde"
	dw "abcdef"
	dw "abcdefg"
	dw "abcdefgh"
	dw 32.0,-32.0
	dw 1.0,-1.0
;	dw 3.6e-46		; these do not work on machines that
;	dw 3.6e-45		; do not support denormalized values,
;	dw 3.6e-44		; so we better leave them out for portable
;	dw 3.6e-43		; tests...
;	dw 3.6e-42
;	dw 3.6e-41
;	dw 3.6e-40
;	dw 3.6e-39
	dw 3.6e-38

	; unpacked storage

	packing	off
	dw "a"
	dw "ab"
	dw "abc"
	dw "abcd"
	dw "abcde"
	dw "abcdef"
	dw "abcdefg"
	dw "abcdefgh"
	