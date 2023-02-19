		ifndef	radix50inc
radix50inc	equ	1

		save		; no listing over this file
		listing	off

		; this translates the ASCII characters 0...127 to their RADIX50
                ; (PDP-11/VAX) representation, as far as possible:

		; NOTE: The save/restore mechanism will assure that after inclusion
                ; of this file, you still use the same character mapping as you
                ; did before.  To activate this mapping, place a 'codepage radix50'
		; in your code!

		codepage radix50

		charset
		charset	0, 31,
		charset	' ', 0
		charset '!', '#',
		charset	'$', 27
		charset	'%', 29
		charset '&', '-',
		charset	'.', 28
		charset	'/',
		charset	'0', '9', 30
		charset	':', '@',
		charset	'A', 'Z', 1
		charset	'[', '`',
		charset	'a', 'z',
		charset	'{', '~',
		charset	127,255,

		; This macro stores the given string in (packed) RADIX 50
		; encoding.  The 'radix50' code page does not need to be
		; selected before usage.

radix50		macro	arg
__rad50_acc	set	0
__rad50_pos	set	0
		rept	strlen(arg)
__rad50_ch	 set	CODEPAGE_VAL(CHARFROMSTR(arg, __rad50_pos), "radix50")
		 if	__rad50_ch == -1
		  error	"string contains characters not representable in RADIX 50"
__rad50_acc	  set	__rad50_acc * 40
		 elseif
__rad50_acc	  set	__rad50_acc * 40 + __rad50_ch
		 endif
__rad50_pos	 set	__rad50_pos + 1
		 if	(__rad50_pos # 3) == 0
		  word	__rad50_acc
__rad50_acc	  set	0
		 endif
		endm
		switch	strlen(arg) # 3
		 case	1
		  word	__rad50_acc * 40 * 40
		 case	2
		  word	__rad50_acc * 40
		 elsecase
		endcase
		endm

		restore		; restore previous listing state and code page

		endif		; radix50inc
