	cpu	68000
	page	0

fwdmac	macro	useit
	if	useit
	jsr	sub
	endif
	jmp	skip
sub	if	symused(sub)
	nop
	rts
	endif
skip	nop
	endm

	fwdmac	0
	fwdmac	1
	fwdmac	0
	fwdmac	1
	fwdmac	0
	fwdmac	1
