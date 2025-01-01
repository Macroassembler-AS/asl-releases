	dd	1
	dd	1.0
	dd	3 dup (1)
	dd	3 dup (1.0)
	dd	"abc"
	dd	1 dup ("abc")
	dd	2 dup ("abc")
	dd	3 dup ("abc")
	dd	0
	dd	2147483647
;	expect	1320		; TODO: leave out until we have common large int arithmetic
;	dd	4294967296
;	endexpect
	dd	-2147483648
;	expect	1320		; TODO: leave out until we have common large int arithmetic
;	dd	-2147483649
;	endexpect
	dd	4 dup(10,20,30)	; all three values duplicated
	dd	4 dup 10,20,30	; only first value duplicated
	ifdef	_MOTO_REP
	dd	[4](10,20,30)	; all three values duplicated
	dd	[4] 10,20,30	; only first value duplicated
	endif
	dd	'A'
	dd	'AB'
	dd	'ABC'
	dd	'ABCD'
	dd	"A"
	dd	"AB"
	dd	"ABC"
	dd	"ABCD"
	dd	10,20,30
