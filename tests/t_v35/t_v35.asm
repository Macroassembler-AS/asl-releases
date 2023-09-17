	cpu	v35
	page 	0

	movspa		; 0F 25

	brkcs	ax	; 0F 2D C0
	brkcs	cx	; 0F 2D C1
	brkcs	dx	; 0F 2D C2
	brkcs	bx	; 0F 2D C3
	brkcs	sp	; 0F 2D C4
	brkcs	bp	; 0F 2D C5
	brkcs	si	; 0F 2D C6
	brkcs	di	; 0F 2D C7

	retrbi		; 0F 91

	fint		; 0F 92

	tsksw	ax	; 0F 94 F8
	tsksw	cx	; 0F 94 F9
	tsksw	dx	; 0F 94 FA
	tsksw	bx	; 0F 94 FB
	tsksw	sp	; 0F 94 FC
	tsksw	bp	; 0F 94 FD
	tsksw	si	; 0F 94 FE
	tsksw	di	; 0F 94 FF

	movspb	ax	; 0F 95 F8
	movspb	cx	; 0F 95 F9
	movspb	dx	; 0F 95 FA
	movspb	bx	; 0F 95 FB
	movspb	sp	; 0F 95 FC
	movspb	bp	; 0F 95 FD
	movspb	si	; 0F 95 FE
	movspb	di	; 0F 95 FF
	
	btclr	12h,7,$+16	; 0F 9C 12 07 0B

	stop		; 0F 9E

	brks	23h	; F1 23
	brkn	23h	; 63 23 

	cpu	v53

	brkxa	23h	; 0F E0 23
	retxa	23h	; 0F F0 23
