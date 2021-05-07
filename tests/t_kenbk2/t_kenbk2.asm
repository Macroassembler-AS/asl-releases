	cpu	kenbak
	page	0

	org	4		; locations 0..3 are registers

dest:

	; the original machine instructions, which carry the
        ; indirect flag in the instruction's mnemonic:

	irp	instr,jpd,jpi,jmd,jmi
	instr	a,nz,dest
	instr	b,non-zero,dest
	instr	x,z,dest
	instr	unconditional,zero,dest
	instr	a,n,dest
	instr	b,negative,dest
	instr	x,p,dest
	instr	unconditional,positive,dest
	instr	a,pnz,dest
	instr	b,positive-non-zero,dest
	endm

	irp	instr,jpd,jpi,jmd,jmi
	instr	dest
	endm

	; more generic variants, that relocate the indirect flag to
        ; the addressing mode, similar to arithmetic/logic instructions:

	irp	instr,jp,jm
	instr	a,nz,dest
	instr	b,non-zero,dest
	instr	x,z,dest
	instr	unconditional,zero,dest
	instr	a,n,dest
	instr	b,negative,dest
	instr	x,p,dest
	instr	unconditional,positive,dest
	instr	a,pnz,dest
	instr	b,positive-non-zero,dest
	instr	a,nz,(dest)
	instr	b,non-zero,(dest)
	instr	x,z,(dest)
	instr	unconditional,zero,(dest)
	instr	a,n,(dest)
	instr	b,negative,(dest)
	instr	x,p,(dest)
	instr	unconditional,positive,(dest)
	instr	a,pnz,(dest)
	instr	b,positive-non-zero,(dest)
	endm

	irp	instr,jp,jm
	instr	dest
	instr	(dest)
	endm

	; SKP0 and SKP1 will always skip two bytes, it is the programmer's
        ; resposibility that this is either one two-byte insn or two one-byte insns.
        ; One may optionally add a skip target.  While this won't show up in the
        ; machine code, the assembler can verify if actually two bytes are skipped:

	skp0	6,12h
	skp1	6,12h

	skp0	6,12h,next1
	add	a,a
next1:

	skp1	6,12h,next2
	noop
	noop
next2:

	if	mompass>1
	expect	1376
	endif
	skp1	6,12h,next3
	if	mompass>1
	endexpect
	endif
	halt
next3:

	set0	6,12h
	set1	6,12h

	irp	instr,sftl,sftr,rotl,rotr
	irp	reg,a,b
	instr	reg
	instr	1,reg
	instr	4,reg
	endm
	endm
