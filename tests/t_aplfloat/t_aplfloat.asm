	cpu	6502
	page	0

	; Note that since the mantissa is in in two's complement
	; representation, its range is asymmetric towards zero.  It
	; can represent -2.0, but not +2.0.  So if the absolute
	; value is an exact power of two, the exponent for the
	; negative number is one less:

	dcm	1.0		; 80 40 00 00 -> 1.0 * 2^0
	dcm	10.0		; 83 50 00 00 -> 1.25 * 2^3
	dcm	-1.0		; 7F 80 00 00 -> -2.0 * 2^-1
	dcm	128.0		; 87 40 00 00 -> 1.0 * 2^7
	dcm	-0.125		; 7c 80 00 00 -> -2.0 * 2^-4

	dcm	+3		; 81 60 00 00 -> 1.5 * 2^1
	dcm	+4		; 82 40 00 00 -> 1.0 * 2^2
	dcm	+5		; 82 50 00 00 -> 1.125 * 2^2
	dcm	+7		; 82 70 00 00 -> 1.75 * 2^2
	dcm	+12		; 83 60 00 00 -> 1.5 * 2^3
	dcm	+15		; 83 78 00 00 -> 1.875 * 2^3
	dcm	+17		; 84 44 00 00 -> 1.0625 * 2^4
	dcm	+20		; 84 50 00 00 -> 1.25 * 2^4
	dcm	+60		; 85 78 00 00 -> 1.875 * 2^5

	dcm	-3		; 81 A0 00 00 -> -1.5 * 2^1
	dcm	-4		; 81 80 00 00 -> -1.0 * 2^2
	dcm	-5		; 82 B0 00 00 -> -1.125 * 2^2
	dcm	-7		; 82 90 00 00 -> -1.75 * 2^2
	dcm	-12		; 83 A0 00 00 -> -1.5 * 2^3
	dcm	-15		; 83 88 00 00 -> -1.875 * 2^3
	dcm	-17		; 84 BC 00 00 -> -1.0625 * 2^4
	dcm	-20		; 84 B0 00 00 -> -1.25 * 2^4
	dcm	-60		; 85 88 00 00 -> -1.875 * 2^5


	;dcm	0.4342945	; ln(10)  : 7E 6F 2D ED
	;dcm	1.4142136	; sqrt(2) : 80 5A 82 7A
	;dcm	0.69314718	; ln(2)   : 7F 58 B9 0C
	;dcm	1.2920074	; a1      : 80 52 B0 40
	;dcm	-2.6398577	; mb      : 81 AB 86 49
	;dcm	1.6567626	; c       : 80 6A 08 66
	dcm	0.5		; 1/2     : 7F 40 00 00 -> 1.0 * 2^-1
	;dcm	1.4426950409	; ld(e)   : 80 5C 55 1E
	;dcm	87.417497202	; a2      : 86 57 6A E1
	;dcm	617.9722695	; b2      : 89 4D 3F 1D
	;dcm	.03465735903	; c2      : 7B 46 FA 70
	;dcm	9.9545957821	; d       : 83 4F A3 03



	