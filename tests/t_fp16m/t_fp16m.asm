	cpu	68000

	; NOTE: The test values commented out work
        ; on a "typical" x86 system, but may not on
        ; others due to rounding errors

;	dc.c	5.96046447754e-8	; smallest positive subnormal number (0001)
;	dc.c	0.000060975552		; largest subnormal number (03ff)
;	dc.c	0.000061035156		; smallest positive normal number (0400)
	dc.c	65504.0			; largest normal number (7bff)
;	dc.c	0.99951172		; largest number less than one (3bff)
	dc.c	1.0			; one (3c00)
;	dc.c	1.00097656		; smallest number larger than one (3c01)
;	dc.c	0.33333333333333	; the rounding of 1/3 to nearest (3555)
	dc.c	-2.0			; minus two (c000)
	dc.c	0.0			; zero (0000)
