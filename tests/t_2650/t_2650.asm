	relaxed	on
	page	0
	cpu	2650

absolute	equ	1234h
immed		equ	$a5
	adda,r0   *absolute
	adda,r0   absolute
	adda,r0   absolute,r0,-
	adda,r0   absolute,r1,-
	adda,r0   absolute,r2,-
	adda,r0   absolute,r3,-
	adda,r0  *absolute,r0
	adda,r0  *absolute,r0,+
	adda,r0  *absolute,r0,-
	adda,r0  *absolute,r1
	adda,r0  *absolute,r1,+
	adda,r0  *absolute,r1,-
	adda,r0  *absolute,r2
	adda,r0  *absolute,r2,+
	adda,r0  *absolute,r2,-
	adda,r0  *absolute,r3
	adda,r0  *absolute,r3,+
	adda,r0  *absolute,r3,-
	adda,r0  absolute,r0
	adda,r0  absolute,r0,+
	adda,r0  absolute,r1
	adda,r0  absolute,r1,+
	adda,r0  absolute,r2
	adda,r0  absolute,r2,+
	adda,r0  absolute,r3
	adda,r0  absolute,r3,+
	adda,r1   *absolute
	adda,r1   absolute
	adda,r2   *absolute
	adda,r2   absolute
	adda,r3   *absolute
	adda,r3   absolute
	addi,r0   immed
	addi,r1   immed
	addi,r2   immed
	addi,r3   immed
	addr,r0   $-23h
	addr,r0   *$-23h
	addr,r1   $-23h
	addr,r1   *$-23h
	addr,r2   $-23h
	addr,r2   *$-23h
	addr,r3   $-23h
	addr,r3   *$-23h
	addz  r0
	addz  r1
	addz  r2
	addz  r3
	anda,r0   *absolute
	anda,r0   absolute
	anda,r0   absolute,r0,-
	anda,r0   absolute,r1,-
	anda,r0   absolute,r2,-
	anda,r0   absolute,r3,-
	anda,r0  *absolute,r0
	anda,r0  *absolute,r0,+
	anda,r0  *absolute,r0,-
	anda,r0  *absolute,r1
	anda,r0  *absolute,r1,+
	anda,r0  *absolute,r1,-
	anda,r0  *absolute,r2
	anda,r0  *absolute,r2,+
	anda,r0  *absolute,r2,-
	anda,r0  *absolute,r3
	anda,r0  *absolute,r3,+
	anda,r0  *absolute,r3,-
	anda,r0  absolute,r0
	anda,r0  absolute,r0,+
	anda,r0  absolute,r1
	anda,r0  absolute,r1,+
	anda,r0  absolute,r2
	anda,r0  absolute,r2,+
	anda,r0  absolute,r3
	anda,r0  absolute,r3,+
	anda,r1   *absolute
	anda,r1   absolute
	anda,r2   *absolute
	anda,r2   absolute
	anda,r3   *absolute
	anda,r3   absolute
	andi,r0   immed
	andi,r1   immed
	andi,r2   immed
	andi,r3   immed
	andr,r0   $-23h
	andr,r0   *$-23h
	andr,r1   $-23h
	andr,r1   *$-23h
	andr,r2   $-23h
	andr,r2   *$-23h
	andr,r3   $-23h
	andr,r3   *$-23h
	expect    1445
	andz  r0
	endexpect
	andz  r1
	andz  r2
	andz  r3
	bcfa,eq   *absolute
	bcfa,eq   absolute
	bcfa,gt   *absolute
	bcfa,gt   absolute
	bcfa,lt   *absolute
	bcfa,lt   absolute
	bcfr,eq   $-23h
	bcfr,eq   *$-23h
	bcfr,gt   $-23h
	bcfr,gt   *$-23h
	bcfr,lt   $-23h
	bcfr,lt   *$-23h
	bcta,always   *absolute
	bcta,always   absolute
	bcta,eq   *absolute
	bcta,eq   absolute
	bcta,gt   *absolute
	bcta,gt   absolute
	bcta,lt   *absolute
	bcta,lt   absolute
	bctr,always   $-23h
	bctr,always   *$-23h
	bctr,eq   $-23h
	bctr,eq   *$-23h
	bctr,gt   $-23h
	bctr,gt   *$-23h
	bctr,lt   $-23h
	bctr,lt   *$-23h
	bdra,r0   *absolute
	bdra,r0   absolute
	bdra,r1   *absolute
	bdra,r1   absolute
	bdra,r2   *absolute
	bdra,r2   absolute
	bdra,r3   *absolute
	bdra,r3   absolute
	bdrr,r0   $-23h
	bdrr,r0   *$-23h
	bdrr,r1   $-23h
	bdrr,r1   *$-23h
	bdrr,r2   $-23h
	bdrr,r2   *$-23h
	bdrr,r3   $-23h
	bdrr,r3   *$-23h
	bira,r0   *absolute
	bira,r0   absolute
	bira,r1   *absolute
	bira,r1   absolute
	bira,r2   *absolute
	bira,r2   absolute
	bira,r3   *absolute
	bira,r3   absolute
	birr,r0   $-23h
	birr,r0   *$-23h
	birr,r1   $-23h
	birr,r1   *$-23h
	birr,r2   $-23h
	birr,r2   *$-23h
	birr,r3   $-23h
	birr,r3   *$-23h
	brna,r0   *absolute
	brna,r0   absolute
	brna,r1   *absolute
	brna,r1   absolute
	brna,r2   *absolute
	brna,r2   absolute
	brna,r3   *absolute
	brna,r3   absolute
	brnr,r0   $-23h
	brnr,r0   *$-23h
	brnr,r1   $-23h
	brnr,r1   *$-23h
	brnr,r2   $-23h
	brnr,r2   *$-23h
	brnr,r3   $-23h
	brnr,r3   *$-23h
	bsfa,eq   *absolute
	bsfa,eq   absolute
	bsfa,gt   *absolute
	bsfa,gt   absolute
	bsfa,lt   *absolute
	bsfa,lt   absolute
	bsfr,eq   $-23h
	bsfr,eq   *$-23h
	bsfr,gt   $-23h
	bsfr,gt   *$-23h
	bsfr,lt   $-23h
	bsfr,lt   *$-23h
	bsna,r0   *absolute
	bsna,r0   absolute
	bsna,r1   *absolute
	bsna,r1   absolute
	bsna,r2   *absolute
	bsna,r2   absolute
	bsna,r3   *absolute
	bsna,r3   absolute
	bsnr,r0   $-23h
	bsnr,r0   *$-23h
	bsnr,r1   $-23h
	bsnr,r1   *$-23h
	bsnr,r2   $-23h
	bsnr,r2   *$-23h
	bsnr,r3   $-23h
	bsnr,r3   *$-23h
	bsta,always   *absolute
	bsta,always   absolute
	bsta,eq   *absolute
	bsta,eq   absolute
	bsta,gt   *absolute
	bsta,gt   absolute
	bsta,lt   *absolute
	bsta,lt   absolute
	bstr,always   $-23h
	bstr,always   *$-23h
	bstr,eq   $-23h
	bstr,eq   *$-23h
	bstr,gt   $-23h
	bstr,gt   *$-23h
	bstr,lt   $-23h
	bstr,lt   *$-23h
	bsxa  *absolute,r3
	bsxa  absolute,r3
	bxa  *absolute,r3
	bxa  absolute,r3
	coma,r0   *absolute
	coma,r0   absolute
	coma,r0   absolute,r0,-
	coma,r0   absolute,r1,-
	coma,r0   absolute,r2,-
	coma,r0   absolute,r3,-
	coma,r0  *absolute,r0
	coma,r0  *absolute,r0,+
	coma,r0  *absolute,r0,-
	coma,r0  *absolute,r1
	coma,r0  *absolute,r1,+
	coma,r0  *absolute,r1,-
	coma,r0  *absolute,r2
	coma,r0  *absolute,r2,+
	coma,r0  *absolute,r2,-
	coma,r0  *absolute,r3
	coma,r0  *absolute,r3,+
	coma,r0  *absolute,r3,-
	coma,r0  absolute,r0
	coma,r0  absolute,r0,+
	coma,r0  absolute,r1
	coma,r0  absolute,r1,+
	coma,r0  absolute,r2
	coma,r0  absolute,r2,+
	coma,r0  absolute,r3
	coma,r0  absolute,r3,+
	coma,r1   *absolute
	coma,r1   absolute
	coma,r2   *absolute
	coma,r2   absolute
	coma,r3   *absolute
	coma,r3   absolute
	comi,r0   immed
	comi,r1   immed
	comi,r2   immed
	comi,r3   immed
	comr,r0   $-23h
	comr,r0   *$-23h
	comr,r1   $-23h
	comr,r1   *$-23h
	comr,r2   $-23h
	comr,r2   *$-23h
	comr,r3   $-23h
	comr,r3   *$-23h
	comz  r0
	comz  r1
	comz  r2
	comz  r3
	cpsl  immed
	cpsu  immed
	dar,r0
	dar,r1
	dar,r2
	dar,r3
	eora,r0   *absolute
	eora,r0   absolute
	eora,r0   absolute,r0,-
	eora,r0   absolute,r1,-
	eora,r0   absolute,r2,-
	eora,r0   absolute,r3,-
	eora,r0  *absolute,r0
	eora,r0  *absolute,r0,+
	eora,r0  *absolute,r0,-
	eora,r0  *absolute,r1
	eora,r0  *absolute,r1,+
	eora,r0  *absolute,r1,-
	eora,r0  *absolute,r2
	eora,r0  *absolute,r2,+
	eora,r0  *absolute,r2,-
	eora,r0  *absolute,r3
	eora,r0  *absolute,r3,+
	eora,r0  *absolute,r3,-
	eora,r0  absolute,r0
	eora,r0  absolute,r0,+
	eora,r0  absolute,r1
	eora,r0  absolute,r1,+
	eora,r0  absolute,r2
	eora,r0  absolute,r2,+
	eora,r0  absolute,r3
	eora,r0  absolute,r3,+
	eora,r1   *absolute
	eora,r1   absolute
	eora,r2   *absolute
	eora,r2   absolute
	eora,r3   *absolute
	eora,r3   absolute
	eori,r0   immed
	eori,r1   immed
	eori,r2   immed
	eori,r3   immed
	eorr,r0   $-23h
	eorr,r0   *$-23h
	eorr,r1   $-23h
	eorr,r1   *$-23h
	eorr,r2   $-23h
	eorr,r2   *$-23h
	eorr,r3   $-23h
	eorr,r3   *$-23h
	eorz  r0
	eorz  r1
	eorz  r2
	eorz  r3
	halt
	iora,r0   *absolute
	iora,r0   absolute
	iora,r0   absolute,r0,-
	iora,r0   absolute,r1,-
	iora,r0   absolute,r2,-
	iora,r0   absolute,r3,-
	iora,r0  *absolute,r0
	iora,r0  *absolute,r0,+
	iora,r0  *absolute,r0,-
	iora,r0  *absolute,r1
	iora,r0  *absolute,r1,+
	iora,r0  *absolute,r1,-
	iora,r0  *absolute,r2
	iora,r0  *absolute,r2,+
	iora,r0  *absolute,r2,-
	iora,r0  *absolute,r3
	iora,r0  *absolute,r3,+
	iora,r0  *absolute,r3,-
	iora,r0  absolute,r0
	iora,r0  absolute,r0,+
	iora,r0  absolute,r1
	iora,r0  absolute,r1,+
	iora,r0  absolute,r2
	iora,r0  absolute,r2,+
	iora,r0  absolute,r3
	iora,r0  absolute,r3,+
	iora,r1   *absolute
	iora,r1   absolute
	iora,r2   *absolute
	iora,r2   absolute
	iora,r3   *absolute
	iora,r3   absolute
	iori,r0   immed
	iori,r1   immed
	iori,r2   immed
	iori,r3   immed
	iorr,r0   $-23h
	iorr,r0   *$-23h
	iorr,r1   $-23h
	iorr,r1   *$-23h
	iorr,r2   $-23h
	iorr,r2   *$-23h
	iorr,r3   $-23h
	iorr,r3   *$-23h
	iorz  r0
	iorz  r1
	iorz  r2
	iorz  r3
	loda,r0   *absolute
	loda,r0   absolute
	loda,r0   absolute,r0,-
	loda,r0   absolute,r1,-
	loda,r0   absolute,r2,-
	loda,r0   absolute,r3,-
	loda,r0  *absolute,r0
	loda,r0  *absolute,r0,+
	loda,r0  *absolute,r0,-
	loda,r0  *absolute,r1
	loda,r0  *absolute,r1,+
	loda,r0  *absolute,r1,-
	loda,r0  *absolute,r2
	loda,r0  *absolute,r2,+
	loda,r0  *absolute,r2,-
	loda,r0  *absolute,r3
	loda,r0  *absolute,r3,+
	loda,r0  *absolute,r3,-
	loda,r0  absolute,r0
	loda,r0  absolute,r0,+
	loda,r0  absolute,r1
	loda,r0  absolute,r1,+
	loda,r0  absolute,r2
	loda,r0  absolute,r2,+
	loda,r0  absolute,r3
	loda,r0  absolute,r3,+
	loda,r1   *absolute
	loda,r1   absolute
	loda,r2   *absolute
	loda,r2   absolute
	loda,r3   *absolute
	loda,r3   absolute
	lodi,r0   immed
	lodi,r1   immed
	lodi,r2   immed
	lodi,r3   immed
	lodr,r0   $-23h
	lodr,r0   *$-23h
	lodr,r1   $-23h
	lodr,r1   *$-23h
	lodr,r2   $-23h
	lodr,r2   *$-23h
	lodr,r3   $-23h
	lodr,r3   *$-23h
	lodz  r0		; 0x60 (IORZ R0) instead of 0x00
	lodz  r1
	lodz  r2
	lodz  r3
	lpsl
	lpsu
	nop
	ppsl  immed
	ppsu  immed
	redc,r0
	redc,r1
	redc,r2
	redc,r3
	redd,r0
	redd,r1
	redd,r2
	redd,r3
	rede,r0   immed
	rede,r1   immed
	rede,r2   immed
	rede,r3   immed
	retc,always
	retc,eq
	retc,gt
	retc,lt
	rete,always
	rete,eq
	rete,gt
	rete,lt
	rrl,r0
	rrl,r1
	rrl,r2
	rrl,r3
	rrr,r0
	rrr,r1
	rrr,r2
	rrr,r3
	spsl
	spsu
	stra,r0   *absolute
	stra,r0   absolute
	stra,r0   absolute,r0,-
	stra,r0   absolute,r1,-
	stra,r0   absolute,r2,-
	stra,r0   absolute,r3,-
	stra,r0  *absolute,r0
	stra,r0  *absolute,r0,+
	stra,r0  *absolute,r0,-
	stra,r0  *absolute,r1
	stra,r0  *absolute,r1,+
	stra,r0  *absolute,r1,-
	stra,r0  *absolute,r2
	stra,r0  *absolute,r2,+
	stra,r0  *absolute,r2,-
	stra,r0  *absolute,r3
	stra,r0  *absolute,r3,+
	stra,r0  *absolute,r3,-
	stra,r0  absolute,r0
	stra,r0  absolute,r0,+
	stra,r0  absolute,r1
	stra,r0  absolute,r1,+
	stra,r0  absolute,r2
	stra,r0  absolute,r2,+
	stra,r0  absolute,r3
	stra,r0  absolute,r3,+
	stra,r1   *absolute
	stra,r1   absolute
	stra,r2   *absolute
	stra,r2   absolute
	stra,r3   *absolute
	stra,r3   absolute
	strr,r0   $-23h
	strr,r0   *$-23h
	strr,r1   $-23h
	strr,r1   *$-23h
	strr,r2   $-23h
	strr,r2   *$-23h
	strr,r3   $-23h
	strr,r3   *$-23h
	expect	1445
	strz  r0
	endexpect
	strz  r1
	strz  r2
	strz  r3
	suba,r0   *absolute
	suba,r0   absolute
	suba,r0   absolute,r0,-
	suba,r0   absolute,r1,-
	suba,r0   absolute,r2,-
	suba,r0   absolute,r3,-
	suba,r0  *absolute,r0
	suba,r0  *absolute,r0,+
	suba,r0  *absolute,r0,-
	suba,r0  *absolute,r1
	suba,r0  *absolute,r1,+
	suba,r0  *absolute,r1,-
	suba,r0  *absolute,r2
	suba,r0  *absolute,r2,+
	suba,r0  *absolute,r2,-
	suba,r0  *absolute,r3
	suba,r0  *absolute,r3,+
	suba,r0  *absolute,r3,-
	suba,r0  absolute,r0
	suba,r0  absolute,r0,+
	suba,r0  absolute,r1
	suba,r0  absolute,r1,+
	suba,r0  absolute,r2
	suba,r0  absolute,r2,+
	suba,r0  absolute,r3
	suba,r0  absolute,r3,+
	suba,r1   *absolute
	suba,r1   absolute
	suba,r2   *absolute
	suba,r2   absolute
	suba,r3   *absolute
	suba,r3   absolute
	subi,r0   immed
	subi,r1   immed
	subi,r2   immed
	subi,r3   immed
	subr,r0   $-23h
	subr,r0   *$-23h
	subr,r1   $-23h
	subr,r1   *$-23h
	subr,r2   $-23h
	subr,r2   *$-23h
	subr,r3   $-23h
	subr,r3   *$-23h
	subz  r0
	subz  r1
	subz  r2
	subz  r3
	tmi,r0   immed
	tmi,r1   immed
	tmi,r2   immed
	tmi,r3   immed
	tpsl  immed
	tpsu  immed
	wrtc,r0
	wrtc,r1
	wrtc,r2
	wrtc,r3
	wrtd,r0
	wrtd,r1
	wrtd,r2
	wrtd,r3
	wrte,r0   immed
	wrte,r1   immed
	wrte,r2   immed
	wrte,r3   immed
	zbrr  *23h
	zbrr  23h
	zbsr  *23h
	zbsr  23h

	db	01h
	db	20 dup (10, "Hello World")
