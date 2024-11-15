	cpu	6502
	page	0

	db	10
	db	10,20,30
	db	3 dup 10
	db	3 dup (10,20,30)
	db	3 dup 10,20,30

	db	[3]10
	db	[3](10,20,30)
	db	[3]10,20,30

	; 3 * 4 = 12 duplications

	db	3 dup [4]10
	db	3 dup [4](10,20,30)
	db	3 dup [4]10,20,30
