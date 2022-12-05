		cpu	8051

src		reg	r4
src2		reg	src
dest		reg	r5
dest2		reg	r5

		db	src == dest	; 0
		db	src <> dest	; 1
		db	src < dest	; 1
		db	src <= dest	; 1
		db	src > dest	; 0
		db	src >= dest	; 0

		db	src2 == src	; 1
		db	src2 <> src	; 0
		db	src2 < src	; 0
		db	src2 <= src	; 1
		db	src2 > src	; 0
		db	src2 >= src	; 1

		db	dest2 == dest	; 1
		db	dest2 <> dest	; 0
		db	dest2 < dest	; 0
		db	dest2 <= dest	; 1
		db	dest2 > dest	; 0
		db	dest2 >= dest	; 1
