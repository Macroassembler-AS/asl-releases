	cpu	z8002:amdsyntax=1
	page	0
	supmode	on

	; rn^ is equal to @rn

	add	r2,r3^
	add	rh2,r3^
	addb	rh2,r3^
	add	rr4,r6^
	addl	rr4,r6^

	; the ^ unary operator transforms a constant (not assigned to a
	; specific address space) to a label and vice versa.  This is
	; necessary since in the AMD syntax, 'n' may mean immediate or
	; absolute, similar to the 8086:

lab	label	$
cst	equ	42

	ld	r4,lab		; absolute
	ld	r4,^lab		; immediate
	ld	r4,cst		; ...the other way around
	ld	r4,^cst
