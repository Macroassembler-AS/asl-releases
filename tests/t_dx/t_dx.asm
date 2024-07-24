	cpu	8086
	page	0

	segment	code

	org	0

	expect	360
	dn	1
	endexpect
	dn	1,2
	expect	360
	dn	1,2,3
	endexpect
	dn	1,2,3,4
	dn	2 dup (1)
	dn	2 dup (1,2)
	dn	2 dup (1,2,3)
	dn	2 dup (1,2,3,4)
	expect	360
	dn	3 dup (1)
	endexpect
	dn	3 dup (1,2)
	expect	360
	dn	3 dup (1,2,3)
	endexpect
	dn	3 dup (1,2,3,4)
	dn	4 dup (1)
	dn	4 dup (1,2)
	dn	4 dup (1,2,3)
	dn	4 dup (1,2,3,4)

	db	3 dup "abc"
	dw	1
	dw	3 dup (1)
	dw	"abc"
	dw	1 dup ("abc")
	dw	2 dup ("abc")
	dw	3 dup ("abc")
	dd	1
	dd	1.0
	dd	3 dup (1)
	dd	"abc"
	dd	1 dup ("abc")
	dd	2 dup ("abc")
	dd	3 dup ("abc")
	dq	1
	dq	1.0
	dq	3 dup (1)
	dq	"abc"
	dq	1 dup ("abc")
	dq	2 dup ("abc")
	dq	3 dup ("abc")
	dt	1
	dt	-1
	dt	-127
	dt	1.0
	dt	-127.0
	dt	3 dup (1)
	dt	'a','b','c'
	dt	"abc"
	dt	1 dup ("abc")
	dt	2 dup ("abc")
	dt	3 dup ("abc")

	expect	360
	dn	0
	endexpect
	expect	360
	dn	15
	endexpect
	expect	360
	dn	-8
	endexpect
	expect	1320
	dn	16
	endexpect

	db	0
	db	255
	db	-128
	db	'A'
	db	'AB'		; treat like "AB"
	db	'ABC'		; treat like "ABC"
	db	'ABCD'		; treat like "ABCD"
	db	"A"
	db	"AB"
	db	"ABC"
	db	"ABCD"

	dw	0
	dw	65535
	dw	-32768
	dw	'A'
	dw	'AB'
	dw	'ABC'		; treat like "ABC"
	dw	'ABCD'		; treat like "ABCD"
	dw	"A"
	dw	"AB"
	dw	"ABC"
	dw	"ABCD"

	dd	0
	dd	4294967295
	dd	-2147483648
	dd	'A'
	dd	'AB'
	dd	'ABC'
	dd	'ABCD'
	dd	"A"
	dd	"AB"
	dd	"ABC"
	dd	"ABCD"

	dq	0
	dq	2147483647
	dq	-2147483648
	dq	'A'
	dq	'AB'
	dq	'ABC'
	dq	'ABCD'
	dq	"A"
	dq	"AB"
	dq	"ABC"
	dq	"ABCD"

	dt	0
	dt	2147483647
	dt	-2147483648
	dt	'A'
	dt	'AB'
	dt	'ABC'
	dt	'ABCD'
	dt	"A"
	dt	"AB"
	dt	"ABC"
	dt	"ABCD"
