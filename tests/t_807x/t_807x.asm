	cpu	8070

	nop
	ret

	ssm
	ssm	p2
	ssm	p3

	ld	a,0x10,pc
	ld	a,0x10,sp
	ld	a,-3,p2
	ld	a,-10,p3
	ld	a,#20
	ld	a,0x12
	ld	a,0xffc0
	ld	a,@+1,p2
	ld	a,@-1,p3
	ld	a,e
	ld	a,s

	ld	ea,0x10,pc
        ld      ea,0x10,sp
        ld      ea,-3,p2
        ld      ea,-10,p3
        ld      ea,#0x2030
        ld      ea,0x12
	ld	ea,0xffc0
        ld      ea,@+1,p2
        ld      ea,@-1,p3
	ld	ea,pc
	ld	ea,sp
	ld	ea,p2
	ld	ea,p3
	ld	ea,t
	ld	pc,ea
	ld	sp,ea
	ld	p2,ea
	ld	p3,ea
	ld	sp,#0x1234
	ld	p2,#0x1234
	ld	p3,#0x1234
	ld	t,0x10,pc
        ld      t,0x10,sp
        ld      t,-3,p2
        ld      t,-10,p3
        ld      t,#0x2030
        ld      t,0x12
	ld	t,0xffc0
        ld      t,@+1,p2
        ld      t,@-1,p3
	ld	t,ea
	ld	e,a
	ld	s,a

	st	a,0x10,pc
	st	a,0x10,sp
	st	a,-3,p2
	st	a,-10,p3
	st	a,0x12
	st	a,0xffc0
	st	a,@+1,p2
	st	a,@-1,p3

        st      ea,0x10,pc
        st      ea,0x10,sp
        st      ea,-3,p2
        st      ea,-10,p3
        st      ea,0x12
	st	ea,0xffc0
        st      ea,@+1,p2
        st      ea,@-1,p3

	add	a,0x10,pc
	add	a,0x10,sp
	add	a,-3,p2
	add	a,-10,p3
	add	a,#20
	add	a,0x12
	add	a,0xffc0
	add	a,@+1,p2
	add	a,@-1,p3
	add	a,e

        add     ea,0x10,pc
        add     ea,0x10,sp
        add     ea,-3,p2
        add     ea,-10,p3
	add	ea,#0x2030
        add     ea,0x12
	add	ea,0xffc0
        add     ea,@+1,p2
        add     ea,@-1,p3

	sub	a,$,pc
	sub	a,0x10,sp
	sub	a,-3,p2
	sub	a,-10,p3
	sub	a,#20
	sub	a,0x12
	sub	a,0xffc0
	sub	a,@+1,p2
	sub	a,@-1,p3
	sub	a,e

        sub     ea,$,pc
        sub     ea,0x10,sp
        sub     ea,-3,p2
        sub     ea,-10,p3
	sub	ea,#0x2030
        sub     ea,0x12
	sub	ea,0xffc0
        sub     ea,@+1,p2
        sub     ea,@-1,p3

        mpy	ea,t
	div	ea,t

        and     a,$,pc
        and     a,0x10,sp
        and     a,-3,p2
        and     a,-10,p3
        and     a,=20
        and     a,0x12
	and	a,0xffc0
        and     a,@+1,p2
        and     a,@-1,p3
        and     a,e
	and	s, #0x10

        or	a,$,pc
        or	a,0x10,sp
        or	a,-3,p2
        or	a,-10,p3
        or	a,#20
        or	a,0x12
	or	a,0xffc0
        or	a,@+1,p2
        or	a,@-1,p3
        or	a,e
	or	s, #0x10

        xor     a,$,pc
        xor     a,0x10,sp
        xor     a,-3,p2
        xor     a,-10,p3
        xor     a,#20
        xor     a,0x12
	xor	a,0xffc0
        xor     a,@+1,p2
        xor     a,@-1,p3
        xor     a,e

	xch	a,e
	xch	e,a
	irp	reg,pc,sp,p2,p3
	xch	ea,reg
	xch	reg,ea
	endm

	sr	a
	sr	ea
	srl	a
	rr	a
	rrl	a
	sl	a
	sl	ea

	push	a
	push	ea
	push	pc
	push	p2
	push	p3

	pop	a
	pop	ea
	pop	p2
	pop	p3

	pli	p2,=0x1234
	pli	p3,=0x1234

	ild	a,$,pc
	ild	a,0x10,sp
	ild	a,-3,p2
	ild	a,-10,p3
	ild	a,0x12
	ild	a,0xffc0
	ild	a,@+1,p2
	ild	a,@-1,p3

	dld	a,$,pc
	dld	a,0x10,sp
	dld	a,-3,p2
	dld	a,-10,p3
	dld	a,0x12
	dld	a,0xffc0
	dld	a,@+1,p2
	dld	a,@-1,p3

	jmp	0x1234
	jsr	0x1234

	call	7

	bnd	$
	bra	$
	bra	10,p2
	bra	-10,p3
	bp	$
	bp	10,p2
	bp	-10,p3
	bz	$
	bz	10,p2
	bz	-10,p3
	bnz	$
	bnz	10,p2
	bnz	-10,p3

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
