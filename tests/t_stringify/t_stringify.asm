	cpu	8051

cnt	set	0
	while	cnt!=10
var{"\{CNT}"} equ	cnt
cnt	set	cnt+1
	endm

	db	var1,var9,var2,var8
