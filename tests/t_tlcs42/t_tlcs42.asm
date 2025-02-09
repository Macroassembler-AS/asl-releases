	cpu	42c70n
	page	0

	org	0

	; in order of documentation

	ld	a,@hl		; 06
	ld	a,#7		; 17
	ld	l,#9		; 29
	ld	a,5		; 95
	ld	mbr,#8		; B8

	ldl	a,@dc		; 67
	ldh	a,@dc		; 66

	st	a,@hl		; 76
	st	#12,@hl		; 3C
	st	a,5		; 9D

	mov	a,l		; 0C
	mov	l,a		; 0F
	mov	d,a		; 0E
	mov	a,d		; 0D
	mov	a,p		; 7E

	in	%3,a		; 63
	in	%3,@hl		; 6B
	out	a,%2		; 72
	out	@hl,%2		; 7A

	expect	1320,1320
	in	%6,a
	in	%12,a
	endexpect

	add	a,@hl		; 03
	addc	a,@hl		; 04
	add	a,#15		; 4F
	add	l,#1		; 51
	subrc	a,@hl		; 05

	inc	d		; 0B
	inc	@hl		; 09
	dec	d		; 0A
	dec	@hl		; 08

	and	a,@hl		; 00
	or	a,@hl		; 01
	xor	a,@hl		; 02

	set	h		; 88
	set	cf		; 89
	set	@hl,3		; 83
	clr	h		; 8A
	clr	cf		; 8B
	clr	@hl,3		; 87
	test	@hl,3		; 8f
	testp	cf		; 77

	bss	63		; FF
	bss	0		; C0
	expect	1910
	bss	64
	endexpect

	calls	10		; A5
	expect	1351
	calls	13
	endexpect

	hold			; 07

	ret			; 6E

	nop			; 7F


	; all opcodes from 00...ff

	org	100h

	and	a,@hl
	or	a,@hl
	xor	a,@hl
	add	a,@hl
	addc	a,@hl
	subrc	a,@hl
	ld	a,@hl
	hold
	dec	@hl
	inc	@hl
	dec	d
	inc	d
	mov	a,l
	mov	a,d
	mov	d,a
	mov	l,a

	ld	a,#0
	ld	a,#1
	ld	a,#2
	ld	a,#3
	ld	a,#4
	ld	a,#5
	ld	a,#6
	ld	a,#7
	ld	a,#8
	ld	a,#9
	ld	a,#10
	ld	a,#11
	ld	a,#12
	ld	a,#13
	ld	a,#14
	ld	a,#15

	ld	l,#0
	ld	l,#1
	ld	l,#2
	ld	l,#3
	ld	l,#4
	ld	l,#5
	ld	l,#6
	ld	l,#7
	ld	l,#8
	ld	l,#9
	ld	l,#10
	ld	l,#11
	ld	l,#12
	ld	l,#13
	ld	l,#14
	ld	l,#15

	st	#0,@hl
	st	#1,@hl
	st	#2,@hl
	st	#3,@hl
	st	#4,@hl
	st	#5,@hl
	st	#6,@hl
	st	#7,@hl
	st	#8,@hl
	st	#9,@hl
	st	#10,@hl
	st	#11,@hl
	st	#12,@hl
	st	#13,@hl
	st	#14,@hl
	st	#15,@hl

	add	a,#0
	add	a,#1
	add	a,#2
	add	a,#3
	add	a,#4
	add	a,#5
	add	a,#6
	add	a,#7
	add	a,#8
	add	a,#9
	add	a,#10
	add	a,#11
	add	a,#12
	add	a,#13
	add	a,#14
	add	a,#15

	add	l,#0
	add	l,#1
	add	l,#2
	add	l,#3
	add	l,#4
	add	l,#5
	add	l,#6
	add	l,#7
	add	l,#8
	add	l,#9
	add	l,#10
	add	l,#11
	add	l,#12
	add	l,#13
	add	l,#14
	add	l,#15

	in	%0,a
	in	%1,a
	in	%2,a
	in	%3,a
	in	%4,a
	in	%5,a
	ldh	a,@dc
	ldl	a,@dc
	in	%0,@hl
	in	%1,@hl
	in	%2,@hl
	in	%3,@hl
	in	%4,@hl
	in	%5,@hl
	ret
	db	6fh

	out	a,%0
	out	a,%1
	out	a,%2
	out	a,%3
	out	a,%4
	out	a,%5
	st	a,@hl
	testp	cf
	out	@hl,%0
	out	@hl,%1
	out	@hl,%2
	out	@hl,%3
	out	@hl,%4
	out	@hl,%5
	mov	a,p
	nop

	set	@hl,0
	set	@hl,1
	set	@hl,2
	set	@hl,3
	clr	@hl,0
	clr	@hl,1
	clr	@hl,2
	clr	@hl,3
	set	h
	set	cf
	clr	h
	clr	cf
	test	@hl,0
	test	@hl,1
	test	@hl,2
	test	@hl,3

	ld	a,0
	ld	a,1
	ld	a,2
	ld	a,3
	ld	a,4
	ld	a,5
	ld	a,6
	ld	a,7
	st	a,0
	st	a,1
	st	a,2
	st	a,3
	st	a,4
	st	a,5
	st	a,6
	st	a,7

	calls	0
	calls	2
	calls	4
	calls	6
	calls	8
	calls	10
	calls	12
	calls	14
	calls	16
	calls	18
	calls	20
	calls	22
	calls	24
	calls	26
	calls	28
	calls	30

	ld	mbr,#0
	ld	mbr,#1
	ld	mbr,#2
	ld	mbr,#3
	ld	mbr,#4
	ld	mbr,#5
	ld	mbr,#6
	ld	mbr,#7
	ld	mbr,#8
	ld	mbr,#9
	ld	mbr,#10
	ld	mbr,#11
	ld	mbr,#12
	ld	mbr,#13
	ld	mbr,#14
	ld	mbr,#15

	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$

	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$

	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$

	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	$
	bss	23fh		; PC is already 200h when executing this

	; target has all ports, but no CMOS-specific instructions

	cpu	4270n

	in	%0,a
	in	%1,a
	in	%2,a
	in	%3,a
	in	%4,a
	in	%5,a
	expect	1350
	mov	a,p
	endexpect
	expect	1200
	hold
	endexpect

	; target has only port 0..2, but CMOS-specific instructions

	cpu	42c60p

	in	%0,a
	in	%1,a
	in	%2,a
	expect	1320
	in	%3,a
	endexpect
	expect	1320
	in	%4,a
	endexpect
	expect	1320
	in	%5,a
	endexpect
	mov	a,p
	hold

	; target has only port 0..2, and no CMOS-specific instructions

	cpu	4260p

	in	%0,a
	in	%1,a
	in	%2,a
	expect	1320
	in	%3,a
	endexpect
	expect	1320
	in	%4,a
	endexpect
	expect	1320
	in	%5,a
	endexpect
	expect	1350
	mov	a,p
	endexpect
	expect	1200
	hold
	endexpect

	; on a target without P register, P is usable as ordinary symbol:

p	equ	6,data
	ld	a,p
	st	a,p

	; bytewise storage in CODE segment

	db	1,2,3
	dw	1,2,3
	dd	1,2,3

	; nibblewise storage in DATA segment

	segment	data

	org	0
	dn	1,2 dup(3)

	org	0
	db	1,2 dup(3)

	org	0
	dw	1,2 dup(3)

	org	0
	dw	1.0,2 dup (3.0)

	org	0
	dd	1,2 dup(3)

	org	0
	dd	1.0,2 dup (3.0)

	org	0
	dq	2 dup (123456)

	org	0
	dq	2 dup (1.0)

	org	0
	do	123456

	org	0
	do	1.0
