	cpu	z80

	org	1000h
	jr	$+129		; OK
	jr	$-126		; OK

	expect	1370,1370
	jr	$+130		; too far
	jr	$-127		; too far
	endexpect

	expect	25,25
	jp	$+129		; JR possible
	jp	$-126		; JR possible
	endexpect

	jp	$+130		; JR not possible
	jp	$-127		; JR not possible

	cpu	z380
	extmode	on
	phase	80000000h

	jr	$+129		; OK
	jr	$-126		; OK
	jr	$+130		; OK for Z380 (16 bit displacement)
	jr	$-127		; OK for Z380 (16 bit displacement)
	jr	$+8003h		; OK for Z380 (just 16 bit displacement)
	jr	$-7ffch		; OK for Z380 (just 16 bit displacement)
	jr	$+8004h		; OK for Z380 (24 bit displacement)
	jr	$-7ffdh		; OK for Z380 (24 bit displacement)
	jr	$+800004h	; OK for Z380 (just 24 bit displacement)
	jr	$-7ffffbh	; OK for Z380 (just 24 bit displacement)
	expect  1370,1370
	jr	$+800005h	; too far
	jr	$-7ffffch	; too far
	endexpect

	expect	25,25
	jp	$+800004h	; JR possible
	jp	$-7ffffbh	; JR possible
	endexpect

	jp	$+800005h	; JR not possible
	jp	$-7ffffch	; JR not possible
