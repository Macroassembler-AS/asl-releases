		ifndef	bcdicinc
bcdicinc	equ	1

		save		; no listing over this file
		listing	off

		; this translates the ASCII characters 0...127
		; to their BCDIC (codepage 359) representation, as far as possible.
		; codepage359 is the closest I could find that matches the (lower shift)
                ; character set listend in the IBM 5100 Comnunications Reference Manual.
		; It is probably NOT the character set used internally by the 5100, which
		; is said to be some variant of EBCDIC.

		; NOTE: The save/restore mechanism will assure that after inclusion
                ; of this file, you still use the same character mapping as you
                ; did before.  To activate this mapping, place a 'codepage cp359'
		; in your code!

		codepage cp359

		; unused codes: 12,13,28,30,31,42,47,58,60,63
                ; where to map 'Start of Transmission' (11, same as '#', maybe 13 instead) ?

__bcdic_none	equ	64		; value not representable in 6 bits
		charset
		charset	0, __bcdic_none	; NUL
		charset	1, __bcdic_none	; SOH
		charset	2, __bcdic_none	; STX
		charset	3, __bcdic_none	; ETX
		charset	4, 15		; EOT
		charset	5, __bcdic_none	; ENQ
		charset	6, __bcdic_none	; ACK
		charset	7, __bcdic_none	; BEL
		charset	8, 46		; BS
		charset	9, 61		; HT
		charset	10, 29		; LF
		charset	11, __bcdic_none; VT
		charset	12, __bcdic_none; FF
		charset	13, 45		; CR->NL?
		charset	14, 14		; SO->USH?
		charset	15, 62		; SI->LSH?
		charset	16, __bcdic_none; DLE
		charset	17, __bcdic_none; DC1
		charset	18, __bcdic_none; DC2
		charset	19, __bcdic_none; DC3
		charset	20, __bcdic_none; DC4
		charset	21, __bcdic_none; NAK
		charset	22, __bcdic_none; SYN
		charset	23, __bcdic_none; ETB
		charset	24, 44		; CAN->RST?
		charset	25, __bcdic_none; EM
		charset	26, __bcdic_none; SUB
		charset	27, __bcdic_none; ESC
		charset	28, __bcdic_none; FS
		charset	29, __bcdic_none; GS
		charset	30, __bcdic_none; RS
		charset	31, __bcdic_none; US
		charset	' ', 0
		charset '!', __bcdic_none
		charset '"', __bcdic_none
		charset	'#', 11
		charset	'$', 43
		charset	'%', __bcdic_none
		charset	'&', 48
		charset	'\'', __bcdic_none
		charset	'(', __bcdic_none
		charset	')', __bcdic_none
		charset	'*', __bcdic_none
		charset	'+', __bcdic_none
		charset	',', 27
		charset	'-', 32
		charset	'.', 59
		charset	'/', 17
		charset	'0', 10
		charset	'1', '9', 1
		charset	':', __bcdic_none
		charset	';', __bcdic_none
		charset	'<', __bcdic_none
		charset	'=', __bcdic_none
		charset	'>', __bcdic_none
		charset	'?', __bcdic_none
		charset	'@', 16
		charset	'A', 'I', 49
		charset 'J', 'R', 33
		charset	'S', 'Z', 18
		charset	'[', __bcdic_none
		charset	'\\', __bcdic_none
		charset	']', __bcdic_none
		charset	'^', __bcdic_none
		charset	'_', __bcdic_none
		charset	'`', __bcdic_none
		charset	'a', 'i', __bcdic_none
		charset 'j', 'r', __bcdic_none
		charset	's', 'z', __bcdic_none
		charset	'{', __bcdic_none
		charset	'|', __bcdic_none
		charset	'}', __bcdic_none
		charset	'~', __bcdic_none
		charset	127, __bcdic_none; DEL

		restore		; restore previous listing state and code page

		endif		; bcdicinc
