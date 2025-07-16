	cpu	z280
	supmode	on
	page	0

	adc	a,a		; 8F
	adc	a
	adc	a,b		; 88
	adc	b
	adc	a,c		; 89
	adc	c
	adc	a,d		; 8A
	adc	d
	adc	a,e		; 8B
	adc	e
	adc	a,h		; 8C
	adc	h
	adc	a,l		; 8D
	adc	l
	adc	a,ixh		; DD 8C
	adc	ixh
	adc	a,ixl		; DD 8D
	adc	ixl
	adc	a,iyh		; FD 8C
	adc	iyh
	adc	a,iyl		; FD 8D
	adc	iyl
	adc	a,12h		; CE 12
	adc	12h
	adc	a,(hl)		; 8E
	adc	(hl)
	adc	a,(1234h)	; DD 8F 34 12
	adc	(1234h)
	adc	a,(ix+1234h)	; FD 89 34 12
	adc	(ix+1234h)
	adc	a,(iy+1234h)	; FD 8A 34 12
	adc	(iy+1234h)
	adc	a,(hl+1234h)	; FD 8B 34 12
	adc	(hl+1234h)
	adc	a,(ix+12h)	; DD 8E 12
	adc	(ix+12h)
	adc	a,(iy-12h)	; FD 8E EE
	adc	(iy-12h)
	adc	a,<$+1238h>	; FD 88 34 12
	adc	<$+1238h>
	adc	a,(pc+1234h)	; FD 88 34 12
	adc	(pc+1234h)
	adc	a,(sp+1234h)	; DD 88 34 12
	adc	(sp+1234h)
	adc	a,(hl+ix)	; DD 89
	adc	(hl+ix)
	adc	a,(hl+iy)	; DD 8A
	adc	(hl+iy)
	adc	a,(ix+iy)	; DD 8B
	adc	(ix+iy)

	adc	hl,bc		; ED 4A
	adc	hl,de		; ED 5A
	adc	hl,hl		; ED 6A
	adc	hl,sp		; ED 7A
	expect	1350
	adc	hl,ix
	endexpect
	expect	1350
	adc	hl,iy
	endexpect
	adc	ix,bc		; DD ED 4A
	adc	ix,de		; DD ED 5A
	expect	1350
	adc	ix,hl
	endexpect
	adc	ix,ix		; DD ED 6A
	expect	1350
	adc	ix,iy
	endexpect
	adc	ix,sp		; DD ED 7A
	adc	iy,bc		; FD ED 4A
	adc	iy,de		; FD ED 5A
	expect	1350
	adc	iy,hl
	endexpect
	expect	1350
	adc	iy,ix
	endexpect
	adc	iy,iy		; FD ED 6A
	adc	iy,sp		; FD ED 7A

	add	a,a		; 87
	add	a
	add	a,b		; 80
	add	b
	add	a,c		; 81
	add	c
	add	a,d		; 82
	add	d
	add	a,e		; 83
	add	e
	add	a,h		; 84
	add	h
	add	a,l		; 85
	add	l
	add	a,ixh		; DD 84
	add	ixh
	add	a,ixl		; DD 85
	add	ixl
	add	a,iyh		; FD 84
	add	iyh
	add	a,iyl		; FD 85
	add	iyl
	add	a,12h		; C6 12
	add	12h
	add	a,(hl)		; 86
	add	(hl)
	add	a,(1234h)	; DD 87 34 12
	add	(1234h)
	add	a,(ix+1234h)	; FD 81 34 12
	add	(ix+1234h)
	add	a,(iy+1234h)	; FD 82 34 12
	add	(iy+1234h)
	add	a,(hl+1234h)	; FD 83 34 12
	add	(hl+1234h)
	add	a,(ix+12h)	; DD 86 12
	add	(ix+12h)
	add	a,(iy-12h)	; FD 86 EE
	add	(iy-12h)
	add	a,<$+1238h>	; FD 80 34 12
	add	<$+1238h>
	add	a,(pc+1234h)	; FD 80 34 12
	add	(pc+1234h)
	add	a,(sp+1234h)	; DD 80 34 12
	add	(sp+1234h)
	add	a,(hl+ix)	; DD 81
	add	(hl+ix)
	add	a,(hl+iy)	; DD 82
	add	(hl+iy)
	add	a,(ix+iy)	; DD 83
	add	(ix+iy)

	add	hl,bc		; 09
	add	hl,de		; 19
	add	hl,hl		; 29
	add	hl,sp		; 39
	expect	1350
	add	hl,ix
	endexpect
	expect	1350
	add	hl,iy
	endexpect
	add	ix,bc		; DD 09
	add	ix,de		; DD 19
	expect	1350
	add	ix,hl
	endexpect
	add	ix,ix		; DD 29
	expect	1350
	add	ix,iy
	endexpect
	add	ix,sp		; DD 39
	add	iy,bc		; FD 09
	add	iy,de		; FD 19
	expect	1350
	add	iy,hl
	endexpect
	expect	1350
	add	iy,ix
	endexpect
	add	iy,iy		; FD 29
	add	iy,sp		; FD 39

	addw	hl,bc		; ED C6
	addw	hl,de		; ED D6
	addw	hl,hl		; ED E6
	addw	hl,sp		; ED F6
	addw	hl,ix		; DD ED E6
	addw	hl,iy		; FD ED E6
	addw	hl,1234h	; FD ED F6 34 12
	addw	hl,(1234h)	; DD ED D6 34 12
	; Caution: no SX mode, uses X!
	addw	hl,(ix+12h)	; FD ED C6 12 00
	addw	hl,(iy-12h)	; FD ED D6 EE FF
	addw	hl,(ix+1234h)	; FD ED C6 34 12
	addw	hl,(iy+1234h)	; FD ED D6 34 12
	addw	hl,<$+1239h>	; DD ED F6 34 12
	addw	hl,(pc+1234h)	; DD ED F6 34 12
	addw	hl,(hl)		; DD ED C6

	and	a,a		; A7
	and	a
	and	a,b		; A0
	and	b
	and	a,c		; A1
	and	c
	and	a,d		; A2
	and	d
	and	a,e		; A3
	and	e
	and	a,h		; A4
	and	h
	and	a,l		; A5
	and	l
	and	a,ixh		; DD A4
	and	ixh
	and	a,ixl		; DD A5
	and	ixl
	and	a,iyh		; FD A4
	and	iyh
	and	a,iyl		; FD A5
	and	iyl
	and	a,12h		; E6 12
	and	12h
	and	a,(hl)		; A6
	and	(hl)
	and	a,(1234h)	; DD A7 34 12
	and	(1234h)
	and	a,(ix+1234h)	; FD A1 34 12
	and	(ix+1234h)
	and	a,(iy+1234h)	; FD A2 34 12
	and	(iy+1234h)
	and	a,(hl+1234h)	; FD A3 34 12
	and	(hl+1234h)
	and	a,(ix+12h)	; DD A6 12
	and	(ix+12h)
	and	a,(iy-12h)	; FD A6 EE
	and	(iy-12h)
	and	a,<$+1238h>	; FD A0 34 12
	and	<$+1238h>
	and	a,(pc+1234h)	; FD A0 34 12
	and	(pc+1234h)
	and	a,(sp+1234h)	; DD A0 34 12
	and	(sp+1234h)
	and	a,(hl+ix)	; DD A1
	and	(hl+ix)
	and	a,(hl+iy)	; DD A2
	and	(hl+iy)
	and	a,(ix+iy)	; DD A3
	and	(ix+iy)

	bit	0,a		; CB 47
	bit	1,a		; CB 4F
	bit	2,a		; CB 57
	bit	3,a		; CB 5F
	bit	4,a		; CB 67
	bit	5,a		; CB 6F
	bit	6,a		; CB 77
	bit	7,a		; CB 7F
	bit	0,b		; CB 40
	bit	1,b		; CB 48
	bit	2,b		; CB 50
	bit	3,b		; CB 58
	bit	4,b		; CB 60
	bit	5,b		; CB 68
	bit	6,b		; CB 70
	bit	7,b		; CB 78
	bit	0,c		; CB 41
	bit	1,c		; CB 49
	bit	2,c		; CB 51
	bit	3,c		; CB 59
	bit	4,c		; CB 61
	bit	5,c		; CB 69
	bit	6,c		; CB 71
	bit	7,c		; CB 79
	bit	0,d		; CB 42
	bit	1,d		; CB 4A
	bit	2,d		; CB 52
	bit	3,d		; CB 5A
	bit	4,d		; CB 62
	bit	5,d		; CB 6A
	bit	6,d		; CB 72
	bit	7,d		; CB 7A
	bit	0,e		; CB 43
	bit	1,e		; CB 4B
	bit	2,e		; CB 53
	bit	3,e		; CB 5B
	bit	4,e		; CB 63
	bit	5,e		; CB 6B
	bit	6,e		; CB 73
	bit	7,e		; CB 7B
	bit	0,h		; CB 44
	bit	1,h		; CB 4C
	bit	2,h		; CB 54
	bit	3,h		; CB 5C
	bit	4,h		; CB 64
	bit	5,h		; CB 6C
	bit	6,h		; CB 74
	bit	7,h		; CB 7C
	bit	0,l		; CB 45
	bit	1,l		; CB 4D
	bit	2,l		; CB 55
	bit	3,l		; CB 5D
	bit	4,l		; CB 65
	bit	5,l		; CB 6D
	bit	6,l		; CB 75
	bit	7,l		; CB 7D
	bit	0,(hl)		; CB 46
	bit	1,(hl)		; CB 4E
	bit	2,(hl)		; CB 56
	bit	3,(hl)		; CB 5E
	bit	4,(hl)		; CB 66
	bit	5,(hl)		; CB 6E
	bit	6,(hl)		; CB 76
	bit	7,(hl)		; CB 7E
	bit	0,(ix+12h)	; DD CB 12 46
	bit	1,(ix+12h)	; DD CB 12 4E
	bit	2,(ix+12h)	; DD CB 12 56
	bit	3,(ix+12h)	; DD CB 12 5E
	bit	4,(ix+12h)	; DD CB 12 66
	bit	5,(ix+12h)	; DD CB 12 6E
	bit	6,(ix+12h)	; DD CB 12 76
	bit	7,(ix+12h)	; DD CB 12 7E
	bit	0,(iy+12h)	; FD CB 12 46
	bit	1,(iy+12h)	; FD CB 12 4E
	bit	2,(iy+12h)	; FD CB 12 56
	bit	3,(iy+12h)	; FD CB 12 5E
	bit	4,(iy+12h)	; FD CB 12 66
	bit	5,(iy+12h)	; FD CB 12 6E
	bit	6,(iy+12h)	; FD CB 12 76
	bit	7,(iy+12h)	; FD CB 12 7E

	call	nz,(hl)		; DD C4
	call	z,(hl)		; DD CC
	call	nc,(hl)		; DD D4
	call	c,(hl)		; DD DC
	call	po,(hl)		; DD E4
	call	nv,(hl)		; DD E4
	call	pe,(hl)		; DD EC
	call	v,(hl)		; DD EC
	call	p,(hl)		; DD F4
	call	ns,(hl)		; DD F4
	call	m,(hl)		; DD FC
	call	s,(hl)		; DD FC
	call	(hl)		; DD CD
	call	nz,1234h	; C4 34 12
	call	z,1234h		; CC 34 12
	call	nc,1234h	; D4 34 12
	call	c,1234h		; DC 34 12
	call	po,1234h	; E4 34 12
	call	nv,1234h	; E4 34 12
	call	pe,1234h	; EC 34 12
	call	v,1234h		; EC 34 12
	call	p,1234h		; F4 34 12
	call	ns,1234h	; F4 34 12
	call	m,1234h		; FC 34 12
	call	s,1234h		; FC 34 12
	call	1234h		; CD 34 12
	call	nz,<$+1238h>	; FD C4 34 12
	call	z,<$+1238h>	; FD CC 34 12
	call	nc,<$+1238h>	; FD D4 34 12
	call	c,<$+1238h>	; FD DC 34 12
	call	po,<$+1238h>	; FD E4 34 12
	call	nv,<$+1238h>	; FD E4 34 12
	call	pe,<$+1238h>	; FD EC 34 12
	call	v,<$+1238h>	; FD EC 34 12
	call	p,<$+1238h>	; FD F4 34 12
	call	ns,<$+1238h>	; FD F4 34 12
	call	m,<$+1238h>	; FD FC 34 12
	call	s,<$+1238h>	; FD FC 34 12
	call	<$+1238h>	; FD CD 34 12

	ccf			; 3F

	cp	a,a		; BF
	cp	a
	cp	a,b		; B8
	cp	b
	cp	a,c		; B9
	cp	c
	cp	a,d		; BA
	cp	d
	cp	a,e		; BB
	cp	e
	cp	a,h		; BC
	cp	h
	cp	a,l		; BD
	cp	l
	cp	a,ixh		; DD BC
	cp	ixh
	cp	a,ixl		; DD BD
	cp	ixl
	cp	a,iyh		; FD BC
	cp	iyh
	cp	a,iyl		; FD BD
	cp	iyl
	cp	a,12h		; FE 12
	cp	12h
	cp	a,(hl)		; BE
	cp	(hl)
	cp	a,(1234h)	; DD BF 34 12
	cp	(1234h)
	cp	a,(ix+1234h)	; FD B9 34 12
	cp	(ix+1234h)
	cp	a,(iy+1234h)	; FD BA 34 12
	cp	(iy+1234h)
	cp	a,(hl+1234h)	; FD BB 34 12
	cp	(hl+1234h)
	cp	a,(ix+12h)	; DD BE 12
	cp	(ix+12h)
	cp	a,(iy-12h)	; FD BE EE
	cp	(iy-12h)
	cp	a,<$+1238h>	; FD B8 34 12
	cp	<$+1238h>
	cp	a,(pc+1234h)	; FD B8 34 12
	cp	(pc+1234h)
	cp	a,(sp+1234h)	; DD B8 34 12
	cp	(sp+1234h)
	cp	a,(hl+ix)	; DD B9
	cp	(hl+ix)
	cp	a,(hl+iy)	; DD BA
	cp	(hl+iy)
	cp	a,(ix+iy)	; DD BB
	cp	(ix+iy)

	cpd			; ED A9

	cpdr			; ED B9

	cpi			; ED A1

	cpir			; ED B1

	cpl	a		; 2F
	cpl

	cpw	hl,bc		; ED C7
	cpw	hl,de		; ED D7
	cpw	hl,hl		; ED E7
	cpw	hl,sp		; ED F7
	cpw	hl,ix		; DD ED E7
	cpw	hl,iy		; FD ED E7
	cpw	hl,1234h	; FD ED F7 34 12
	cpw	hl,(1234h)	; DD ED D7 34 12
	; Caution: no SX mode, uses X!
	cpw	hl,(ix+12h)	; FD ED C7 12 00
	cpw	hl,(iy-12h)	; FD ED D7 EE FF
	cpw	hl,(ix+1234h)	; FD ED C7 34 12
	cpw	hl,(iy+1234h)	; FD ED D7 34 12
	cpw	hl,<$+1239h>	; DD ED F7 34 12
	cpw	hl,(pc+1234h)	; DD ED F7 34 12
	cpw	hl,(hl)		; DD ED C7

	daa			; 27

	dec	a		; 3D
	dec	b		; 05
	dec	c		; 0D
	dec	d		; 15
	dec	e		; 1D
	dec	h		; 25
	dec	l		; 2D
	dec	ixh		; DD 25
	dec	ixl		; DD 2D
	dec	iyl		; FD 25
	dec	iyh		; FD 2D
	dec	(hl)		; 35
	dec	(1234h)		; DD 3D 34 12
	dec	(ix+1234h)	; FD 0D 34 12
	dec	(iy+1234h)	; FD 15 34 12
	dec	(hl+1234h)	; FD 1D 34 12
	dec	(ix+12h)	; DD 35 12
	dec	(iy-12h)	; FD 35 EE
	dec	<$+1238h>	; FD 05 34 12
	dec	(pc+1234h)	; FD 05 34 12
	dec	(sp+1234h)	; DD 05 34 12
	dec	(hl+ix)		; DD 0D
	dec	(iy+hl)		; DD 15
	dec	(iy+ix)		; DD 1D

	dec	bc		; 0B
	decw	bc		; 0B
	dec	de		; 1B
	decw	de		; 1B
	dec	hl		; 2B
	decw	hl		; 2B
	dec	sp		; 3B
	decw	sp		; 3B
	dec	ix		; DD 2B
	decw	ix		; DD 2B
	dec	iy		; FD 2B
	decw	iy		; FD 2B
	decw	(hl)		; DD 0B
	decw	(1234h)		; DD 1B 34 12
	; Caution: no SX mode, uses X!
	decw	(ix+12h)	; FD 0B 12 00
	decw	(iy-12h)	; FD 1B EE FF
	decw	(ix+1234h)	; FD 0B 34 12
	decw	(iy+1234h)	; FD 1B 34 12
	decw	<$+1238h>	; DD 3B 34 12
	decw	(pc+1234h)	; DD 3B 34 12

	di			; F3
	di	23h		; ED 77 23

	div	hl,a		; ED FC
	div	a
	div	hl,b		; ED C4
	div	b
	div	hl,c		; ED CC
	div	c
	div	hl,d		; ED D4
	div	d
	div	hl,e		; ED DC
	div	e
	div	hl,h		; ED E4
	div	h
	div	hl,l		; ED EC
	div	l
	div	hl,ixh		; DD ED E4
	div	ixh
	div	hl,ixl		; DD ED EC
	div	ixl
	div	hl,iyh		; FD ED E4
	div	iyh
	div	hl,iyl		; FD ED EC
	div	iyl
	div	hl,12h		; FD ED FC 12
	div	12h
	div	hl,(1234h)	; DD ED FC 34 12
	div	(1234h)
	div	hl,(ix+1234h)	; FD ED CC 34 12
	div	(ix+1234h)
	div	hl,(iy+1234h)	; FD ED D4 34 12
	div	(iy+1234h)
	div	hl,(hl+1234h)	; FD ED DC 34 12
	div	(hl+1234h)
	div	hl,(ix+12h)	; DD ED F4 12
	div	(ix+12h)
	div	hl,(iy-12h)	; FD ED F4 EE
	div	(ix+12h)
	div	hl,<$+1239h>	; FD ED C4 34 12
	div	<$+1239h>
	div	hl,(pc+1234h)	; FD ED C4 34 12
	div	(pc+1234h)
	div	hl,(sp+1234h)	; DD ED C4 34 12
	div	(sp+1234h)
	div	hl,(ix+hl)	; DD ED CC
	div	(ix+hl)
	div	hl,(iy+hl)	; DD ED D4
	div	(iy+hl)
	div	hl,(ix+iy)	; DD ED DC
	div	(ix+iy)
	div	hl,(hl)		; ED F4 (not ED E4 as written in manual)
	div	(hl)

	divu	hl,a		; ED FD
	divu	a
	divu	hl,b		; ED C5
	divu	b
	divu	hl,c		; ED CD
	divu	c
	divu	hl,d		; ED D5
	divu	d
	divu	hl,e		; ED DD
	divu	e
	divu	hl,h		; ED E5
	divu	h
	divu	hl,l		; ED ED
	divu	l
	divu	hl,ixh		; DD ED E5
	divu	ixh
	divu	hl,ixl		; DD ED ED
	divu	ixl
	divu	hl,iyh		; FD ED E5
	divu	iyh
	divu	hl,iyl		; FD ED ED
	divu	iyl
	divu	hl,12h		; FD ED FD 12
	divu	12h
	divu	hl,(1234h)	; DD ED FD 34 12
	divu	(1234h)
	divu	hl,(ix+1234h)	; FD ED CD 34 12
	divu	(ix+1234h)
	divu	hl,(iy+1234h)	; FD ED DD 34 12
	divu	(iy+1234h)
	divu	hl,(hl+1234h)	; FD ED DD 34 12
	divu	(hl+1234h)
	divu	hl,(ix+12h)	; DD ED F5 12
	divu	(ix+12h)
	divu	hl,(iy-12h)	; FD ED F5 EE
	divu	(ix+12h)
	divu	hl,<$+1239h>	; FD ED C5 34 12
	divu	<$+1239h>
	divu	hl,(pc+1234h)	; FD ED C5 34 12
	divu	(pc+1234h)
	divu	hl,(sp+1234h)	; DD ED C5 34 12
	divu	(sp+1234h)
	divu	hl,(ix+hl)	; DD ED CD
	divu	(ix+hl)
	divu	hl,(iy+hl)	; DD ED D5
	divu	(iy+hl)
	divu	hl,(ix+iy)	; DD ED DD
	divu	(ix+iy)
	divu	hl,(hl)		; ED F5
	divu	(hl)

	divuw	dehl,bc		; ED CB
	divuw	bc
	divuw	dehl,de		; ED DB
	divuw	de
	divuw	dehl,hl		; ED EB
	divuw	hl
	divuw	dehl,sp		; ED FB
	divuw	sp
	divuw	dehl,ix		; DD ED EB
	divuw	ix
	divuw	dehl,iy		; FD ED EB
	divuw	iy
	divuw	dehl,1234h	; FD ED FB 34 12
	divuw	1234h
	divuw	dehl,(1234h)	; DD ED DB 34 12
	divuw	(1234h)
	divuw	dehl,(ix+12h)	; FD ED CB 12 00
	divuw	(ix+12h)
	divuw	dehl,(iy-12h)	; FD ED DB EE FF
	divuw	(iy-12h)
	divuw	dehl,(ix+1234h)	; FD ED CB 34 12
	divuw	(ix+1234h)
	divuw	dehl,(iy+1234h)	; FD ED DB 34 12
	divuw	(iy+1234h)
	divuw	dehl,<$+1239h>	; DD ED FB 34 12
	divuw	<$+1239h>
	divuw	dehl,(pc+1234h)	; DD ED FB 34 12
	divuw	(pc+1234h)
	divuw	dehl,(hl)	; DD ED CB
	divuw	(hl)

	divw	dehl,bc		; ED CA
	divw	bc
	divw	dehl,de		; ED DA
	divw	de
	divw	dehl,hl		; ED EA
	divw	hl
	divw	dehl,sp		; ED FA
	divw	sp
	divw	dehl,ix		; DD ED EA
	divw	ix
	divw	dehl,iy		; FD ED EA
	divw	iy
	divw	dehl,1234h	; FD ED FA 34 12
	divw	1234h
	divw	dehl,(1234h)	; DD ED DA 34 12
	divw	(1234h)
	divw	dehl,(ix+12h)	; FD ED CA 12 00
	divw	(ix+12h)
	divw	dehl,(iy-12h)	; FD ED DA EE FF
	divw	(iy-12h)
	divw	dehl,(ix+1234h)	; FD ED CA 34 12
	divw	(ix+1234h)
	divw	dehl,(iy+1234h)	; FD ED DA 34 12
	divw	(iy+1234h)
	divw	dehl,<$+1239h>	; DD ED FA 34 12
	divw	<$+1239h>
	divw	dehl,(pc+1234h)	; DD ED FA 34 12
	divw	(pc+1234h)
	divw	dehl,(hl)	; DD ED CA
	divw	(hl)

	djnz	$+129		; 10 7F
	expect	1370
	djnz	$+130
	endexpect
	djnz	$-126		; 10 80
	expect	1370
	djnz	$-127
	endexpect

	ei			; FD
	ei	49h		; ED 7F 49

	ex	af,af'		; 08
	ex	af',af
	ex	(sp),hl		; E3
	ex	hl,(sp)
	ex	(sp),ix		; DD E3
	ex	ix,(sp)
	ex	(sp),iy		; FD E3
	ex	iy,(sp)
	ex	h,l		; ED EF
	ex	l,h
	ex	de,hl		; EB
	ex	hl,de
	ex	ix,hl		; DD EB
	ex	hl,ix
	ex	iy,hl		; FD EB
	ex	hl,iy
	ex	a,a		; ED 3F
	ex	a,a
	ex	a,b		; ED 07
	ex	b,a
	ex	a,c		; ED 0F
	ex	c,a
	ex	a,d		; ED 17
	ex	d,a
	ex	a,e		; ED 1F
	ex	e,a
	ex	a,h		; ED 27
	ex	h,a
	ex	a,l		; ED 2F
	ex	l,a
	ex	a,ixh		; DD ED 27
	ex	ixh,a
	ex	a,ixl		; DD ED 2F
	ex	ixl,a
	ex	a,iyh		; FD ED 27
	ex	iyh,a
	ex	a,iyl		; FD ED 2F
	ex	iyl,a
	ex	a,(hl)		; ED 37
	ex	(hl),a
	ex	a,(1234h)	; DD ED 3F 34 12
	ex	(1234h),a
	ex	a,(ix+1234h)	; FD DD 0F 34 12
	ex	(ix+1234h),a
	ex	a,(iy+1234h)	; FD DD 17 34 12
	ex	(iy+1234h),a
	ex	a,(hl+1234h)	; FD DD 1F 34 12
	ex	(hl+1234h),a
	ex	a,(ix+12h)	; DD ED 37 12
	ex	(ix+12h),a
	ex	a,(iy-12h)	; FD ED 37 EE
	ex	(iy-12h),a
	ex	a,<$+1239h>	; FD ED 07 34 12
	ex	<$+1239h>,a
	ex	a,(pc+1234h)	; FD ED 07 34 12
	ex	(pc+1234h),a
	ex	a,(sp+1234h)	; DD ED 07 34 12
	ex	(sp+1234h),a
	ex	a,(hl+ix)	; DD ED 0F
	ex	(ix+hl),a
	ex	a,(hl+iy)	; DD ED 17
	ex	(hl+iy),a
	ex	a,(ix+iy)	; DD ED 1F
	ex	(ix+iy),a

	exts			; ED 64
	exts	a
	exts	hl		; ED 6C

	exx			; D9

	halt			; 76

	im	0		; ED 46
	im	1		; ED 56
	im	2		; ED 5E
	im	3		; ED 4E

	inc	a		; 3C
	inc	b		; 04
	inc	c		; 0C
	inc	d		; 14
	inc	e		; 1C
	inc	h		; 24
	inc	l		; 2C
	inc	ixh		; DD 24
	inc	ixl		; DD 2C
	inc	iyl		; FD 24
	inc	iyh		; FD 2C
	inc	(hl)		; 34
	inc	(1234h)		; DD 3C 34 12
	inc	(ix+1234h)	; FD 0C 34 12
	inc	(iy+1234h)	; FD 14 34 12
	inc	(hl+1234h)	; FD 1C 34 12
	inc	(ix+12h)	; DD 34 12
	inc	(iy-12h)	; FD 34 EE
	inc	<$+1238h>	; FD 04 34 12
	inc	(pc+1234h)	; FD 04 34 12
	inc	(sp+1234h)	; DD 04 34 12
	inc	(hl+ix)		; DD 0C
	inc	(iy+hl)		; DD 14
	inc	(iy+ix)		; DD 1C

	inc	bc		; 03
	incw	bc		; 03
	inc	de		; 13
	incw	de		; 13
	inc	hl		; 23
	incw	hl		; 23
	inc	sp		; 33
	incw	sp		; 33
	inc	ix		; DD 23
	incw	ix		; DD 23
	inc	iy		; FD 23
	incw	iy		; FD 23
	incw	(hl)		; DD 03
	incw	(1234h)		; DD 13 34 12
	; Caution: no SX mode, uses X!
	incw	(ix+12h)	; FD 03 12 00
	incw	(iy-12h)	; FD 13 EE FF
	incw	(ix+1234h)	; FD 03 34 12
	incw	(iy+1234h)	; FD 13 34 12
	incw	<$+1238h>	; DD 33 34 12
	incw	(pc+1234h)	; DD 33 34 12

	ind			; ED AA
	indw			; ED 8A

	indr			; ED BA
	indrw			; ED 9A

	ini			; ED A2
	iniw			; ED 82

	inir			; ED B2
	inirw			; ED 92

	inw	hl,(c)		; ED B7
	in	hl,(c)

	jaf	$-125		; DD 28 80
	expect	1370
	jaf	$-126
	endexpect
	jaf	$+130		; DD 28 7F
	expect	1370
	jaf	$+131
	endexpect

	jar	$-125		; DD 20 80
	expect	1370
	jar	$-126
	endexpect
	jar	$+130		; DD 20 7F
	expect	1370
	jar	$+131
	endexpect

	jp	nz,(hl)		; DD C2
	jp	z,(hl)		; DD CA
	jp	nc,(hl)		; DD D2
	jp	c,(hl)		; DD DA
	jp	po,(hl)		; DD E2
	jp	nv,(hl)		; DD E2
	jp	pe,(hl)		; DD EA
	jp	v,(hl)		; DD EA
	jp	p,(hl)		; DD F2
	jp	ns,(hl)		; DD F2
	jp	m,(hl)		; DD FA
	jp	s,(hl)		; DD FA
	jp	(hl)		; E9
	jp	(ix)		; DD E9
	jp	(iy)		; FD E9
	jp	nz,1234h	; C2 34 12
	jp	z,1234h		; CA 34 12
	jp	nc,1234h	; D2 34 12
	jp	c,1234h		; DA 34 12
	jp	po,1234h	; E2 34 12
	jp	nv,1234h	; E2 34 12
	jp	pe,1234h	; EA 34 12
	jp	v,1234h		; EA 34 12
	jp	p,1234h		; F2 34 12
	jp	ns,1234h	; F2 34 12
	jp	m,1234h		; FA 34 12
	jp	s,1234h		; FA 34 12
	jp	1234h		; C3 34 12
	jp	nz,<$+1238h>	; FD C2 34 12
	jp	z,<$+1238h>	; FD CA 34 12
	jp	nc,<$+1238h>	; FD D2 34 12
	jp	c,<$+1238h>	; FD DA 34 12
	jp	po,<$+1238h>	; FD E2 34 12
	jp	nv,<$+1238h>	; FD E2 34 12
	jp	pe,<$+1238h>	; FD EA 34 12
	jp	v,<$+1238h>	; FD EA 34 12
	jp	p,<$+1238h>	; FD F2 34 12
	jp	ns,<$+1238h>	; FD F2 34 12
	jp	m,<$+1238h>	; FD FA 34 12
	jp	s,<$+1238h>	; FD FA 34 12
	jp	<$+1238h>	; FD C3 34 12

	jr	nz,$-126	; 20 80
	expect	1370
	jr	nz,$-127
	endexpect
	jr	nz,$+129	; 20 7F
	expect	1370
	jr	nz,$+130
	endexpect
	jr	z,$-126		; 28 80
	expect	1370
	jr	z,$-127
	endexpect
	jr	z,$+129		; 28 7F
	expect	1370
	jr	z,$+130
	endexpect
	jr	nc,$-126	; 30 80
	expect	1370
	jr	nc,$-127
	endexpect
	jr	nc,$+129	; 30 7F
	expect	1370
	jr	nc,$+130
	endexpect
	jr	c,$-126		; 38 80
	expect	1370
	jr	c,$-127
	endexpect
	jr	c,$+129		; 38 7F
	expect	1370
	jr	c,$+130
	endexpect
	jr	$-126		; 18 80
	expect	1370
	jr	$-127
	endexpect
	jr	$+129		; 18 7F
	expect	1370
	jr	$+130
	endexpect

	j	nz,(hl)		; DD C2
	j	z,(hl)		; DD CA
	j	nc,(hl)		; DD D2
	j	c,(hl)		; DD DA
	j	po,(hl)		; DD E2
	j	nv,(hl)		; DD E2
	j	pe,(hl)		; DD EA
	j	v,(hl)		; DD EA
	j	p,(hl)		; DD F2
	j	ns,(hl)		; DD F2
	j	m,(hl)		; DD FA
	j	s,(hl)		; DD FA
	j	(hl)		; E9
	j	(ix)		; DD E9
	j	(iy)		; FD E9
	j	nz,1234h	; C2 34 12
	j	z,1234h		; CA 34 12
	j	nc,1234h	; D2 34 12
	j	c,1234h		; DA 34 12
	j	po,1234h	; E2 34 12
	j	nv,1234h	; E2 34 12
	j	pe,1234h	; EA 34 12
	j	v,1234h		; EA 34 12
	j	p,1234h		; F2 34 12
	j	ns,1234h	; F2 34 12
	j	m,1234h		; FA 34 12
	j	s,1234h		; FA 34 12
	j	1234h		; C3 34 12
	j	nz,<$+1238h>	; FD C2 34 12
	j	z,<$+1238h>	; FD CA 34 12
	j	nc,<$+1238h>	; FD D2 34 12
	j	c,<$+1238h>	; FD DA 34 12
	j	po,<$+1238h>	; FD E2 34 12
	j	nv,<$+1238h>	; FD E2 34 12
	j	pe,<$+1238h>	; FD EA 34 12
	j	v,<$+1238h>	; FD EA 34 12
	j	p,<$+1238h>	; FD F2 34 12
	j	ns,<$+1238h>	; FD F2 34 12
	j	m,<$+1238h>	; FD FA 34 12
	j	s,<$+1238h>	; FD FA 34 12
	j	<$+1238h>	; FD C3 34 12
	j	$		; 18 FE
	j	nc,$		; 30 FE
	j	c,$		; 38 FE
	j	nz,$		; 20 FE
	j	z,$		; 28 FE
	j	po,$		; E2 xx xx
	j	c,$+200h	; DA xx xx

	ld	a,a		; 7F
	ld	a,b		; 78
	ld	a,c		; 79
	ld	a,d		; 7A
	ld	a,e		; BB
	ld	a,h		; 7C
	ld	a,l		; 7D
	ld	a,ixh		; DD 7C
	ld	a,ixl		; DD 7D
	ld	a,iyh		; FD 7C
	ld	a,iyl		; FD 7D
	ld	a,12h		; 3E 12
	ld	a,(hl)		; 7E
	ld	a,(bc)		; 0A
	ld	a,(de)		; 1A
	ld	a,(1234h)	; 3A 34 12
	ld	a,(ix+1234h)	; FD 79 34 12
	ld	a,(iy+1234h)	; FD 7A 34 12
	ld	a,(hl+1234h)	; FD 7B 34 12
	ld	a,(ix+12h)	; DD 7E 12
	ld	a,(iy-12h)	; FD 7E EE
	ld	a,<$+1238h>	; FD 78 34 12
	ld	a,(pc+1234h)	; FD 78 34 12
	ld	a,(sp+1234h)	; DD 78 34 12
	ld	a,(hl+ix)	; DD 79
	ld	a,(hl+iy)	; DD 7A
	ld	a,(ix+iy)	; DD 7B

	ld	a,a		; 7F
	ld	b,a		; 47
	ld	c,a		; 4F
	ld	d,a		; 57
	ld	e,a		; 5F
	ld	h,a		; 67
	ld	l,a		; 6F
	ld	ixh,a		; DD 67
	ld	ixl,a		; DD 6F
	ld	iyh,a		; FD 67
	ld	iyl,a		; FD 6F
	ld	(hl),a		; 77
	ld	(bc),a		; 02
	ld	(de),a		; 12
	ld	(1234h),a	; 32 34 12
	ld	(ix+1234h),a	; ED 2B 34 12
	ld	(iy+1234h),a	; ED 33 34 12
	ld	(hl+1234h),a	; ED 3B 34 12
	ld	(ix+12h),a	; DD 77 12
	ld	(iy-12h),a	; FD 77 EE
	ld	<$+1238h>,a	; ED 23 34 12
	ld	(pc+1234h),a	; ED 23 34 12
	ld	(sp+1234h),a	; ED 03 34 12
	ld	(hl+ix),a	; ED 0B
	ld	(hl+iy),a	; ED 13
	ld	(ix+iy),a	; ED 1B

	ld	a,i		; ED 57
	ld	a,r		; ED 5F

	ld	a,12h		; 3E 12
	ld	b,12h		; 06 12
	ld	c,12h		; 0E 12
	ld	d,12h		; 16 12
	ld	e,12h		; 1E 12
	ld	h,12h		; 26 12
	ld	l,12h		; 2E 12
	ld	ixh,12h		; DD 26 12
	ld	ixl,12h		; DD 2E	12
	ld	iyh,12h		; FD 26 12
	ld	iyl,12h		; FD 2E	12
	ld	(hl),12h	; 36 12
	ld	(1234h),56h	; DD 3E 34 12 56
	ld	(ix+1234h),56h	; FD 0E 34 12 56
	ld	(iy+1234h),56h	; FD 16 34 12 56
	ld	(hl+1234h),56h	; FD 1E 34 12 56
	ld	(ix+12h),56h	; DD 36 12 56
	ld	(iy-12h),56h	; FD 36 EE 56
	ld	<$+1239h>,56h	; FD 06 34 12 56
	ld	(pc+1234h),56h	; FD 06 34 12 56
	ld	(sp+1234h),56h	; DD 06 34 12 56
	ld	(hl+ix),56h	; DD 0E 56
	ld	(hl+iy),56h	; DD 16 56
	ld	(ix+iy),56h	; DD 2E 56

	ld	a,a		; 7F
	ld	a,b		; 78
	ld	a,c		; 79
	ld	a,d		; 7A
	ld	a,e		; 7B
	ld	a,h		; 7C
	ld	a,l		; 7D
	ld	b,a		; 47
	ld	b,b		; 40
	ld	b,c		; 41
	ld	b,d		; 42
	ld	b,e		; 43
	ld	b,h		; 44
	ld	b,l		; 45
	ld	c,a		; 4F
	ld	c,b		; 48
	ld	c,c		; 49
	ld	c,d		; 4A
	ld	c,e		; 4B
	ld	c,h		; 4C
	ld	c,l		; 4D
	ld	d,a		; 57
	ld	d,b		; 50
	ld	d,c		; 51
	ld	d,d		; 52
	ld	d,e		; 53
	ld	d,h		; 54
	ld	d,l		; 55
	ld	e,a		; 5F
	ld	e,b		; 58
	ld	e,c		; 59
	ld	e,d		; 5A
	ld	e,e		; 5B
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
	ld	a,ixh		; DD 7C
	ld	b,ixh		; DD 44
	ld	c,ixh		; DD 4C
	ld	d,ixh		; DD 54
	ld	e,ixh		; DD 5C
	expect	1350
	ld	h,ixh
	endexpect
	expect	1350
	ld	l,ixh
	endexpect
	ld	a,ixl		; DD 7D
	ld	b,ixl		; DD 45
	ld	c,ixl		; DD 4D
	ld	d,ixl		; DD 55
	ld	e,ixl		; DD 5D
	expect	1350
	ld	h,ixl
	endexpect
	expect	1350
	ld	l,ixl
	endexpect
	ld	a,iyh		; FD 7C
	ld	b,iyh		; FD 44
	ld	c,iyh		; FD 4C
	ld	d,iyh		; FD 54
	ld	e,iyh		; FD 5C
	expect	1350
	ld	h,iyh
	endexpect
	expect	1350
	ld	l,iyh
	endexpect
	ld	a,iyl		; FD 7D
	ld	b,iyl		; FD 45
	ld	c,iyl		; FD 4D
	ld	d,iyl		; FD 55
	ld	e,iyl		; FD 5D
	expect	1350
	ld	h,iyl
	endexpect
	expect	1350
	ld	l,iyl
	endexpect
	ld	ixh,ixh		; DD 64
	ld	ixh,ixl		; DD 65
	expect	1350
	ld	ixh,iyh
	endexpect
	expect	1350
	ld	ixh,iyl
	endexpect
	ld	ixl,ixh		; DD 6C
	ld	ixl,ixl		; DD 6D
	expect	1350
	ld	ixl,iyh
	endexpect
	expect	1350
	ld	ixl,iyl
	endexpect
	expect	1350
	ld	iyh,ixh
	endexpect
	expect	1350
	ld	iyh,ixl
	endexpect
	ld	iyh,iyh		; FD 64
	ld	iyh,iyl		; FD 65
	expect	1350
	ld	iyl,ixh
	endexpect
	expect	1350
	ld	iyl,ixl
	endexpect
	ld	iyl,iyh		; FD 6C
	ld	iyl,iyl		; FD 6D
	ld	ixh,a		; DD 67
	ld	ixh,b		; DD 60
	ld	ixh,c		; DD 61
	ld	ixh,d		; DD 62
	ld	ixh,e		; DD 63
	expect	1350
	ld	ixh,h
	endexpect
	expect	1350
	ld	ixh,l
	endexpect
	ld	ixl,a		; DD 6F
	ld	ixl,b		; DD 68
	ld	ixl,c		; DD 69
	ld	ixl,d		; DD 6A
	ld	ixl,e		; DD 6B
	expect	1350
	ld	ixl,h
	endexpect
	expect	1350
	ld	ixl,l
	endexpect
	ld	iyh,a		; FD 67
	ld	iyh,b		; FD 60
	ld	iyh,c		; FD 61
	ld	iyh,d		; FD 62
	ld	iyh,e		; FD 63
	expect	1350
	ld	iyh,h
	endexpect
	expect	1350
	ld	iyh,l
	endexpect
	ld	iyl,a		; FD 6F
	ld	iyl,b		; FD 68
	ld	iyl,c		; FD 69
	ld	iyl,d		; FD 6A
	ld	iyl,e		; FD 6B
	expect	1350
	ld	iyl,h
	endexpect
	expect	1350
	ld	iyl,l
	endexpect
	ld	a,12h		; 3E 12
	ld	b,12h		; 06 12
	ld	c,12h		; 0E 12
	ld	d,12h		; 16 12
	ld	e,12h		; 1E 12
	ld	h,12h		; 26 12
	ld	l,12h		; 2E 12
	ld	ixh,12h		; DD 26 12
	ld	ixl,12h		; DD 2E 12
	ld	iyh,12h		; FD 26 12
	ld	iyl,12h		; FD 2E 12
	ld	a,(hl)		; 7E
	ld	b,(hl)		; 46
	ld	c,(hl)		; 4E
	ld	d,(hl)		; 56
	ld	e,(hl)		; 5E
	ld	h,(hl)		; 66
	ld	l,(hl)		; 6E
	ld	a,(ix+12h)	; DD 7E 12
	ld	b,(ix+12h)	; DD 46 12
	ld	c,(ix+12h)	; DD 4E 12
	ld	d,(ix+12h)	; DD 56 12
	ld	e,(ix+12h)	; DD 5E 12
	ld	h,(ix+12h)	; DD 66 12
	ld	l,(ix+12h)	; DD 6E 12
	ld	a,(iy+12h)	; FD 7E 12
	ld	b,(iy+12h)	; FD 46 12
	ld	c,(iy+12h)	; FD 4E 12
	ld	d,(iy+12h)	; FD 56 12
	ld	e,(iy+12h)	; FD 5E 12
	ld	h,(iy+12h)	; FD 66 12
	ld	l,(iy+12h)	; FD 6E 12

	ld	(hl),a		; 77
	ld	(hl),b		; 70
	ld	(hl),c		; 71
	ld	(hl),d		; 72
	ld	(hl),e		; 73
	ld	(hl),h		; 74
	ld	(hl),l		; 75
	ld	(ix+12h),a	; DD 77 12
	ld	(ix+12h),b	; DD 70 12
	ld	(ix+12h),c	; DD 71 12
	ld	(ix+12h),d	; DD 72 12
	ld	(ix+12h),e	; DD 73 12
	ld	(ix+12h),h	; DD 74 12
	ld	(ix+12h),l	; DD 75 12
	ld	(iy+12h),a	; FD 77 12
	ld	(iy+12h),b	; FD 70 12
	ld	(iy+12h),c	; FD 71 12
	ld	(iy+12h),d	; FD 72 12
	ld	(iy+12h),e	; FD 73 12
	ld	(iy+12h),h	; FD 74 12
	ld	(iy+12h),l	; FD 75 12

	ld	i,a		; ED 47
	ld	r,a		; ED 4F

	lda	hl,(1234h)	; 21 34 12
	lda	ix,(1234h)	; DD 21 34 12
	lda	iy,(1234h)	; FD 21 34 12
	lda	hl,(ix+1234h)	; ED 2A 34 21
	lda	ix,(ix+1234h)	; DD ED 2A 34 21
	lda	iy,(ix+1234h)	; FD ED 2A 34 21
	lda	hl,(iy+1234h)	; ED 32 34 21
	lda	ix,(iy+1234h)	; DD ED 32 34 21
	lda	iy,(iy+1234h)	; FD ED 32 34 21
	lda	hl,(hl+1234h)	; ED 3A 34 21
	lda	ix,(hl+1234h)	; DD ED 3A 34 21
	lda	iy,(hl+1234h)	; FD ED 3A 34 21
	; SX treated as X
	lda	hl,(ix+12h)	; ED 2A 12 00
	lda	ix,(ix+12h)	; DD ED 2A 12 00
	lda	iy,(ix+12h)	; FD ED 2A 12 00
	lda	hl,(iy-12h)	; ED 32 EE FF
	lda	ix,(iy-12h)	; DD ED 32 EE FF
	lda	iy,(iy-12h)	; FD ED 32 EE FF
	lda	hl,<$+1238h>	; ED 22 34 12
	lda	ix,<$+1239h>	; DD ED 22 34 12
	lda	iy,<$+1239h>	; FD ED 22 34 12
	lda	hl,(pc+1234h)	; ED 22 34 12
	lda	ix,(pc+1234h)	; DD ED 22 34 12
	lda	iy,(pc+1234h)	; FD ED 22 34 12
	lda	hl,(sp+1234h)	; ED 02 34 12
	lda	ix,(sp+1234h)	; DD ED 02 34 12
	lda	iy,(sp+1234h)	; FD ED 02 34 12
	lda	hl,(ix+hl)	; ED 0A
	lda	ix,(ix+hl)	; DD ED 0A
	lda	iy,(ix+hl)	; FD ED 0A
	lda	hl,(iy+hl)	; ED 12
	lda	ix,(iy+hl)	; DD ED 12
	lda	iy,(iy+hl)	; FD ED 12
	lda	hl,(ix+iy)	; ED 1A
	lda	ix,(ix+iy)	; DD ED 1A
	lda	iy,(ix+iy)	; FD ED 1A

	ldctl	hl,(c)		; ED 66
	ldctl	ix,(c)		; DD ED 66
	ldctl	iy,(c)		; FD ED 66
	ldctl	(c),hl		; ED 6E
	ldctl	(c),ix		; DD ED 6E
	ldctl	(c),iy		; FD ED 6E
	ldctl	hl,usp		; ED 87
	ldctl	ix,usp		; DD ED 87
	ldctl	iy,usp		; FD ED 87
	ldctl	usp,hl		; ED 8F
	ldctl	usp,ix		; DD ED 8F
	ldctl	usp,iy		; FD ED 8F

	ldd			; ED A8
	lddr			; ED B8

	ldi			; ED A0
	ldir			; ED B0

	ldud	a,(hl)		; ED 86
	ldud	a,(ix+12h)	; DD ED 86 12
	ldud	a,(iy-12h)	; FD ED 86 EE
	ldud	(hl),a		; ED 8E
	ldud	(ix+12h),a	; DD ED 8E 12
	ldud	(iy-12h),a	; FD ED 8E EE

	ldup	a,(hl)		; ED 96
	ldup	a,(ix+12h)	; DD ED 96 12
	ldup	a,(iy-12h)	; FD ED 96 EE
	ldup	(hl),a		; ED 9E
	ldup	(ix+12h),a	; DD ED 9E 12
	ldup	(iy-12h),a	; FD ED 9E EE

	ld	bc,1234h	; 01 34 12
	ldw	bc,1234h
	ld	de,1234h	; 11 34 12
	ldw	de,1234h
	ld	hl,1234h	; 21 34 12
	ldw	hl,1234h
	ld	sp,1234h	; 31 34 12
	ldw	sp,1234h
	ld	ix,1234h	; DD 21 34 12
	ldw	ix,1234h
	ld	iy,1234h	; FD 21 34 12
	ldw	ix,1234h
	ldw	(hl),1234h	; DD 01 34 12
	ldw	(1234h),5678h	; DD 21 34 12 78 56
	ldw	<$+123ah>,5678h	; DD 31 34 12 78 56
	ldw	(pc+1234h),5678h; DD 31 34 12 78 56

	ld	hl,1234h	; 21 34 12
	ldw	hl,1234h
	ld	ix,1234h	; DD 21 34 12
	ldw	ix,1234h
	ld	iy,1234h	; FD 21 34 12
	ldw	iy,1234h
	ld	hl,(1234h)	; 2A 34 12
	ldw	hl,(1234h)
	ld	ix,(1234h)	; DD 2A 34 12
	ldw	ix,(1234h)
	ld	iy,(1234h)	; FD 2A 34 12
	ldw	iy,(1234h)
	ld	hl,(ix+1234h)	; ED 2C 34 12
	ldw	hl,(ix+1234h)
	ld	ix,(ix+1234h)	; DD ED 2C 34 12
	ldw	ix,(ix+1234h)
	ld	iy,(ix+1234h)	; FD ED 2C 34 12
	ldw	iy,(ix+1234h)
	ld	hl,(iy+1234h)	; ED 34 34 12
	ldw	hl,(iy+1234h)
	ld	ix,(iy+1234h)	; DD ED 34 34 12
	ldw	ix,(iy+1234h)
	ld	iy,(iy+1234h)	; FD ED 34 34 12
	ldw	iy,(iy+1234h)
	ld	hl,(hl+1234h)	; ED 3C 34 12
	ldw	hl,(hl+1234h)
	ld	ix,(hl+1234h)	; DD ED 3C 34 12
	ldw	ix,(hl+1234h)
	ld	iy,(hl+1234h)	; FD ED 3C 34 12
	ldw	iy,(hl+1234h)
	ld	hl,<$+1238h>	; ED 24 34 12
	ldw	hl,<$+1238h>
	ld	ix,<$+1239h>	; DD ED 24 34 12
	ldw	ix,<$+1239h>
	ld	iy,<$+1239h>	; FD ED 24 34 12
	ldw	iy,<$+1239h>
	ld	hl,(pc+1234h)	; ED 24 34 12
	ldw	hl,(pc+1234h)
	ld	ix,(pc+1234h)	; DD ED 24 34 12
	ldw	ix,(pc+1234h)
	ld	iy,(pc+1234h)	; FD ED 24 34 12
	ldw	iy,(pc+1234h)
	ld	hl,(sp+1234h)	; ED 04 34 12
	ldw	hl,(sp+1234h)
	ld	ix,(sp+1234h)	; DD ED 04 34 12
	ldw	ix,(sp+1234h)
	ld	iy,(sp+1234h)	; FD ED 04 34 12
	ldw	iy,(sp+1234h)
	ld	hl,(ix+hl)	; ED 0C
	ldw	hl,(ix+hl)
	ld	ix,(ix+hl)	; DD ED 0C
	ldw	ix,(ix+hl)
	ld	iy,(ix+hl)	; FD ED 0C
	ldw	iy,(ix+hl)
	ld	hl,(iy+hl)	; ED 14
	ldw	hl,(iy+hl)
	ld	ix,(iy+hl)	; DD ED 14
	ldw	ix,(iy+hl)
	ld	iy,(iy+hl)	; FD ED 14
	ldw	iy,(iy+hl)
	ld	hl,(iy+ix)	; ED 1C
	ldw	hl,(iy+ix)
	ld	ix,(iy+ix)	; DD ED 1C
	ldw	ix,(iy+ix)
	ld	iy,(iy+ix)	; FD ED 1C
	ldw	iy,(iy+ix)

	ld	(1234h),hl	; 22 34 12
	ldw	(1234h),hl
	ld	(1234h),ix	; DD 22 34 12
	ldw	(1234h),ix
	ld	(1234h),iy	; FD 22 34 12
	ldw	(1234h),iy
	ld	(ix+1234h),hl	; ED 2D 34 12
	ldw	(ix+1234h),hl
	ld	(ix+1234h),ix	; DD ED 2D 34 12
	ldw	(ix+1234h),ix
	ld	(ix+1234h),iy	; FD ED 2D 34 12
	ldw	(ix+1234h),iy
	ld	(iy+1234h),hl	; ED 35 34 12
	ldw	(iy+1234h),hl
	ld	(iy+1234h),ix	; DD ED 35 34 12
	ldw	(iy+1234h),ix
	ld	(iy+1234h),iy	; FD ED 35 34 12
	ldw	(iy+1234h),iy
	ld	(hl+1234h),hl	; ED 3D 34 12
	ldw	(hl+1234h),hl
	ld	(hl+1234h),ix	; DD ED 3D 34 12
	ldw	(hl+1234h),ix
	ld	(hl+1234h),iy	; FD ED 3D 34 12
	ldw	(hl+1234h),iy
	ld	<$+1238h>,hl	; ED 25 34 12
	ldw	<$+1238h>,hl
	ld	<$+1239h>,ix	; DD ED 25 34 12
	ldw	<$+1239h>,ix
	ld	<$+1239h>,iy	; FD ED 25 34 12
	ldw	<$+1239h>,iy
	ld	(pc+1234h),hl	; ED 25 34 12
	ldw	(pc+1234h),hl
	ld	(pc+1234h),ix	; DD ED 25 34 12
	ldw	(pc+1234h),ix
	ld	(pc+1234h),iy	; FD ED 25 34 12
	ldw	(pc+1234h),iy
	ld	(sp+1234h),hl	; ED 05 34 12
	ldw	(sp+1234h),hl
	ld	(sp+1234h),ix	; DD ED 05 34 12
	ldw	(sp+1234h),ix
	ld	(sp+1234h),iy	; FD ED 05 34 12
	ldw	(sp+1234h),iy
	ld	(ix+hl),hl	; ED 0D
	ldw	(ix+hl),hl
	ld	(ix+hl),ix	; DD ED 0D
	ldw	(ix+hl),ix
	ld	(ix+hl),iy	; FD ED 0D
	ldw	(ix+hl),iy
	ld	(iy+hl),hl	; ED 15
	ldw	(iy+hl),hl
	ld	(iy+hl),ix	; DD ED 15
	ldw	(iy+hl),ix
	ld	(iy+hl),iy	; FD ED 15
	ldw	(iy+hl),iy
	ld	(iy+ix),hl	; ED 1D
	ldw	(iy+ix),hl
	ld	(iy+ix),ix	; DD ED 1D
	ldw	(iy+ix),ix
	ld	(iy+ix),iy	; FD ED 1D
	ldw	(iy+ix),iy

	; LDW RR,...
	ld	bc,1234h	; 01 34 12
	ldw	bc,1234h
	ld	de,1234h	; 11 34 12
	ldw	de,1234h
	ld	hl,1234h	; 21 34 12
	ldw	hl,1234h
	ld	sp,1234h	; 31 34 12
	ldw	sp,1234h
	ld	bc,(hl)		; ED 06
	ldw	bc,(hl)
	ld	de,(hl)		; ED 16
	ldw	de,(hl)
	ld	hl,(hl)		; ED 26
	ldw	hl,(hl)
	ld	sp,(hl)		; ED 36
	ldw	sp,(hl)
	; (HL) -> X since previous form not allowed for IX/IY as dest:
	ld	ix,(hl)		; DD ED 3C 00 00
	ldw	ix,(hl)
	ld	iy,(hl)		; FD ED 3C 00 00
	ldw	iy,(hl)
	;
	ld	bc,(1234h)	; ED 4B 34 12
	ldw	bc,(1234h)
	ld	de,(1234h)	; ED 5B 34 12
	ldw	de,(1234h)
	ld	hl,(1234h)	; 2A 34 12 (!)
	ldw	hl,(1234h)
	ld	sp,(1234h)	; ED 7B 34 12
	ldw	sp,(1234h)
	ld	bc,(ix+12h)	; DD ED 06 12
	ldw	bc,(ix+12h)
	ld	de,(ix+12h)	; DD ED 16 12
	ldw	de,(ix+12h)
	ld	hl,(ix+12h)	; DD ED 26 12
	ldw	hl,(ix+12h)
	ld	sp,(ix+12h)	; DD ED 36 12
	ldw	sp,(ix+12h)
	ld	bc,(iy-12h)	; FD ED 06 EE
	ldw	bc,(iy-12h)
	ld	de,(iy-12h)	; FD ED 16 EE
	ldw	de,(iy-12h)
	ld	hl,(iy-12h)	; FD ED 26 EE
	ldw	hl,(iy-12h)
	ld	sp,(iy-12h)	; FD ED 36 EE
	ldw	sp,(iy-12h)
	; SX -> X since previous form not allowed for IX/IY as dest:
	ld	ix,(ix+12h)	; DD ED 2C 12 00
	ldw	ix,(ix+12h)
	ld	iy,(ix+12h)	; FD ED 2C 12 00
	ldw	iy,(ix+12h)
	ld	ix,(iy-12h)	; DD ED 34 EE FF
	ldw	ix,(iy-12h)
	ld	iy,(iy-12h)	; FD ED 34 EE FF
	ldw	iy,(iy-12h)
	;

	; LDW ...,RR
	ld	(hl),bc		; ED 0E
	ldw	(hl),bc
	ld	(hl),de		; ED 1E
	ldw	(hl),de
	ld	(hl),hl		; ED 2E
	ldw	(hl),hl
	ld	(hl),sp		; ED 3E
	ldw	(hl),sp
	; (HL) -> X since previous form not allowed for IX/IY as source:
	ld	(hl),ix		; DD ED 3D 00 00
	ldw	(hl),ix
	ld	(hl),iy		; FD ED 3D 00 00
	ldw	(hl),iy
	;
	ld	(1234h),bc	; ED 43 34 12
	ldw	(1234h),bc
	ld	(1234h),de	; ED 53 34 12
	ldw	(1234h),de
	ld	(1234h),hl	; 22 34 12 (!)
	ldw	(1234h),hl
	ld	(1234h),sp	; ED 73 34 12
	ldw	(1234h),sp
	ld	(ix+12h),bc	; DD ED 0E 12
	ldw	(ix+12h),bc
	ld	(ix+12h),de	; DD ED 1E 12
	ldw	(ix+12h),de
	ld	(ix+12h),hl	; DD ED 2E 12
	ldw	(ix+12h),hl
	ld	(ix+12h),sp	; DD ED 3E 12
	ldw	(ix+12h),sp
	; SX -> X since previous form not allowed for IX/IY as source:
	ld	(ix+12h),ix	; DD ED 2D 12 00
	ldw	(ix+12h),ix
	ld	(ix+12h),iy	; FD ED 2D 12 00
	ldw	(ix+12h),iy
	;
	ld	(iy-12h),bc	; FD ED 0E EE
	ldw	(iy-12h),bc
	ld	(iy-12h),de	; FD ED 1E EE
	ldw	(iy-12h),de
	ld	(iy-12h),hl	; FD ED 2E EE
	ldw	(iy-12h),hl
	ld	(iy-12h),sp	; FD ED 3E EE
	ldw	(iy-12h),sp
	; SX -> X since previous form not allowed for IX/IY as source:
	ld	(iy-12h),ix	; DD ED 35 EE FF
	ldw	(iy-12h),ix
	ld	(iy-12h),iy	; FD ED 35 EE FF
	ldw	(iy-12h),iy
	;

	ld	sp,hl		; F9
	ldw	sp,hl
	ld	sp,ix		; DD F9
	ldw	sp,ix
	ld	sp,iy		; FD F9
	ldw	sp,iy
	ld	sp,1234h	; 31 34 12
	ldw	sp,1234h
	ld	sp,(hl)		; ED 36
	ldw	sp,(hl)
	ld	sp,(1234h)	; ED 7B 34 12
	ldw	sp,(1234h)
	ld	sp,(ix+12h)	; DD ED 36 12
	ldw	sp,(ix+12h)
	ld	sp,(iy-12h)	; FD ED 36 EE
	ldw	sp,(iy-12h)

	ld	(hl),sp		; ED 3E
	ldw	(hl),sp
	ld	(1234h),sp	; ED 73 34 12
	ldw	(1234h),sp
	ld	(ix+12h),sp	; DD ED 3E 12
	ldw	(ix+12h),sp
	ld	(iy-12h),sp	; FD ED 3E EE
	ldw	(iy-12h),sp

	mult	a,a		; ED F8
	mult	a
	mult	a,b		; ED C0
	mult	b
	mult	a,c		; ED C8
	mult	c
	mult	a,d		; ED D0
	mult	d
	mult	a,e		; ED D8
	mult	e
	mult	a,h		; ED E0
	mult	h
	mult	a,l		; ED E8
	mult	l
	mult	a,ixh		; DD ED E0
	mult	ixh
	mult	a,ixl		; DD ED E8
	mult	ixl
	mult	a,iyh		; FD ED E0
	mult	iyh
	mult	a,iyl		; FD ED E8
	mult	iyl
	mult	a,12h		; FD ED F8 12
	mult	12h
	mult	a,(1234h)	; DD ED F8 34 12
	mult	(1234h)
	mult	a,(ix+1234h)	; FD ED C8 34 12
	mult	(ix+1234h)
	mult	a,(iy+1234h)	; FD ED D0 34 12
	mult	(iy+1234h)
	mult	a,(hl+1234h)	; FD ED D8 34 12
	mult	(hl+1234h)
	mult	a,(ix+12h)	; DD ED F0 12
	mult	(ix+12h)
	mult	a,(iy-12h)	; FD ED F0 EE
	mult	(ix+12h)
	mult	a,<$+1239h>	; FD ED C0 34 12
	mult	<$+1239h>
	mult	a,(pc+1234h)	; FD ED C0 34 12
	mult	(pc+1234h)
	mult	a,(sp+1234h)	; DD ED C0 34 12
	mult	(sp+1234h)
	mult	a,(ix+hl)	; DD ED C8
	mult	(ix+hl)
	mult	a,(iy+hl)	; DD ED D0
	mult	(iy+hl)
	mult	a,(ix+iy)	; DD ED D8
	mult	(ix+iy)
	mult	a,(hl)		; ED F0
	mult	(hl)

	multu	a,a		; ED F9
	multu	a
	multu	a,b		; ED C1
	multu	b
	multu	a,c		; ED C9
	multu	c
	multu	a,d		; ED D1
	multu	d
	multu	a,e		; ED D9
	multu	e
	multu	a,h		; ED E1
	multu	h
	multu	a,l		; ED E9
	multu	l
	multu	a,ixh		; DD ED E1
	multu	ixh
	multu	a,ixl		; DD ED E9
	multu	ixl
	multu	a,iyh		; FD ED E1
	multu	iyh
	multu	a,iyl		; FD ED E9
	multu	iyl
	multu	a,12h		; FD ED F9 12
	multu	12h
	multu	a,(1234h)	; DD ED F9 34 12
	multu	(1234h)
	multu	a,(ix+1234h)	; FD ED C9 34 12
	multu	(ix+1234h)
	multu	a,(iy+1234h)	; FD ED D1 34 12
	multu	(iy+1234h)
	multu	a,(hl+1234h)	; FD ED D9 34 12
	multu	(hl+1234h)
	multu	a,(ix+12h)	; DD ED F1 12
	multu	(ix+12h)
	multu	a,(iy-12h)	; FD ED F1 EE
	multu	(ix+12h)
	multu	a,<$+1239h>	; FD ED C1 34 12
	multu	<$+1239h>
	multu	a,(pc+1234h)	; FD ED C1 34 12
	multu	(pc+1234h)
	multu	a,(sp+1234h)	; DD ED C1 34 12
	multu	(sp+1234h)
	multu	a,(ix+hl)	; DD ED C9
	multu	(ix+hl)
	multu	a,(iy+hl)	; DD ED D1
	multu	(iy+hl)
	multu	a,(ix+iy)	; DD ED D9
	multu	(ix+iy)
	multu	a,(hl)		; ED F1
	multu	(hl)

	multuw	hl,bc		; ED C3
	multuw	bc
	multuw	hl,de		; ED D3
	multuw	de
	multuw	hl,hl		; ED E3
	multuw	hl
	multuw	hl,sp		; ED F3
	multuw	sp
	multuw	hl,ix		; DD ED E3
	multuw	ix
	multuw	hl,iy		; FD ED E3
	multuw	iy
	multuw	hl,1234h	; FD ED F3 34 12
	multuw	1234h
	multuw	hl,(1234h)	; DD ED D3 34 12
	multuw	(1234h)
	multuw	hl,(ix+12h)	; FD ED C3 12 00
	multuw	(ix+12h)
	multuw	hl,(iy-12h)	; FD ED D3 EE FF
	multuw	(iy-12h)
	multuw	hl,(ix+1234h)	; FD ED C3 34 12
	multuw	(ix+1234h)
	multuw	hl,(iy+1234h)	; FD ED D3 34 12
	multuw	(iy+1234h)
	multuw	hl,<$+1239h>	; DD ED F3 34 12
	multuw	<$+1239h>
	multuw	hl,(pc+1234h)	; DD ED F3 34 12
	multuw	(pc+1234h)
	multuw	hl,(hl)		; DD ED C3
	multuw	(hl)

	multw	hl,bc		; ED C2
	multw	bc
	multw	hl,de		; ED D2
	multw	de
	multw	hl,hl		; ED E2
	multw	hl
	multw	hl,sp		; ED F2
	multw	sp
	multw	hl,ix		; DD ED E2
	multw	ix
	multw	hl,iy		; FD ED E2
	multw	iy
	multw	hl,1234h	; FD ED F2 34 12
	multw	1234h
	multw	hl,(1234h)	; DD ED D2 34 12
	multw	(1234h)
	multw	hl,(ix+12h)	; FD ED C2 12 00
	multw	(ix+12h)
	multw	hl,(iy-12h)	; FD ED D2 EE FF
	multw	(iy-12h)
	multw	hl,(ix+1234h)	; FD ED C2 34 12
	multw	(ix+1234h)
	multw	hl,(iy+1234h)	; FD ED D2 34 12
	multw	(iy+1234h)
	multw	hl,<$+1239h>	; DD ED F2 34 12
	multw	<$+1239h>
	multw	hl,(pc+1234h)	; DD ED F2 34 12
	multw	(pc+1234h)
	multw	hl,(hl)		; DD ED C2
	multw	(hl)

	neg			; ED 44
	neg	a
	neg	hl		; ED 4C
	negw
	negw	hl

	nop			; 00

	or	a,a		; B7
	or	a
	or	a,b		; B0
	or	b
	or	a,c		; B1
	or	c
	or	a,d		; B2
	or	d
	or	a,e		; B3
	or	e
	or	a,h		; B4
	or	h
	or	a,l		; B5
	or	l
	or	a,ixh		; DD B4
	or	ixh
	or	a,ixl		; DD B5
	or	ixl
	or	a,iyh		; FD B4
	or	iyh
	or	a,iyl		; FD B5
	or	iyl
	or	a,12h		; F6 12
	or	12h
	or	a,(hl)		; B6
	or	(hl)
	or	a,(1234h)	; DD B7 34 12
	or	(1234h)
	or	a,(ix+1234h)	; FD B1 34 12
	or	(ix+1234h)
	or	a,(iy+1234h)	; FD B2 34 12
	or	(iy+1234h)
	or	a,(hl+1234h)	; FD B3 34 12
	or	(hl+1234h)
	or	a,(ix+12h)	; DD B6 12
	or	(ix+12h)
	or	a,(iy-12h)	; FD B6 EE
	or	(iy-12h)
	or	a,<$+1238h>	; FD B0 34 12
	or	<$+1238h>
	or	a,(pc+1234h)	; FD B0 34 12
	or	(pc+1234h)
	or	a,(sp+1234h)	; DD B0 34 12
	or	(sp+1234h)
	or	a,(hl+ix)	; DD B1
	or	(hl+ix)
	or	a,(hl+iy)	; DD B2
	or	(hl+iy)
	or	a,(ix+iy)	; DD B3
	or	(ix+iy)

	otdr			; ED BB
	otdrw			; ED 9B

	otir			; ED B3
	otirw			; ED 93

	out	(c),a		; ED 79
	out	(c),b		; ED 41
	out	(c),c		; ED 49
	out	(c),d		; ED 51
	out	(c),e		; ED 59
	out	(c),h		; ED 61
	out	(c),l		; ED 69
	out	(c),ixh		; DD ED 61
	out	(c),ixl		; DD ED 69
	out	(c),iyh		; FD ED 61
	out	(c),iyl		; FD ED 69
	out	(c),(1234h)	; DD ED 79 34 12
	out	(c),(ix+1234h)	; FD ED 49 34 12
	out	(c),(iy+1234h)	; FD ED 51 34 12
	out	(c),(hl+1234h)	; FD ED 59 34 12
	; SX -> X
	out	(c),(ix+12h)	; FD ED 49 12 00
	out	(c),(iy-12h)	; FD ED 51 EE FF
	out	(c),(hl)	; FD ED 59 00 00
	out	(c),<$+1239h>	; FD ED 41 34 12
	out	(c),(pc+1234h)	; FD ED 41 34 12
	out	(c),(sp+1234h)	; DD ED 41 34 12
	out	(c),(hl+ix)	; DD ED 49
	out	(c),(hl+iy)	; DD ED 51
	out	(c),(ix+iy)	; DD ED 59
	out	(12h),a		; D3 12

	outd			; ED AB
	outdw			; ED 8B

	outi			; ED A3
	outiw			; ED 83

	outw	(c),hl		; ED BF
	out	(c),hl

	pcache			; ED 65

	pop	bc		; C1
	pop	de		; D1
	pop	hl		; E1
	pop	af		; F1
	pop	ix		; DD E1
	pop	iy		; FD E1
	pop	(hl)		; DD C1
	pop	(1234h)		; DD D1 34 12
	pop	<$+1238h>	; DD F1 34 12
	pop	(pc+1234h)	; DD F1 34 12

	push	bc		; C5
	push	de		; D5
	push	hl		; E5
	push	af		; F5
	push	ix		; DD E5
	push	iy		; FD E5
	push	1234h		; FD F5 34 12
	push	(hl)		; DD C5
	push	(1234h)		; DD D5 34 12
	push	<$+1238h>	; DD F5 34 12
	push	(pc+1234h)	; DD F5 34 12

	res	0,a		; CB 87
	res	1,a		; CB 8F
	res	2,a		; CB 97
	res	3,a		; CB 9F
	res	4,a		; CB A7
	res	5,a		; CB AF
	res	6,a		; CB B7
	res	7,a		; CB BF
	res	0,b		; CB 80
	res	1,b		; CB 88
	res	2,b		; CB 90
	res	3,b		; CB 98
	res	4,b		; CB A0
	res	5,b		; CB A8
	res	6,b		; CB B0
	res	7,b		; CB B8
	res	0,c		; CB 81
	res	1,c		; CB 89
	res	2,c		; CB 91
	res	3,c		; CB 99
	res	4,c		; CB A1
	res	5,c		; CB A9
	res	6,c		; CB B1
	res	7,c		; CB B9
	res	0,d		; CB 82
	res	1,d		; CB 8A
	res	2,d		; CB 92
	res	3,d		; CB 9A
	res	4,d		; CB A2
	res	5,d		; CB AA
	res	6,d		; CB B2
	res	7,d		; CB BA
	res	0,e		; CB 83
	res	1,e		; CB 8B
	res	2,e		; CB 93
	res	3,e		; CB 9B
	res	4,e		; CB A3
	res	5,e		; CB AB
	res	6,e		; CB B3
	res	7,e		; CB BB
	res	0,h		; CB 84
	res	1,h		; CB 8C
	res	2,h		; CB 94
	res	3,h		; CB 9C
	res	4,h		; CB A4
	res	5,h		; CB AC
	res	6,h		; CB B4
	res	7,h		; CB BC
	res	0,l		; CB 85
	res	1,l		; CB 8D
	res	2,l		; CB 95
	res	3,l		; CB 9D
	res	4,l		; CB A5
	res	5,l		; CB AD
	res	6,l		; CB B5
	res	7,l		; CB BD
	res	0,(hl)		; CB 86
	res	1,(hl)		; CB 8E
	res	2,(hl)		; CB 96
	res	3,(hl)		; CB 9E
	res	4,(hl)		; CB A6
	res	5,(hl)		; CB AE
	res	6,(hl)		; CB B6
	res	7,(hl)		; CB BE
	res	0,(ix+12h)	; DD CB 12 86
	res	1,(ix+12h)	; DD CB 12 8E
	res	2,(ix+12h)	; DD CB 12 96
	res	3,(ix+12h)	; DD CB 12 9E
	res	4,(ix+12h)	; DD CB 12 A6
	res	5,(ix+12h)	; DD CB 12 AE
	res	6,(ix+12h)	; DD CB 12 B6
	res	7,(ix+12h)	; DD CB 12 BE
	res	0,(iy+12h)	; FD CB 12 86
	res	1,(iy+12h)	; FD CB 12 8E
	res	2,(iy+12h)	; FD CB 12 96
	res	3,(iy+12h)	; FD CB 12 9E
	res	4,(iy+12h)	; FD CB 12 A6
	res	5,(iy+12h)	; FD CB 12 AE
	res	6,(iy+12h)	; FD CB 12 B6
	res	7,(iy+12h)	; FD CB 12 BE

	ret	nz		; C0
	ret	z		; C8
	ret	nc		; D0
	ret	c		; D8
	ret	po		; E0
	ret	nv		; E0
	ret	pe		; E8
	ret	v		; E8
	ret	p		; F0
	ret	ns		; F0
	ret	m		; F8
	ret	s		; F8
	ret			; C9

	reti			; ED 4D

	retil			; ED 55
	retn			; ED 45

	rl	a		; CB 17
	rl	b		; CB 10
	rl	c		; CB 11
	rl	d		; CB 12
	rl	e		; CB 13
	rl	h		; CB 14
	rl	l		; CB 15
	rl	(hl)		; CB 16
	rl	(ix+12h)	; DD CB 16 12
	rl	(iy-12h)	; FD CB 16 EE
	rla			; 17

	rl	a		; CB 17
	rl	b		; CB 10
	rl	c		; CB 11
	rl	d		; CB 12
	rl	e		; CB 13
	rl	h		; CB 14
	rl	l		; CB 15
	rl	(hl)		; CB 16
	rl	(ix+12h)	; DD CB 16 12
	rl	(iy-12h)	; FD CB 16 EE
	rla			; 17

	rlc	a		; CB 07
	rlc	b		; CB 00
	rlc	c		; CB 01
	rlc	d		; CB 02
	rlc	e		; CB 03
	rlc	h		; CB 04
	rlc	l		; CB 05
	rlc	(hl)		; CB 06
	rlc	(ix+12h)	; DD CB 06 12
	rlc	(iy-12h)	; FD CB 06 EE
	rlca			; 07

	rlc	a		; CB 07
	rlc	b		; CB 00
	rlc	c		; CB 01
	rlc	d		; CB 02
	rlc	e		; CB 03
	rlc	h		; CB 04
	rlc	l		; CB 05
	rlc	(hl)		; CB 06
	rlc	(ix+12h)	; DD CB 06 12
	rlc	(iy-12h)	; FD CB 06 EE
	rlca			; 07

	rld			; ED 6F

	rr	a		; CB 1F
	rr	b		; CB 18
	rr	c		; CB 19
	rr	d		; CB 1A
	rr	e		; CB 1B
	rr	h		; CB 1C
	rr	l		; CB 1D
	rr	(hl)		; CB 1E
	rr	(ix+12h)	; DD CB 1E 12
	rr	(iy-12h)	; FD CB 1E EE
	rra			; 1F

	rrc	a		; CB 0F
	rrc	b		; CB 08
	rrc	c		; CB 09
	rrc	d		; CB 0A
	rrc	e		; CB 0B
	rrc	h		; CB 0C
	rrc	l		; CB 0D
	rrc	(hl)		; CB 0E
	rrc	(ix+12h)	; DD CB 0E 12
	rrc	(iy-12h)	; FD CB 0E EE
	rrca			; 0F

	rrd			; ED 67

	rst	00h		; C7
	rst	08h		; CF
	rst	10h		; D7
	rst	18h		; DF
	rst	20h		; E7
	rst	28h		; EF
	rst	30h		; F7
	rst	38h		; FF

	sbc	a,a		; 9F
	sbc	a
	sbc	a,b		; 98
	sbc	b
	sbc	a,c		; 99
	sbc	c
	sbc	a,d		; 9A
	sbc	d
	sbc	a,e		; 9B
	sbc	e
	sbc	a,h		; 9C
	sbc	h
	sbc	a,l		; 9D
	sbc	l
	sbc	a,ixh		; DD 9C
	sbc	ixh
	sbc	a,ixl		; DD 9D
	sbc	ixl
	sbc	a,iyh		; FD 9C
	sbc	iyh
	sbc	a,iyl		; FD 9D
	sbc	iyl
	sbc	a,12h		; DE 12
	sbc	12h
	sbc	a,(hl)		; 8E
	sbc	(hl)
	sbc	a,(1234h)	; DD 9F 34 12
	sbc	(1234h)
	sbc	a,(ix+1234h)	; FD 99 34 12
	sbc	(ix+1234h)
	sbc	a,(iy+1234h)	; FD 9A 34 12
	sbc	(iy+1234h)
	sbc	a,(hl+1234h)	; FD 9B 34 12
	sbc	(hl+1234h)
	sbc	a,(ix+12h)	; DD 9E 12
	sbc	(ix+12h)
	sbc	a,(iy-12h)	; FD 9E EE
	sbc	(iy-12h)
	sbc	a,<$+1238h>	; FD 98 34 12
	sbc	<$+1238h>
	sbc	a,(pc+1234h)	; FD 98 34 12
	sbc	(pc+1234h)
	sbc	a,(sp+1234h)	; DD 98 34 12
	sbc	(sp+1234h)
	sbc	a,(hl+ix)	; DD 99
	sbc	(hl+ix)
	sbc	a,(hl+iy)	; DD 9A
	sbc	(hl+iy)
	sbc	a,(ix+iy)	; DD 9B
	sbc	(ix+iy)

	sbc	hl,bc		; ED 42
	sbc	hl,de		; ED 52
	sbc	hl,hl		; ED 62
	expect	1350
	sbc	hl,ix
	endexpect
	expect	1350
	sbc	hl,iy
	endexpect
	sbc	hl,sp		; ED 72
	sbc	ix,bc		; DD ED 42
	sbc	ix,de		; DD ED 52
	expect	1350
	sbc	ix,hl
	endexpect
	sbc	ix,ix		; DD ED 62
	expect	1350
	sbc	ix,iy
	endexpect
	sbc	ix,sp		; DD ED 72
	sbc	iy,bc		; FD ED 42
	sbc	iy,de		; FD ED 52
	expect	1350
	sbc	iy,hl
	endexpect
	expect	1350
	sbc	iy,ix
	endexpect
	sbc	iy,iy		; FD ED 62
	sbc	iy,sp		; FD ED 72

	sc	1234h		; ED 71 34 12

	scf			; 37

	set	0,a		; CB C7
	set	1,a		; CB CF
	set	2,a		; CB D7
	set	3,a		; CB DF
	set	4,a		; CB E7
	set	5,a		; CB EF
	set	6,a		; CB F7
	set	7,a		; CB FF
	set	0,b		; CB C0
	set	1,b		; CB C8
	set	2,b		; CB D0
	set	3,b		; CB D8
	set	4,b		; CB E0
	set	5,b		; CB E8
	set	6,b		; CB F0
	set	7,b		; CB F8
	set	0,c		; CB C1
	set	1,c		; CB C9
	set	2,c		; CB D1
	set	3,c		; CB D9
	set	4,c		; CB E1
	set	5,c		; CB E9
	set	6,c		; CB F1
	set	7,c		; CB F9
	set	0,d		; CB C2
	set	1,d		; CB CA
	set	2,d		; CB D2
	set	3,d		; CB DA
	set	4,d		; CB E2
	set	5,d		; CB EA
	set	6,d		; CB F2
	set	7,d		; CB FA
	set	0,e		; CB C3
	set	1,e		; CB CB
	set	2,e		; CB D3
	set	3,e		; CB DB
	set	4,e		; CB E3
	set	5,e		; CB EB
	set	6,e		; CB F3
	set	7,e		; CB FB
	set	0,h		; CB C4
	set	1,h		; CB CC
	set	2,h		; CB D4
	set	3,h		; CB DC
	set	4,h		; CB E4
	set	5,h		; CB EC
	set	6,h		; CB F4
	set	7,h		; CB FC
	set	0,l		; CB C5
	set	1,l		; CB CD
	set	2,l		; CB D5
	set	3,l		; CB DD
	set	4,l		; CB E5
	set	5,l		; CB ED
	set	6,l		; CB F5
	set	7,l		; CB FD
	set	0,(hl)		; CB C6
	set	1,(hl)		; CB CE
	set	2,(hl)		; CB D6
	set	3,(hl)		; CB DE
	set	4,(hl)		; CB E6
	set	5,(hl)		; CB EE
	set	6,(hl)		; CB F6
	set	7,(hl)		; CB FE
	set	0,(ix+12h)	; DD CB 12 C6
	set	1,(ix+12h)	; DD CB 12 CE
	set	2,(ix+12h)	; DD CB 12 D6
	set	3,(ix+12h)	; DD CB 12 DE
	set	4,(ix+12h)	; DD CB 12 E6
	set	5,(ix+12h)	; DD CB 12 EE
	set	6,(ix+12h)	; DD CB 12 F6
	set	7,(ix+12h)	; DD CB 12 FE
	set	0,(iy+12h)	; FD CB 12 C6
	set	1,(iy+12h)	; FD CB 12 CE
	set	2,(iy+12h)	; FD CB 12 D6
	set	3,(iy+12h)	; FD CB 12 DE
	set	4,(iy+12h)	; FD CB 12 E6
	set	5,(iy+12h)	; FD CB 12 EE
	set	6,(iy+12h)	; FD CB 12 F6
	set	7,(iy+12h)	; FD CB 12 FE

	sla	a		; CB 27
	sla	b		; CB 20
	sla	c		; CB 21
	sla	d		; CB 22
	sla	e		; CB 23
	sla	h		; CB 24
	sla	l		; CB 25
	sla	(hl)		; CB 26
	sla	(ix+12h)	; DD CB 26 12
	sla	(iy-12h)	; FD CB 26 EE

	sra	a		; CB 2F
	sra	b		; CB 28
	sra	c		; CB 29
	sra	d		; CB 2A
	sra	e		; CB 2B
	sra	h		; CB 2C
	sra	l		; CB 2D
	sra	(hl)		; CB 2E
	sra	(ix+12h)	; DD CB 2E 12
	sra	(iy-12h)	; FD CB 2E EE

	srl	a		; CB 3F
	srl	b		; CB 38
	srl	c		; CB 39
	srl	d		; CB 3A
	srl	e		; CB 3B
	srl	h		; CB 3C
	srl	l		; CB 3D
	srl	(hl)		; CB 3E
	srl	(ix+12h)	; DD CB 3E 12
	srl	(iy-12h)	; FD CB 3E EE

	sub	a,a		; 97
	sub	a
	sub	a,b		; 90
	sub	b
	sub	a,c		; 91
	sub	c
	sub	a,d		; 92
	sub	d
	sub	a,e		; 93
	sub	e
	sub	a,h		; 94
	sub	h
	sub	a,l		; 95
	sub	l
	sub	a,ixh		; DD 94
	sub	ixh
	sub	a,ixl		; DD 95
	sub	ixl
	sub	a,iyh		; FD 94
	sub	iyh
	sub	a,iyl		; FD 95
	sub	iyl
	sub	a,12h		; D6 12
	sub	12h
	sub	a,(hl)		; 96
	sub	(hl)
	sub	a,(1234h)	; DD 97 34 12
	sub	(1234h)
	sub	a,(ix+1234h)	; FD 91 34 12
	sub	(ix+1234h)
	sub	a,(iy+1234h)	; FD 92 34 12
	sub	(iy+1234h)
	sub	a,(hl+1234h)	; FD 93 34 12
	sub	(hl+1234h)
	sub	a,(ix+12h)	; DD 96 12
	sub	(ix+12h)
	sub	a,(iy-12h)	; FD 96 EE
	sub	(iy-12h)
	sub	a,<$+1238h>	; FD 90 34 12
	sub	<$+1238h>
	sub	a,(pc+1234h)	; FD 90 34 12
	sub	(pc+1234h)
	sub	a,(sp+1234h)	; DD 90 34 12
	sub	(sp+1234h)
	sub	a,(hl+ix)	; DD 91
	sub	(hl+ix)
	sub	a,(hl+iy)	; DD 92
	sub	(hl+iy)
	sub	a,(ix+iy)	; DD 93
	sub	(ix+iy)

	subw	hl,bc		; ED CE
	subw	hl,de		; ED DE
	subw	hl,hl		; ED EE
	subw	hl,sp		; ED FE
	subw	hl,ix		; DD ED EE
	subw	hl,iy		; FD ED EE
	subw	hl,1234h	; FD ED FE 34 12
	subw	hl,(1234h)	; DD ED DE 34 12
	; Caution: no SX mode, uses X!
	subw	hl,(ix+12h)	; FD ED CE 12 00
	subw	hl,(iy-12h)	; FD ED DE EE FF
	subw	hl,(ix+1234h)	; FD ED CE 34 12
	subw	hl,(iy+1234h)	; FD ED DE 34 12
	subw	hl,<$+1239h>	; DD ED FE 34 12
	subw	hl,(pc+1234h)	; DD ED FE 34 12
	subw	hl,(hl)		; DD ED CE

	tset	a		; CB 37
	tset	b		; CB 30
	tset	c		; CB 31
	tset	d		; CB 32
	tset	e		; CB 33
	tset	h		; CB 34
	tset	l		; CB 35
	tset	(hl)		; CB 36
	tset	(ix+12h)	; DD CB 12 36
	tset	(iy-12h)	; FD CB 12 36

	tsti	(c)		; ED 70

	xor	a,a		; AF
	xor	a
	xor	a,b		; A8
	xor	b
	xor	a,c		; A9
	xor	c
	xor	a,d		; AA
	xor	d
	xor	a,e		; AB
	xor	e
	xor	a,h		; AC
	xor	h
	xor	a,l		; AD
	xor	l
	xor	a,ixh		; DD AC
	xor	ixh
	xor	a,ixl		; DD AD
	xor	ixl
	xor	a,iyh		; FD AC
	xor	iyh
	xor	a,iyl		; FD AD
	xor	iyl
	xor	a,12h		; EC 12
	xor	12h
	xor	a,(hl)		; AE
	xor	(hl)
	xor	a,(1234h)	; DD AF 34 12
	xor	(1234h)
	xor	a,(ix+1234h)	; FD A9 34 12
	xor	(ix+1234h)
	xor	a,(iy+1234h)	; FD AA 34 12
	xor	(iy+1234h)
	xor	a,(hl+1234h)	; FD AB 34 12
	xor	(hl+1234h)
	xor	a,(ix+12h)	; DD AE 12
	xor	(ix+12h)
	xor	a,(iy-12h)	; FD AE EE
	xor	(iy-12h)
	xor	a,<$+1238h>	; FD A8 34 12
	xor	<$+1238h>
	xor	a,(pc+1234h)	; FD A8 34 12
	xor	(pc+1234h)
	xor	a,(sp+1234h)	; DD A8 34 12
	xor	(sp+1234h)
	xor	a,(hl+ix)	; DD A9
	xor	(hl+ix)
	xor	a,(hl+iy)	; DD AA
	xor	(hl+iy)
	xor	a,(ix+iy)	; DD AB
	xor	(ix+iy)

	; Z280 prohibits I/O instructions in user mode if I bit is set:

	supmode	on
	assume	i:0
	in	a,(c)
	out	(c),a
	inir
	otirw

	supmode	on
	assume	i:1
	in	a,(c)
	out	(c),a
	inir
	otirw

	supmode	off
	assume	i:0
	in	a,(c)
	out	(c),a
	inir
	otirw

	supmode	off
	assume	i:1
	expect	50
	in	a,(c)
	endexpect
	expect	50
	out	(c),a
	endexpect
	expect	50
	inir
	endexpect
	expect	50
	otirw
	endexpect

