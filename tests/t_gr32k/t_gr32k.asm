	cpu	ns32cg16
	supmode	on
	page	0

	; not implemented on 32CG16

	expect	1500
	movsuw	(r4),(r5)
	endexpect

	bband		; 0E 2B 01
	bband	s	; 0E 2B 01
	bband	ia	; 0E 2B 01
	bband	s,ia	; 0E 2B 01
	bband	-s	; 0E AB 01
	bband	da	; 0E 2B 03
	bband	-s,da	; 0E AB 03
	expect	2254
	bband	s,s	; duplicate opt
	endexpect
	expect	2254
	bband	s,-s	; contradicting opt
	endexpect
	expect	2254
	bband	da,ia	; contradicting opt
	endexpect
	bbor	-s,da	; 0E 99 02
	bbxor	s,da	; 0E 39 02
	bbfor	s,ia	; 0E 31 00
	expect	2255
	bbfor	-s	; -S not allowed for BBFOR
	endexpect
	expect	2255
	bbfor	da	; DA not allowed for BBFOR
	endexpect
	bbstod	-s,da	; 0E 91 02

	bitwt		; 0E 21 00
	extblt		; 0E 17 00
	movmpb		; 0E 1C 00
	movmpw		; 0E 1D 00
	movmpd		; 0E 1F 00
	tbits	0	; 0E 27 00
	tbits	1	; 0E A7 00
	expect	1320
	tbits	2
	endexpect
