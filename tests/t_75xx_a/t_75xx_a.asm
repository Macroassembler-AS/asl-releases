	cpu	7508

	lai	4
	ldi	7
	lei	14
	lhi	3
	lli	1
	lam	dl
	lam	de
	lam	hl+
	lam	hl-
	lam	hl
	ladr	78h
	ldei	0ffh
	lhli	23
	lhlt	0c0h
	lhlt	0cfh
	lamtl
	st
	tad
	tae
	tah
	tal
	tda
	tea
	tha
	tla
	xad
	xae
	xah
	xal
	xam	dl
	xam	de
	xam	hl+
	xam	hl-
	xam	hl
	xadr	23h
	xhdr	34h
	xldr	45h

	aisc	12
	asc
	acsc
	exl
	anl
	orl

	cma
	rar

	rc
	sc

	ies
	ils
	idrs	4fh
	des
	dls
	ddrs	74h

	rmb	1
	smb	3

	jmp	0f23h
	jcp	target
	jam	4

	call	321h
	calt	0d0h
	calt	0efh
	rt
	rts
	rtpsw
	pshde
	pshhl
	popde
	pophl
	tamsp
	tspam
target:

	skc
	skabt	1
	skmbt	2
	skmbf	3
	skaem
	skaei	13
	skdei	5
	skeei	6
	skhei	10
	sklei	15

	tamsio
	tsioam
	sio

	tammod
	timer
	tcntam

	ei	7
	di	4
	ski	0

	ipl
	ip	3
	ip1
	ip54
	opl
	op	8
	op3
	op54
	anp	7,4
	orp	3,5

	halt
	stop
	nop

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
