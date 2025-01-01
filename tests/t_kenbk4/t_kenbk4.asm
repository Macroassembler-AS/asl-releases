	page	0
	cpu	KENBAK
	include	kenbak.inc

	ORG	004		;First non-special address

	; First, a small sample program...

Loop:
	ADD	Constant, RegisterA, 1
	STORE	Memory, RegisterA, Lamps
	JPD	Unconditional, Zero,  Loop

	HALT
	NOOP

	; ...then, things in a systematic way:
        ; addressing mode given as extra keyword:

	add	constant,a,12h	
	add	a,#12h
	sub	memory,b,12h
	sub	b,12h
	load	indexed,x,12h
	load	x,12h,x
	store	indirect,a,12h
	store	a,(12h)
	or	Indirect-Indexed,a,12h
	or	a,(12h),x

	; bit value to set as extra argument:

mybit	bit	6,34h
	set1	6,34h
	set1	mybit
	set	1,6,34h
	set	1,mybit
	set0	6,34h
	set0	mybit
	set	0,6,34h
	set	0,mybit

	skp1	6,34h
	skp	1,6,34h
	skp1	mybit
	skp	1,mybit
	skp1	6,34h,$+4
	skp	1,6,34h,$+4
	skp1	mybit,$+4
	skp	1,mybit,$+4
	skp0	6,34h
	skip	0,6,34h
	skp0	mybit
	skip	0,mybit
	skp0	6,34h,$+4
	skip	0,6,34h,$+4
	skp0	mybit,$+4
	skip	0,mybit,$+4

	; shift/rotate direction given as extra argument

	shift	left,a
	sftl	a
	shift	left,2,b
	sftl	2,b
	shift	right,a
	sftr	a
	shift	right,2,b
	sftr	2,b

	rotate	left,a
	rotl	a
	rotate	left,2,b
	rotl	2,b
	rotate	right,a
	rotr	a
	rotate	right,2,b
	rotr	2,b

	; CLEAR x is alias for SUB x,x

	irp	reg,a,b,x,RegisterA
	clear	reg
	sub	reg,reg
	endm

	; move to P -> jump to (reg)+2

	store	a,p
	store	b,p
	store	x,p

; [ADD/SUB/LOAD/STORE] [Addressing Mode], [Register], [Address]
; [OR/AND/LNEG] [Addressing Mode], [Register]
; [JPD/JPI/JMD/JMI] [Register], [Condition], [Address]
; SET [0/1], [Position], [Address]
; SKIP  [0/1], [Position], [Address]
; SHIFT [Direction], [Places], [Register]
; ROTATE [Direction], [Places], [Register]
; NOOP (no parameters)
; HALT (no parameters)

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	db	1,2,3
	dw	3,2,1
	dd	1000
