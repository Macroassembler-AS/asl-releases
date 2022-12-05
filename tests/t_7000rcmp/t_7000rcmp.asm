	cpu	sh7000

	; Registers are 'ordered', and SP is equal to R15:

	dc.b	r4 == r10	; 0
	dc.b	r4 <> r10	; 1
	dc.b	r4 <= r10	; 1
	dc.b	r4 < r10	; 1
	dc.b	r4 >= r10	; 0
	dc.b	r4 > r10	; 0

	dc.b	r15 == sp	; 1
	dc.b	r15 <> sp	; 0
	dc.b	r15 <= sp	; 1
	dc.b	r15 < sp	; 0
	dc.b	r15 >= sp	; 1
	dc.b	r15 > sp	; 0
