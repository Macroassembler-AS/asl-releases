	cpu	uPD546
	page	0

	; CZP target addresses:

vect0	label	0
vect1	label	4
vect2	label	8
vect3	label	12
vect4	label	16

	cla			; 90
	clc			; 0B
	cma			; 10
	cia			; 11
	inc			; 0D
	dec			; 0F
	stc			; 1B
	xc			; 1A
	rar			; 30
	inm			; 1D
	dem			; 1F
	ad			; 08
	ads			; 09
	adc			; 19
	daa			; 06
	das			; 0A
	exl			; 18
	li	0		; 90
	li	1		; 91
	li	2		; 92
	li	3		; 93
	li	7		; 93
	li	8		; 98
	li	15		; 9F
	li	-8		; 98
	li	-1		; 9F
	expect	1320
	li	16
	endexpect
	s			; 02
	l			; 38
	lm	0		; 38
	lm	1		; 39
	lm	2		; 3A
	lm	3		; 3B
	x			; 28
	xm	0		; 28
	xm	1		; 29
	xm	2		; 2A
	xm	3		; 2B
	xd			; 2C
	xmd	0		; 2C
	xmd	1		; 2D
	xmd	2		; 2E
	xmd	3		; 2F
	xi			; 3C
	xmi	0		; 3C
	xmi	1		; 3D
	xmi	2		; 3E
	xmi	3		; 3F
	ldi	0		; 15 00
	ldi 	95		; 15 5F
	expect	260
	ldi	96		; 15 60
	endexpect
	expect	1320
	ldi	128
	endexpect
	ldz 	0		; 80
	ldz	15		; 8F
	ded			; 13
	ind			; 33
	tal			; 07
	tla			; 12
	xhx			; 4F
	xly			; 4E
	thx			; 47
	tly			; 46
	xaz			; 4A
	xaw			; 4B
	taz			; 42
	taw			; 43
	xhr			; 4D
	xls			; 4C
	smb 	0		; 78
	smb	1		; 79
	smb	2		; 7A
	smb	3		; 7B
	rmb	0		; 68
	rmb	1		; 69
	rmb	2		; 6A
	rmb	3		; 6B
	tmb 	0		; 58
	tmb	1		; 59
	tmb	2		; 5A
	tmb	3		; 5B
	tab 	0		; 24
	tab	1		; 25
	tab	2		; 26
	tab	3		; 27
	cmb	0		; 34
	cmb	1		; 35
	cmb	2		; 36
	cmb	3		; 37
	sfb	0		; 7C
	sfb	1		; 7D
	sfb	2		; 7E
	sfb	3		; 7F
	rfb	0		; 6C
	rfb	1		; 6D
	rfb	2		; 6E
	rfb	3		; 6F
	fbt	0		; 5C
	fbt	1		; 5D
	fbt	2		; 5E
	fbt	3		; 5F
	fbf	0		; 20
	fbf	1		; 21
	fbf	2		; 22
	fbf	3		; 23
	cm			; 0C
	ci 	0		; 17 C0
	ci	7		; 17 C7
	ci	8		; 17 C8
	ci	-8		; 17 C8
	ci	15		; 17 CF
	ci	-1		; 17 CF
	cli	0		; 16 E0
	cli	7		; 16 E7
	cli	8		; 16 E8
	cli	15		; 16 EF
	tc			; 04
	ttm			; 05
	tit			; 03
	jcp	($&7c0h)|20h	; E0
	expect	1331
	jcp	$+64
	endexpect
	jmp	123h		; A1 23
	jmp	7ffh		; A7 FF
	expect	1320
	jmp	800h
	endexpect
	jpa			; 41
	ei			; 31
	di			; 01
	czp	0		; B0
	czp	1		; B1
	czp	2		; B2
	czp	3		; B3
	expect	480
	czp	4		; B4
	endexpect
	czp	5		; B5
	czp	6		; B6
	czp	7		; B7
	expect	480
	czp	8		; B8
	endexpect
	czp	9		; B9
	czp	10		; BA
	czp	11		; BB
	expect	480
	czp	12		; BC
	endexpect
	czp	13		; BD
	czp	14		; BE
	czp	15		; BF
	czp	vect0		; B0
	czp	vect1		; B1
	czp	vect2		; B2
	czp	vect3		; B3
	czp	30h		; BC
	czp	34h		; BD
	czp	38h		; BE
	czp	3ch		; BF
	expect	1905
	czp	40h
	endexpect
	expect	1905
	czp	3eh
	endexpect
	cal	123h		; A9 23
	cal	7ffh		; AF FF
	expect	1320
	cal	800h
	endexpect
	rt			; 48
	rts			; 49
	seb	0		; 74
	seb	1		; 75
	seb	2		; 76
	seb	3		; 77
	reb	0		; 64
	reb	1		; 65
	reb	2		; 66
	reb	3		; 67
	spb	0		; 70
	spb	1		; 71
	spb	2		; 72
	spb	3		; 73
	rpb	0		; 60
	rpb	1		; 61
	rpb	2		; 62
	rpb	3		; 63
	tpa	0		; 54
	tpa	1		; 55
	tpa	2		; 56
	tpa	3		; 57
	tpb	0		; 50
	tpb	1		; 51
	tpb	2		; 52
	tpb	3		; 53
	oe			; 44
	op			; 0E
	ocd 	0		; 1E 00
	ocd	0ffh		; 1E FF
	ia			; 40
	ip			; 32
	stm	0		; 14 80
	stm	31		; 14 9F
	stm	32		; 14 A0
	stm	63		; 14 BF
	stm	-1		; 14 BF
	stm	-32		; 14 A0
	expect	1320
	stm	64
	endexpect
	expect	1315
	stm	-33
	endexpect
	nop			; 00

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
