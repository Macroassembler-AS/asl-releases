	ifndef	cp5110inc
cp5110inc equ	1

	save		; no listing over this file
	listing	off

	; this translates printable ASCII characters 
        ; (0x20...0x7e) to the IBM 5110's variant of
	; EBCDIC, as far as possible...

	; NOTE: The save/restore mechanism will assure that after inclusion
	; of this file, you still use the same character mapping as you
	; did before.  To activate this mapping, place a 'codepage cp5110'
	; in your code!

	codepage cp5110

	charset
	charset	0,31,		; 0x00..0x1f -> nothing
	charset	' ',64		; 0x20 -> 0x40
	charset	'!',90		; 0x21 -> 0x5a
	charset	'"',127		; 0x22 -> 0x7f
	charset	'#',123		; 0x23 -> 0x7b
	charset	'$',91		; 0x24 -> 0x5b
	charset	'%',108		; 0x25 -> 0x6c
	charset	'&',80		; 0x26 -> 0x50
	charset	'\'',185	; 0x27 -> 0xb9
	charset	'(',77		; 0x28 -> 0x4d
	charset	')',93		; 0x29 -> 0x5d
	charset	'*',92		; 0x2a -> 0x5c
	charset	'+',78		; 0x2b -> 0x4e
	charset	',',107		; 0x2c -> 0x6b
	charset	'-',96		; 0x2d -> 0x60
	charset	'.',75		; 0x2e -> 0x4b
	charset	'/',97		; 0x2f -> 0x61
	charset	'0','9',240	; 0x30..0x39 -> 0xf0..0xf9
	charset	':',122		; 0x3a -> 0x7a
	charset	';',94		; 0x3b -> 0x5e
	charset	'<',76		; 0x3c -> 0x4c
	charset	'=',126		; 0x3d -> 0x7e
	charset	'>','?',110	; 0x3e..0x3f -> 0x6e..0x6f
	charset	'@',124		; 0x40 -> 0x7c
	charset	'A','I',193	; 0x41..0x49 -> 0xc1..0xc9
	charset	'J','R',209	; 0x4a..0x52 -> 0xd1..0xd9
	charset	'S','Z',226	; 0x53..0x5a -> 0xe2..0xe9
	charset	'[',173		; 0x5b -> 0xad
	charset	'\\',183	; 0x5c -> 0xb7
	charset	']',189		; 0x5d -> 0xbd
	charset	'^',113		; 0x5e -> 0x71 (centered up arrow)
	charset	'_',109		; 0x5f -> 0x6d
	charset	'`',121		; 0x60 -> 0x79
	charset	'a','i',129	; 0x61..0x69 -> 0x81..0x89
	charset	'j','r',145	; 0x6a..0x72 -> 0x91..0x99
	charset	's','z',162	; 0x73..0x7a -> 0xa2..0xa9
	charset	'{',192		; 0x7b -> 0xc0 (according to BASIC character table)
	charset	'|',106		; 0x7c -> 0x6a (191 is also possible)
	charset	'}',208		; 0x7d -> 0xd0 (according to BASIC character table)
	charset	'~',128		; 0x7e -> 0x80
	charset	127,255,	; 0x7f..0xff -> nothing

	restore		; restore previous listing state and code page

	endif		; cp5110inc
