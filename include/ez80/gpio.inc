		ifndef	__ez80gpioinc
__ez80gpioinc	equ	1

__defgpio	macro	NUM,Base
P{NUM}_DR	port	Base+0		; Port x Data Register (r/w)
P{NUM}_DDR	port	Base+1		; Port x Data Direction Register (r/w)
P{NUM}_ALT1	port	Base+2		; Port x Alternate Register 1 (r/w)
P{NUM}_ALT2	port	Base+3		; Port x Alternate Register 2 (r/w)
		endm			; __defgpio

		endif			; __ez80gpioinc
