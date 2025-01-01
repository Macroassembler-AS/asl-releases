	db	1
	db	3 dup (1)
	db	"abc"
	db	1 dup ("abc")
	db	2 dup ("abc")
	db	3 dup "abc"
	ifdef	_MOTO_REP
	db	[1]("abc")
	db	[2]("abc")
	db	[3]"abc"
	endif
	db	0
	db	255
	expect	1320
	db	256
	endexpect
	db	-128
	expect	1320
	db	-129
	endexpect
	db	10,20,30
	db	4 dup(10,20,30)	; all three values duplicated
	db	4 dup 10,20,30	; only first value duplicated
	ifdef	_MOTO_REP
	db	[4](10,20,30)	; all three values duplicated
	db	[4] 10,20,30	; only first value duplicated
	endif
	db	'A'
	db	'AB'		; treat like "AB"
	db	'ABC'		; treat like "ABC"
	db	'ABCD'		; treat like "ABCD"
	db	"A"
	db	"AB"
	db	"ABC"
	db	"ABCD"
