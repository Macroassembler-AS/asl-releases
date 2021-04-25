	cpu	z80
	page	0,0

	jp	reset

DUP:	ld	a,12
	ret

reset:	ld	hl,(jmptab)
	jp	(hl)

jmptab:	dw	DUP, DUP
	dw	2 DUP (DUP)
