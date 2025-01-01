	do	1
	do	1.0
	do	3 dup (1)
	do	3 dup (1.0)
	do	"abc"
	do	1 dup ("abc")
	do	2 dup ("abc")
	do	3 dup ("abc")
	do	0
	do	2147483647
	do	-2147483648
	do	4 dup(10,20,30)	; all three values duplicated
	do	4 dup 10,20,30	; only first value duplicated
	ifdef	_MOTO_REP
	do	[4](10,20,30)	; all three values duplicated
	do	[4] 10,20,30	; only first value duplicated
	endif
	do	'A'
	do	'AB'
	do	'ABC'
	do	'ABCD'
	do	"A"
	do	"AB"
	do	"ABC"
	do	"ABCD"
