	cpu	8051

	; in the first pass, there should be no warning,
	; even if 0xe0 is inserted as a "don't know" value:

	org	0e0h
	if	MOMPASS > 1
	expect	140
	endif
	mov	a,acc
	if	MOMPASS > 1
	endexpect
	endif

acc	equ	0e0h

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
