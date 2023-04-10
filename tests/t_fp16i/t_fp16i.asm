	cpu	8086

	; NOTE: We compute the values instead of writing
        ; out the decimal notation, to avoid rounding
	; errors when converting ASCII to binary:

	dw	1.0/16777216		; smallest positive subnormal number (01 00, 2^-24, ~5.96046447754e-8)
	dw	1.0/16384*(1023.0/1024)	; largest subnormal number (ff 03, 2^-14+(1023/1024), ~0.000060975552)
	dw	1.0/16384		; smallest positive normal number (00 04, 2^-14, ~0.000061035156)
	dw	65504.0			; largest normal number (ff 7b)
	dw	0.5*(1.0+1023.0/1024)	; largest number less than one (ff 3b, ~0.99951172)
	dw	1.0			; one (00 3c)
	dw	(1.0+1.0/1024.0)	; smallest number larger than one (01 3c, ~1.00097656)
	dw	1.0/3.0			; the rounding of 1/3 to nearest (55 35)
	dw	-2.0			; minus two (00 c0)
	dw	0.0			; zero (00 00)

;	dw	1.7e308*100		; positive infinity (00 7c)
;	dw	-1.7e308*100		; negative infinity (00 fc)
;	dw	sqrt(-2)		; NaN (00 fe, but won't assemble anyway...)

