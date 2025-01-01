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
	ifdef	_MOTO_REP
	dn	[3] (1,2,3,4)
	dn	[4] (1)
	dn	[4] (1,2)
	dn	[4] (1,2,3)
	dn	[4] (1,2,3,4)
	endif

	expect	360
	dn	0
	endexpect
	expect	360
	dn	15
	endexpect
	expect	1320
	dn	16
	endexpect
	expect	360
	dn	-8
	endexpect
	expect	1320
	dn	-9
	endexpect
