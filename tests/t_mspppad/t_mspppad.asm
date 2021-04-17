	cpu	msp430x

mynop	macro
	nop
	endm

	.byte	"blabl"		; force PC to odd address
lab				; lab is assigned odd address
	.word	1		; padding inserted, lab's value gets changed

	.byte	"blabl"		; similar if next machine insn is in a macro...
lab2
	mynop

	.byte	"blabl"		; ...or a repetition
lab3
	rept	2
	nop
	endm

	.word	lab, lab2, lab3

