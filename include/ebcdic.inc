		ifndef	ebcdicinc
ebcdicinc	equ	1

		save		; no listing over this file
		listing	off

		; this translates the ASCII characters 0...127
		; to their EBCDIC (codepage 037) representation:

		; NOTE: The save/restore mechanism will assure that after inclusion
                ; of this file, you still use the same character mapping as you
                ; did before.  To activate this mapping, place a 'codepage cp037'
		; in your code!

		codepage cp037

		charset
		charset	0, 0		; NUL
		charset	1, 1		; SOH
		charset	2, 2		; STX
		charset	3, 3		; ETX
		charset	4, 55		; EOT
		charset	5, 45		; ENQ
		charset	6, 46		; ACK
		charset	7, 47		; BEL
		charset	8, 22		; BS
		charset	9, 5		; HT
		charset	10, 37		; LF
		charset	11, 11		; VT
		charset	12, 12		; FF
		charset	13, 13		; CR
		charset	14, 14		; SO
		charset	15, 15		; SI
		charset	16, 16		; DLE
		charset	17, 17		; DC1
		charset	18, 18		; DC2
		charset	19, 19		; DC3
		charset	20, 60		; DC4
		charset	21, 61		; NAK
		charset	22, 50		; SYN
		charset	23, 38		; ETB
		charset	24, 24		; CAN
		charset	25, 25		; EM
		charset	26, 63		; SUB
		charset	27, 39		; ESC
		charset	28, 28		; FS
		charset	29, 29		; GS
		charset	30, 30		; RS
		charset	31, 31		; US
		charset	' ', 64
		charset '!', 90
		charset '"', 127
		charset	'#', 123
		charset	'$', 91
		charset	'%', 108
		charset	'&', 80
		charset	'\'', 125
		charset	'(', 77
		charset	')', 93
		charset	'*', 92
		charset	'+', 78
		charset	',', 107
		charset	'-', 96
		charset	'.', 75
		charset	'/', 66
		charset	'0','9', 240
		charset	':', 122
		charset	';', 94
		charset	'<', 76
		charset	'=', 126
		charset	'>', 110
		charset	'?', 111
		charset	'@', 124
		charset	'A','I', 193
		charset 'J','R', 209
		charset	'S','Z', 226
		charset	'[', 186
		charset	'\\', 240
		charset	']', 187
		charset	'^', 176
		charset	'_', 109
		charset	'`', 190
		charset	'a','i', 129
		charset 'j','r', 145
		charset	's','z', 162
		charset	'{', 192
		charset	'|', 79
		charset	'}', 208
		charset	'~', 161
		charset	127, 7		; DEL

		restore		; restore previous listing state and code page

		endif		; ebcdicinc
