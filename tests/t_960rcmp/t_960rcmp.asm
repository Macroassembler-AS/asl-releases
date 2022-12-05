	cpu	80960
	fpu	on

	; Data registers are 'ordered'. 
        ; Registers 0, 1, 2, and 31 are equal to pfp, sp, rip and fp:

	db	r4 == g2	; 0
	db	r4 <> g2	; 1
	db	r4 <= g2	; 1
	db	r4 < g2		; 1
	db	r4 >= g2	; 0
	db	r4 > g2		; 0

	db	r1 == sp	; 1
	db	r1 <> sp	; 0
	db	r1 <= sp	; 1
	db	r1 < sp		; 0
	db	r1 >= sp	; 1
	db	r1 > sp		; 0

	; Floating point registers are in a different 'dimension', so there is
	; no lesser/greater:

	db	fp2 == r2	; 0
	db	fp2 <> r2	; 1
	db	fp2 <= r2	; 0
	db	fp2 < r2	; 0
	db	fp2 >= r2	; 0
	db	fp2 > r2	; 0
