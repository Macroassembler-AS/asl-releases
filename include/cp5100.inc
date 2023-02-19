	ifndef	cp5100inc
cp5100inc equ	1

	save		; no listing over this file
	listing	off

	; this translates the ASCII characters 0...127
	; and a few ISO-Latin 1 characters to their IBM
	; 5100 equivalent, as far as possible:

	; NOTE: The save/restore mechanism will assure that after inclusion
	; of this file, you still use the same character mapping as you
	; did before.  To activate this mapping, place a 'codepage cp5100'
	; in your code!

	codepage cp5100

	charset
	charset	 0,31,		; 0x00...0x1f -> none
	charset	' ',0		; 0x20 -> 0x00
	charset	'!',96		; 0x21 -> 0x60
	charset	'"',110		; 0x22 -> 0x6e
	charset	'#',113		; 0x23 -> 0x71
	charset	'$',114		; 0x24 -> 0x72
	charset	'%',115		; 0x25 -> 0x73
	charset	'&',111		; 0x26 -> 0x6f
	charset	'\'',55		; 0x27 -> 0x37
	charset	'(',85		; 0x28 -> 0x55
	charset	')',86		; 0x29 -> 0x56
	charset	'*',50		; 0x2a -> 0x3c
	charset	'+',38		; 0x2b -> 0x26
	charset	',',43		; 0x2c -> 0x2b
	charset	'-',82		; 0x2d -> 0x52
	charset	'.',44		; 0x2e -> 0x2c
	charset	'/',37		; 0x2f -> 0x25
	charset	'0','9',27	; 0x30..0x39 -> 0x1b..0x24
	charset	':',88		; 0x3a -> 0x58
	charset	';',87		; 0x3b -> 0x57
	charset	'<',74		; 0x3c -> 0x4a
	charset	'=',76		; 0x3d -> 0x4c
	charset	'>',78		; 0x3e -> 0x4e
	charset	'?',61		; 0x3f -> 0x3d
	charset	'@',112		; 0x40 -> 0x70
	charset	'A','Z',1	; 0x41..0x5a -> 0x01..0x1a
	charset	'[',41		; 0x5b -> 0x29
	charset	'\\',81		; 0x5c -> 0x51
	charset	']',42		; 0x5d -> 0x2a
	charset	'^',71		; 0x5e -> 0x47
	charset	'_',50		; 0x5f -> 0x32
	charset	'`',		; 0x60 -> none
	charset	'a','z',	; 0x61..0x69 -> none
	charset	'{',		; 0x7b -> none
	charset	'|',57		; 0x7c -> 0x39
	charset	'}',		; 0x7d -> none
	charset	'~',64		; 0x7e -> 0x40
	charset	127,255,	; DEL..0xff -> (mostly) none
	charset	196,116		; 0xc4 -> 0x74 (Adiaresis)
	charset	214,118		; 0xd6 -> 0x76 (Odiaresis)
	charset	220,119		; 0xdc -> 0x77 (Udiaresis)
	charset	197,120		; 0xc5 -> 0x78 (Akringel)
	charset	198,121		; 0xc6 -> 0x79 (Aelig)
	charset	209,123		; 0xd1 -> 0x7b (Ntilde)
	charset 163,124		; 0xa3 -> 0x7c (pound)
	charset	199,125		; 0xc7 -> 0x7d (Ccedilla)
	charset	213,126		; 0xd5 -> 0x7e (Otilde)
	charset	195,127		; 0xc3 -> 0x7f (Atilde)

	restore		; restore previous listing state and code page

	endif		; cp5100inc
