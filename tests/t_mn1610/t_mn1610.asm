;;;
;;;	Test MN1613
;;;

	CPU	MN1613

	ORG	X'0000'
ADR:	DC	0

	INCLUDE "inst1610.asm"

	INCLUDE "inst1613.asm"

	PACKING	ON

	DC	1,2,3		; one word per number
	EXPECT	1320,1320,1320
	DC	1000,2000,3000	; forbidden (8 bit range)
	ENDEXPECT
	DC	"Hello World"	; one word per character
	DC	1.0,2.0,3.0	; two words per number

	PACKING	OFF

	DC	1,2,3		; one half-word per number
	DC	1000,2000,3000	; allowed (16 bit range)
	DC	"Hello World"	; one half-word per character
	DC	1.0,2.0,3.0	; two words per number

	END
