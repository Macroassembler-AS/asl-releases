	cpu	78310
	page 	0

	include "reg78310.inc"

saddr	equ	0ff02h
saddr2	equ	0fe80h
sfr	equ	cric00
psw	equ	pswl

	mov	c,#55h
	mov	r7,#0aah

	mov	a,r0
	mov	a,c
	mov	h,c
	mov	h,r4
	mov	r7,r5

	mov	a,[de+]
	mov	a,[hl+]
	mov	a,[de-]
	mov	a,[hl-]
	mov	a,[de]
	mov	a,[hl]
	mov	a,[vp]
	mov	a,[up]
	mov	a,[rp6+]
	mov	a,[rp7+]
	mov	a,[rp6-]
	mov	a,[rp7-]
	mov	a,[rp6]
	mov	a,[rp7]
	mov	a,[rp4]
	mov	a,[rp5]
	mov	a,[de+a]
	mov	a,[hl+a]
	mov	a,[de+b]
	mov	a,[hl+b]
	mov	a,[vp+de]
	mov	a,[vp+hl]
	mov	a,[de+17]
	mov	a,[sp-2]
	mov	a,[hl+55h+2]
	mov	a,[up-7]
	mov	a,[vp+13]
	mov	a,1234h[de]
	mov	a,2345h[a]
	mov	a,3456h[hl]
	mov	a,4567h[b]
	mov	r1,1234h
	mov	r1,saddr2
	mov	r1,pswl
	mov	a,pswh
	mov	a,[saddr]
	mov	saddr,#57h
	mov	saddr,r1
	mov	saddr,saddr2
	mov	pswl,#55h
	mov	pswh,a
	mov	[hl],a
	mov	1234[hl],a
        mov     1234h,r1
	mov	[saddr],a

; note you do not need to explicitly use MOVW, as long as the operand
; size can be deduced from the arguments

	mov	ax,#1
	movw	rp0,#1
	movw	saddr,#9876h
	movw	psw,#0ffeeh
	mov	bc,hl
	movw	bc,hl
	mov	rp6,rp7
	movw	rp6,rp7
	mov	ax,saddr
	movw	ax,saddr
	mov	saddr,rp0
	movw	saddr,rp0
	movw	saddr,saddr+2
	mov	ax,psw
	movw	ax,psw
	mov	psw,ax
	movw	psw,ax

	xch	a,c
	xch	r3,r1
	xch	b,c
	xch	b,h
	xch	r6,r2
	xch	a,[de]
	xch	a,saddr
	xch	a,pswh
	xch	a,[saddr]
	xch	saddr,a
	xch	saddr,saddr2
	xch	pswh,a
	xch	[de],a
	xch	[saddr],a

; again, xchw only needed if op size unknown

	xch	ax,hl
	xchw	ax,hl
	xch	rp0,saddr
	xchw	rp0,saddr
	xch	rp0,psw
	xchw	rp0,psw
	xch	saddr,rp0
	xchw	saddr,rp0
	xchw	saddr,saddr2
	xch	psw,rp0
	xchw	psw,rp0

	irp	op,add,sub,addc,subc,and,or,xor,cmp
	op	a,#67h
	op	saddr,#99
	op	pswl,#'a'
	op	h,c
	op	c,b
	op	r1,saddr2
	op	r1,pswh
	op	saddr2,saddr
	op	a,[de+5]
	op	10[hl],a

	endm

	irp	op,add,sub,cmp
	op	ax,#1234h
	op	hl,bc
	op	ax,saddr
	op	ax,psw

	endm

	irp	opw,addw,subw,cmpw
	opw	ax,#1234h
	opw	saddr,#1234h
	opw	psw,#55aah
        opw	hl,bc
        opw	ax,saddr
        opw	ax,psw
	opw	saddr2,saddr2

	endm

	mulu	c
	mulu	r1
	divu	x
	divu	r3
	mulu	ax
	muluw	ax
	divu	hl
	divux	hl

	inc	c
	inc	saddr
	dec	b
	dec	saddr
	inc	vp
	incw	vp
	incw	saddr
	dec	hl
	decw	hl
	decw	saddr

	ror	c,1
	rol	b,2
	rorc	c,3
	rolc	b,4
	shr	c,5
	shl	b,6
	shr	ax,7
	shrw	ax,7
	shl	hl,1
	shlw	hl,1

	rol4	[de]
	ror4	[ax]
	adj4

	mov1	cy,saddr.2
	mov1	cy,sfr.4
	mov1	cy,a.1
	mov1	cy,x.5
	mov1	cy,pswh.3
	mov1	cy,pswl.6
	mov1	saddr.2,cy
	mov1	sfr.4,cy
	mov1	a.1,cy
	mov1	x.5,cy
	mov1	pswh.3,cy
	mov1	pswl.6,cy

	irp	op1,and1,or1
	op1	cy,saddr.2
	op1	cy,/saddr.2
	op1	cy,sfr.4
	op1	cy,/sfr.4
	op1	cy,a.1
	op1	cy,/a.1
        op1	cy,x.5
	op1	cy,/x.5
        op1	cy,pswh.3
	op1	cy,/pswh.3
        op1	cy,pswl.6
	op1	cy,/pswl.6

	endm

	xor1	cy,saddr.2
	xor1	cy,sfr.4
	xor1	cy,a.1
        xor1	cy,x.5
        xor1	cy,pswh.3
        xor1	cy,pswl.6

	irp	op1,set1,clr1,not1
	op1	cy
	op1	saddr.2
	op1	sfr.4
	op1	a.1
        op1	x.5
        op1	pswh.3
        op1	pswl.6

	endm

	call	6666h
	call	de
	call	[de]
	callf	900h
	callt	[42h]
	brk
	ret
	reti

	push	rp0,rp2,rp4
	push	psw
	pushu	ax,hl
	pop	rp0,rp2,rp4
	pop	psw
	popu	ax,hl

	mov	sp,#8000h
	movw	sp,#8000h
	mov	sp,ax
	movw	sp,ax
	mov	ax,sp
	movw	ax,sp
	inc	sp
	incw	sp
	dec	sp
	decw	sp

	br	!pc+10
	br	8000h
	br	bc
	br	[hl]
	br	pc+10
	br	pc-150

	irp	op,bc,bl,bnc,bnl,bz,be,bnz,bne,bv,bpe,bnv,bpo,bn,bp
	op	pc+5
	endm
	irp	op,bgt,bge,blt,ble,bh,bnh
	op	pc+5
	endm

	bt	saddr.1,pc+5
	bt	sfr.2,pc+5
	bt	a.3,pc+5
	bt	x.4,pc+5
	bt	pswh.5,pc+5
	bt	pswl.6,pc+5

	bf	saddr.1,pc+5
	bf	sfr.2,pc+5
	bf	a.3,pc+5
	bf	x.4,pc+5
	bf	pswh.5,pc+5
	bf	pswl.6,pc+5

	btclr	saddr.1,pc+5
	btclr	sfr.2,pc+5
	btclr	a.3,pc+5
	btclr	x.4,pc+5
	btclr	pswh.5,pc+5
	btclr	pswl.6,pc+5

	bfset	saddr.1,pc+5
	bfset	sfr.2,pc+5
	bfset	a.3,pc+5
	bfset	x.4,pc+5
	bfset	pswh.5,pc+5
	bfset	pswl.6,pc+5

	dbnz	b,pc+5
	dbnz	c,pc+5
	dbnz	saddr,pc+5

	brkcs	rb4
	retcs	8001h

	irp	instr,movm,xchm,cmpme,cmpmne,cmpmc,cmpmnc
	instr	[de+],a
	instr	[de-],a
	endm

	irp	instr,movbk,xchbk,cmpbke,cmpbke,cmpbkne,cmpbkc,cmpbknc
	instr	[de+],[hl+]
	instr	[de-],[hl-]
	endm

	mov	stbc,#34h
	mov	wdm,#0cbh

	swrs
	sel	rb5
	sel	rb7,alt
	nop
	ei
	di

; ----------------------------------------------------------------------
; alternative register bank: A/X and B/C map to R4..R7 instead of R0..R3

	assume	rss:1

	mov	a,[de+]		; just like with RSS=0
	mov	a,c		; codes in R6 instead of R2
	mov	r5,1234h	; R5 is A with RSS=1

	mov	ax,#1		; codes in RP2 instead of RP0
	mov	rp2,saddr	; only allowed with A which is RP2

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
