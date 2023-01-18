	cpu	pps-4
	page	0

	ad		; 0b
	adc		; 0a
	adsk		; 09
	adcsk		; 08
	expect	1985
	adi	0	; !
	endexpect
	adi	1	; 6e
	expect	1985
	adi	10	; would be opcode of DC
	endexpect
	adi	15	; 60
	expect	1320
	adi	16	; !
	endexpect
	dc		; 65

	and		; 0d
	or		; 0f
	eor		; 0c
	comp		; 0e

	sc		; 20
	rc		; 24
	sf1		; 22
	rf1		; 26
	sf2		; 21
	rf2		; 25
	ld	3	; 33
	ex	2	; 3a
	exd	4	; 2c
	ldi	0	; 70
	ldi	15	; 7f
	expect	1320
	ldi	16
	endexpect
	lax		; 12
	lxa		; 1b
	labl		; 11
	lbmx		; 10
	lbua		; 04
	xabl		; 19
	xbmx		; 18
	xax		; 1a
	xs		; 06
	cys		; 6f
	lb	0	; 70
	lb	15	; 7f
	expect	1320
	lb	16
	endexpect
	lbl	0	; 00 00
	lbl	-1	; 00 ff
	lbl	-128	; 00 80
	lbl	255	; 00 ff
	expect	1315
	lbl	-129
	endexpect
	expect	1320
	lbl	256
	endexpect
	incb		; 17
	decb		; 1f

	t	$	; 8x..bx
	expect	1910
	t	$+64
	endexpect
	expect	1315
	tm	0cfh
	endexpect
	tm	0d0h	; d0
	tm	0ffh	; ff
	expect	1320
	tm	100h
	endexpect
	tl	567h	; 55 67
	expect	1315
	tml	0ffh
	endexpect
	tml	100h	; 01 00
	tml	3ffh	; 03 ff
	expect	1320
	tml	400h
	endexpect
	skc		; 15
	skz		; 1e
	skbi	0	; 40
	skbi	15	; 4f
	expect	1320
	skbi	16
	endexpect
	skf1		; 16
	skf2		; 14
	rtn		; 05
	rtnsk		; 07

	iol	0	; 1c 00
	iol	0ffh	; 1c ff
	expect	1320
	iol	256
	endexpect
	dia		; 27
	dib		; 23
	doa		; 1d
	sag		; 13
