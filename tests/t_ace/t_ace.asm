	cpu	ace1202
	page	0

	adc	a, #055h
	adc	a, 030h
	adc	a, [x]
	adc	a, [#33h, x]

	add	a, #055h
	add	a, 030h
	add	a, [x]
	add	a, [#33h, x]

	and	a, #055h
	and	a, 030h
	and	a, [x]
	and	a, [#33h, x]

	clr	a
	clr	x
	clr	030h

	dec	a
	dec	x
	dec	030h

	ifbit	7, a
	ifbit	7, 033h
	ifbit	7, [x]

	ifc

	ifeq	a, #033h
	ifeq	a, 033h
	ifeq	a, [x]
	ifeq	a, [#044h, x]
	ifeq	x, #00344h
	ifeq	033h, #044h
	
	ifgt	a, #033h
	ifgt	a, 033h
	ifgt	a, [x]
	ifgt	a, [#044h, x]
	ifgt	x, #00344h

	iflt	x, #00344h
	
	ifnc

	ifne	a, #055h
	ifne	a, 030h
	ifne	a, [x]
	ifne	a, [#33h, x]

	inc	a
	inc	x
	inc	030h

	intr

	invc

	jmp	070eh
	jmp	[#01h, x]

	jp	$-30
	jp      $-15
	jp	$-1
	jp	$
	jp	$+1
	jp	$+15
	jp	$+31
	jp	$+32

	jsr	070eh
	jsr	[#01h, x]

	ld	a, #033h
	ld	a, 033h
	ld	a, [x]
	ld	a, [#044h, x]
	ld	x, #00344h
	ld	033h, #044h
	ld	033h, 034h

	ldc	2, 033h

	nop

        or      a, #055h
        or      a, 030h
        or      a, [x]
        or      a, [#33h, x]

	rbit	2, 033h
	rbit	2, [x]
	rbit	2, a		; Makro

	rc

	ret

	reti

	rlc	a
	rlc	030h

	rrc	a
	rrc	030h

	sbit	2, 033h
	sbit	2, [x]
	sbit	2, a		; Makro

	sc

	st	a, 033h
	st	a, [x]
	st	a, [#044h, x]

	stc	2, 033h

        subc    a, #055h
        subc    a, 030h
        subc    a, [x]
        subc    a, [#33h, x]

        xor     a, #055h
        xor     a, 030h
        xor     a, [x]
        xor     a, [#33h, x]

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
