		ifndef	ebcdicinc
ebcdicinc	equ	1

		save		; no listing over this file
		listing	off

		; this translates the ASCII printable characters 32...126
		; to their EBCDIC (codepage 037) representation:

		; NOTE: The save/restore mechanism will assure that after inclusion
                ; of this file, you still use the same character mapping as you
                ; did before.  To activate this mapping, place a 'codepage cp037'
		; in your code!

		codepage cp037

		charset	' ',64
		charset '!',90
		charset '"',127
		charset	'#',123
		charset	'$',91
		charset	'%',108
		charset	'&',80
		charset	'\'',125
		charset	'(',77
		charset	')',93
		charset	'*',92
		charset	'+',78
		charset	',',107
		charset	'-',96
		charset	'.',75
		charset	'/',66
		charset	'0','9',240
		charset	':',122
		charset	';',94
		charset	'<',76
		charset	'=',126
		charset	'>',110
		charset	'?',111
		charset	'@',124
		charset	'A','I',193
		charset 'J','R',209
		charset	'S','Z',226
		charset	'[',186
		charset	'\\',240
		charset	']',187
		charset	'^',176
		charset	'_',109
		charset	'`',190
		charset	'a','i',129
		charset 'j','r',145
		charset	's','z',162
		charset	'{',192
		charset	'|',79
		charset	'}',208
		charset	'~',161

		restore		; restore previous listing state and code page

		endif		; ebcdicinc
