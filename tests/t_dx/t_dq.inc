	dq	1
	dq	1.0
	dq	3 dup (1)
	dq	3 dup (1.0)
	dq	"abc"
	dq	1 dup ("abc")
	dq	2 dup ("abc")
	dq	3 dup ("abc")
	dq	0
	dq	2147483647
	dq	-2147483648
	dq	4 dup(10,20,30)	; all three values duplicated
	dq	4 dup 10,20,30	; only first value duplicated
	ifdef	_MOTO_REP
	dq	[4](10,20,30)	; all three values duplicated
	dq	[4] 10,20,30	; only first value duplicated
	endif
	dq	'A'
	dq	'AB'
	dq	'ABC'
	dq	'ABCD'
	dq	"A"
	dq	"AB"
	dq	"ABC"
	dq	"ABCD"
