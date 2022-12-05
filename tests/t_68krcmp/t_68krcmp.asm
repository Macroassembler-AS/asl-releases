	cpu	68020
	fpu	on
	padding	off

	; Data and address registers are 'ordered'.  Address registers are treated
        ; as registers 8..15, and SP is equal to A7:

	dc.b	d4 == a2	; 0
	dc.b	d4 <> a2	; 1
	dc.b	d4 <= a2	; 1
	dc.b	d4 < a2		; 1
	dc.b	d4 >= a2	; 0
	dc.b	d4 > a2		; 0

	dc.b	a7 == sp	; 1
	dc.b	a7 <> sp	; 0
	dc.b	a7 <= sp	; 1
	dc.b	a7 < sp		; 0
	dc.b	a7 >= sp	; 1
	dc.b	a7 > sp		; 0

	; Floating point registers are in a different 'dimension', so there is
	; no lesser/greater:

	dc.b	fp4 == a2	; 0
	dc.b	fp4 <> a2	; 1
	dc.b	fp4 <= a2	; 0
	dc.b	fp4 < a2	; 0
	dc.b	fp4 >= a2	; 0
	dc.b	fp4 > a2	; 0

	; The same is true for the FPU control registers:

	dc.b	fp4 == fpcr	; 0
	dc.b	fp4 <> fpcr	; 1
	dc.b	fp4 <= fpcr	; 0
	dc.b	fp4 < fpcr	; 0
	dc.b	fp4 >= fpcr	; 0
	dc.b	fp4 > fpcr	; 0
