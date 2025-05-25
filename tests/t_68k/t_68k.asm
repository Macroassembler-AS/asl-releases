	cpu	68020
	page	0

	bftst	d1{31:32}	; E8C1 07C0
	bfchg	d1{31:32}	; EAC1 07C0
	bfclr	d1{31:32}	; ECC1 07C0
	bfset	d1{31:32}	; EEC1 07C0

	; Note that the Dh field in MULU.L/MULS.L is don't care
        ; if dest op size is 32 bits, DIVS.L/DIVU.L always
	; regard the respective Dr field, even if the divident
	; is only 32 bits.  If no remainder is desired, Dr must
	; be set to the same value as Dq, since writing quotient
	; has priority.  Since ASL handles all four instructions
	; with the same code, Dh was simply set to Dl in versions
	; prior to 1.42 Bld 287.  Ths was principally OK, but
	; considered 'dirty':

	divs.l	d4,d1		; 4C44 1801
	divs.l	d4,d2:d1	; 4C44 1C02
	divs.l	d4,d2:d2	; 4C44 2C02 (quotient is stored in D2)

	divu.l	d4,d1		; 4C44 1001
	divu.l	d4,d2:d1	; 4C44 1402
	divu.l	d4,d2:d2	; 4C44 2402 (quotient is stored in D2)

	muls.l	d2,d7		; 4C02 7800
	muls.l	d2,d7:d6	; 4C02 6C07
	expect	140
	muls.l	d2,d7:d7	; 4C02 7C07 (result undefined)
	endexpect

	mulu.l	d2,d7		; 4C02 7000
	mulu.l	d2,d7:d6	; 4C02 6407
	expect	140
	mulu.l	d2,d7:d7	; 4C02 7407 (result undefined)
	endexpect
