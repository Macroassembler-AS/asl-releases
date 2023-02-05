	cpu	ipc-16

flag:	word	0		; if non-zero, swap made during pass
tab:				; vector address
.	=	.+1
tabend:				; sort limit
.	=	.+1
regs:				; register save area
.	=	.+4
ssort:	st	0,regs		; save registers
	st	1,regs+1
	st	2,regs+2
	st	3,regs+3
	pull	2		; obtain address of parameter list
	push	2
	li	0,0
	st	0,flag
	ld	3,1(2)		; end of vector
	aisz	3,-1
	st	3,tabend
	ld	3,(2)		; vector address
	st	3,tab
loop:	ld	0,(3)		; get a value
	skg	0,1(3)		; compare against next value
	jmp	test		; values in order
	ld	1,1(3)		; swap value
	st	0,1(3)
	st	1,0(3)
	li	1,1		; set sort flg non-zero
	st	1,flag
test:	aisz	3,1		; increment table pointer
	rcpy	3,0
	skg	0,tabend	; finished this pass?
	jmp	loop		; no
	ld	0,flag		; yes - did we make a swap?
	aisz	0,0
	jmp	.+2		; yes - continue sort
	jmp	out		; no - sort done
	li	0,0		; initialize for next pass
	st	0,flag
	ld	3,tab
	jmp	loop
out:	ld	0,regs		; restore registers
	ld	1,regs+1
	ld	2,regs+2
	ld	3,regs+3
	rts	2

	end
