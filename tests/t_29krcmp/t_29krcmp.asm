		cpu	am29000
		page	0

		; comparison within same register types:

		db	r12 = r14	; 0
		db	r12 <> r14	; 1
		db	r12 < r14	; 1
		db	r12 <= r14	; 1
		db	r12 > r14	; 0
		db	r12 >= r14	; 0

		db	gr12 = gr14	; 0
		db	gr12 <> gr14	; 1
		db	gr12 < gr14	; 1
		db	gr12 <= gr14	; 1
		db	gr12 > gr14	; 0
		db	gr12 >= gr14	; 0

		db	lr12 = lr14	; 0
		db	lr12 <> lr14	; 1
		db	lr12 < lr14	; 1
		db	lr12 <= lr14	; 1
		db	lr12 > lr14	; 0
		db	lr12 >= lr14	; 0

		; comparison of different types of registers:

		db	r12 = lr14	; 0
		db	r12 <> lr14	; 1
		db	r12 < lr14	; 0
		db	r12 <= lr14	; 0
		db	r12 > lr14	; 0
		db	r12 >= lr14	; 0

		db	r12 = gr14	; 0
		db	r12 <> gr14	; 1
		db	r12 < gr14	; 0
		db	r12 <= gr14	; 0
		db	r12 > gr14	; 0
		db	r12 >= gr14	; 0

		db	lr12 = gr14	; 0
		db	lr12 <> gr14	; 1
		db	lr12 < gr14	; 0
		db	lr12 <= gr14	; 0
		db	lr12 > gr14	; 0
		db	lr12 >= gr14	; 0
