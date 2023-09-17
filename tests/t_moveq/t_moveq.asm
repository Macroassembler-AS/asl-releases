	cpu	68000

	moveq	#0,d2
	moveq	#$7f,d2
	moveq	#-1,d2
	moveq	#$ffffffc0,d2   ; equiv. to -64
	expect	430
	moveq	#$ff,d2		; allowed with warning
	endexpect
	expect	1315
	moveq	#-1000,d2	; expect range underflow
	endexpect
	expect	1320
	moveq	#256,d2		; expect range overflow
	endexpect
