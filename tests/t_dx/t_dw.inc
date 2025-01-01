	dw	1
	dw	3 dup (1)
	dw	"abc"
	dw	1 dup ("abc")
	dw	2 dup ("abc")
	dw	3 dup ("abc")
	ifdef	_MOTO_REP
	dw	[1]("abc")
	dw	[2]("abc")
	dw	[3]("abc")
	endif
	dw	0
	dw	65535
	expect	1320
	dw	65536
	endexpect
	dw	-32768
	expect	1320
	dw	-32769
	endexpect
	dw	10,20,30
	dw	4 dup(10,20,30)	; all three values duplicated
	dw	4 dup 10,20,30	; only first value duplicated
	ifdef	_MOTO_REP
	dw	[4](10,20,30)	; all three values duplicated
	dw	[4] 10,20,30	; only first value duplicated
	endif
	dw	'A'
	dw	'AB'
	dw	'ABC'		; treat like "ABC"
	dw	'ABCD'		; treat like "ABCD"
	dw	"A"
	dw	"AB"
	dw	"ABC"
	dw	"ABCD"
