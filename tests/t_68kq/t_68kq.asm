	cpu	68000

	moveq	#0,d1
	moveq	#127,d2
	expect	430
	moveq	#128,d3
	endexpect
	moveq	#-128,d4
	expect	1315
	moveq	#-129,d5
	endexpect
	moveq	#$ffffff80,d6
	expect	1315
	moveq	#$ffffff7f,d7
	endexpect

	expect	1390
	addq	#0,d1
	endexpect
	addq	#1,d2
	addq	#8,d3
	subq	#7,a0
	expect	1390
	addq	#9,d4
	endexpect
	expect	1390
	subq    #15,a0
	endexpect
	expect	1390
	addq	#16,d5
	endexpect
