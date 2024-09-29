	cpu	z80

x1	equ	1
x3	equ	3

	if	defined(x1) || defined(x2)
	db	12h
	endif

	if	defined(x1) || defined(x3)
	db	13h
	endif

	if	defined(x1) || defined(x4)
	db	14h
	endif

	if	defined(x2) || defined(x3)
	db	23h
	endif

	if	defined(x2) || defined(x4)
	db	24h
	endif

	if	defined(x3) || defined(x4)
	db	34h
	endif

back:	nop
	jr	back

myfunc	function x,x*x

	db	defined(back)	; has been defined previously -> 1
	db	defined(back+5)	; -> 1
	db	defined(forw)	; is defined later -> 0
	db	defined(forw-2)	; -> 0
	db	defined(back+forw)	; contains undefined symbols -> 0
	db	defined('[')	; plain constant is always defined -> 1
	db	defined(forww)	; never defined -> 0
	db	defined($)	; built-in symbol -> 1
	db	defined(exp(1))	; -> 1
	db	defined(exp(forw))	; -> 0
	db	defined(myfunc(1))	; -> 1
	db	defined(myfunc(forw))	; -> 0

	jr	forw
	nop
forw	nop
