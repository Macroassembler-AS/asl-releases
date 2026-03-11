	cpu	iso650

	org	07f6h

	naa			; 45
	; avoid using JTB explicitly.  The assembler
	; will automatically insert it in front of a
	; CAL or JMP instruction if the target is
	; in a different 2K page.
	jtb			; 18

dest_0:	jmp	dest_0		; A7 F8
	jmp	dest_1		; 1C A0 02
	cal	dest_0		; AF F8
	cal	dest_1		; 1C A8 02
	; now in second 2K page:
dest_1:	jmp	dest_0		; 1C A7 F8
	jmp	dest_1		; A0 02
	cal	dest_0		; 1C AF F8
	cal	dest_1		; A8 02
