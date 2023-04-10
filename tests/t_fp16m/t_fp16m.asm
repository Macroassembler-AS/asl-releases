	cpu	68000

	; NOTE: We compute the values instead of writing
	; out the decimal notation, to avoid rounding
	; errors when converting ASCII to binary:

	dc.c	1.0/16777216		; smallest positive subnormal number (0001, 2^-24, ~5.96046447754e-8)
	dc.c	1.0/16384*(1023.0/1024)	; largest subnormal number (03ff, 2^-14+(1023/1024), ~0.000060975552)
	dc.c	1.0/16384		; smallest positive normal number (0400, 2^-14, ~0.000061035156)
	dc.c	65504.0			; largest normal number (7bff)
	dc.c	0.5*(1.0+1023.0/1024)	; largest number less than one (3bff, ~0.99951172)
	dc.c	1.0			; one (3c00)
	dc.c	(1.0+1.0/1024.0)	; smallest number larger than one (3c01, ~1.00097656)
	dc.c	1.0/3.0			; the rounding of 1/3 to nearest (3555)
	dc.c	-2.0			; minus two (c000)
	dc.c	0.0			; zero (0000)
