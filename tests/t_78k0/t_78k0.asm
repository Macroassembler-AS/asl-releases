        cpu     78070
        page	0
        relaxed	on

        include reg78k0.inc

saddr	equ	0fe80h
sfr	equ	0ff90h

        brk
        ret
        retb
        reti
        nop
        ei
        di
        halt
        stop
        adjba
        adjbs

        mov	d,#20
        mov	saddr,#30
        mov	sfr,#40
        mov	a,c
        mov	e,a
        mov	a,saddr
        mov	saddr,a
        mov	a,sfr
        mov	sfr,a
        mov	a,!saddr
        mov	!saddr,r1
        mov	psw,#40h
        mov	a,psw
        mov	psw,a
        mov	a,[de]
        mov	[de],a
        mov	a,[hl]
        mov	[hl],a
        mov	a,[hl+10]
        mov	[hl+10],a
        mov	a,[hl+b]
        mov	[hl+b],a
        mov	a,[hl+c]
        mov	[hl+c],a

        xch	a,d
        xch	l,a
        xch	a,saddr
        xch	a,sfr
        xch	a,!saddr
        xch	a,[de]
        xch	a,[hl]
        xch	a,[hl+10]
        xch	a,[hl+b]
        xch	a,[hl+c]

        movw	de,#1000
        movw	saddr,#2000
        movw	sfr,#3000
        movw	ax,saddr
        movw	saddr,ax
        movw	ax,sfr
        movw	sfr,ax
        movw	ax,de
        movw	hl,ax
        movw	ax,!saddr
        movw	!saddr,ax

        xchw	ax,de
        xchw	hl,ax

        add	a,#10
        add	saddr,#20
        add	a,c
        add	h,a
        add	a,saddr
        add	a,!saddr
        add	a,[hl]
        add	a,[hl+10]
        add	a,[hl+b]
        add	a,[hl+c]

        addc	a,#10
        addc	saddr,#20
        addc	a,c
        addc	h,a
        addc	a,saddr
        addc	a,!saddr
        addc	a,[hl]
        addc	a,[hl+10]
        addc	a,[hl+b]
        addc	a,[hl+c]

        sub	a,#10
        sub	saddr,#20
        sub	a,c
        sub	h,a
        sub	a,saddr
        sub	a,!saddr
        sub	a,[hl]
        sub	a,[hl+10]
        sub	a,[hl+b]
        sub	a,[hl+c]

        subc	a,#10
        subc	saddr,#20
        subc	a,c
        subc	h,a
        subc	a,saddr
        subc	a,!saddr
        subc	a,[hl]
        subc	a,[hl+10]
        subc	a,[hl+b]
        subc	a,[hl+c]

        and	a,#10
        and	saddr,#20
        and	a,c
        and	h,a
        and	a,saddr
        and	a,!saddr
        and	a,[hl]
        and	a,[hl+10]
        and	a,[hl+b]
        and	a,[hl+c]

        or	a,#10
        or	saddr,#20
        or	a,c
        or	h,a
        or	a,saddr
        or	a,!saddr
        or	a,[hl]
        or	a,[hl+10]
        or	a,[hl+b]
        or	a,[hl+c]

        xor	a,#10
        xor	saddr,#20
        xor	a,c
        xor	h,a
        xor	a,saddr
        xor	a,!saddr
        xor	a,[hl]
        xor	a,[hl+10]
        xor	a,[hl+b]
        xor	a,[hl+c]

        cmp	a,#10
        cmp	saddr,#20
        cmp	a,c
        cmp	h,a
        cmp	a,saddr
        cmp	a,!saddr
        cmp	a,[hl]
        cmp	a,[hl+10]
        cmp	a,[hl+b]
        cmp	a,[hl+c]

        addw	ax,#1234h
        subw	rp0,#2345h
        cmpw	ax,#3456h

        mulu	x
        divuw	c

        inc	d
        inc	saddr
        dec	e
        dec	saddr

        incw	hl
        decw	de

        ror	a,1
        rol	a,1
        rorc	a,1
        rolc	a,1

        ror4	[hl]
        rol4	[hl]

	mov1	cy,saddr.3
        mov1	cy,sfr.4
        mov1	cy,a.5
        mov1	cy,psw.6
        mov1	cy,[hl].7
	mov1	saddr.3,cy
        mov1	sfr.4,cy
        mov1	a.5,cy
        mov1	psw.6,cy
        mov1	[hl].7,cy

	and1	cy,saddr.3
        and1	cy,sfr.4
        and1	cy,a.5
        and1	cy,psw.6
        and1	cy,[hl].7

	or1	cy,saddr.3
        or1	cy,sfr.4
        or1	cy,a.5
        or1	cy,psw.6
        or1	cy,[hl].7

	xor1	cy,saddr.3
        xor1	cy,sfr.4
        xor1	cy,a.5
        xor1	cy,psw.6
        xor1	cy,[hl].7

	set1	saddr.3
        set1	sfr.4
        set1	a.5
        set1	psw.6
        set1	[hl].7

	clr1	saddr.3
        clr1	sfr.4
        clr1	a.5
        clr1	psw.6
        clr1	[hl].7

        set1	cy
        clr1	cy
        not1	cy

        call	1234h
        callf	 234h
	callt	[12h]

        push	psw
        push	de
        pop	psw
        pop	hl

	movw	sp,#1234h
	movw	sp,ax
	movw	ax,sp

        br	ax
        br	rp0
        br	1234h
        br	pc
        br	$pc
        br	!pc

        bc	pc
        bnc	pc
        bz	pc
        bnz	pc

        bt	saddr.3,pc
        bt	sfr.4,pc
        bt	a.5,pc
        bt	psw.6,pc
        bt	[hl].7,pc

        bf	saddr.3,pc
        bf	sfr.4,pc
        bf	a.5,pc
        bf	psw.6,pc
        bf	[hl].7,pc

        btclr	saddr.3,pc
        btclr	sfr.4,pc
        btclr	a.5,pc
        btclr	psw.6,pc
        btclr	[hl].7,pc

        dbnz	b,pc
        dbnz	c,pc
        dbnz	saddr,pc

        sel	rb0
        sel	rb1
        sel	rb2
        sel	rb3

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"

        end
