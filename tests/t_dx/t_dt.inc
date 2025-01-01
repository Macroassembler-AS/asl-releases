	dt	1
	dt	-1
	dt	-127
	dt	1.0
	dt	-127.0
	dt	3 dup (1)
	dt	3 dup (1.0)
	dt	'a','b','c'
	dt	"abc"
	dt	1 dup ("abc")
	dt	2 dup ("abc")
	dt	3 dup ("abc")
	dt	0
	dt	2147483647
	dt	-2147483648
	dt	4 dup(10,20,30)	; all three values duplicated
	dt	4 dup 10,20,30	; only first value duplicated
	ifdef	_MOTO_REP
	dt	[4](10,20,30)	; all three values duplicated
	dt	[4] 10,20,30	; only first value duplicated
	endif
	dt	'A'
	dt	'AB'
	dt	'ABC'
	dt	'ABCD'
	dt	"A"
	dt	"AB"
	dt	"ABC"
	dt	"ABCD"
