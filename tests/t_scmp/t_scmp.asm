	cpu	sc/mp
	page	0
	relaxed on

ptr0	reg	p0
ptr1	equ	1
ptr2	equ	p2
ptr3	equ	p3
ereg	reg	e
pcnt	equ	pc

        lde
        xae
        ane
        ore
        xre
        dae
        ade
        cae

        sio
        sr
        srl
        rr
        rrl

        halt
        ccl
        scl
        dint
        ien
        csa
        cas
        nop

        ldi	0x12
        ani	0x23
        ori	0x34
        xri	0x45
        dai	0x56
        adi	0x67
        cai	0x78
        dly	0x89

        xpal	pc
        xpah	p2		; it is valid to use just
	xpah	2		; the register # instead of Pn
        xppc	p1
	xppc	1

	expect	1350		; E cannot be used as displacement if
	ld	e(pc)		; pointer register is PC
	endexpect
        st	@e(p2)
	st	@ereg(2)
	ld	-127(pc)
	ld	-128(pc)	; 0x80 as displacement is allowed for PC...
	ld	-127(p1)
	expect	440
	ld	-128(p2)	; ...but not on P1...P3
	endexpect
	expect	1445
	ld	@-127(pc)	; no auto-increment with PC
	endexpect
        and	10(p1)
	and	10(1)
        or	@-20(p3)
	or	@-20(3)
        xor	vari
vari:	dad	-30(p2)
	dad	-30(2)
	add	@40(p1)
	add	@40(1)
	add	@x'28'(p1)
	add	@x'28(p1)
        cad	vari

	jmp	vari
        jp	10(p2)
	jp	10(2)
        jz	vari
        jnz	vari

        ild	vari
        dld	-5(p2)
	dld	-5(2)

;        org     0xfff
;        ldi     0x20

	org	384

	ld	$-127		; displacement of -128 is allowed if pointer reg is PC

	expect	1330		; would result in a displacement
	ld	$-128		; of -129, which is out of range
	endexpect

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
