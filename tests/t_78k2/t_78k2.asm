	cpu	78214

saddr	equ	0fe34h
saddr2	equ	saddr+2
sfr	equ	0ff34h

b_saddr bit	saddr.2
b_sfr	bit	sfr.3
b_a	bit	a.4
b_x	bit	x.5
b_psw	bit	psw.6

	nop
	di
	ei
	brk
	ret
	reti
	retb

	mov	a,#12h

        mov     saddr,#12h

	mov	sfr,#12h

	mov	a,l
	mov	b,c
	mov	d,e
	mov	h,l

	mov	a,a
	mov	a,x
	mov	a,b
	mov	a,c
	mov	a,d
	mov	a,e
	mov	a,h
	mov	a,l

	mov	a,saddr

	mov	saddr,a

	mov	a,sfr

	mov	sfr,a

	mov	saddr2,saddr

	mov	a,[de]
	mov	a,[hl]
	mov	a,[de+]
	mov	a,[de-]
	mov	a,[hl+]
	mov	a,[hl-]

	mov	a,[de+2]
	mov	a,[sp+5]
	mov	a,[hl+7]
	mov	a,17[de]
	mov	a,1234h[a]
	mov	a,10000[hl]
	mov	a,1[b]

	mov	a,&[de]
	mov	a,&[hl]
	mov	a,&[de+]
	mov	a,&[de-]
	mov	a,&[hl+]
	mov	a,&[hl-]

	mov	a,&[de+2]
	mov	a,&[sp+5]
	mov	a,&[hl+7]
	mov	a,&17[de]
	mov	a,&1234h[a]
	mov	a,&10000[hl]
	mov	a,&1[b]

	mov	[de],a
	mov	[hl],a
	mov	[de+],a
	mov	[de-],a
	mov	[hl+],a
	mov	[hl-],a

	mov	[de+2],a
	mov	[sp+5],a
	mov	[hl+7],a
	mov	17[de],a
	mov	1234h[a],a
	mov	10000[hl],a
	mov	1[b],a

	mov	&[de],a
	mov	&[hl],a
	mov	&[de+],a
	mov	&[de-],a
	mov	&[hl+],a
	mov	&[hl-],a

	mov	&[de+2],a
	mov	&[sp+5],a
	mov	&[hl+7],a
	mov	&17[de],a
	mov	&1234h[a],a
	mov	&10000[hl],a
	mov	&1[b],a

	mov	a,1234h
	mov	a,!saddr

	mov	a,&1234h
	mov	a,&!saddr

	mov	1234h,a
	mov	!saddr,a

	mov	&1234h,a
	mov	&!saddr,a

	mov	psw,#12h

	mov	psw,a

	mov	a,psw

;

        xch     a,a
        xch     a,x
        xch     a,b
        xch     a,c
        xch     a,d
        xch     a,e
        xch     a,h
        xch     a,l

	xch	d,e

        xch     a,saddr
	xch	saddr,a

        xch     a,sfr
	xch	sfr,a

	xch	saddr,saddr2

        xch     a,[de]
	xch	[de],a
        xch     a,[hl]
	xch	[hl],a
        xch     a,[de+]
	xch	[de+],a
        xch     a,[de-]
	xch	[de-],a
        xch     a,[hl+]
	xch	[hl+],a
        xch     a,[hl-]
	xch	[hl-],a

        xch     a,[de+2]
	xch	[de+2],a
        xch     a,[sp+5]
	xch	[sp+5],a
        xch     a,[hl+7]
	xch	[hl+7],a
        xch     a,17[de]
	xch	17[de],a
        xch     a,1234h[a]
	xch	1234h[a],a
        xch     a,10000[hl]
	xch	10000[hl],a
        xch     a,1[b]
	xch	1[b],a

        xch     a,&[de]
	xch	&[de],a
        xch     a,&[hl]
	xch	&[hl],a
        xch     a,&[de+]
	xch	&[de+],a
        xch     a,&[de-]
	xch	&[de-],a
        xch     a,&[hl+]
	xch	&[hl+],a
        xch     a,&[hl-]
	xch	&[hl-],a

        xch     a,&[de+2]
	xch	&[de+2],a
        xch     a,&[sp+5]
	xch	&[sp+5],a
        xch     a,&[hl+7]
	xch	&[hl+7],a
        xch     a,&17[de]
	xch	&17[de],a
        xch     a,&1234h[a]
	xch	&1234h[a],a
        xch     a,&10000[hl]
	xch	&10000[hl],a
        xch     a,&1[b]
	xch	&1[b],a

;

	movw	ax,#1234h
	movw	bc,#1234h
	movw	de,#1234h
	movw	hl,#1234h

	movw	saddr,#1234h

	movw	sfr,#1234h

	irp	reg1,ax,bc,de,hl
	irp	reg2,ax,bc,de,hl
	movw	reg1,reg2
	endm
	endm

	movw	ax,saddr

	movw	saddr,ax

	movw	ax,sfr

	movw	sfr,ax

	movw	ax,[de]
	movw	ax,[hl]

	movw	[de],ax
	movw	[hl],ax

        movw    ax,&[de]
        movw    ax,&[hl]

        movw    &[de],ax
        movw    &[hl],ax

;

	irp	op,add,addc,sub,subc,and,or,xor,cmp

	op	a,#'a'

	op	saddr,#'0'

	op	sfr,#0aah

	irp	reg1,x,a,c,b,e,d,l,h
	irp	reg2,x,a,c,b,e,d,l,h
	op	reg1,reg2
	endm
	endm

	op	a,saddr

	op	a,sfr

	op	saddr,saddr2

        op	a,[de]
        op	a,[hl]
        op	a,[de+]
	op	a,[de-]
	op	a,[hl+]
	op	a,[hl-]

	op	a,[de+2]
	op	a,[sp+5]
	op	a,[hl+7]
	op	a,1234h[a]
	op	a,10000[hl]
	op	a,1[b]

	op	a,&[de]
	op	a,&[hl]
	op	a,&[de+]
        op	a,&[de-]
        op	a,&[hl+]
        op	a,&[hl-]

        op	a,&[de+2]
        op	a,&[sp+5]
        op	a,&[hl+7]
        op	a,&17[de]
        op	a,&1234h[a]
        op	a,&10000[hl]
        op	a,&1[b]

	endm

	irp	op,addw,subw,cmpw

	op	ax,#1234h

	irp	reg,ax,bc,de,hl
	op	ax,reg
	endm

	op	ax,saddr

	op	ax,sfr

	endm

	irp	reg,x,a,c,b,e,d,l,h
	mulu	reg
	divuw	reg
	endm

	irp     reg,x,a,c,b,e,d,l,h,saddr
	inc	reg
	dec	reg
	endm

	irp	reg,ax,bc,de,hl
        incw	reg
	decw	reg
	endm

__cnt	set	0
	irp     reg,x,a,c,b,e,d,l,h
	irp	op,ror,rol,rorc,rolc,shr,shl
	op	reg,__cnt
	endm
__cnt	set	__cnt+1
	endm

__cnt   set     0
	irp	reg,ax,bc,de,hl
	irp	op,shrw,shlw
	op	reg,__cnt
	endm
__cnt   set     __cnt+1
	endm

	irp	op,ror4,rol4
	irp	reg,de,hl
	op	[reg]
	op	&[reg]
	endm
	endm

	adjba
	adjbs

	mov1	cy,saddr.2
	mov1	cy,b_saddr
	mov1	cy,sfr.3
	mov1	cy,b_sfr
	mov1	cy,a.4
	mov1	cy,b_a
	mov1	cy,x.5
	mov1	cy,b_x
	mov1	cy,psw.6
	mov1	cy,b_psw
	mov1	saddr.2,cy
	mov1	b_saddr,cy
	mov1	sfr.3,cy
	mov1	b_sfr,cy
	mov1	a.4,cy
	mov1	b_a,cy
	mov1	x.5,cy
	mov1	b_x,cy
	mov1	psw.6,cy
	mov1	b_psw,cy

        and1    cy,saddr.2
	and1	cy,/saddr.2
        and1    cy,b_saddr
	and1	cy,/b_saddr
        and1    cy,sfr.3
	and1	cy,/sfr.3
        and1    cy,b_sfr
	and1	cy,/b_sfr
        and1    cy,a.4
	and1	cy,/a.4
        and1    cy,b_a
	and1	cy,/b_a
        and1    cy,x.5
	and1	cy,/x.5
        and1    cy,b_x
	and1	cy,/b_x
        and1    cy,psw.6
	and1	cy,/psw.6
        and1    cy,b_psw
	and1	cy,/b_psw

        or1	cy,saddr.2
	or1	cy,/saddr.2
        or1	cy,b_saddr
	or1	cy,/b_saddr
        or1	cy,sfr.3
	or1	cy,/sfr.3
        or1	cy,b_sfr
	or1	cy,/b_sfr
        or1	cy,a.4
	or1	cy,/a.4
        or1	cy,b_a
	or1	cy,/b_a
        or1	cy,x.5
	or1	cy,/x.5
        or1	cy,b_x
	or1	cy,/b_x
        or1	cy,psw.6
	or1	cy,/psw.6
        or1	cy,b_psw
	or1	cy,/b_psw

        xor1	cy,saddr.2
        xor1	cy,b_saddr
        xor1	cy,sfr.3
        xor1	cy,b_sfr
        xor1	cy,a.4
        xor1	cy,b_a
        xor1	cy,x.5
        xor1	cy,b_x
        xor1	cy,psw.6
        xor1	cy,b_psw

	irp	op,set1,clr1,not1
        op	saddr.2
        op	b_saddr
        op	sfr.3
        op	b_sfr
        op	a.4
        op	b_a
        op	x.5
        op	b_x
        op	psw.6
        op	b_psw
	op	cy
	endm

	irp	op,bt,bf,btclr
        op	saddr.2,$pc
        op	b_saddr,$pc
        op	sfr.3,$pc
        op	b_sfr,$pc
        op	a.4,$pc
        op	b_a,$pc
        op	x.5,$pc
        op	b_x,$pc
        op	psw.6,$pc
        op	b_psw,$pc
	endm

	call	1234h
	call	!1234h
	irp	reg,ax,bc,de,hl
	call	reg
	endm

	callf	800h
	callf	!0abch

	callt	[40h]
	callt	[60h]
	callt	[7eh]

;-----

	irp	op,push,pop
	irp	reg,ax,bc,de,hl
	op	reg
	endm
	op	psw
	op	sfr
	endm

	movw	sp,#1234h
	movw	sp,ax
	movw	ax,sp

	incw	sp
	decw	sp

	br	1234h
	br	!1234h
	irp     reg,ax,bc,de,hl
        br      reg
        endm
	br	pc
	br	$pc

	irp	op,bc,bl,bnc,bnl,bz,be,bnz,bne
	op	pc
	op	$pc
	endm

;-----

	dbnz	b,pc
	dbnz	c,pc
	dbnz	saddr,pc

	mov	stbc,#55h

	sel	rb2
	sel	rb1

;-----

r8      equ     0fe30h
rp4	equ	0fe32h

	mov	a,r0
	mov	a,r8
	movw	ax,rp2
	movw	ax,rp4

