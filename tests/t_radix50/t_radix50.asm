	cpu	micropdp-11/93
	page	0

	; Load RADIX 50 code page and the macro's definition:

	include	"radix50.inc"

	; RADIX 50 packs three characters into one machine word,
	; with the encoding ch[0]*40^2 + ch[1]*40^1 + ch[2]*40^0.
	; So the expected outcome is
	; 1*40^2 + 2*40^1 + 3*40^0 = 1683 = 0x693,
	; and 4*40^2 + 5*40^1 + 6*40^0 = 6606 = 0x19ce:

	radix50	"ABCDEF"

	; If the number of characters is not a multiple of three,
	; the string is padded with spaces, which encode as zero.
	; So the expected outcome is
	; 30*40^2 + 0*40^1 + 0*40^0 = 48000 = 0xbb80:

	radix50	"0"

	; ...and here, it is
	; 27*40^2 + 28*40^1 + 0*40^0 = 44320 = 0xad20:

	radix50	"$."

	; unencodable characters should produce an error:

	expect	9990
	radix50 "ABc"
	endexpect

