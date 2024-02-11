	cpu	6809

	lda	x	; allowed with command line default
	lda	,x

	plainbase off

	expect	1110
	lda	x+	; no longer allowed
	endexpect
	lda	,x+

	plainbase on

	lda	-x	; allowed again
	lda	,-x
