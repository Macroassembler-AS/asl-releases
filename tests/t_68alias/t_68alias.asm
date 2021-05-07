	cpu	6811
	page	0

	; all three variants in each block should result in the
	; same machine code:

	ldab	$aa
	lda	b,$aa
	ldb	$aa

	ldab	$aa,x
	lda	b,$aa,x
	ldb	$aa,x

	ldaa	$aa
	lda	a,$aa
	lda	$aa

	ldaa	$aa,x
	lda	a,$aa,x
	lda	$aa,x

	stab	$aa
	sta	b,$aa
	stb	$aa

	stab	$aa,x
	sta	b,$aa,x
	stb	$aa,x

	staa	$aa
	sta	a,$aa
	sta	$aa

	staa	$aa,x
	sta	a,$aa,x
	sta	$aa,x

	orab	$aa
	ora	b,$aa
	orb	$aa

	orab	$aa,x
	ora	b,$aa,x
	orb	$aa,x

	oraa	$aa
	ora	a,$aa
	ora	$aa

	oraa	$aa,x
	ora	a,$aa,x
	ora	$aa,x

	; These aliases are simpler...

	cpx	$aa
	cmpx	$aa

	cpx	$aa,x
	cmpx	$aa,x

	cpy	$aa
	cmpy	$aa

	cpy	$aa,y
	cmpy	$aa,y

	cpd	$aa
	cmpd	$aa

	cpd	$aa,x
	cmpd	$aa,x
