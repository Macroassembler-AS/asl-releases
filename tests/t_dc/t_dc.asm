	cpu	68020
	page	0

	irp	op,b,w,l,q,s,d,x,p
	dc.op	[3]25
	dc.op	"Hello World"
	ds.op	9
	endm

	; since the precision of PI depends on the host system,
	; disposing it with highest-possible resolution may result
	; in data that differs in the lowest bits.  So better
	; use a constant that can be represented precisely in binary:

my_pi	equ	3.140625	; = 2 + 1 + 1/8 + 1/64
	irp	op,s,d,x,p
	dc.op	1.5
	dc.op	my_pi*1024
        dc.op	my_pi/1024
        dc.op   -my_pi
	endm

	padding	off

	dc.b	0
	dc.b	255
	dc.b	-128
	dc.b	'A'
	dc.b	'AB'	; treated like "AB"
	dc.b	'ABC'	; treated like "ABC"
	dc.b	'ABCD'	; treated like "ABCD"
	dc.b	"A"
	dc.b	"AB"
	dc.b	"ABC"
	dc.b	"ABCD"

	dc.w	0
	dc.w	65535
	dc.w	-32768
	dc.w	'A'
	dc.w	'AB'
	dc.w	'ABC'	; treated like "ABC"
	dc.w	'ABCD'	; treated like "ABCD"
	dc.w	"A"
	dc.w	"AB"
	dc.w	"ABC"
	dc.w	"ABCD"

	dc.l	0
	dc.l	4294967295
	dc.l	-2147483648
	dc.l	'A'
	dc.l	'AB'
	dc.l	'ABC'
	dc.l	'ABCD'
	dc.l	"A"
	dc.l	"AB"
	dc.l	"ABC"
	dc.l	"ABCD"

	dc.q	0
	dc.q	2147483647
	dc.q	-2147483648
	dc.q	'A'
	dc.q	'AB'
	dc.q	'ABC'
	dc.q	'ABCD'
	dc.q	"A"
	dc.q	"AB"
	dc.q	"ABC"
	dc.q	"ABCD"

	dc.s	0
	dc.s	2147483647
	dc.s	-2147483648
	dc.s	'A'
	dc.s	'AB'
	dc.s	'ABC'
	dc.s	'ABCD'
	dc.s	"A"
	dc.s	"AB"
	dc.s	"ABC"
	dc.s	"ABCD"

	dc.d	0
	dc.d	2147483647
	dc.d	-2147483648
	dc.d	'A'
	dc.d	'AB'
	dc.d	'ABC'
	dc.d	'ABCD'
	dc.d	"A"
	dc.d	"AB"
	dc.d	"ABC"
	dc.d	"ABCD"

	dc.x	0
	dc.x	2147483647
	dc.x	-2147483648
	dc.x	'A'
	dc.x	'AB'
	dc.x	'ABC'
	dc.x	'ABCD'
	dc.x	"A"
	dc.x	"AB"
	dc.x	"ABC"
	dc.x	"ABCD"

	dc.p	0
	dc.p	2147483647
	dc.p	-2147483648
	dc.p	'A'
	dc.p	'AB'
	dc.p	'ABC'
	dc.p	'ABCD'
	dc.p	"A"
	dc.p	"AB"
	dc.p	"ABC"
	dc.p	"ABCD"
