	cpu	eZ80F91
	page	0

	ld	a,a		; (7F)
	expect	470
	ld	b,b		; (00)
	endexpect
	expect	470
	ld	c,c		; (00)
	endexpect
	expect	470
	ld	d,d		; (00)
	endexpect
	expect	470
	ld	e,e		; (00)
	endexpect
	ld	h,h		; (64)
	ld	l,l		; (6D)

	.assume	adl:0
	.assume mbase:00h

	ld	hl,3456h	; (21 56 34) HL[23:0] <- {00h, 3456h}
	expect	1320
	ld	hl,123456h	; () Invalid - Z80 mode cannot load 24-bit value
	endexpect
	ld.sis	hl,3456h	; (40 21 56 34) Same as LD HL, 3456h
	ld.lil	hl,123456h	; (5B 21 56 34 12) HL[23:0] <- 123456h
	ld.lis	hl,3456h	; (49 21 56 34) HL[23:0] <- {00h, 3456h}
	ld.sil	hl,123456h	; (52 21 56 34 12) HL[23:0] <- {00h, 3456h}, bits 16...24 ignored because of .s

	ld	hl,(3456h)	; (2A 56 34) HL[15:0] <- {MBASE, 3456h}
	expect	110
	ld	hl,(123456h)	; () can only reach addresses 00xxxx
	endexpect
	ld.sis	hl,(3456h)	; (40 2A 56 34) Same as LD HL, (3456h)
	ld.lil	hl,(123456h)	; (5B 2A 56 34 12) HL[23:0] <- {123456h}
	ld.lis	hl,(3456h)	; (49 2A 56 34) HL[23:0] <- {0, 3456h}
	expect	110
	ld.sil	hl,(123456h)	; (52 2A 56 34 12) HL[15:0] <- {MBASE, 3456h}, bits 16...24 ignored because of .s
	endexpect

	ld	(hl),bc		; (ED 0F) {MBASE, HL[15:0]} <- BC[15:0]
	ld.sis	(hl),bc		; (40 ED 0F) same as 'ld (hl),bc', '.is' is ignored
	ld.lil	(hl),bc		; (5B ED 0F) {HL[23:0]} <- BC[23:0]}, '.il' is ignored
	ld.sil	(hl),bc		; (52 ED 0F) same as 'ld (hl),bc', '.il' is ignored
	ld.lis	(hl),bc		; (49 ED 0F) {HL[23:0]} <- BC[23:0]}, '.is' is ignored

	.assume	adl:1
	ld	hl,3456h	; (21 56 34 00) HL[23:0] <- 003456h
	ld	hl,123456h	; (21 56 34 12) HL[23:0] <- 123456h
	ld.sis	hl,3456h	; (40 21 56 34) HL[23:0] <- {00h, 3456h}
	ld.lil	hl,123456h	; (5B 21 56 34 12) Same as LD HL, 123456h
	ld.lis	hl,3456h	; (49 21 56 34) HL[23:0] <- {00h, 3456h}
	ld.sil	hl,123456h	; (52 21 56 34 12) HL[23:0] <- {00h, 3456h}, bits 16...24 ignored because of .s

	ld	hl,(3456h)	; (2A 56 34 00) HL[23:0] <- {003456h}
	ld	hl,(123456h)	; (2A 56 34 12) HL[23:0] <- {123456h}
	ld.sis	hl,(3456h)	; (40 2A 56 34) HL[15:0] <- {MBASE, 3456h}
	ld.lil	hl,(123456h)	; (5B 2A 56 34 12) Same as LD HL, (3456h)
	ld.lis	hl,(3456h)	; (49 2A 56 34) HL[23:0] <- {0, 3456h}
	expect	110
	ld.sil	hl,(123456h)	; (52 2A 56 34 12) HL[15:0] <- {MBASE, 3456h}, bits 16...24 ignored because of .s
	endexpect

	ld	(hl),bc		; (ED 0F) {HL[23:0]} <- BC[23:0]
	ld.sis	(hl),bc		; (40 ED 0F) {MBASE, HL[15:0]} <- BC[15:0], '.is' is ignored
	ld.lil	(hl),bc		; (5B ED 0F) same as 'ld (hl),bc', '.il' is ignored
	ld.sil	(hl),bc		; (52 ED 0F) {MBASE, HL[15:0]} <- BC[15:0], '.il' is ignored
	ld.lis	(hl),bc		; (49 ED 0F) {HL[23:0]} <- BC[23:0]}, '.is' is ignored

	; ----------------------

	; ADC A, (HL)
	adc	a,(hl)		; 8E
	assume	adl:1
	adc.s	a,(hl)		; 52 8E
	assume	adl:0
	adc.l	a,(hl)		; 49 8E

	; ADC A, ir
	adc	a,ixh		; DD 8C
	adc	a,ixl		; DD 8D
	adc	a,iyh		; FD 8C
	adc	a,iyl		; FD 8D

	; ADC A, (IX/Y+d)
	adc	a,(ix+12h)	; DD 8E 12
	assume	adl:1
	adc.s	a,(ix+12h)	; 52 DD 8E 12
	assume	adl:0
	adc.l	a,(ix+12h)	; 49 DD 8E 12
	adc	a,(iy+12h)	; FD 8E 12
	assume	adl:1
	adc.s	a,(iy+12h)	; 52 FD 8E 12
	assume	adl:0
	adc.l	a,(iy+12h)	; 49 FD 8E 12

	; ADC A, n
	adc	a,12h		; CE 12

	; ADC A, r
	adc	a,a		; 8F
	adc	a,b		; 88
	adc	a,c		; 89
	adc	a,d		; 8A
	adc	a,e		; 8B
	adc	a,h		; 8C
	adc	a,l		; 8D

	; ADC HL, rr
	adc	hl,bc		; ED 4A
	adc	hl,de		; ED 5A
	adc	hl,hl		; ED 6A
	assume	adl:1
	adc.s	hl,bc		; 52 ED 4A
	adc.s	hl,de		; 52 ED 5A
	adc.s	hl,hl		; 52 ED 6A
	assume	adl:0
	adc.l	hl,bc		; 49 ED 4A
	adc.l	hl,de		; 49 ED 5A
	adc.l	hl,hl		; 49 ED 6A

	expect	1350
	adc	hl,ix
	endexpect
	expect	1350
	adc	ix,hl
	endexpect

	; ADC HL, SP
	adc	hl,sp		; ED 7A
	assume	adl:1
	adc.s	hl,sp		; 52 ED 7A
	assume	adl:0
	adc.l	hl,sp		; 49 ED 7A

	; ADD A, (HL)
	add	a,(hl)		; 86
	assume	adl:1
	add.s	a,(hl)		; 52 86
	assume	adl:0
	add.l	a,(hl)		; 49 86

	; ADD A, ir
	add	a,ixh		; DD 84
	add	a,ixl		; DD 85
	add	a,iyh		; FD 84
	add	a,iyl		; FD 85

	; ADD A, (IX/Y+d)
	add	a,(ix+12h)	; DD 86 12
	assume	adl:1
	add.s	a,(ix+12h)	; 52 DD 86 12
	assume	adl:0
	add.l	a,(ix+12h)	; 49 DD 86 12
	add	a,(iy+12h)	; FD 86 12
	assume	adl:1
	add.s	a,(iy+12h)	; 52 FD 86 12
	assume	adl:0
	add.l	a,(iy+12h)	; 49 FD 86 12

	; ADD A, n
	add	a,12h		; C6 12

	; ADD A, r
	add	a,a		; 87
	add	a,b		; 80
	add	a,c		; 81
	add	a,d		; 82
	add	a,e		; 83
	add	a,h		; 84
	add	a,l		; 85

	; ADD HL, rr
	add	hl,bc		; 09
	add	hl,de		; 19
	add	hl,hl		; 29
	assume	adl:1
	add.s	hl,bc		; 52 09
	add.s	hl,de		; 52 19
	add.s	hl,hl		; 52 29
	assume	adl:0
	add.l	hl,bc		; 49 09
	add.l	hl,de		; 49 19
	add.l	hl,hl		; 49 29

	; ADD HL, SP
	add	hl,sp		; 39
	assume	adl:1
	add.s	hl,sp		; 52 39
	assume	adl:0
	add.l	hl,sp		; 49 39

	; ADD IX/Y, rxy
	add	ix,bc		; DD 09
	add	ix,de		; DD 19
	add	ix,ix		; DD 29
	assume	adl:1
	add.s	ix,bc		; 52 DD 09
	add.s	ix,de		; 52 DD 19
	add.s	ix,ix		; 52 DD 29
	assume	adl:0
	add.l	ix,bc		; 49 DD 09
	add.l	ix,de		; 49 DD 19
	add.l	ix,ix		; 49 DD 29
	add	iy,bc		; FD 09
	add	iy,de		; FD 19
	add	iy,iy		; FD 29
	assume	adl:1
	add.s	iy,bc		; 52 FD 09
	add.s	iy,de		; 52 FD 19
	add.s	iy,iy		; 52 FD 29
	assume	adl:0
	add.l	iy,bc		; 49 FD 09
	add.l	iy,de		; 49 FD 19
	add.l	iy,iy		; 49 FD 29

	; AND A, (HL)
	and	a,(hl)		; A6
	assume	adl:1
	and.s	a,(hl)		; 52 A6
	assume	adl:0
	and.l	a,(hl)		; 49 A6

	; AND A, ir
	and	a,ixh		; DD A4
	and	a,ixl		; DD A5
	and	a,iyh		; FD A4
	and	a,iyl		; FD A5

	; AND A, (IX/Y+d)
	and	a,(ix+12h)	; DD A6 12
	assume	adl:1
	and.s	a,(ix+12h)	; 52 DD A6 12
	assume	adl:0
	and.l	a,(ix+12h)	; 49 DD A6 12
	and	a,(iy+12h)	; FD A6 12
	assume	adl:1
	and.s	a,(iy+12h)	; 52 FD A6 12
	assume	adl:0
	and.l	a,(iy+12h)	; 49 FD A6 12

	; AND A, n
	and	a,12h		; E6 12

	; AND A, r
	and	a,a		; A7
	and	a,b		; A0
	and	a,c		; A1
	and	a,d		; A2
	and	a,e		; A3
	and	a,h		; A4
	and	a,l		; A5

	; BIT b, (HL)
	bit	5,(hl)		; CB 6E
	assume	adl:1
	bit.s	5,(hl)		; 52 CB 6E
	assume	adl:0
	bit.l	5,(hl)		; 49 CB 6E

	; BIT b, (IX/Y+d)
	bit	5,(ix+12h)	; DD CB 12 6E
	assume	adl:1
	bit.s	5,(ix+12h)	; 52 DD CB 12 6E
	assume	adl:0
	bit.l	5,(ix+12h)	; 49 DD CB 12 6E
	bit	5,(iy+12h)	; FD CB 12 6E
	assume	adl:1
	bit.s	5,(iy+12h)	; 52 FD CB 12 6E
	assume	adl:0
	bit.l	5,(iy+12h)	; 49 FD CB 12 6E

	; BIT b, r
	bit	5,a		; CB 6F
	bit	5,b		; CB 68
	bit	5,c		; CB 69
	bit	5,d		; CB 6A
	bit	5,e		; CB 6B
	bit	5,h		; CB 6C
	bit	5,l		; CB 6D

	; CALL cc, Mmn
	assume	adl:0
	call	nz,1234h	; C4 34 12
	assume	adl:1
	call	nz,123456h	; C4 56 34 12
	assume	adl:0
	call.is	nz,1234h	; 40 C4 34 12
	assume	adl:1
	call.is	nz,1234h	; 49 C4 34 12
	assume	adl:0
	call.il	nz,123456h	; 52 C4 56 34 12
	assume	adl:1
	call.il	nz,123456h	; 5B C4 56 34 12

	assume	adl:0
	call	z,1234h		; CC 34 12
	assume	adl:1
	call	z,123456h	; CC 56 34 12
	assume	adl:0
	call.is	z,1234h		; 40 CC 34 12
	assume	adl:1
	call.is	z,1234h		; 49 CC 34 12
	assume	adl:0
	call.il	z,123456h	; 52 CC 56 34 12
	assume	adl:1
	call.il	z,123456h	; 5B CC 56 34 12

	assume	adl:0
	call	nc,1234h	; D4 34 12
	assume	adl:1
	call	nc,123456h	; D4 56 34 12
	assume	adl:0
	call.is	nc,1234h	; 40 D4 34 12
	assume	adl:1
	call.is	nc,1234h	; 49 D4 34 12
	assume	adl:0
	call.il	nc,123456h	; 52 D4 56 34 12
	assume	adl:1
	call.il	nc,123456h	; 5B D4 56 34 12

	assume	adl:0
	call	c,1234h		; DC 34 12
	assume	adl:1
	call	c,123456h	; DC 56 34 12
	assume	adl:0
	call.is	c,1234h		; 40 DC 34 12
	assume	adl:1
	call.is	c,1234h		; 49 DC 34 12
	assume	adl:0
	call.il	c,123456h	; 52 DC 56 34 12
	assume	adl:1
	call.il	c,123456h	; 5B DC 56 34 12

	assume	adl:0
	call	po,1234h	; E4 34 12
	assume	adl:1
	call	po,123456h	; E4 56 34 12
	assume	adl:0
	call.is	po,1234h	; 40 E4 34 12
	assume	adl:1
	call.is	po,1234h	; 49 E4 34 12
	assume	adl:0
	call.il	po,123456h	; 52 E4 56 34 12
	assume	adl:1
	call.il	po,123456h	; 5B E4 56 34 12

	assume	adl:0
	call	pe,1234h	; EC 34 12
	assume	adl:1
	call	pe,123456h	; EC 56 34 12
	assume	adl:0
	call.is	pe,1234h	; 40 EC 34 12
	assume	adl:1
	call.is	pe,1234h	; 49 EC 34 12
	assume	adl:0
	call.il	pe,123456h	; 52 EC 56 34 12
	assume	adl:1
	call.il	pe,123456h	; 5B EC 56 34 12

	assume	adl:0
	call	p,1234h		; F4 34 12
	assume	adl:1
	call	p,123456h	; F4 56 34 12
	assume	adl:0
	call.is	p,1234h		; 40 F4 34 12
	assume	adl:1
	call.is	p,1234h		; 49 F4 34 12
	assume	adl:0
	call.il	p,123456h	; 52 F4 56 34 12
	assume	adl:1
	call.il	p,123456h	; 5B F4 56 34 12

	assume	adl:0
	call	m,1234h		; FC 34 12
	assume	adl:1
	call	m,123456h	; FC 56 34 12
	assume	adl:0
	call.is	m,1234h		; 40 FC 34 12
	assume	adl:1
	call.is	m,1234h		; 49 FC 34 12
	assume	adl:0
	call.il	m,123456h	; 52 FC 56 34 12
	assume	adl:1
	call.il	m,123456h	; 5B FC 56 34 12

	; CALL Mmn
	assume	adl:0
	call	1234h		; CD 34 12
	assume	adl:1
	call	123456h		; CD 56 34 12
	assume	adl:0
	call.is	1234h		; 40 CD 34 12
	assume	adl:1
	call.is	1234h		; 49 CD 34 12
	assume	adl:0
	call.il	123456h		; 52 CD 56 34 12
	assume	adl:1
	call.il	123456h		; 5B CD 56 34 12

	; CCF
	ccf			; 3F

	; CP A, (HL)
	cp	a,(hl)		; BE
	assume	adl:1
	cp.s	a,(hl)		; 52 BE
	assume	adl:0
	cp.l	a,(hl)		; 49 BE

	; CP A, ir
	cp	a,ixh		; DD BC
	cp	a,ixl		; DD BD
	cp	a,iyh		; FD BC
	cp	a,iyl		; FD BD

	; CP A, (IX/Y+d)
	cp	a,(ix+12h)	; DD BE 12
	assume	adl:1
	cp.s	a,(ix+12h)	; 52 DD BE 12
	assume	adl:0
	cp.l	a,(ix+12h)	; 49 DD BE 12
	cp	a,(iy+12h)	; FD BE 12
	assume	adl:1
	cp.s	a,(iy+12h)	; 52 FD BE 12
	assume	adl:0
	cp.l	a,(iy+12h)	; 49 FD BE 12

	; CP A, n
	cp	a,12h		; FE 12

	; CP A, r
	cp	a,a		; BF
	cp	a,b		; B8
	cp	a,c		; B9
	cp	a,d		; BA
	cp	a,e		; BB
	cp	a,h		; BC
	cp	a,l		; BD

	; CPD
	cpd			; ED A9
	assume adl:1
	cpd.s			; 52 ED A9
	assume adl:0
	cpd.l			; 49 ED A9

	; CPDR
	cpdr			; ED B9
	assume adl:1
	cpdr.s			; 52 ED B9
	assume adl:0
	cpdr.l			; 49 ED B9

	; CPI
	cpi			; ED A1
	assume adl:1
	cpi.s			; 52 ED A1
	assume adl:0
	cpi.l			; 49 ED A1

	; CPIR
	cpir			; ED B1
	assume adl:1
	cpir.s			; 52 ED B1
	assume adl:0
	cpir.l			; 49 ED B1

	; CPL
	cpl			; 2F

	; DAA
	daa			; 27

	; DEC (HL)
	dec	(hl)		; 35
	assume	adl:1
	dec.s	(hl)		; 52 35
	assume	adl:0
	dec.l	(hl)		; 49 35

	; DEC ir
	dec	ixh		; DD 25
	dec	ixl		; DD 2D
	dec	iyh		; FD 25
	dec	iyl		; FD 2D

	; DEC IX/Y
	dec	ix		; DD 2B
	assume adl:1
	dec.s	ix		; 52 DD 2B
	assume adl:0
	dec.l	ix		; 49 DD 2B
	dec	iy		; FD 2B
	assume adl:1
	dec.s	iy		; 52 FD 2B
	assume adl:0
	dec.l	iy		; 49 FD 2B

	; DEC (IX/Y+d)
	dec	(ix+12h)	; DD 35 12
	assume	adl:1
	dec.s	(ix+12h)	; 52 DD 35 12
	assume	adl:0
	dec.l	(ix+12h)	; 49 DD 35 12
	dec	(iy+12h)	; FD 35 12
	assume	adl:1
	dec.s	(iy+12h)	; 52 FD 35 12
	assume	adl:0
	dec.l	(iy+12h)	; 49 FD 35 12

	; DEC r
	dec	a		; 3D
	dec	b		; 05
	dec	c		; 0D
	dec	d		; 15
	dec	e		; 1D
	dec	h		; 25
	dec	l		; 2D

	; DEC rr
	dec	bc		; 0B
	dec	de		; 1B
	dec	hl		; 2B
	assume	adl:1
	dec.s	bc		; 52 0B
	dec.s	de		; 52 1B
	dec.s	hl		; 52 2B
	assume	adl:0
	dec.l	bc		; 49 0B
	dec.l	de		; 49 1B
	dec.l	hl		; 49 2B

	; DEC SP
	dec	sp		; 3B
	assume	adl:1
	dec.s	sp		; 52 3B
	assume	adl:0
	dec.l	sp		; 49 3B

	; DI
	di			; F3

	; DJNZ d
	djnz	$		; 10 FE

	; EI
	ei			; FB

	; EX AF,AF'
	ex	af,af'		; 08

	; EX DE, HL
	ex	de,hl		; EB

	; EX (SP), HL
	ex	(sp),hl		; E3
	assume	adl:1
	ex.s	(sp),hl		; 52 E3
	assume	adl:0
	ex.l	(sp),hl		; 49 E3

	; EX (SP), IX/Y
	ex	(sp),ix		; DD E3
	assume	adl:1
	ex.s	(sp),ix		; 52 DD E3
	assume	adl:0
	ex.l	(sp),ix		; 49 DD E3
	ex	(sp),iy		; FD E3
	assume	adl:1
	ex.s	(sp),iy		; 52 FD E3
	assume	adl:0
	ex.l	(sp),iy		; 49 FD E3

	; EXX
	exx			; D9

	; HALT
	halt			; 76

	; IM n
	im	0		; ED 46
	im	1		; ED 56
	im	2		; ED 5E

	; IN A, (n)
	in	a,(12h)		; DB 12

	; IN r, ([B]C)
	in	a,(bc)		; ED 78
	in	b,(c)		; ED 40
	in	c,(bc)		; ED 48
	in	d,(c)		; ED 50
	in	e,(bc)		; ED 58
	in	h,(c)		; ED 60
	in	l,(bc)		; ED 68

	; IN0 r, (n)
	in0	a,(12h)		; ED 38 12
	in0	b,(12h)		; ED 00 12
	in0	c,(12h)		; ED 08 12
	in0	d,(12h)		; ED 10 12
	in0	e,(12h)		; ED 18 12
	in0	h,(12h)		; ED 20 12
	in0	l,(12h)		; ED 28 12

	; INC (HL)
	inc	(hl)		; 34
	assume	adl:1
	inc.s	(hl)		; 52 34
	assume	adl:0
	inc.l	(hl)		; 49 34

	; INC ir
	inc	ixh		; DD 24
	inc	ixl		; DD 2C
	inc	iyh		; FD 24
	inc	iyl		; FD 2C

	; INC IX/Y
	inc	ix		; DD 23
	assume adl:1
	inc.s	ix		; 52 DD 23
	assume adl:0
	inc.l	ix		; 49 DD 23
	inc	iy		; FD 23
	assume adl:1
	inc.s	iy		; 52 FD 23
	assume adl:0
	inc.l	iy		; 49 FD 23

	; INC (IX/Y+d)
	inc	(ix+12h)	; DD 34 12
	assume	adl:1
	inc.s	(ix+12h)	; 52 DD 34 12
	assume	adl:0
	inc.l	(ix+12h)	; 49 DD 34 12
	inc	(iy+12h)	; FD 34 12
	assume	adl:1
	inc.s	(iy+12h)	; 52 FD 34 12
	assume	adl:0
	inc.l	(iy+12h)	; 49 FD 34 12

	; INC r
	inc	a		; 3C
	inc	b		; 04
	inc	c		; 0C
	inc	d		; 14
	inc	e		; 1C
	inc	h		; 24
	inc	l		; 2C

	; INC rr
	inc	bc		; 03
	inc	de		; 13
	inc	hl		; 23
	assume	adl:1
	inc.s	bc		; 52 03
	inc.s	de		; 52 13
	inc.s	hl		; 52 23
	assume	adl:0
	inc.l	bc		; 49 03
	inc.l	de		; 49 13
	inc.l	hl		; 49 23

	; INC SP
	inc	sp		; 33
	assume	adl:1
	inc.s	sp		; 52 33
	assume	adl:0
	inc.l	sp		; 49 33

	; IND
	ind			; ED AA
	assume	adl:1
	ind.s			; 52 ED AA
	assume	adl:0
	ind.l			; 49 ED AA

	; IND2
	ind2			; ED 8C
	assume	adl:1
	ind2.s			; 52 ED 8C
	assume	adl:0
	ind2.l			; 49 ED 8C

	; IND2R
	ind2r			; ED 9C
	assume	adl:1
	ind2r.s			; 52 ED 9C
	assume	adl:0
	ind2r.l			; 49 ED 9C

	; INDM
	indm			; ED 8A
	assume	adl:1
	indm.s			; 52 ED 8A
	assume	adl:0
	indm.l			; 49 ED 8A

	; INDMR
	indmr			; ED 9A
	assume	adl:1
	indmr.s			; 52 ED 9A
	assume	adl:0
	indmr.l			; 49 ED 9A

	; INDR
	indr			; ED BA
	assume	adl:1
	indr.s			; 52 ED BA
	assume	adl:0
	indr.l			; 49 ED BA

	; INDRX
	indrx			; ED CA
	assume	adl:1
	indrx.s			; 52 ED CA
	assume	adl:0
	indrx.l			; 49 ED CA

	; INI
	ini			; ED A2
	assume	adl:1
	ini.s			; 52 ED A2
	assume	adl:0
	ini.l			; 49 ED A2

	; INI2
	ini2			; ED 84
	assume	adl:1
	ini2.s			; 52 ED 84
	assume	adl:0
	ini2.l			; 49 ED 84

	; INI2R
	ini2r			; ED 94
	assume	adl:1
	ini2r.s			; 52 ED 94
	assume	adl:0
	ini2r.l			; 49 ED 94

	; INIM
	inim			; ED 82
	assume	adl:1
	inim.s			; 52 ED 82
	assume	adl:0
	inim.l			; 49 ED 82

	; INIMR
	inimr			; ED 92
	assume	adl:1
	inimr.s			; 52 ED 92
	assume	adl:0
	inimr.l			; 49 ED 92

	; INIR
	inir			; ED B2
	assume	adl:1
	inir.s			; 52 ED B2
	assume	adl:0
	inir.l			; 49 ED B2

	; INIRX
	inirx			; ED C2
	assume	adl:1
	inirx.s			; 52 ED C2
	assume	adl:0
	inirx.l			; 49 ED C2

	; JP cc, Mmn
	assume	adl:0
	jp	nz,1234h	; C2 34 12
	assume	adl:1
	jp	nz,123456h	; C2 56 34 12
	assume	adl:0
	jp.lil	nz,123456h	; 5B C2 56 34 12
	assume	adl:1
	jp.sis	nz,1234h	; 40 C2 34 12

	; .SIL and .LIS are illegal for JP
	assume	adl:1
	expect	1107
	jp.is	nz,1234h
	endexpect
	assume	adl:0
	expect	110,1107
	jp.il	nz,123456h
	endexpect

	assume	adl:0
	jp	z,1234h		; CA 34 12
	assume	adl:1
	jp	z,123456h	; CA 56 34 12
	assume	adl:0
	jp.lil	z,123456h	; 5B CA 56 34 12
	assume	adl:1
	jp.sis	z,1234h		; 40 CA 34 12

	assume	adl:0
	jp	nc,1234h	; D2 34 12
	assume	adl:1
	jp	nc,123456h	; D2 56 34 12
	assume	adl:0
	jp.lil	nc,123456h	; 5B D2 56 34 12
	assume	adl:1
	jp.sis	nc,1234h	; 40 D2 34 12

	assume	adl:0
	jp	c,1234h		; DA 34 12
	assume	adl:1
	jp	c,123456h	; DA 56 34 12
	assume	adl:0
	jp.lil	c,123456h	; 5B DA 56 34 12
	assume	adl:1
	jp.sis	c,1234h		; 40 DA 34 12

	assume	adl:0
	jp	po,1234h	; E2 34 12
	assume	adl:1
	jp	po,123456h	; E2 56 34 12
	assume	adl:0
	jp.lil	po,123456h	; 5B E2 56 34 12
	assume	adl:1
	jp.sis	po,1234h	; 40 E2 34 12

	assume	adl:0
	jp	pe,1234h	; EA 34 12
	assume	adl:1
	jp	pe,123456h	; EA 56 34 12
	assume	adl:0
	jp.lil	pe,123456h	; 5B EA 56 34 12
	assume	adl:1
	jp.sis	pe,1234h	; 40 EA 34 12

	assume	adl:0
	jp	p,1234h		; F2 34 12
	assume	adl:1
	jp	p,123456h	; F2 56 34 12
	assume	adl:0
	jp.lil	p,123456h	; 5B F2 56 34 12
	assume	adl:1
	jp.sis	p,1234h		; 40 F2 34 12

	assume	adl:0
	jp	m,1234h		; FA 34 12
	assume	adl:1
	jp	m,123456h	; FA 56 34 12
	assume	adl:0
	jp.lil	m,123456h	; 5B FA 56 34 12
	assume	adl:1
	jp.sis	m,1234h		; 40 FA 34 12

	; JP (HL)
	jp	(hl)		; E9
	assume	adl:1
	jp.s	(hl)		; 52 E9
	assume	adl:0
	jp.l	(hl)		; 49 E9

	; JP (IX/Y)
	jp	(ix)		; DD E9
	assume	adl:1
	jp.s	(ix)		; 52 DD E9 (TODO: should jp.s be forced to jp.sis?)
	assume	adl:0
	jp.l	(ix)		; 49 DD E9 (TODO: should jp.l be forced to jp.lil?)
	; JP (IX/Y)
	jp	(iy)		; FD E9
	assume	adl:1
	jp.s	(iy)		; 52 FD E9
	assume	adl:0
	jp.l	(iy)		; 49 FD E9


	; JP Mmn
	assume	adl:0
	jp	1234h		; C3 34 12
	assume	adl:1
	jp	123456h		; C3 56 34 12
	assume	adl:0
	jp.lil	123456h		; 5B C3 56 34 12
	assume	adl:1
	jp.sis	1234h		; 40 C3 34 12

	; JR cc', d
	jr	nz,$		; 20 FE
	jr	z,$		; 28 FE
	jr	nc,$		; 30 FE
	jr	c,$		; 38 FE

	; JR d
	jr	$		; 18 FE

	; LD A, I
	ld	a,i		; ED 57

	; LD A, (IX/Y+d)
	ld	a,(ix+12h)	; DD 7E 12
	assume	adl:1
	ld.s	a,(ix+12h)	; 52 DD 7E 12
	assume	adl:0
	ld.l	a,(ix+12h)	; 49 DD 7E 12
	ld	a,(iy+12h)	; FD 7E 12
	assume	adl:1
	ld.s	a,(iy+12h)	; 52 FD 7E 12
	assume	adl:0
	ld.l	a,(iy+12h)	; 49 FD 7E 12

	; LD A, MB
	ld	a,mb		; ED 6E

	; LD A, (Mmn)
	assume	adl:0
	ld	a,(1234h)	; 3A 34 12
	assume	adl:1
	ld	a,(123456h)	; 3A 56 34 12
	assume	adl:0
	ld.lil	a,(123456h)	; 5B 3A 56 34 12
	assume	adl:1
	ld.sis	a,(1234h)	; 40 3A 34 12

	; LD A, R
	ld	a,r		; ED 5F

	; LD A, (rr)
	ld	a,(bc)		; 0A
	assume	adl:1
	ld.s	a,(bc)		; 52 0A
	assume	adl:0
	ld.l	a,(bc)		; 49 0A
	ld	a,(de)		; 1A
	assume	adl:1
	ld.s	a,(de)		; 52 1A
	assume	adl:0
	ld.l	a,(de)		; 49 1A
	ld	a,(hl)		; 7E
	assume	adl:1
	ld.s	a,(hl)		; 52 7E
	assume	adl:0
	ld.l	a,(hl)		; 49 7E

	; LD HL, I
	ld	hl,i		; ED D7

	; LD (HL), IX/Y
	ld	(hl),ix		; ED 3F
	assume	adl:1
	ld.s	(hl),ix		; 52 ED 3F
	assume	adl:0
	ld.l	(hl),ix		; 49 ED 3F
	ld	(hl),iy		; ED 3E
	assume	adl:1
	ld.s	(hl),iy		; 52 ED 3E
	assume	adl:0
	ld.l	(hl),iy		; 49 ED 3E

	; LD (HL), n
	ld	(hl),12h	; 36 12
	assume	adl:1
	ld.s	(hl),12h	; 52 36 12
	assume	adl:0
	ld.l	(hl),12h	; 49 36 12

	; LD (HL), r
	ld	(hl),a		; 77
	assume	adl:1
	ld.s	(hl),a		; 52 77
	assume	adl:0
	ld.l	(hl),a		; 49 77
	ld	(hl),b		; 70
	assume	adl:1
	ld.s	(hl),b		; 52 70
	assume	adl:0
	ld.l	(hl),b		; 49 70
	ld	(hl),c		; 71
	assume	adl:1
	ld.s	(hl),c		; 52 71
	assume	adl:0
	ld.l	(hl),c		; 49 71
	ld	(hl),d		; 72
	assume	adl:1
	ld.s	(hl),d		; 52 72
	assume	adl:0
	ld.l	(hl),d		; 49 72
	ld	(hl),e		; 73
	assume	adl:1
	ld.s	(hl),e		; 52 73
	assume	adl:0
	ld.l	(hl),e		; 49 73
	ld	(hl),h		; 74
	assume	adl:1
	ld.s	(hl),h		; 52 74
	assume	adl:0
	ld.l	(hl),h		; 49 74
	ld	(hl),l		; 75
	assume	adl:1
	ld.s	(hl),l		; 52 75
	assume	adl:0
	ld.l	(hl),l		; 49 75

	; LD (HL), rr
	ld	(hl),bc		; ED 0F
	assume	adl:1
	ld.s	(hl),bc		; 52 ED 0F
	assume	adl:0
	ld.l	(hl),bc		; 49 ED 0F
	ld	(hl),de		; ED 1F
	assume	adl:1
	ld.s	(hl),de		; 52 ED 1F
	assume	adl:0
	ld.l	(hl),de		; 49 ED 1F
	ld	(hl),hl		; ED 1F
	assume	adl:1
	ld.s	(hl),hl		; 52 ED 2F
	assume	adl:0
	ld.l	(hl),hl		; 49 ED 2F

	; LD I, HL
	ld	i,hl		; ED C7

	; LD I, A
	ld	i,a		; ED 47

	; LD ir,ir'
	ld	ixh,ixh		; DD 64
	ld	ixh,ixl		; DD 65
	ld	ixl,ixh		; DD 6C
	ld	ixl,ixl		; DD 6D
	ld	iyh,iyh		; FD 64
	ld	iyh,iyl		; FD 65
	ld	iyl,iyh		; FD 6C
	ld	iyl,iyl		; FD 6D

	; LD ir, n
	ld	ixh,12h		; DD 26 12
	ld	ixl,12h		; DD 2E 12
	ld	iyh,12h		; FD 26 12
	ld	iyl,12h		; FD 2E 12

	; LD ir, r
	ld	ixh,a		; DD 67
	ld	ixh,b		; DD 60
	ld	ixh,c		; DD 61
	ld	ixh,d		; DD 62
	ld	ixh,e		; DD 63
	ld	ixl,a		; DD 6F
	ld	ixl,b		; DD 68
	ld	ixl,c		; DD 69
	ld	ixl,d		; DD 6A
	ld	ixl,e		; DD 6B
	ld	iyh,a		; FD 67
	ld	iyh,b		; FD 60
	ld	iyh,c		; FD 61
	ld	iyh,d		; FD 62
	ld	iyh,e		; FD 63
	ld	iyl,a		; FD 6F
	ld	iyl,b		; FD 68
	ld	iyl,c		; FD 69
	ld	iyl,d		; FD 6A
	ld	iyl,e		; FD 6B

	; LD IX/Y, (HL)
	ld	ix,(hl)		; ED 37
	assume	adl:1
	ld.s	ix,(hl)		; 52 ED 37
	assume	adl:0
	ld.l	ix,(hl)		; 49 ED 37
	ld	iy,(hl)		; ED 31
	assume	adl:1
	ld.s	iy,(hl)		; 52 ED 31
	assume	adl:0
	ld.l	iy,(hl)		; 49 ED 31

	; LD IX/Y, (IX/Y+d)
	ld	ix,(ix+12h)	; DD 37 12
	assume	adl:1
	ld.s	ix,(ix+12h)	; 52 DD 37 12
	assume	adl:0
	ld.l	ix,(ix+12h)	; 49 DD 37 12
	ld	iy,(ix+12h)	; DD 31 12
	assume	adl:1
	ld.s	iy,(ix+12h)	; 52 DD 31 12
	assume	adl:0
	ld.l	iy,(ix+12h)	; 49 DD 31 12
	ld	ix,(iy+12h)	; FD 31 12
	assume	adl:1
	ld.s	ix,(iy+12h)	; 52 FD 31 12
	assume	adl:0
	ld.l	ix,(iy+12h)	; 49 FD 31 12
	ld	iy,(iy+12h)	; FD 37 12
	assume	adl:1
	ld.s	iy,(iy+12h)	; 52 FD 37 12
	assume	adl:0
	ld.l	iy,(iy+12h)	; 49 FD 37 12

	; LD IX/Y, Mmn
	assume	adl:0
	ld	ix,1234h	; DD 21 34 12
	assume	adl:1
	ld	ix,123456h	; DD 21 56 34 12
	assume	adl:0
	ld.lil	ix,123456h	; 5B DD 21 56 34 12
	assume	adl:1
	ld.sis	ix,1234h	; 40 DD 21 34 12
	assume	adl:0
	ld	iy,1234h	; FD 21 34 12
	assume	adl:1
	ld	iy,123456h	; FD 21 56 34 12
	assume	adl:0
	ld.lil	iy,123456h	; 5B FD 21 56 34 12
	assume	adl:1
	ld.sis	iy,1234h	; 40 FD 21 34 12

	; LD IX/Y, (Mmn)
	assume	adl:0
	ld	ix,(1234h)	; DD 2A 34 12
	assume	adl:1
	ld	ix,(123456h)	; DD 2A 56 34 12
	assume	adl:0
	ld.lil	ix,(123456h)	; 5B DD 2A 56 34 12
	assume	adl:1
	ld.sis	ix,(1234h)	; 40 DD 2A 34 12
	assume	adl:0
	ld	iy,(1234h)	; FD 2A 34 12
	assume	adl:1
	ld	iy,(123456h)	; FD 2A 56 34 12
	assume	adl:0
	ld.lil	iy,(123456h)	; 5B FD 2A 56 34 12
	assume	adl:1
	ld.sis	iy,(1234h)	; 40 FD 2A 34 12

	; LD (IX/Y+d), IX/Y
	ld	(ix+12h),ix	; DD 3F 12
	assume	adl:1
	ld.s	(ix+12h),ix	; 52 DD 3F 12
	assume	adl:0
	ld.l	(ix+12h),ix	; 49 DD 3F 12
	ld	(ix+12h),iy	; DD 3E 12
	assume	adl:1
	ld.s	(ix+12h),iy	; 52 DD 3E 12
	assume	adl:0
	ld.l	(ix+12h),iy	; 49 DD 3E 12
	ld	(iy+12h),ix	; FD 3E 12
	assume	adl:1
	ld.s	(iy+12h),ix	; 52 FD 3E 12
	assume	adl:0
	ld.l	(iy+12h),ix	; 49 FD 3E 12
	ld	(iy+12h),iy	; FD 3F 12
	assume	adl:1
	ld.s	(iy+12h),iy	; 52 FD 3F 12
	assume	adl:0
	ld.l	(iy+12h),iy	; 49 FD 3F 12

	; LD (IX/Y+d), n
	ld	(ix+12h), 34h	; DD 36 12 34
	assume	adl:1
	ld.s	(ix+12h), 34h	; 52 DD 36 12 34
	assume	adl:0
	ld.l	(ix+12h), 34h	; 49 DD 36 12 34
	ld	(iy+12h), 34h	; FD 36 12 34
	assume	adl:1
	ld.s	(iy+12h), 34h	; 52 FD 36 12 34
	assume	adl:0
	ld.l	(iy+12h), 34h	; 49 FD 36 12 34

	; LD (IX/Y+d), r
	ld	(ix+12h),a	; DD 77 12
	ld	(ix+12h),b	; DD 70 12
	ld	(ix+12h),c	; DD 71 12
	ld	(ix+12h),d	; DD 72 12
	ld	(ix+12h),e	; DD 73 12
	ld	(ix+12h),h	; DD 74 12
	ld	(ix+12h),l	; DD 75 12
	assume	adl:1
	ld.s	(ix+12h),a	; 52 DD 77 12
	ld.s	(ix+12h),b	; 52 DD 70 12
	ld.s	(ix+12h),c	; 52 DD 71 12
	ld.s	(ix+12h),d	; 52 DD 72 12
	ld.s	(ix+12h),e	; 52 DD 73 12
	ld.s	(ix+12h),h	; 52 DD 74 12
	ld.s	(ix+12h),l	; 52 DD 75 12
	assume	adl:0
	ld.l	(ix+12h),a	; 49 DD 77 12
	ld.l	(ix+12h),b	; 49 DD 70 12
	ld.l	(ix+12h),c	; 49 DD 71 12
	ld.l	(ix+12h),d	; 49 DD 72 12
	ld.l	(ix+12h),e	; 49 DD 73 12
	ld.l	(ix+12h),h	; 49 DD 74 12
	ld.l	(ix+12h),l	; 49 DD 75 12
	ld	(iy+12h),a	; FD 77 12
	ld	(iy+12h),b	; FD 70 12
	ld	(iy+12h),c	; FD 71 12
	ld	(iy+12h),d	; FD 72 12
	ld	(iy+12h),e	; FD 73 12
	ld	(iy+12h),h	; FD 74 12
	ld	(iy+12h),l	; FD 75 12
	assume	adl:1
	ld.s	(iy+12h),a	; 52 FD 77 12
	ld.s	(iy+12h),b	; 52 FD 70 12
	ld.s	(iy+12h),c	; 52 FD 71 12
	ld.s	(iy+12h),d	; 52 FD 72 12
	ld.s	(iy+12h),e	; 52 FD 73 12
	ld.s	(iy+12h),h	; 52 FD 74 12
	ld.s	(iy+12h),l	; 52 FD 75 12
	assume	adl:0
	ld.l	(iy+12h),a	; 49 FD 77 12
	ld.l	(iy+12h),b	; 49 FD 70 12
	ld.l	(iy+12h),c	; 49 FD 71 12
	ld.l	(iy+12h),d	; 49 FD 72 12
	ld.l	(iy+12h),e	; 49 FD 73 12
	ld.l	(iy+12h),h	; 49 FD 74 12
	ld.l	(iy+12h),l	; 49 FD 75 12

	; LD (IX/Y+d), rr
	ld	(ix+12h),bc	; DD 0F 12
	assume	adl:1
	ld.s	(ix+12h),bc	; 52 DD 0F 12
	assume	adl:0
	ld.l	(ix+12h),bc	; 49 DD 0F 12
	ld	(ix+12h),de	; DD 1F 12
	assume	adl:1
	ld.s	(ix+12h),de	; 52 DD 1F 12
	assume	adl:0
	ld.l	(ix+12h),de	; 49 DD 1F 12
	ld	(ix+12h),hl	; DD 2F 12
	assume	adl:1
	ld.s	(ix+12h),hl	; 52 DD 2F 12
	assume	adl:0
	ld.l	(ix+12h),hl	; 49 DD 2F 12
	ld	(iy+12h),bc	; FD 0F 12
	assume	adl:1
	ld.s	(iy+12h),bc	; 52 FD 0F 12
	assume	adl:0
	ld.l	(iy+12h),bc	; 49 FD 0F 12
	ld	(iy+12h),de	; FD 1F 12
	assume	adl:1
	ld.s	(iy+12h),de	; 52 FD 1F 12
	assume	adl:0
	ld.l	(iy+12h),de	; 49 FD 1F 12
	ld	(iy+12h),hl	; FD 2F 12
	assume	adl:1
	ld.s	(iy+12h),hl	; 52 FD 2F 12
	assume	adl:0
	ld.l	(iy+12h),hl	; 49 FD 2F 12

	; LD MB, A
	ld	mb,a		; ED 6D

	; LD (Mmn), A
	assume	adl:0
	ld	(1234h),a	; 32 34 12
	assume	adl:1
	ld	(123456h),a	; 32 56 34 12
	; NOTE: User Manual lists .is and .il as prefix, but it should
	; be .sis and .lil like for LD A,(Mmn)
	assume	adl:1
	ld.sis	(1234h),a	; 40 32 34 12
	assume	adl:0
	ld.lil	(123456h),a	; 5B 32 56 34 12

	; LD (Mmn), IX/Y
	assume	adl:0
	ld	(1234h),ix	; DD 22 34 12
	assume	adl:1
	ld	(123456h),ix	; DD 22 56 34 12
	assume	adl:0
	ld.lil	(123456h),ix	; 5B DD 22 56 34 12
	assume	adl:1
	ld.sis	(1234h),ix	; 40 DD 22 34 12
	assume	adl:0
	ld	(1234h),iy	; FD 22 34 12
	assume	adl:1
	ld	(123456h),iy	; FD 22 56 34 12
	assume	adl:0
	ld.lil	(123456h),iy	; 5B FD 22 56 34 12
	assume	adl:1
	ld.sis	(1234h),iy	; 40 FD 22 34 12

	; LD (Mmn), rr
	assume	adl:0
	ld	(1234h),bc	; ED 43 34 12
	assume	adl:1
	ld	(123456h),bc	; ED 43 56 34 12
	assume	adl:1
	ld.sis	(1234h),bc	; 40 ED 43 34 12
	assume	adl:0
	ld.lil	(123456h),bc	; 5B ED 43 56 34 12
	assume	adl:0
	ld	(1234h),de	; ED 53 34 12
	assume	adl:1
	ld	(123456h),de	; ED 53 56 34 12
	assume	adl:1
	ld.sis	(1234h),de	; 40 ED 53 34 12
	assume	adl:0
	ld.lil	(123456h),de	; 5B ED 53 56 34 12
	assume	adl:0
	ld	(1234h),hl	; 22 34 12
	assume	adl:1
	ld	(123456h),hl	; 22 56 34 12
	assume	adl:1
	ld.sis	(1234h),hl	; 40 22 34 12
	assume	adl:0
	ld.lil	(123456h),hl	; 5B 22 56 34 12

	; LD (Mmn), SP
	assume	adl:0
	ld	(1234h),sp	; ED 73 34 12
	assume	adl:1
	ld	(123456h),sp	; ED 73 56 34 12
	assume	adl:1
	ld.sis	(1234h),sp	; 40 ED 73 34 12
	assume	adl:0
	ld.lil	(123456h),sp	; 5B ED 73 56 34 12

	; LD R, A
	ld	r,a		; ED 4F

	; LD r, (HL)
	ld	a,(hl)		; 7E
	ld	b,(hl)		; 46
	ld	c,(hl)		; 4E
	ld	d,(hl)		; 56
	ld	e,(hl)		; 5E
	ld	h,(hl)		; 66
	ld	l,(hl)		; 6E
	assume	adl:1
	ld.s	a,(hl)		; 52 7E
	ld.s	b,(hl)		; 52 46
	ld.s	c,(hl)		; 52 4E
	ld.s	d,(hl)		; 52 56
	ld.s	e,(hl)		; 52 5E
	ld.s	h,(hl)		; 52 66
	ld.s	l,(hl)		; 52 6E
	assume	adl:0
	ld.l	a,(hl)		; 49 7E
	ld.l	b,(hl)		; 49 46
	ld.l	c,(hl)		; 49 4E
	ld.l	d,(hl)		; 49 56
	ld.l	e,(hl)		; 49 5E
	ld.l	h,(hl)		; 49 66
	ld.l	l,(hl)		; 49 6E

	; LD r, ir
	ld	a,ixh		; DD 7C
	ld	a,ixl		; DD 7D
	ld	a,iyh		; FD 7C
	ld	a,iyl		; FD 7D
	ld	b,ixh		; DD 44
	ld	b,ixl		; DD 45
	ld	b,iyh		; FD 44
	ld	b,iyl		; FD 45
	ld	c,ixh		; DD 4C
	ld	c,ixl		; DD 4D
	ld	c,iyh		; FD 4C
	ld	c,iyl		; FD 4D
	ld	d,ixh		; DD 54
	ld	d,ixl		; DD 55
	ld	d,iyh		; FD 54
	ld	d,iyl		; FD 55
	ld	e,ixh		; DD 5C
	ld	e,ixl		; DD 5D
	ld	e,iyh		; FD 5C
	ld	e,iyl		; FD 5D

	; LD r, (IX/Y+d)
	ld	a,(ix+12h)	; DD 7E 12
	ld	b,(ix+12h)	; DD 46 12
	ld	c,(ix+12h)	; DD 4E 12
	ld	d,(ix+12h)	; DD 56 12
	ld	e,(ix+12h)	; DD 5E 12
	ld	h,(ix+12h)	; DD 66 12
	ld	l,(ix+12h)	; DD 6E 12
	assume	adl:1
	ld.s	a,(ix+12h)	; 52 DD 7E 12
	ld.s	b,(ix+12h)	; 52 DD 46 12
	ld.s	c,(ix+12h)	; 52 DD 4E 12
	ld.s	d,(ix+12h)	; 52 DD 56 12
	ld.s	e,(ix+12h)	; 52 DD 5E 12
	ld.s	h,(ix+12h)	; 52 DD 66 12
	ld.s	l,(ix+12h)	; 52 DD 6E 12
	assume	adl:0
	ld.l	a,(ix+12h)	; 49 DD 7E 12
	ld.l	b,(ix+12h)	; 49 DD 46 12
	ld.l	c,(ix+12h)	; 49 DD 4E 12
	ld.l	d,(ix+12h)	; 49 DD 56 12
	ld.l	e,(ix+12h)	; 49 DD 5E 12
	ld.l	h,(ix+12h)	; 49 DD 66 12
	ld.l	l,(ix+12h)	; 49 DD 6E 12
	ld	a,(iy+12h)	; FD 7E 12
	ld	b,(iy+12h)	; FD 46 12
	ld	c,(iy+12h)	; FD 4E 12
	ld	d,(iy+12h)	; FD 56 12
	ld	e,(iy+12h)	; FD 5E 12
	ld	h,(iy+12h)	; FD 66 12
	ld	l,(iy+12h)	; FD 6E 12
	assume	adl:1
	ld.s	a,(iy+12h)	; 52 FD 7E 12
	ld.s	b,(iy+12h)	; 52 FD 46 12
	ld.s	c,(iy+12h)	; 52 FD 4E 12
	ld.s	d,(iy+12h)	; 52 FD 56 12
	ld.s	e,(iy+12h)	; 52 FD 5E 12
	ld.s	h,(iy+12h)	; 52 FD 66 12
	ld.s	l,(iy+12h)	; 52 FD 6E 12
	assume	adl:0
	ld.l	a,(iy+12h)	; 49 FD 7E 12
	ld.l	b,(iy+12h)	; 49 FD 46 12
	ld.l	c,(iy+12h)	; 49 FD 4E 12
	ld.l	d,(iy+12h)	; 49 FD 56 12
	ld.l	e,(iy+12h)	; 49 FD 5E 12
	ld.l	h,(iy+12h)	; 49 FD 66 12
	ld.l	l,(iy+12h)	; 49 FD 6E 12

	; LD r, n
	ld	a,12h		; 3E 12
	ld	b,12h		; 06 12
	ld	c,12h		; 0E 12
	ld	d,12h		; 16 12
	ld	e,12h		; 1E 12
	ld	h,12h		; 26 12
	ld	l,12h		; 2E 12

	; LD r, r'
	ld	a,a		; 7F
	ld	a,b		; 78
	ld	a,c		; 79
	ld	a,d		; 7A
	ld	a,e		; 7B
	ld	a,h		; 7C
	ld	a,l		; 7D
	ld	b,a		; 47
	expect	470
	ld	b,b		; 00 !!
	endexpect
	ld	b,c		; 41
	ld	b,d		; 42
	ld	b,e		; 43
	ld	b,h		; 44
	ld	b,l		; 45
	ld	c,a		; 4F
	ld	c,b		; 48
	expect	470
	ld	c,c		; 00 !!
	endexpect
	ld	c,d		; 4A
	ld	c,e		; 4B
	ld	c,h		; 4C
	ld	c,l		; 4D
	ld	d,a		; 57
	ld	d,b		; 50
	ld	d,c		; 51
	expect	470
	ld	d,d		; 00 !!
	endexpect
	ld	d,e		; 53
	ld	d,h		; 54
	ld	d,l		; 55
	ld	e,a		; 5F
	ld	e,b		; 58
	ld	e,c		; 59
	ld	e,d		; 5A
	expect	470
	ld	e,e		; 00 !!
	endexpect
	ld	e,h		; 5C
	ld	e,l		; 5D
	ld	h,a		; 67
	ld	h,b		; 60
	ld	h,c		; 61
	ld	h,d		; 62
	ld	h,e		; 63
	ld	h,h		; 64
	ld	h,l		; 65
	ld	l,a		; 6F
	ld	l,b		; 68
	ld	l,c		; 69
	ld	l,d		; 6A
	ld	l,e		; 6B
	ld	l,h		; 6C
	ld	l,l		; 6D

	; LD rr, (HL)
	ld	bc,(hl)		; ED 07
	assume	adl:1
	ld.s	bc,(hl)		; 52 ED 07
	assume	adl:0
	ld.l	bc,(hl)		; 49 ED 07
	ld	de,(hl)		; ED 17
	assume	adl:1
	ld.s	de,(hl)		; 52 ED 17
	assume	adl:0
	ld.l	de,(hl)		; 49 ED 17
	ld	hl,(hl)		; ED 27
	assume	adl:1
	ld.s	hl,(hl)		; 52 ED 27
	assume	adl:0
	ld.l	hl,(hl)		; 49 ED 27

	; LD rr, (IX/Y+d)
	ld	bc,(ix+12h)	; DD 07 12
	assume	adl:1
	ld.s	bc,(ix+12h)	; 52 DD 07 12
	assume	adl:0
	ld.l	bc,(ix+12h)	; 49 DD 07 12
	ld	de,(ix+12h)	; DD 17 12
	assume	adl:1
	ld.s	de,(ix+12h)	; 52 DD 17 12
	assume	adl:0
	ld.l	de,(ix+12h)	; 49 DD 17 12
	ld	hl,(ix+12h)	; DD 27 12
	assume	adl:1
	ld.s	hl,(ix+12h)	; 52 DD 27 12
	assume	adl:0
	ld.l	hl,(ix+12h)	; 49 DD 27 12
	ld	bc,(iy+12h)	; FD 07 12
	assume	adl:1
	ld.s	bc,(iy+12h)	; 52 FD 07 12
	assume	adl:0
	ld.l	bc,(iy+12h)	; 49 FD 07 12
	ld	de,(iy+12h)	; FD 17 12
	assume	adl:1
	ld.s	de,(iy+12h)	; 52 FD 17 12
	assume	adl:0
	ld.l	de,(iy+12h)	; 49 FD 17 12
	ld	hl,(iy+12h)	; FD 27 12
	assume	adl:1
	ld.s	hl,(iy+12h)	; 52 FD 27 12
	assume	adl:0
	ld.l	hl,(iy+12h)	; 49 FD 27 12

	; LD rr, Mmn
	assume	adl:0
	ld	bc,1234h	; 01 34 12
	ld	de,1234h	; 11 34 12
	ld	hl,1234h	; 21 34 12
	assume	adl:1
	ld	bc,123456h	; 01 56 34 12
	ld	de,123456h	; 11 56 34 12
	ld	hl,123456h	; 21 56 34 12
	assume	adl:0
	ld.lil	bc,123456h	; 5B 01 56 34 12
	ld.lil	de,123456h	; 5B 11 56 34 12
	ld.lil	hl,123456h	; 5B 21 56 34 12
	assume	adl:1
	ld.sis	bc,1234h	; 40 01 34 12
	ld.sis	de,1234h	; 40 11 34 12
	ld.sis	hl,1234h	; 40 21 34 12

	; LD rr, (Mmn)
	assume	adl:0
	ld	bc,(1234h)	; ED 4B 34 12
	ld	de,(1234h)	; ED 5B 34 12
	ld	hl,(1234h)	; 2A 34 12
	assume	adl:1
	ld	bc,(123456h)	; ED 4B 56 34 12
	ld	de,(123456h)	; ED 5B 56 34 12
	ld	hl,(123456h)	; 2A 56 34 12
	assume	adl:0
	ld.lil	bc,(123456h)	; 5B ED 4B 56 34 12
	ld.lil	de,(123456h)	; 5B ED 5B 56 34 12
	ld.lil	hl,(123456h)	; 5B 2A 56 34 12
	assume	adl:1
	ld.sis	bc,(1234h)	; 40 ED 4B 34 12
	ld.sis	de,(1234h)	; 40 ED 5B 34 12
	ld.sis	hl,(1234h)	; 40 2A 34 12

	; LD (rr), A
	ld	(bc),a		; 02
	assume	adl:1
	ld.s	(bc),a		; 52 02
	assume	adl:0
	ld.l	(bc),a		; 49 02
	ld	(de),a		; 12
	assume	adl:1
	ld.s	(de),a		; 52 12
	assume	adl:0
	ld.l	(de),a		; 49 12
	ld	(hl),a		; 77
	assume	adl:1
	ld.s	(hl),a		; 52 77
	assume	adl:0
	ld.l	(hl),a		; 49 77

	; LD SP, HL
	ld	sp,hl		; F9
	assume	adl:1
	ld.s	sp,hl		; 52 F9
	assume	adl:0
	ld.l	sp,hl		; 49 F9

	; LD SP, IX/Y
	ld	sp,ix		; DD F9
	assume	adl:1
	ld.s	sp,ix		; 52 DD F9
	assume	adl:0
	ld.l	sp,ix		; 49 DD F9
	ld	sp,iy		; FD F9
	assume	adl:1
	ld.s	sp,iy		; 52 FD F9
	assume	adl:0
	ld.l	sp,iy		; 49 FD F9

	; LD SP, Mmn
	assume	adl:0
	ld	sp,1234h	; 31 34 12
	assume	adl:1
	ld	sp,123456h	; 31 56 34 12
	assume	adl:0
	ld.lil	sp,123456h	; 5B 31 56 34 12
	assume	adl:1
	ld.sis	sp,1234h	; 40 31 34 12

	; LD SP, (Mmn)
	assume	adl:0
	ld	sp,(1234h)	; ED 7B 34 12
	assume	adl:1
	ld	sp,(123456h)	; ED 7B 56 34 12
	assume	adl:0
	ld.lil	sp,(123456h)	; 5B ED 7B 56 34 12
	assume	adl:1
	ld.sis	sp,(1234h)	; 40 ED 7B 34 12

	; LDD
	ldd			; ED A8
	assume	adl:1
	ldd.s			; 52 ED A8
	assume	adl:0
	ldd.l			; 49 ED A8

	; LDDR
	lddr			; ED B8
	assume	adl:1
	lddr.s			; 52 ED B8
	assume	adl:0
	lddr.l			; 49 ED B8

	; LDI
	ldi			; ED A0
	assume	adl:1
	ldi.s			; 52 ED A0
	assume	adl:0
	ldi.l			; 49 ED A0

	; LDIR
	ldir			; ED B0
	assume	adl:1
	ldir.s			; 52 ED B0
	assume	adl:0
	ldir.l			; 49 ED B0

	; LEA IX/Y, IX+d
	lea	ix,ix+12h	; ED 32 12
	assume	adl:1
	lea.s	ix,ix+12h	; 52 ED 32 12
	assume	adl:0
	lea.l	ix,ix+12h	; 49 ED 32 12
	lea	iy,ix+12h	; ED 55 12
	assume	adl:1
	lea.s	iy,ix+12h	; 52 ED 55 12
	assume	adl:0
	lea.l	iy,ix+12h	; 49 ED 55 12

	; LEA IX/Y, IY+d
	lea	ix,iy+12h	; ED 54 12
	assume	adl:1
	lea.s	ix,iy+12h	; 52 ED 54 12
	assume	adl:0
	lea.l	ix,iy+12h	; 49 ED 54 12
	lea	iy,iy+12h	; ED 33 12
	assume	adl:1
	lea.s	iy,iy+12h	; 52 ED 33 12
	assume	adl:0
	lea.l	iy,ix+12h	; 49 ED 33 12

	; LEA rr, IX+d
	lea	bc,ix+12h	; ED 02 12
	assume	adl:1
	lea.s	bc,ix+12h	; 52 ED 02 12
	assume	adl:0
	lea.l	bc,ix+12h	; 49 ED 02 12
	lea	de,ix+12h	; ED 12 12
	assume	adl:1
	lea.s	de,ix+12h	; 52 ED 12 12
	assume	adl:0
	lea.l	de,ix+12h	; 49 ED 12 12
	lea	hl,ix+12h	; ED 22 12
	assume	adl:1
	lea.s	hl,ix+12h	; 52 ED 22 12
	assume	adl:0
	lea.l	hl,ix+12h	; 49 ED 22 12

	; LEA rr, IY+d
	lea	bc,iy+12h	; ED 03 12
	assume	adl:1
	lea.s	bc,iy+12h	; 52 ED 03 12
	assume	adl:0
	lea.l	bc,iy+12h	; 49 ED 03 12
	lea	de,iy+12h	; ED 13 12
	assume	adl:1
	lea.s	de,iy+12h	; 52 ED 13 12
	assume	adl:0
	lea.l	de,iy+12h	; 49 ED 13 12
	lea	hl,iy+12h	; ED 23 12
	assume	adl:1
	lea.s	hl,iy+12h	; 52 ED 23 12
	assume	adl:0
	lea.l	hl,iy+12h	; 49 ED 23 12

	; MLT rr
	mlt	bc		; ED 4C
	mlt	de		; ED 5C
	mlt	hl		; ED 6C

	; MLT SO
	mlt	sp		; ED 7C
	assume	adl:0
	mlt.l	sp		; 49 ED 7C
	assume	adl:1
	mlt.s	sp		; 52 ED 7C

	; NEG
	neg			; ED 44

	; NOP
	nop			; 00

	; OR A, (HL)
	or	a,(hl)		; B6
	assume	adl:1
	or.s	a,(hl)		; 52 B6
	assume	adl:0
	or.l	a,(hl)		; 49 B6

	; OR A, ir
	or	a,ixh		; DD B4
	or	a,ixl		; DD B5
	or	a,iyh		; FD B4
	or	a,iyl		; FD B5

	; OR A, (IX/Y+d)
	or	a,(ix+12h)	; DD B6 12
	assume	adl:1
	or.s	a,(ix+12h)	; 52 DD B6 12
	assume	adl:0
	or.l	a,(ix+12h)	; 49 DD B6 12
	or	a,(iy+12h)	; FD B6 12
	assume	adl:1
	or.s	a,(iy+12h)	; 52 FD B6 12
	assume	adl:0
	or.l	a,(iy+12h)	; 49 FD B6 12

	; OR A, n
	or	a,12h		; F6 12

	; OR A, r
	or	a,a		; B7
	or	a,b		; B0
	or	a,c		; B1
	or	a,d		; B2
	or	a,e		; B3
	or	a,h		; B4
	or	a,l		; B5

	; OTD2R
	otd2r			; ED BC
	assume	adl:1
	otd2r.s			; 52 ED BC
	assume	adl:0
	otd2r.l			; 49 ED BC

	; OTDM
	otdm			; ED 8B
	assume	adl:1
	otdm.s			; 52 ED 8B
	assume	adl:0
	otdm.l			; 49 ED 8B

	; OTDMR
	otdmr			; ED 9B
	assume	adl:1
	otdmr.s			; 52 ED 9B
	assume	adl:0
	otdmr.l			; 49 ED 9B

	; OTDR
	otdr			; ED BB
	assume	adl:1
	otdr.s			; 52 ED BB
	assume	adl:0
	otdr.l			; 49 ED BB

	; OTDRX
	otdrx			; ED CB
	assume	adl:1
	otdrx.s			; 52 ED CB
	assume	adl:0
	otdrx.l			; 49 ED CB

	; OTI2R
	oti2r			; ED B4
	assume	adl:1
	oti2r.s			; 52 ED B4
	assume	adl:0
	oti2r.l			; 49 ED B4

	; OTIM
	otim			; ED 83
	assume	adl:1
	otim.s			; 52 ED 83
	assume	adl:0
	otim.l			; 49 ED 83

	; OTIMR
	otimr			; ED 93
	assume	adl:1
	otimr.s			; 52 ED 93
	assume	adl:0
	otimr.l			; 49 ED 93

	; OTIR
	otir			; ED B3
	assume	adl:1
	otir.s			; 52 ED B3
	assume	adl:0
	otir.l			; 49 ED B3

	; OTIRX
	otirx			; ED C3
	assume	adl:1
	otirx.s			; 52 ED C3
	assume	adl:0
	otirx.l			; 49 ED C3

	; OUT ([B]C), r
	out	(bc),a		; ED 79
	out	(c),b		; ED 41
	out	(bc),c		; ED 49
	out	(c),d		; ED 51
	out	(bc),e		; ED 59
	out	(c),h		; ED 61
	out	(bc),l		; ED 69

	; OUT (n), A
	out	(12h),a		; D3 12

	; OUT0 (n), r
	out0	(12h),a		; ED 39 12
	out0	(12h),b		; ED 01 12
	out0	(12h),c		; ED 09 12
	out0	(12h),d		; ED 11 12
	out0	(12h),e		; ED 19 12
	out0	(12h),h		; ED 21 12
	out0	(12h),l		; ED 29 12

	; OUTD
	outd			; ED AB
	assume	adl:1
	outd.s			; 52 ED AB
	assume	adl:0
	outd.l			; 49 ED AB

	; OUTD2
	outd2			; ED AC
	assume	adl:1
	outd2.s			; 52 ED AC
	assume	adl:0
	outd2.l			; 49 ED AC

	; OUTI
	outi			; ED A3
	assume	adl:1
	outi.s			; 52 ED A3
	assume	adl:0
	outi.l			; 49 ED A3

	; OUTI2
	outi2			; ED AC4
	assume	adl:1
	outi2.s			; 52 ED A4
	assume	adl:0
	outi2.l			; 49 ED A4

	; PEA IX+d
	pea	ix+12h		; ED 65 12
	assume	adl:1
	pea.s	ix+12h		; 52 ED 65 12
	assume	adl:0
	pea.l	ix+12h		; 49 ED 65 12

	; PEA IY+d
	pea	iy+12h		; ED 66 12
	assume	adl:1
	pea.s	iy+12h		; 52 ED 66 12
	assume	adl:0
	pea.l	iy+12h		; 49 ED 66 12

	; POP AF
	pop	af		; F1
	assume	adl:1
	pop.s	af		; 52 F1
	assume	adl:0
	pop.l	af		; 49 F1

	; POP IX/Y
	pop	ix		; DD E1
	assume	adl:1
	pop.s	ix		; 52 DD E1
	assume	adl:0
	pop.l	ix		; 49 DD E1
	pop	iy		; FD E1
	assume	adl:1
	pop.s	iy		; 52 FD E1
	assume	adl:0
	pop.l	iy		; 49 FD E1

	; POP rr
	pop	bc		; C1
	pop	de		; D1
	pop	hl		; E1
	assume	adl:1
	pop.s	bc		; 52  C1
	pop.s	de		; 52  D1
	pop.s	hl		; 52  E1
	assume	adl:0
	pop.l	bc		; 49  C1
	pop.l	de		; 49  D1
	pop.l	hl		; 49  E1

	; PUSH AF
	push	af		; F5
	assume	adl:1
	push.s	af		; 52 F5
	assume	adl:0
	push.l	af		; 49 F5

	; PUSH IX/Y
	push	ix		; DD E5
	assume	adl:1
	push.s	ix		; 52 DD E5
	assume	adl:0
	push.l	ix		; 49 DD E5
	push	iy		; FD E5
	assume	adl:1
	push.s	iy		; 52 FD E5
	assume	adl:0
	push.l	iy		; 49 FD E5

	; PUSH rr
	push	bc		; C5
	push	de		; D5
	push	hl		; E5
	assume	adl:1
	push.s	bc		; 52  C5
	push.s	de		; 52  D5
	push.s	hl		; 52  E5
	assume	adl:0
	push.l	bc		; 49  C5
	push.l	de		; 49  D5
	push.l	hl		; 49  E5

	; RES b, (HL)
	res	5,(hl)		; CB AE
	assume	adl:1
	res.s	5,(hl)		; 52 CB AE
	assume	adl:0
	res.l	5,(hl)		; 49 CB AE

	; RES b, (IX/Y+d)
	res	5,(ix+12h)	; DD CB 12 AE
	assume	adl:1
	res.s	5,(ix+12h)	; 52 DD CB 12 AE
	assume	adl:0
	res.l	5,(ix+12h)	; 49 DD CB 12 AE
	res	5,(iy+12h)	; FD CB 12 AE
	assume	adl:1
	res.s	5,(iy+12h)	; 52 FD CB 12 AE
	assume	adl:0
	res.l	5,(iy+12h)	; 49 FD CB 12 AE

	; RES b, r
	res	5,a		; CB AF
	res	5,b		; CB A8
	res	5,c		; CB A9
	res	5,d		; CB AA
	res	5,e		; CB AB
	res	5,h		; CB AC
	res	5,l		; CB AD

	; RET
	ret			; C9
	assume	adl:0
	ret.l			; 49 C9
	assume	adl:1
	ret.l			; 5B C9

	; RET cc
	ret	nz		; C0
	assume	adl:0
	ret.l	nz		; 49 C0
	assume	adl:1
	ret.l	nz		; 5B C0
	ret	z		; C8
	assume	adl:0
	ret.l	z		; 49 C8
	assume	adl:1
	ret.l	z		; 5B C8
	ret	nc		; D0
	assume	adl:0
	ret.l	nc		; 49 D0
	assume	adl:1
	ret.l	nc		; 5B D0
	ret	c		; D8
	assume	adl:0
	ret.l	c		; 49 D8
	assume	adl:1
	ret.l	c		; 5B D8
	ret	po		; E0
	assume	adl:0
	ret.l	po		; 49 E0
	assume	adl:1
	ret.l	po		; 5B E0
	ret	pe		; E8
	assume	adl:0
	ret.l	pe		; 49 E8
	assume	adl:1
	ret.l	pe		; 5B E8
	ret	p		; F0
	assume	adl:0
	ret.l	p		; 49 F0
	assume	adl:1
	ret.l	p		; 5B F0
	ret	m		; F8
	assume	adl:0
	ret.l	m		; 49 F8
	assume	adl:1
	ret.l	m		; 5B F8

	; RETI
	reti			; ED 4D
	assume	adl:0
	reti.l			; 49 ED 4D
	assume	adl:1
	reti.l			; 5B ED 4D

	; RETN
	retn			; ED 45
	assume	adl:0
	retn.l			; 49 ED 45
	assume	adl:1
	retn.l			; 5B ED 45

	; RL (HL)
	rl	(hl)		; CB 16
	assume	adl:1
	rl.s	(hl)		; 52 CB 16
	assume	adl:0
	rl.l	(hl)		; 49 CB 16

	; RL (IX/Y+d)
	rl	(ix+12h)	; DD CB 12 16
	assume	adl:1
	rl.s	(ix+12h)	; 52 DD CB 12 16
	assume	adl:0
	rl.l	(ix+12h)	; 49 DD CB 12 16
	rl	(iy+12h)	; FD CB 12 16
	assume	adl:1
	rl.s	(iy+12h)	; 52 FD CB 12 16
	assume	adl:0
	rl.l	(iy+12h)	; 49 FD CB 12 16

	; RL r
	rl	a		; CB 17
	rl	b		; CB 10
	rl	c		; CB 11
	rl	d		; CB 12
	rl	e		; CB 13
	rl	h		; CB 14
	rl	l		; CB 15

	; RLA
	rla			; 17

	; RLC (HL)
	rlc	(hl)		; CB 06
	assume	adl:1
	rlc.s	(hl)		; 52 CB 06
	assume	adl:0
	rlc.l	(hl)		; 49 CB 06

	; RLC (IX/Y+d)
	rlc	(ix+12h)	; DD CB 12 06
	assume	adl:1
	rlc.s	(ix+12h)	; 52 DD CB 12 06
	assume	adl:0
	rlc.l	(ix+12h)	; 49 DD CB 12 06
	rlc	(iy+12h)	; FD CB 12 06
	assume	adl:1
	rlc.s	(iy+12h)	; 52 FD CB 12 06
	assume	adl:0
	rlc.l	(iy+12h)	; 49 FD CB 12 06

	; RLC r
	rlc	a		; CB 07
	rlc	b		; CB 00
	rlc	c		; CB 01
	rlc	d		; CB 02
	rlc	e		; CB 03
	rlc	h		; CB 04
	rlc	l		; CB 05

	; RLCA
	rlca			; 07

	; RLD
	rld			; ED 6F

	; RR (HL)
	rr	(hl)		; CB 1E
	assume	adl:1
	rr.s	(hl)		; 52 CB 1E
	assume	adl:0
	rr.l	(hl)		; 49 CB 1E

	; RR (IX/Y+d)
	rr	(ix+12h)	; DD CB 12 1E
	assume	adl:1
	rr.s	(ix+12h)	; 52 DD CB 12 1E
	assume	adl:0
	rr.l	(ix+12h)	; 49 DD CB 12 1E
	rr	(iy+12h)	; FD CB 12 1E
	assume	adl:1
	rr.s	(iy+12h)	; 52 FD CB 12 1E
	assume	adl:0
	rr.l	(iy+12h)	; 49 FD CB 12 1E

	; RR r
	rr	a		; CB 1F
	rr	b		; CB 18
	rr	c		; CB 19
	rr	d		; CB 1A
	rr	e		; CB 1B
	rr	h		; CB 1C
	rr	l		; CB 1D

	; RRA
	rra			; 1F

	; RRC (HL)
	rrc	(hl)		; CB 0E
	assume	adl:1
	rrc.s	(hl)		; 52 CB 0E
	assume	adl:0
	rrc.l	(hl)		; 49 CB 0E

	; RRC (IX/Y+d)
	rrc	(ix+12h)	; DD CB 12 0E
	assume	adl:1
	rrc.s	(ix+12h)	; 52 DD CB 12 0E
	assume	adl:0
	rrc.l	(ix+12h)	; 49 DD CB 12 0E
	rrc	(iy+12h)	; FD CB 12 0E
	assume	adl:1
	rrc.s	(iy+12h)	; 52 FD CB 12 0E
	assume	adl:0
	rrc.l	(iy+12h)	; 49 FD CB 12 0E

	; RRC r
	rrc	a		; CB 0F
	rrc	b		; CB 08
	rrc	c		; CB 09
	rrc	d		; CB 0A
	rrc	e		; CB 0B
	rrc	h		; CB 0C
	rrc	l		; CB 0D

	; RRCA
	rrca			; 0F

	; RRD
	rrd			; ED 67

	; RSMIX
	rsmix			; ED 7E

	; RST
	rst	00h		; C7
	rst	08h		; CF
	rst	10h		; D7
	rst	18h		; DF
	rst	20h		; E7
	rst	28h		; EF
	rst	30h		; F7
	rst	38h		; FF
	assume	adl:1
	rst.s	00h		; 52 C7
	rst.s	08h		; 52 CF
	rst.s	10h		; 52 D7
	rst.s	18h		; 52 DF
	rst.s	20h		; 52 E7
	rst.s	28h		; 52 EF
	rst.s	30h		; 52 F7
	rst.s	38h		; 52 FF
	assume	adl:0
	rst.l	00h		; 49 C7
	rst.l	08h		; 49 CF
	rst.l	10h		; 49 D7
	rst.l	18h		; 49 DF
	rst.l	20h		; 49 E7
	rst.l	28h		; 49 EF
	rst.l	30h		; 49 F7
	rst.l	38h		; 49 FF

	; SBC A, (HL)
	sbc	a,(hl)		; 9E
	assume	adl:1
	sbc.s	a,(hl)		; 52 9E
	assume	adl:0
	sbc.l	a,(hl)		; 49 9E

	; SBC A, ir
	sbc	a,ixh		; DD 9C
	sbc	a,ixl		; DD 9D
	sbc	a,iyh		; FD 9C
	sbc	a,iyl		; FD 9D

	; SBC A, (IX/Y+d)
	sbc	a,(ix+12h)	; DD 9E 12
	assume	adl:1
	sbc.s	a,(ix+12h)	; 52 DD 9E 12
	assume	adl:0
	sbc.l	a,(ix+12h)	; 49 DD 9E 12
	sbc	a,(iy+12h)	; FD 9E 12
	assume	adl:1
	sbc.s	a,(iy+12h)	; 52 FD 9E 12
	assume	adl:0
	sbc.l	a,(iy+12h)	; 49 FD 9E 12

	; SBC A, n
	sbc	a,12h		; DE 12

	; SBC A, r
	sbc	a,a		; 9F
	sbc	a,b		; 98
	sbc	a,c		; 99
	sbc	a,d		; 9A
	sbc	a,e		; 9B
	sbc	a,h		; 9C
	sbc	a,l		; 9D

	; SBC HL, rr
	sbc	hl,bc		; ED 42
	sbc	hl,de		; ED 52
	sbc	hl,hl		; ED 62
	assume	adl:1
	sbc.s	hl,bc		; 52 ED 42
	sbc.s	hl,de		; 52 ED 52
	sbc.s	hl,hl		; 52 ED 62
	assume	adl:0
	sbc.l	hl,bc		; 49 ED 42
	sbc.l	hl,de		; 49 ED 52
	sbc.l	hl,hl		; 49 ED 62

	expect	1350
	sbc	hl,ix
	endexpect
	expect	1350
	sbc	ix,hl
	endexpect

	; SBC HL, SP
	sbc	hl,sp		; ED 72
	assume	adl:1
	sbc.s	hl,sp		; 52 ED 72
	assume	adl:0
	sbc.l	hl,sp		; 49 ED 72

	; SCF
	scf			; 37

	; SET b, (HL)
	set	5,(hl)		; CB EE
	assume	adl:1
	set.s	5,(hl)		; 52 CB EE
	assume	adl:0
	set.l	5,(hl)		; 49 CB EE

	; SET b, (IX/Y+d)
	set	5,(ix+12h)	; DD CB 12 EE
	assume	adl:1
	set.s	5,(ix+12h)	; 52 DD CB 12 EE
	assume	adl:0
	set.l	5,(ix+12h)	; 49 DD CB 12 EE
	set	5,(iy+12h)	; FD CB 12 EE
	assume	adl:1
	set.s	5,(iy+12h)	; 52 FD CB 12 EE
	assume	adl:0
	set.l	5,(iy+12h)	; 49 FD CB 12 EE

	; SET b, r
	set	5,a		; CB EF
	set	5,b		; CB E8
	set	5,c		; CB E9
	set	5,d		; CB EA
	set	5,e		; CB EB
	set	5,h		; CB EC
	set	5,l		; CB ED

	; SLA (HL)
	sla	(hl)		; CB 26
	assume	adl:1
	sla.s	(hl)		; 52 CB 26
	assume	adl:0
	sla.l	(hl)		; 49 CB 26

	; SLA (IX/Y+d)
	sla	(ix+12h)	; DD CB 12 26
	assume	adl:1
	sla.s	(ix+12h)	; 52 DD CB 12 26
	assume	adl:0
	sla.l	(ix+12h)	; 49 DD CB 12 26
	sla	(iy+12h)	; FD CB 12 26
	assume	adl:1
	sla.s	(iy+12h)	; 52 FD CB 12 26
	assume	adl:0
	sla.l	(iy+12h)	; 49 FD CB 12 26

	; SLA r
	sla	a		; CB 27
	sla	b		; CB 20
	sla	c		; CB 21
	sla	d		; CB 22
	sla	e		; CB 23
	sla	h		; CB 24
	sla	l		; CB 25

	; SLP
	slp			; ED 76

	; SRA (HL)
	sra	(hl)		; CB 2E
	assume	adl:1
	sra.s	(hl)		; 52 CB 2E
	assume	adl:0
	sra.l	(hl)		; 49 CB 2E

	; SRA (IX/Y+d)
	sra	(ix+12h)	; DD CB 12 2E
	assume	adl:1
	sra.s	(ix+12h)	; 52 DD CB 12 2E
	assume	adl:0
	sra.l	(ix+12h)	; 49 DD CB 12 2E
	sra	(iy+12h)	; FD CB 12 2E
	assume	adl:1
	sra.s	(iy+12h)	; 52 FD CB 12 2E
	assume	adl:0
	sra.l	(iy+12h)	; 49 FD CB 12 2E

	; SRA r
	sra	a		; CB 2F
	sra	b		; CB 28
	sra	c		; CB 29
	sra	d		; CB 2A
	sra	e		; CB 2B
	sra	h		; CB 2C
	sra	l		; CB 2D

	; SRL (HL)
	srl	(hl)		; CB 3E
	assume	adl:1
	srl.s	(hl)		; 52 CB 3E
	assume	adl:0
	srl.l	(hl)		; 49 CB 3E

	; SRL (IX/Y+d)
	srl	(ix+12h)	; DD CB 12 3E
	assume	adl:1
	srl.s	(ix+12h)	; 52 DD CB 12 3E
	assume	adl:0
	srl.l	(ix+12h)	; 49 DD CB 12 3E
	srl	(iy+12h)	; FD CB 12 3E
	assume	adl:1
	srl.s	(iy+12h)	; 52 FD CB 12 3E
	assume	adl:0
	srl.l	(iy+12h)	; 49 FD CB 12 3E

	; SRL r
	srl	a		; CB 3F
	srl	b		; CB 38
	srl	c		; CB 39
	srl	d		; CB 3A
	srl	e		; CB 3B
	srl	h		; CB 3C
	srl	l		; CB 3D

	; STMIX
	stmix			; ED 7D

	; SUB A, (HL)
	sub	a,(hl)		; 96
	assume	adl:1
	sub.s	a,(hl)		; 52 96
	assume	adl:0
	sub.l	a,(hl)		; 49 96

	; SUB A, ir
	sub	a,ixh		; DD 94
	sub	a,ixl		; DD 95
	sub	a,iyh		; FD 94
	sub	a,iyl		; FD 95

	; SUB A, (IX/Y+d)
	sub	a,(ix+12h)	; DD 96 12
	assume	adl:1
	sub.s	a,(ix+12h)	; 52 DD 96 12
	assume	adl:0
	sub.l	a,(ix+12h)	; 49 DD 96 12
	sub	a,(iy+12h)	; FD 96 12
	assume	adl:1
	sub.s	a,(iy+12h)	; 52 FD 96 12
	assume	adl:0
	sub.l	a,(iy+12h)	; 49 FD 96 12

	; SUB A, n
	sub	a,12h		; D6 12

	; SUB A, r
	sub	a,a		; 97
	sub	a,b		; 90
	sub	a,c		; 91
	sub	a,d		; 92
	sub	a,e		; 93
	sub	a,h		; 94
	sub	a,l		; 95

	; TST A, (HL)
	TST	a,(hl)		; ED 34
	assume	adl:1
	TST.s	a,(hl)		; 52 ED 34
	assume	adl:0
	TST.l	a,(hl)		; 49 ED 34

	; TST A, n
	TST	a,12h		; ED 64 12

	; TST A, r
	TST	a,a		; ED 3C
	TST	a,b		; ED 04
	TST	a,c		; ED 0C
	TST	a,d		; ED 14
	TST	a,e		; ED 1C
	TST	a,h		; ED 24
	TST	a,l		; ED 2C

	; TSTIO n
	tstio	12h		; ED 74 12

	; XOR A, (HL)
	xor	a,(hl)		; AE
	assume	adl:1
	xor.s	a,(hl)		; 52 AE
	assume	adl:0
	xor.l	a,(hl)		; 49 AE

	; XOR A, ir
	xor	a,ixh		; DD AC
	xor	a,ixl		; DD AD
	xor	a,iyh		; FD AC
	xor	a,iyl		; FD AD

	; XOR A, (IX/Y+d)
	xor	a,(ix+12h)	; DD AE 12
	assume	adl:1
	xor.s	a,(ix+12h)	; 52 DD AE 12
	assume	adl:0
	xor.l	a,(ix+12h)	; 49 DD AE 12
	xor	a,(iy+12h)	; FD AE 12
	assume	adl:1
	xor.s	a,(iy+12h)	; 52 FD AE 12
	assume	adl:0
	xor.l	a,(iy+12h)	; 49 FD AE 12

	; XOR A, n
	xor	a,12h		; EE 12

	; XOR A, r
	xor	a,a		; AF
	xor	a,b		; A8
	xor	a,c		; A9
	xor	a,d		; AA
	xor	a,e		; AB
	xor	a,h		; AC
	xor	a,l		; AD
