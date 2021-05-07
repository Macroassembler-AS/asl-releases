	cpu	f3850
	page	0

	adc
	ai	55h
	am
	amd
	as	5
	asd	10

	bc	$
	bf	10,$+1
	bm	$-2
	bnc	$+3
	bno	$-4
	bnz	$+5
	bp	$-6
	br	$+7
	br7	$-8
	bt	3,$+9
	bz	$-10

	ci	55h
	clr
	cm
	com

	dci	55aah
	di
	ds	s

	ei

	in	07h
	inc
	ins	4

	jmp	55aah

	li	88h
	lis	5
	lisl	4
	lisu	6
	lm
	lnk
	lr	a,ku
	lr	a,kl
	lr	a,qu
	lr	a,ql
	lr	ku,a
	lr	kl,a
	lr	qu,a
	lr	ql,a
	lr	k,p
	lr	p,k
	lr	a,is
	lr	is,a
	lr	p0,q
	lr	q,dc
	lr	dc,q
	lr	h,dc
	lr	dc,h
	lr	w,j
	lr	j,w
	lr	a,10
	lr	i,a

	ni	0feh
	nm
	nop
	ns	i

	oi	01h
	om
	out	55h
	outs	7

	pi	55aah
	pk
	pop

	sl
	sl	1
	sl	4
	sr
	sr	1
	sr	4
	st

	xdc
	xi	0ffh
	xm
	xs	d

; ---- sample memcpy() from F8 Programmer's Manual

BUFA	equ	800h	; SET THE VALUE OF SYMBOL BUFA
BUFB	equ	8a0h	; SET THE VALUE OF SYMBOL BUFB
	org	100h
one:	dci	BUFA	; SET DCO TO BUFA STARTING ADDRESS
two:	xdc		; STORE IN DC1
three:	dci	BUFB	; SET DCO TO BUFB STARTING ADDRESS
four:	li	80h	; LOAD BUFFER LENGTH INTO ACCUMULATOR
five:	lr	1,a	; SAVE BUFFER LENGTH IN SCRATCHPAD BYTE 1
loop:	lm		; LOAD CONTENTS OF MEMORY BYTE ADDRESSED BY DCO
six:	xdc		; EXCHANGE DCO AND DC1
seven:	st		; STORE ACCUMULATOR IN MEMORY BYTE ADDRESSED BY DCO
eight:	xdc		; EXCHANGE DCO AND DC 1
nine:	ds	1	; DECREMENT SCRATCHPAD BYTE 1
ten:	bnz	loop	; SCRATCHPAD BYTE 1 IS NOT ZERO. RETURN TO LOOP

; ---- taken from https://www.chessprogramming.org/Fairchild_F8, with some
; ---- OCR mistakes fixed ('OD' -> '0D', 'LR AS' -> 'LR A,S')
; ANSWER-BACK PROGRAM FOR MOSTEK F8 EVALUATION KIT
; D. EDWARDS, ELECTRONICS AUSTRALIA 19/10/76

	ORG	400h
INIT:	
	LI	0FFh	; LOAD AC WITH FF
	LR	IS,A	; INITIALIZE ISAR TO 3F
	LR	4,A	; COPY AC INTO REG 4
	DS	4	; DECREMENT REG 4 TO FE
	LR	6,A	; COPY AC INTO REG 6
	LIS	01h	; LOAD AC WITH 01
	OUTS	6	; TRANSFER AC TO TIMER PORT TO ENABLE EXT INT
	EI		; ENABLE I/O ROUTINES
START:	PI	03F3h	; CALL TTYIN SUBROUTINE
	LR	A,S	; COPY CHAR INTO AC FROM RS
	CI	0Dh	; COMPARE WITH CR
	BZ	MESSAGE	; JUMP TO MESSAGE IF CR
	PI	035Dh	; SEND CHAR TO TTYOUT SUBROUTINE
	BR	START	; LOOP BACK TO START
MESSAGE:DCI	MSG	; LOAD DC WITH MESSAGE ADDRESS
ANSWER:	CLR		; CLEAR AC
	AM		; ADD CHAR TO AC AND INC DC
	BZ	START	; LOOP BACK TO START
	LR	S,A	; COPY CHAR INTO RS
	PI	035Dh	; SEND CHAR TO TTYOUT SUBROUTINE
	BR	ANSWER	; LOOP BACK TO ANSWER
MSG:	db	0Dh, 47h, 4Fh  ; START OF ANSWER BUFFER
	db	20h, 41h, 57h
	db	41h, 59h, 2Ch
	db	20h, 49h, 27h
	db	4Dh, 20h, 42h
	db	55h, 53h, 59h
	db	21h, 0Dh, 00h  ; ANSWER MUST END WITH A ZERO BYTE
