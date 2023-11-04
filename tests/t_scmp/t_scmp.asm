	cpu	sc/mp
	page	0
	relaxed on

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

	ld	e(pc)
        st	@e(p2)
	st	@e(2)
	ld	-127(pc)
	expect	1445
	ld	@-127(pc)
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

	org	256

	expect	1330		; would result in a displacement
	ld	$-127		; of -128, which means using the
	endexpect		; E register's contents
