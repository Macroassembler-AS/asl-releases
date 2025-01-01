	cpu	1802

	page	0

allregs_nozero	macro	instr
	irp	reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	instr	reg
	endm
	endm

allregs	macro	instr
        instr	0
	allregs_nozero instr
	endm

allio	macro	instr
	irp	addr,1,2,3,4,5,6,7
	instr	addr
	endm
	endm

	allregs_nozero	ldn
	allregs	lda
	ldx
	ldxa
	ldi	55h
	allregs	str
	stxd

	allregs	inc
	allregs	dec
	irx
	allregs	glo
	allregs	plo
	allregs	ghi
	allregs	phi

	or
	ori	55h
	xor
	xri	55h
	and
	ani	55h
	shr
	shrc
	rshr
	shl
	shlc
	rshl

	add
	adi	55h
	adc
	adci	55h
	sd
	sdi	55h
	sdb
	sdbi	55h
	sm
	smi	55h
	smb
	smbi	55h

	br	$+2
	nbr	$+2
	bz	$+2
	bnz	$+2
	bdf	$+2
	bpz	$+2
	bge	$+2
	bnf	$+2
	bm	$+2
	bl	$+2
	bq	$+2
	bnq	$+2
	b1	$+2
	bn1	$+2
	b2	$+2
	bn2	$+2
	b3	$+2
	bn3	$+2
	b4	$+2
	bn4	$+2

	lbr	$+3
	nlbr	$+3
	lbz	$+3
	lbnz	$+3
	lbdf	$+3
	lbnf	$+3
	lbq	$+3
	lbnq	$+3

	skp
	lskp
	lsz
	lsnz
	lsdf
	lsnf
	lsq
	lsnq
	lsie

	idl
	nop
	allregs	sep	
	allregs	sex
	seq
	req
	sav
	mark
	ret
	dis

	allio	out
	allio	inp

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
