	cpu	v55
	page	0

	mov	bx,ds3		; 8C F3
	mov	[bx+di],ds3	; 8C 31
	mov	bx,ds2		; 8C FB
	mov	[bx+di],ds2	; 8C 39

	mov	ds3,bx		; 8E F3
	mov	ds3,[bx+di]	; 8E 31
	mov	ds2,bx		; 8E FB
	mov	ds2,[bx+di]	; 8E 39

	push	ds3		; 0F 76
	push	ds2		; 0F 7E

	pop	ds3		; 0F 77
	pop	ds2		; 0F 7F

	lds3	si,[1234h]	; 0F 36 36 34 12
	mov	ds3,si,[1234h]	; 0F 36 36 34 12
	lds2	di,[bp+si+56h]	; 0F 3E 7A 56
	mov	ds2,di,[bp+si+56h] ; 0F 3E 7A 56

	segds2			; 63
	segds3			; D6
	iram			; F1

	bsch	cl
	bsch	dx
	bsch	byte ptr [1234h]
	bsch	word ptr [bp+si+56h]

	rstwdt	12h,65h		; 0F 96 12 65

	btclrl	12h,65h,$+10h	; 0F 9D 12 65 0B

	qhout	1234h		; 0F E0 34 12
	qout	1234h		; 0F E1 34 12
	qtin	1234h		; 0F E2 34 12

	cpu	v55sc

	idle			; 0F 9F

	cpu	v55pi

	albit 			; 0F 9A
	coltrp			; 0F 9B
	mhenc 			; 0F 93
	mrenc 			; 0F 97
	scheol			; 0F 78
	getbit			; 0F 79
	mhdec 			; 0F 7C
	mrdec 			; 0F 7D
	cnvtrp			; 0F 7A
	