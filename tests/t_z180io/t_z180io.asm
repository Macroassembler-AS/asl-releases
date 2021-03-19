	cpu	z180

	; dummy value to avoid different errors in pass 1 & 2
f	equ	10

	in0	a,(12h)
	in0	(12h)
	in0	f,(12h)
	in0	b,(12h)
	in0	c,(12h)
	in0	d,(12h)
	in0	e,(12h)
	in0	h,(12h)
	in0	l,(12h)

	out0	(12h),a
	expect	1110		; no implicit F reg for OUT0
	out0	(12h)
	endexpect
	expect	1350		; F not allowed as source for OUT0
	out0	(12h),f
	endexpect
	out0	(12h),b
	out0	(12h),c
	out0	(12h),d
	out0	(12h),e
	out0	(12h),h
	out0	(12h),l

	in	a,(c)
	expect	1500		; implicit F arg only allowed for Z80UNDOC
	in	(c)
	endexpect
	in	f,(c)
	in	b,(c)
	in	c,(c)
	in	d,(c)
	in	e,(c)
	in	h,(c)
	in	l,(c)

	out	(c),a
	expect	1110		; no implicit F reg for OUT0
	out	(c)
	endexpect
	expect	1500
	out	(c),f		; F not allowed for Z180 as reg, may be address on Z380
	endexpect
	out	(c),b
	out	(c),c
	out	(c),d
	out	(c),e
	out	(c),h
	out	(c),l
