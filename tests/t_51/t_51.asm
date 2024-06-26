	cpu	8051

	; in the first pass, there should be no warning,
	; even if 0xe0 is inserted as a "don't know" value:

	org	0e0h
	if	MOMPASS > 1
	expect	140
	endif
	mov	a,acc
	if	MOMPASS > 1
	endexpect
	endif

acc	equ	0e0h
