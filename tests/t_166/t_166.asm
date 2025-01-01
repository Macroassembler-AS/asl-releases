        cpu     80c167
	page	0
        include reg166

	diswdt
	einit
	idle
	nop
	pwrdn
	ret
	reti
	rets
	srst
	srvwdt

targ:	jmpr	targ
	jmpr	nz,targ
	jmpr	ult,targ
	jmpr	sgt,targ
	jmpr	nv,targ
	jmpr	net,targ
	callr	targ

	jmpa	targ
	jmpa	nz,targ
	jmpa	ult,targ
	jmpa	sgt,targ
	jmpa	nv,targ
	jmpa	net,targ
	calla	1234h

	jmps	targ
	jmps	1,2345h
	calls	targ

	jmpi	[r3]
	jmpi	z,[r12]
	calli	nc,[r4]

	jmp	10000h
	jmp	3ffffh
	jmp	12345h
	jmp	80h
	jmp	2000h
	jmp	[r14]

	call	10000h
	call	3ffffh
	call	12345h
	call	z,80h
	call	sle,2000h
	call	nn,[r14]

	add	1234h,r10
	addb	2345h,0fe00h
	addc	r7,1234h
	addcb	0fe02h,2345h
	sub	r12,#4
	subb	r5,#200
	subc	0fe04h,#4
	subcb	0fe06h,#200
	cmp	r5,r3
	cmpb	rh0,rl0
	and	r1,[r2]
	andb	r5,[r1]
	or	r3,[r3+]
	xorb	r15,[r0+]

	ashr	r2,r4
	rol	r5,#4
	ror	r10,r1
	shl	r3,#12
	shr	r15,r0

	band	r2.5,r7.14
	bmov	r3.7,0fd08h.5
	bmovn	r7.10,0ff10h.12
	bcmp	r7.14,r2.5
	bor	0fd08h.5,r3.7
	bxor	0ff10h.12,r7.10
	bset	r5.2
	bclr	0ff80h.13
	bfldl	r5,#0c3h,#3ch
	bfldh	0fd02h,#0c3h,#3ch

	cmpd1	r5,#10
	cmpd2	r12,#200
	cmpi1	r9,Targ
	cmpi2	r14,#(5*4)-7

	cpl	r5
	cplb	rh1
	neg	r12
	negb	r10

	div	r3
	divl	r7
	divu	r10
	divlu	r0

targ2:  jb      r5.2,targ2
        jbc     0fd30h.12,targ2
        jnb     r7.14,targ2
        jnbs    0ff58h.2,targ2

	mov	r2,r5
	movb	r0,rh5
	mov	r4,#2
	movb	r6,#10
	mov	r12,#1234
	movb	r0,#23h
	mov	r0,[r2]
	movb	rl4,[r10]
	mov	r3,[r4+]
	movb	r10,[r7+]
	mov	r1,[r1+300]
	movb	r10,[30+r4-40]
	mov	r12,Targ
        movb    rl2,targ2

	mov	0fe00h,#10
	movb	0fe02h,#100
	mov	0fe04h,[r2]
	movb	0fe06h,[r7]
	mov	0fe08h,Targ
        movb    0fe0ah,targ2
	mov	[r4],r3
	movb	[r7],rl5
	mov	[r10],[r2]
	movb	[r14],[r0]
	mov	[r13],[r6+]
	movb	[r14],[r2+]
        mov     [r11],targ2
	movb	[r15],Targ
	mov	[-r4],r2
	movb	[-r15],rh7
	mov	[r6+],[r13]
	movb	[r2+],[r14]
	mov	[r15+20],r4
	movb	[r0-7],rh1
        mov     targ2,[r7]
	movb	Targ,[r4]
	mov	Targ,0fe10h
        movb    targ2,0ff10h

	movbs	r10,rh1
        movbs   0fe04h,targ2
        movbs   targ2,0fe04h
	movbz	r11,rl1
        movbz   0fe08h,targ2
        movbz   targ2,0fe40h

	mul	r12,r15
	mulu	r0,r7
	prior	r2,r4

        pcall   r10,targ2
	pcall	0fe02h,8000h

	push	r2
	pop	0ff20h
	retp	r14

	scxt	0fe20h,#1234h
        scxt    r5,targ2

	trap	#10
	trap	#127

;-------------------------------
; Pipeline-Tests

	mov	dpp0,#4
	assume	dpp0:4
	expect	200
	 mov	 r0,12345h    	; DPP0 noch nicht veraenert
	endexpect
	mov	r0,12345h    	; ab hier wieder gut
	mov	dpp0,#0
	assume	dpp0:0

	mov	cp,0fc00h
	expect	200,200
	 mov	 r5,r3           ; gleich doppelt
	endexpect
	movb	r3,r1

	mov	sp,0fd00h	; SP noch in der Pipe
	expect	200
	 pop	 r4
	endexpect
	ret

;-------------------------------
; Bit-Tests

	bset	123h
tbit	bit	0ff80h.4

;-------------------------------

Str	equ	"PSW+5"
tmp	equ	Val(Str)

;-------------------------------
; Adressierungs-Tests

	atomic	#2

	extr	#1
	mov	0f000h,#1234h
	mov	0fe00h,#1234h

ebit	bit	0f100h.4
sbit	bit	0ff00h.4

	extr	#1
	bclr	ebit
	bclr	sbit

	extr	#1
	bset	0f1deh.12
	bset	0ffdeh.12

	extp	r5,#1
	mov	r0,0abcdh
	mov	r0,0abcdh

	extpr	#4,#1
	extp	#4,#1
	mov	r0,12345h
	expect	110
	 mov	 r0,12345h
	endexpect

	extsr	#1,#1
	exts	#1,#1
	mov	r0,12345h
	expect	110
	 mov	 r0,12345h
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
