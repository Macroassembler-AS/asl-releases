	cpu	cop410

	asc
	add
	aisc	12
	clra
	comp
	nop
	rc
	sc
	xor
	jid
	jmp	0x123
	jp	0x023
	jp	(.&0x3f)+7
	jsrp	0x87
	jsr	0x123
	ret
	retsk
	camq
	ld	0
	ld	1
	ld	2
	ld	3
	lqid
	rmb	0
	rmb	1
	rmb	2
	rmb	3
	smb	0
	smb	1
	smb	2
	smb	3
	stii	4
	x	0
	x	1
	x	2
	x	3
	xad	3,15
	xds	0
	xds	1
	xds	2
	xds	3
	xis	0
	xis	1
	xis	2
	xis	3
	cab
	cba
	lbi	2,9
	lei	12
	skc
	ske
	skgz
	skgbz	0
        skgbz   1
        skgbz   2
        skgbz   3
	skmbz	0
        skmbz   1
        skmbz   2
        skmbz   3
	ing
	inl
	obd
	omg
	xas

; new instructions on COP420

	cpu	cop420

	lbi	1,7          ; extended operand range

	adt
	casc
	inin
	inil
	cqma
	xabr
	skt

	ogi	3

	ldd	1,15

	xad	2,12        ; complete new cos

; instructions on COP444

	cpu	cop444

	halt
	it

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
