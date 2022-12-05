		cpu	hd641016
		page	0

		; comparison within same register types:

		dc.b	r12 = r14	; 0
		dc.b	r12 <> r14	; 1
		dc.b	r12 < r14	; 1
		dc.b	r12 <= r14	; 1
		dc.b	r12 > r14	; 0
		dc.b	r12 >= r14	; 0

		dc.b	cr12 = cr14	; 0
		dc.b	cr12 <> cr14	; 1
		dc.b	cr12 < cr14	; 1
		dc.b	cr12 <= cr14	; 1
		dc.b	cr12 > cr14	; 0
		dc.b	cr12 >= cr14	; 0

		dc.b	pr12 = pr14	; 0
		dc.b	pr12 <> pr14	; 1
		dc.b	pr12 < pr14	; 1
		dc.b	pr12 <= pr14	; 1
		dc.b	pr12 > pr14	; 0
		dc.b	pr12 >= pr14	; 0

		; comparison of different types of registers:

		dc.b	r12 = pr14	; 0
		dc.b	r12 <> pr14	; 1
		dc.b	r12 < pr14	; 0
		dc.b	r12 <= pr14	; 0
		dc.b	r12 > pr14	; 0
		dc.b	r12 >= pr14	; 0

		dc.b	r12 = cr14	; 0
		dc.b	r12 <> cr14	; 1
		dc.b	r12 < cr14	; 0
		dc.b	r12 <= cr14	; 0
		dc.b	r12 > cr14	; 0
		dc.b	r12 >= cr14	; 0

		dc.b	pr12 = cr14	; 0
		dc.b	pr12 <> cr14	; 1
		dc.b	pr12 < cr14	; 0
		dc.b	pr12 <= cr14	; 0
		dc.b	pr12 > cr14	; 0
		dc.b	pr12 >= cr14	; 0
