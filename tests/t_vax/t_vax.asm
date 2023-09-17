		cpu	vax-11/780
		page	0

pcr		equ	pc
spr		equ	sp
fpr		equ	fp
apr		equ	ap

r_0		equ	r0
r_1		equ	r1
r_2		equ	r2
r_3		equ	r3
r_4		equ	r4
r_5		equ	r5
r_6		equ	r6
r_7		equ	r7
r_8		equ	r8
r_9		equ	r9
r_10		equ	r10
r_11		equ	r11
r_12		equ	r12
r_13		equ	r13
r_14		equ	r14
r_15		equ	r15

		halt			; 00
		nop			; 01

		movb	r4,r5		; 90 54 55
		movb	@r4,r5		; 90 64 55
		movb	(r4),r5		; 90 64 55
		movb	@(r4),r5	; 90 B4 00 55
		movb	-(r4),r5	; 90 74 55
		movb	(r4)+,r5	; 90 84 55
		movb	@(r4)+,r5	; 90 94 55
		movb	4(r4),r5	; 90 A4 04 55
		movb	@4(r4),r5	; 90 B4 04 55
		movb	b^4(r4),r5	; 90 A4 04 55
		movb	@b^4(r4),r5	; 90 B4 04 55
		movb	w^4(r4),r5	; 90 C4 04 00 55
		movb	@w^4(r4),r5	; 90 D4 04 00 55
		movb	l^4(r4),r5	; 90 E4 04 00 00 00 55
		movb	@l^4(r4),r5	; 90 F4 04 00 00 00 55
		expect	1107,1107
		movb	s^4(r4),r5	; invalid attribute
		movb	@s^4(r4),r5	; invalid attribute
		endexpect
		movb	444(r4),r5	; 90 C4 BC 01 55
		movb	@444(r4),r5	; 90 D4 BC 01 55
		expect	1320,1320
		movb	b^444(r4),r5	; out of range
		movb	@b^444(r4),r5	; out of range
		endexpect
		movb	w^444(r4),r5	; 90 C4 BC 01 55
		movb	@w^444(r4),r5	; 90 D4 BC 01 55
		movb	l^444(r4),r5	; 90 E4 BC 01 00 00 55
		movb	@l^444(r4),r5	; 90 F4 BC 01 00 00 55
		movb	444444(r4),r5	; 90 E4 1C C8 06 00 55
		movb	@444444(r4),r5	; 90 F4 1C C8 06 00 55
		expect	1320,1320,1320,1320
		movb	b^444444(r4),r5
		movb	@b^444444(r4),r5
		movb	w^444444(r4),r5
		movb	@w^444444(r4),r5
		endexpect
		movb	l^444444(r4),r5 ; 90 E4 1C C8 06 00 55 
		movb	@l^444444(r4),r5 ; 90 E4 1C C8 06 00 55
		movb	#4,r5		; 90 04 55
		movb	s^#4,r5		; 90 04 55
		movb	i^#4,r5		; 90 8F 04 55
		movb	#88,r5		; 90 8F 58 55
		expect	1107
		movb	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		movb	s^#88,r5	; out of range 0..63
		endexpect
		movb	i^#88,r5	; 90 8F 58 55
		movb	@#44,r5		; 90 9F 2C 00 00 00 55
		movb	@#444,r5	; 90 9F BC 01 00 00 55
		movb	@#444444,r5	; 90 9F 1C C8 06 00 55
		movb	*+44,r5		; 90 AF 29 55
		movb	b^*+44,r5	; 90 AF 29 55
		movb	w^*+44,r5	; 90 CF 28 00 55
		movb	l^*+44,r5	; 90 EF 26 00 00 00 55
		movb	*+444,r5	; 90 CF B8 01 55
		expect 1320
		movb	b^*+444,r5	; out of range
		endexpect
		movb	w^*+444,r5	; 90 CF B8 01 55
		movb	l^*+444,r5	; 90 EF B6 01 00 00 55
		movb	*+444444,r5	; 90 EF 16 C8 06 00 55
		expect	1320,1320
		movb	b^*+444444,r5	; out of range
		movb	w^*+444444,r5	; out of range
		endexpect
		movb	l^*+444444,r5	; 90 EF 16 C8 06 00 55
		expect	1107
		movb	s^*+444444,r5	; invalid attribute
		endexpect
		expect	1350
		movb	r4[r6],r5	; not allowed
		endexpect
		movb	(r4)[r6],r5	; 90 46 64 55
		movb	(r4)+[r6],r5	; 90 46 84 55
		movb	@(r4)+[r6],r5	; 90 46 94 55
		movb	-(r4)[r6],r5	; 90 46 74 55
		movb	@#444[r6],r5	; 90 46 9F BC 01 00 00 55
		movb	@4(r4)[r6],r5	; 90 46 B4 04 55
		movb	*+44[r6],r5	; 90 46 AF 28 55
		movb	*+444[r6],r5	; 90 46 CF B7 01 55
		movb	*+444444[r6],r5	; 90 46 EF 15 C8 06 00 55

		movw	r4,r5		; B0 54 55
		movw	@r4,r5		; B0 64 55
		movw	(r4),r5		; B0 64 55
		movw	@(r4),r5	; B0 B4 00 55
		movw	-(r4),r5	; B0 74 55
		movw	(r4)+,r5	; B0 84 55
		movw	@(r4)+,r5	; B0 94 55
		movw	4(r4),r5	; B0 A4 04 55
		movw	@4(r4),r5	; B0 B4 04 55
		movw	444(r4),r5	; B0 C4 BC 01 55
		movw	@444(r4),r5	; B0 D4 BC 01 55
		movw	444444(r4),r5	; B0 E4 1C C8 06 00 55
		movw	@444444(r4),r5	; B0 F4 1C C8 06 00 55
		movw	#4,r5		; B0 04 55
		movw	#444,r5		; B0 8F BC 01 55
		movw	@#444444,r5	; B0 9F 1C C8 06 00 55
		movw	*+44,r5		; B0 AF 29 55
		movw	*+444,r5	; B0 CF B8 01 55
		movw	*+444444,r5	; B0 EF 16 C8 06 00 55
		movw	(r4)[r6],r5	; B0 46 64 55
		movw	(r4)+[r6],r5	; B0 46 84 55
		movw	@(r4)+[r6],r5	; B0 46 94 55
		movw	-(r4)[r6],r5	; B0 46 74 55
		movw	@#444444[r6],r5	; B0 46 9F 1C C8 06 00 55
		movw	@4(r4)[r6],r5	; B0 46 B4 04 55

		addl3	(r3)[r4],737(r2),r1 ; C1 44 63 C2 E1 02 51
