	cpu	z8002
	page	0
	supmode	on

	adc	r2,r3
	adc	rh2,rl3
	adcb	rh2,rl3
	expect	1130
        adc     rr4,rr6
	endexpect

	; ADD R,R
	add	r2,r3
	add	rh2,rl3
	addb	rh2,rl3
	add	rr4,rr6
	addl	rr4,rr6
	; ADD R,IM
	add	rh2,#57h
	addb	rh2,#57h
	add	r5,#5678h
	add	rr8,#12345678h
	addl	rr8,#12345678h
	; ADD R,IR
	add	r2,@r3
	add	rh2,@r3
	addb	rh2,@r3
	add	rr4,@r6
	addl	rr4,@r6
	; ADD R,DA
	add	r2,1234h
	add	rh2,1234h
	addb	rh2,1234h
	add	rr4,1234h
	addl	rr4,1234h
	; ADD R,X
	add	r2,1234h(r10)
	add	rh2,1234h(r10)
	addb	rh2,1234h(r10)
	add	rr4,1234h(r10)
	addl	rr4,1234h(r10)

	; AND R,R
	and	r2,r3
	and	rh2,rl3
	andb	rh2,rl3
	expect	1130,1200
	and	rr4,rr6
	andl	rr4,rr6
	endexpect
	; AND R,IM
	and	rh2,#57h
	andb	rh2,#57h
	and	r5,#5678h
	; AND R,IR
	and	r2,@r3
	and	rh2,@r3
	andb	rh2,@r3
	; AND R,DA
	and	r2,1234h
	and	rh2,1234h
	andb	rh2,1234h
	; AND R,X
	and	r2,1234h(r10)
	and	rh2,1234h(r10)
	andb	rh2,1234h(r10)

	; BIT R,IM
	bit	rl3,#4
	bitb	rl3,#4
	bit	r13,#12
	; BIT IR,IM
	bit	@r9,#12
	bitb	@r9,#4
	bit	@r9,#12
	; BIT DA,IM
	bit	1234h,#12
	bitb	1234h,#4
	bit	1234h,#12
	; BIT X,IM
	bit	1234h(r6),#12
	bitb	1234h(r6),#4
	bit	1234h(r6),#12
	; BIT R,R
	bit	rl3,r4
	bitb	rl3,r4
	bit	r13,r12

	call	@r8
	call	1234h
	call	1234h(r6)

	calr	$+300h
	calr	$-80h

	clr	rh6
	clrb	rh6
	clr	r12
	clrb	@r4
	clr	@r4
	clrb	1234h
	clr	1234h
	clrb	1234h(r2)
	clr	1234h(r2)

	com	rh6
	comb	rh6
	com	r12
	comb	@r4
	com	@r4
	comb	1234h
	com	1234h
	comb	1234h(r2)
	com	1234h(r2)

	expect 	1110
	comflg
	endexpect
	comflg	c
	comflg	z
	comflg	s
	comflg	p
	comflg	v
	comflg	p/v
	comflg	c,z,p/v
	expect	1366
	comflg	x
	endexpect
	expect	1367
	comflg	c,z,c,s
	endexpect

	; CP R,R
	cp	r2,r3
	cp	rh2,rl3
	cpb	rh2,rl3
	cp	rr4,rr6
	cpl	rr4,rr6
	; CP R,IM
	cp	rh2,#57h
	cpb	rh2,#57h
	cp	r5,#5678h
	cp	rr8,#12345678h
	cpl	rr8,#12345678h
	; CP R,IR
	cp	r2,@r3
	cp	rh2,@r3
	cpb	rh2,@r3
	cp	rr4,@r6
	cpl	rr4,@r6
	; CP R,DA
	cp	r2,1234h
	cp	rh2,1234h
	cpb	rh2,1234h
	cp	rr4,1234h
	cpl	rr4,1234h
	; CP R,X
	cp	r2,1234h(r10)
	cp	rh2,1234h(r10)
	cpb	rh2,1234h(r10)
	cp	rr4,1234h(r10)
	cpl	rr4,1234h(r10)
	; CP IR,IM
	cpb	@r2,#57h
	cp	@r5,#5678h
	expect	1130
	cpl	@r8,#12345678h
	endexpect
	; CP DA,IM
	cpb	1234h,#57h
	cp	1234h,#5678h
	expect	1130
	cpl	1234h,#12345678h
	endexpect
	; CP X,IM
	cpb	1234h(r14),#57h
	cp	1234h(r14),#5678h
	expect	1130
	cpl	1234h(r14),#12345678h
	endexpect

	cpd	rh5,@r10,r4,eq
	cpdb	rh5,@r10,r4,eq
	cpd	r5,@r10,r4,eq
	expect	1131
	cpdb	r5,@r10,r4,eq	; expect operand size conflict
	endexpect
	expect	1130
	cpd	rr8,@r10,r4,eq	; expect invalid operand size
	endexpect
	expect	95
	cpd	rh5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpd	r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpd	rh5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpd	r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpd	r5,@r6,r6,eq	; expect overlap src<->cnt
	endexpect

	cpdr	rh5,@r10,r4,eq
	cpdrb	rh5,@r10,r4,eq
	cpdr	r5,@r10,r4,eq
	expect	1131
	cpdrb	r5,@r10,r4,eq	; expect operand size conflict
	endexpect
	expect	1130
	cpdr	rr8,@r10,r4,eq	; expect invalid operand size
	endexpect
	expect	95
	cpdr	rh5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpdr	r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpdr	rh5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpdr	r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpdr	r5,@r6,r6,eq	; expect overlap src<->cnt
	endexpect

	cpi	rh5,@r10,r4,eq
	cpib	rh5,@r10,r4,eq
	cpi	r5,@r10,r4,eq
	expect	1131
	cpib	r5,@r10,r4,eq	; expect operand size conflict
	endexpect
	expect	1130
	cpi	rr8,@r10,r4,eq	; expect invalid operand size
	endexpect
	expect	95
	cpi	rh5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpi	r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpi	rh5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpi	r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpi	r5,@r6,r6,eq	; expect overlap src<->cnt
	endexpect

	cpi	rh5,@r10,r4
	cpib	rh5,@r10,r4
	cpi	r5,@r10,r4
	expect	1131
	cpib	r5,@r10,r4	; expect operand size conflict
	endexpect
	expect	1130
	cpi	rr8,@r10,r4	; expect invalid operand size
	endexpect
	expect	95
	cpi	rh5,@r5,r4	; expect overlap dst<->src
	endexpect
	expect	95
	cpi	r5,@r5,r4	; expect overlap dst<->src
	endexpect
	expect	95
	cpi	rh5,@r6,r5	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpi	r5,@r6,r5	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpi	r5,@r6,r6	; expect overlap src<->cnt
	endexpect

	cpir	rh5,@r10,r4,eq
	cpirb	rh5,@r10,r4,eq
	cpir	r5,@r10,r4,eq
	expect	1131
	cpirb	r5,@r10,r4,eq	; expect operand size conflict
	endexpect
	expect	1130
	cpir	rr8,@r10,r4,eq	; expect invalid operand size
	endexpect
	expect	95
	cpir	rh5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpir	r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpir	rh5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpir	r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpir	r5,@r6,r6,eq	; expect overlap src<->cnt
	endexpect

	cpsdb	@r5,@r10,r4,eq
	cpsd	@r5,@r10,r4,eq
	expect	95
	cpsd	@r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpsdb	@r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpsd	@r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpsdb	@r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpsd	@r5,@r6,r6,eq	; expect overlap src<->cnt
	endexpect

	cpsdrb	@r5,@r10,r4,eq
	cpsdr	@r5,@r10,r4,eq
	expect	95
	cpsdr	@r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpsdrb	@r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpsdr	@r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpsdrb	@r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpsdr	@r5,@r6,r6,eq	; expect overlap src<->cnt
	endexpect

	cpsib	@r5,@r10,r4,eq
	cpsi	@r5,@r10,r4,eq
	expect	95
	cpsi	@r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpsib	@r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpsi	@r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpsib	@r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpsi	@r5,@r6,r6,eq	; expect overlap src<->cnt
	endexpect

	cpsirb	@r5,@r10,r4,eq
	cpsir	@r5,@r10,r4,eq
	expect	95
	cpsir	@r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpsirb	@r5,@r5,r4,eq	; expect overlap dst<->src
	endexpect
	expect	95
	cpsir	@r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpsirb	@r5,@r6,r5,eq	; expect overlap dst<->cnt
	endexpect
	expect	95
	cpsir	@r5,@r6,r6,eq	; expect overlap src<->cnt
	endexpect

	dab	rh4
	expect	1131
	dab	r5
	endexpect

	dec	rh7
	decb	rh7
	dec	r12
	expect	1130,1130
	dec	rr4
	dec	rq8
	endexpect
	dec	@r5
	decb	@r9
	dec	1234h
	decb	1234h
	dec	1234h(r7)
	decb	1234h(r7)
	dec	r12,#1
	dec	r12,#16
	expect	1315
	dec	r12,#0
	endexpect
	expect	1320
	dec	r12,#17
	endexpect

	expect	1110
	di
	endexpect
	di	vi
	di	nvi
	di	vi,nvi
	expect	1368
	di	nmi
	endexpect
	expect	1369
	di	vi,vi
	endexpect

	div	rq8,rr2
	divl	rq8,rr2
	div	rr8,r2
	expect	1130
	div	r8,rh2		; 16:8 not supported
	endexpect
	expect	1131
	divl	rq12,rq4	; dest must have double with of src	
	endexpect
	div	rr4,#100
	div	rq4,#100
	divl	rq4,#100
	div	rr8,@r5
	div	rq8,@r5
	divl	rq8,@r5
	div	rr12,1234h
	div	rq12,1234h
	divl	rq12,1234h
	div	rr4,1234h(r5)
	div	rq4,1234h(r5)
	divl	rq4,1234h(r5)

	djnz	rh2,$-252
	dbjnz	rh2,$+2
	djnz	r5,$

	expect	1110
	ei
	endexpect
	ei	vi
	ei	nvi
	ei	vi,nvi
	expect	1368
	ei	nmi
	endexpect
	expect	1369
	ei	vi,vi
	endexpect

	ex	r5,r6
	ex	rh5,rh6
	exb	rh5,rh6
	expect	1130
	ex	rr6,rr8
	endexpect
	ex	r5,@r6
	ex	rh5,@r6
	ex	r5,1234h
	ex	rh5,1234h
	ex	r5,1234h(r6)
	ex	rh5,1234h(r6)
	ex	@r6,r5
	ex	@r6,rh5
	ex	1234h,r5
	ex	1234h,rh5
	ex	1234h(r6),r5
	ex	1234h(r6),rh5

	exts	r5
	extsb	r5
	exts	rr6
	exts	rq4
	extsl	rq4
	expect	1130
	exts	rh2
	endexpect

	halt

        in	r12,@r13
	inb	rh4,@r13
	in	rh4,@r13
	expect	1130
	in	rr12,@r13
	endexpect
	expect	1131
	inb	r4,@r13
	endexpect
	in	r12,1234h
	inb	rh4,1234h
	in	rh4,1234h
	sin	r12,1234h
	sinb	rh4,1234h
	sin	rh4,1234h

	inc	rh7
	incb	rh7
	inc	r12
	expect	1130,1130
	inc	rr4
	inc	rq8
	endexpect
	inc	@r5
	incb	@r9
	inc	1234h
	incb	1234h
	inc	1234h(r7)
	incb	1234h(r7)
	inc	r12,#1
	inc	r12,#16
	expect	1315
	inc	r12,#0
	endexpect
	expect	1320
	inc	r12,#17
	endexpect

	ind	@r4,@r8,r12
	indb	@r4,@r8,r12
	sind	@r4,@r8,r12
	sindb	@r4,@r8,r12
	indr	@r4,@r8,r12
	indrb	@r4,@r8,r12
	sindr	@r4,@r8,r12
	sindrb	@r4,@r8,r12
	ini	@r4,@r8,r12
	inib	@r4,@r8,r12
	sini	@r4,@r8,r12
	sinib	@r4,@r8,r12
	inir	@r4,@r8,r12
	inirb	@r4,@r8,r12
	sinir	@r4,@r8,r12
	sinirb	@r4,@r8,r12

	iret

	jp	@r8
	jp	1234h
	jp	1234h(r6)
	jp	,@r8
	jp	,1234h
	jp	,1234h(r6)
	jp	f,@r8
	jp	f,1234h
	jp	f,1234h(r6)
	jp	nc,@r8
	jp	nc,1234h
	jp	nc,1234h(r6)

	jr	f,$+80h
	jr	f,$-90h
	jr	$+80h
	jr	$-90h
	irp	cond,,z,nz,c,nc,pl,mi,ne,eq,ov,nov,pe,po,ge,lt,gt,le,uge,ult,ugt,ule
	jr	cond,$+80h
	jr	cond,$-90h
	endm

	; LD R,R
	ld	r2,r3
	ld	rh2,rl3
	ldb	rh2,rl3
	ld	rr4,rr6
	ldl	rr4,rr6
	; LD R,IR
	ld	r2,@r3
	ld	rh2,@r3
	ldb	rh2,@r3
	ld	rr4,@r6
	ldl	rr4,@r6
	; LD R,DA
	ld	r2,1234h
	ld	rh2,1234h
	ldb	rh2,1234h
	ld	rr4,1234h
	ldl	rr4,1234h
	; LD R,X
	ld	r2,1234h(r10)
	ld	rh2,1234h(r10)
	ldb	rh2,1234h(r10)
	ld	rr4,1234h(r10)
	ldl	rr4,1234h(r10)
	; LD R,BA
	ld	r2,r3(#4321h)
	ld	rh2,r3(#4321h)
	ldb	rh2,r3(#4321h)
	ld	rr4,r3(#4321h)
	ldl	rr4,r3(#4321h)
	; LD R,BX
	ld	r2,r3(r11)
	ld	rh2,r3(r11)
	ldb	rh2,r3(r11)
	ld	rr4,r3(r11)
	ldl	rr4,r3(r11)
	; LD IR,R
	ld	@r3,r2
	ld	@r3,rh2
	ldb	@r3,rh2
	ld	@r6,rr4
	ldl	@r6,rr4
	; LD DA,R
	ld	1234h,r2
	ld	1234h,rh2
	ldb	1234h,rh2
	ld	1234h,rr4
	ldl	1234h,rr4
	; LD X,R
	ld	1234h(r10),r2
	ld	1234h(r10),rh2
	ldb	1234h(r10),rh2
	ld	1234h(r10),rr4
	ldl	1234h(r10),rr4
	; LD BA,R
	ld	r7(#4321h),r2
	ld	r7(#4321h),rh2
	ldb	r7(#4321h),rh2
	ld	r7(#4321h),rr4
	ldl	r7(#4321h),rr4
	; LD BX,R
	ld	r7(r11),r2
	ld	r7(r11),rh2
	ldb	r7(r11),rh2
	ld	r7(r11),rr4
	ldl	r7(r11),rr4
	; LD R,IM
	ld	rh2,#57h
	ldb	rh2,#57h
	ld	r5,#5678h
	ld	rr8,#12345678h
	ldl	rr8,#12345678h
	; LD IR,IM
	ldb	@r2,#57h
	ld	@r5,#5678h
	expect	1130
	ldl	@r8,#12345678h
	endexpect
	; LD DA,IM
	ldb	1234h,#57h
	ld	1234h,#5678h
	expect	1130
	ldl	1234h,#12345678h
	endexpect
	; LD X,IM
	ldb	1234h(r14),#57h
	ld	1234h(r14),#5678h
	expect	1130
	ldl	1234h(r14),#12345678h
	endexpect

	lda	r4,1234h
	lda	r4,1234h(r5)
	lda	r4,r5(#1000)
	lda	r4,r5(r6)

	ldar	r4,$+2000

	ldctl	fcw,r9
	ldctl	refresh,r9
	expect	1440
	ldctl	psapseg,r9
	endexpect
	ldctl	psap,r9
	expect	1440
	ldctl	nspseg,r9
	endexpect
	ldctl	nsp,r9
	ldctl	r9,fcw
	ldctl	r9,refresh
	expect	1440
	ldctl	r9,psapseg
	endexpect
	ldctl	r9,psap
	expect	1440
	ldctl	r9,nspseg
	endexpect
	ldctl	r9,nsp

	ldctl	flags,rl0
	ldctlb	flags,rl0
	ldctl	rl0,flags
	ldctlb	rl0,flags

	ldd	@r4,@r5,r6
	lddb	@r4,@r5,r6
	lddr	@r4,@r5,r6
	lddrb	@r4,@r5,r6
	ldi	@r4,@r5,r6
	ldib	@r4,@r5,r6
	ldir	@r4,@r5,r6
	ldirb	@r4,@r5,r6

	ldk	r5,#10
	expect	1320
	ldk	r5,#20
	endexpect
	ld	r5,#10		; assembles as LDK

	ldm	r5,@r4,#7
	expect	1130
	ldm	rh5,@r4,#7	; invalid register size
	endexpect
	expect	370
	ldm	r5,@r4,#15	; register # wrapover
	endexpect
	ldm	r5,1234h,#7
	ldm	r5,1234h(r4),#7
	ldm	@r4,r5,#7
	ldm	1234h,r5,#7
	ldm	1234h(r4),r5,#7

	ldps	@r9
	ldps	4321h
	ldps	4321h(r9)

	ldr	rh5,$+100
	ldrb	rh5,$+100
	ldr	r5,$+100
	ldr     rr6,$+100
	ldrl	rr6,$+100

	ldr	$+100,rh5
	ldrb	$+100,rh5
	ldr	$+100,r5
	ldr     $+100,rr6
	ldrl	$+100,rr6
	expect	1130
	ldr	$+100,rq8
	endexpect

	mbit

	mreq	r14

	mres

	mset

	mult	rq8,rr2
	multl	rq8,rr2
	mult	rr8,r2
	expect	1130
	mult	r8,rh2		; 8b*8b->16b not supported
	endexpect
	expect	1131
	multl	rq12,rq4	; dest must have double with of src	
	endexpect
	mult	rr4,#100
	mult	rq4,#100
	multl	rq4,#100
	mult	rr8,@r5
	mult	rq8,@r5
	multl	rq8,@r5
	mult	rr12,1234h
	mult	rq12,1234h
	multl	rq12,1234h
	mult	rr4,1234h(r5)
	mult	rq4,1234h(r5)
	multl	rq4,1234h(r5)

	neg	rh6
	negb	rh6
	neg	r12
	negb	@r4
	neg	@r4
	negb	1234h
	neg	1234h
	negb	1234h(r2)
	neg	1234h(r2)

	nop

	; OR R,R
	or	r2,r3
	or	rh2,rl3
	orb	rh2,rl3
	expect	1130,1200
	or	rr4,rr6
	orl	rr4,rr6
	endexpect
	; OR R,IM
	or	rh2,#57h
	orb	rh2,#57h
	or	r5,#5678h
	; OR R,IR
	or	r2,@r3
	or	rh2,@r3
	orb	rh2,@r3
	; OR R,DA
	or	r2,1234h
	or	rh2,1234h
	orb	rh2,1234h
	; OR R,X
	or	r2,1234h(r10)
	or	rh2,1234h(r10)
	orb	rh2,1234h(r10)

	otdr	@r4,@r8,r12
	otdrb	@r4,@r8,r12
	sotdr	@r4,@r8,r12
	sotdrb	@r4,@r8,r12
	otir	@r4,@r8,r12
	otirb	@r4,@r8,r12
	sotir	@r4,@r8,r12
	sotirb	@r4,@r8,r12

        out	@r13,r12
	outb	@r13,rh4
	out	@r13,rh4
	expect	1130
	out	@r13,rr12
	endexpect
	expect	1131
	outb	@r13,r4
	endexpect
	out	1234h,r12
	outb	1234h,rh4
	out	1234h,rh4
	sout	1234h,r12
	soutb	1234h,rh4
	sout	1234h,rh4

	outd	@r4,@r8,r12
	outdb	@r4,@r8,r12
	soutd	@r4,@r8,r12
	soutdb	@r4,@r8,r12
	outi	@r4,@r8,r12
	outib	@r4,@r8,r12
	souti	@r4,@r8,r12
	soutib	@r4,@r8,r12

	pop	r7,@r6
	pop	rr8,@r6
	popl	rr8,@r6
	expect	1130
	pop	rh2,@r6		; byte op not supported
	endexpect
	expect	95
	pop	r7,@r7		; overlap
	endexpect
	expect	95
	pop	rr10,@r11	; overlap
	endexpect
	pop	@r7,@r6
	popl	@r7,@r6
	expect	95
	popl	@r6,@r6		; overlap
	endexpect
	pop	1234h,@r6
	popl	1234h,@r6
	pop	1234h(r7),@r6
	popl	1234h(r7),@r6

	push	@r6,r7
	push	@r6,rr8
	pushl	@r6,rr8
	expect	1130
	push	@r6,rh2		; byte op not supported
	endexpect
	expect	95
	push	@r7,r7		; overlap
	endexpect
	expect	95
	push	@r11,rr10	; overlap
	endexpect
	push	@r6,@r7
	pushl	@r6,@r7
	expect	95
	pushl	@r6,@r6		; overlap
	endexpect
	push	@r6,1234h
	pushl	@r6,1234h
	push	@r6,1234h(r7)
	pushl	@r6,1234h(r7)
	push	@r6,#1234h
	expect	1130
	pushl	@r6,#12345678h
	endexpect

	; RES R,IM
	res	rl3,#4
	resb	rl3,#4
	res	r13,#12
	; RES IR,IM
	res	@r9,#12
	resb	@r9,#4
	res	@r9,#12
	; RES DA,IM
	res	1234h,#12
	resb	1234h,#4
	res	1234h,#12
	; RES X,IM
	res	1234h(r6),#12
	resb	1234h(r6),#4
	res	1234h(r6),#12
	; RES R,R
	res	rl3,r4
	resb	rl3,r4
	res	r13,r12

	expect 	1110
	resflg
	endexpect
	resflg	c
	resflg	z
	resflg	s
	resflg	p
	resflg	v
	resflg	p/v
	resflg	c,z,p/v
	expect	1366
	resflg	x
	endexpect
	expect	1367
	resflg	c,z,c,s
	endexpect

	ret
	ret	eq
	ret	mi
	ret	c

	rl	r2
	rl	rh2
	rlb	rh2
	expect	1130
	rl	rr4	; wrong size
	endexpect
	rl	r2,#2
	expect	1320
	rl	r2,#3	; range overflow
	endexpect

	rlc	r2
	rlc	rh2
	rlcb	rh2
	expect	1130
	rlc	rr4	; wrong size
	endexpect
	rlc	r2,#2
	expect	1320
	rlc	r2,#3	; range overflow
	endexpect

	rldb	rh1,rl1

	rr	r2
	rr	rh2
	rrb	rh2
	expect	1130
	rr	rr4	; wrong size
	endexpect
	rr	r2,#2
	expect	1320
	rr	r2,#3	; range overflow
	endexpect

	rrc	r2
	rrc	rh2
	rrcb	rh2
	expect	1130
	rrc	rr4	; wrong size
	endexpect
	rrc	r2,#2
	expect	1320
	rrc	r2,#3	; range overflow
	endexpect

	rrdb	rh1,rl1

	sbc	r2,r3
	sbc	rh2,rl3
	sbcb	rh2,rl3
	expect	1130
        sbc     rr4,rr6
	endexpect

	sc	#170
	sc	170

	sda	r5,r1
	sda	rh4,r1
	sdab	rh4,r1
	sda	rr4,r1
	sdal	rr4,r1
	expect	1130
	sda	rq4,r1	; invalid size
	endexpect
	
	sdl	r5,r1
	sdl	rh4,r1
	sdlb	rh4,r1
	sdl	rr4,r1
	sdll	rr4,r1
	expect	1130
	sdl	rq4,r1	; invalid size
	endexpect


	; SET R,IM
	set	rl3,#4
	setb	rl3,#4
	set	r13,#12
	; SET IR,IM
	set	@r9,#12
	setb	@r9,#4
	set	@r9,#12
	; SET DA,IM
	set	1234h,#12
	setb	1234h,#4
	set	1234h,#12
	; SET X,IM
	set	1234h(r6),#12
	setb	1234h(r6),#4
	set	1234h(r6),#12
	; SET R,R
	set	rl3,r4
	setb	rl3,r4
	set	r13,r12

	expect 	1110
	setflg
	endexpect
	setflg	c
	setflg	z
	setflg	s
	setflg	p
	setflg	v
	setflg	p/v
	setflg	c,z,p/v
	expect	1366
	setflg	x
	endexpect
	expect	1367
	setflg	c,z,c,s
	endexpect

	sla	r10
	sla	rl2
	slab	rl2
	sla	rr10
	slal	rr10
	sla	rl2,#0
	sla	rl2,#8
	expect	1320
	sla	rl2,#9		; overflow
	endexpect
	sla	r10,#0
	sla	r10,#16
	expect	1320
	sla	r10,#17		; overflow
	endexpect
	sla	rr10,#0
	sla	rr10,#32
	expect	1320
	sla	rr10,#33	; overflow
	endexpect

	sll	r10
	sll	rl2
	sllb	rl2
	sll	rr10
	slll	rr10
	sll	rl2,#0
	sll	rl2,#8
	expect	1320
	sll	rl2,#9		; overflow
	endexpect
	sll	r10,#0
	sll	r10,#16
	expect	1320
	sll	r10,#17		; overflow
	endexpect
	sll	rr10,#0
	sll	rr10,#32
	expect	1320
	sll	rr10,#33	; overflow
	endexpect

	sra	r10
	sra	rl2
	srab	rl2
	sra	rr10
	sral	rr10
	expect	1315
	sra	rl2,#0		; underflow
	endexpect
	sra	rl2,#1
	sra	rl2,#8
	expect	1320
	sra	rl2,#9		; overflow
	endexpect
	expect	1315
	sra	r10,#0		; underflow
	endexpect
	sra	r10,#1
	sra	r10,#16
	expect	1320
	sra	r10,#17		; overflow
	endexpect
	expect	1315
	sra	rr10,#0
	endexpect
	sra	rr10,#1
	sra	rr10,#32
	expect	1320
	sra	rr10,#33	; overflow
	endexpect

	srl	r10
	srl	rl2
	srlb	rl2
	srl	rr10
	srll	rr10
	expect	1315
	srl	rl2,#0		; underflow
	endexpect
	srl	rl2,#1
	srl	rl2,#8
	expect	1320
	srl	rl2,#9		; overflow
	endexpect
	expect	1315
	srl	r10,#0		; underflow
	endexpect
	srl	r10,#1
	srl	r10,#16
	expect	1320
	srl	r10,#17		; overflow
	endexpect
	expect	1315
	srl	rr10,#0		; underflow
	endexpect
	srl	rr10,#1
	srl	rr10,#32
	expect	1320
	srl	rr10,#33	; overflow
	endexpect

	; SUB R,R
	sub	r2,r3
	sub	rh2,rl3
	subb	rh2,rl3
	sub	rr4,rr6
	subl	rr4,rr6
	; SUB R,IM
	sub	rh2,#57h
	subb	rh2,#57h
	sub	r5,#5678h
	sub	rr8,#12345678h
	subl	rr8,#12345678h
	; SUB R,IR
	sub	r2,@r3
	sub	rh2,@r3
	subb	rh2,@r3
	sub	rr4,@r6
	subl	rr4,@r6
	; SUB R,DA
	sub	r2,1234h
	sub	rh2,1234h
	subb	rh2,1234h
	sub	rr4,1234h
	subl	rr4,1234h
	; SUB R,X
	sub	r2,1234h(r10)
	sub	rh2,1234h(r10)
	subb	rh2,1234h(r10)
	sub	rr4,1234h(r10)
	subl	rr4,1234h(r10)

	tcc	r10
	tcc	rl2
	tccb	rl2
	tcc	eq,r10
	tcc	eq,rl2
	tccb	eq,rl2

	test	rh6
	testb	rh6
	test	r12
	test	rr12
	testl	rr12
	testb	@r4
	test	@r4
	testl	@r4
	testb	1234h
	test	1234h
	testl	1234h
	testb	1234h(r2)
	test	1234h(r2)
	testl	1234h(r2)

	trdb	@r6,@r7,r8
	trdrb	@r6,@r7,r8
	trib	@r6,@r7,r8
	trirb	@r6,@r7,r8
	trtdb	@r6,@r7,r8
	trtdrb	@r6,@r7,r8
	trtib	@r6,@r7,r8
	trtirb	@r6,@r7,r8

	tset	rh6
	tsetb	rh6
	tset	r12
	tsetb	@r4
	tset	@r4
	tsetb	1234h
	tset	1234h
	tsetb	1234h(r2)
	tset	1234h(r2)

	; XOR R,R
	xor	r2,r3
	xor	rh2,rl3
	xorb	rh2,rl3
	expect	1130,1200
	xor	rr4,rr6
	xorl	rr4,rr6
	endexpect
	; XOR R,IM
	xor	rh2,#57h
	xorb	rh2,#57h
	xor	r5,#5678h
	; XOR R,IR
	xor	r2,@r3
	xor	rh2,@r3
	xorb	rh2,@r3
	; XOR R,DA
	xor	r2,1234h
	xor	rh2,1234h
	xorb	rh2,1234h
	; XOR R,X
	xor	r2,1234h(r10)
	xor	rh2,1234h(r10)
	xorb	rh2,1234h(r10)

	; testing a bit the parser about how 'good' it is at detecting
	; expressions of the form xxx(yyy), without producing false positives:

	ld	r7,10(r5)		; is
	ld	r7,10 (r5)		; is
	ld	r7,(10)+(20)		; is NOT
	ld	r7,(10) + (20)		; is NOT
	ld	r7,10+20		; is for sure NOT
	ld	r7,abs(-10)(r5)		; IS
	ld	r7,r2(#'(')		; IS

	; bit symbols

	res	200h,#5
mybitb	defbitb	200h,#5
mybit	defbit	200h,#5
	res	mybitb
	res	mybit

	; register aliases

reg_rh0	equ	rh0
reg_rh1	equ	rh1
reg_rh2	equ	rh2
reg_rh3	equ	rh3
reg_rh4	equ	rh4
reg_rh5	equ	rh5
reg_rh6	equ	rh6
reg_rh7	equ	rh7
reg_rl0	equ	rl0
reg_rl1	equ	rl1
reg_rl2	equ	rl2
reg_rl3	equ	rl3
reg_rl4	equ	rl4
reg_rl5	equ	rl5
reg_rl6	equ	rl6
reg_rl7	equ	rl7

reg_r0	equ	r0
reg_r1	equ	r1
reg_r2	equ	r2
reg_r3	equ	r3
reg_r4	equ	r4
reg_r5	equ	r5
reg_r6	equ	r6
reg_r7	equ	r7
reg_r8	equ	r8
reg_r9	equ	r9
reg_r10	equ	r10
reg_r11	equ	r11
reg_r12	equ	r12
reg_r13	equ	r13
reg_r14	equ	r14
reg_r15	equ	r15

reg_rr0	reg	rr0
reg_rr2	reg	rr2
reg_rr4	reg	rr4
reg_rr6	reg	rr6
reg_rr8	reg	rr8
reg_rr10 reg	rr10
reg_rr12 reg	rr12
reg_rr14 reg	rr14

reg_rq0	equ	rq0
reg_rq4	equ	rq4
reg_rq8	equ	rq8
reg_rq12 equ	rq12

	ld	r12,#20
	ld	reg_r12,#20

	; all right, now the (rudimentary) support for segmented mode:

	cpu	z8001

	; indirect addressing is done via register pairs:

	ld	r3,@rr4
	expect	2212
	ld	r3,@r4
	endexpect

	; long addresses: may be just 'linear 23 bit':

	ld	r3,45678h
	ld	r3,45678h(r3)

	; short addresses (offset < 256) currently must be forced:

	ld	r3,40078h
	ld	r3,40078h(r3)
	ld	r3,|40078h|
	ld	r3,|40078h|(r3)

	; <<segment>>offset syntax

	ld	r3,<<4>>5678h
	ld	r3,<<4>>78h
	ld	r3,|<<4>>78h|
	ld	r3,|<<4>>78h|(r4)

	; robust enough against spaces?

	ld	r3, << 4 >> 5678h
	ld	r3, << 4 >> 78h
	ld	r3, | << 4 >> 78h |
	ld	r3, | << 4 >> 78h | (r4)

	; a << or >> operator in the segment part makes things even
	; more interesting.  Use parentheses to clarify:

	ld	r3,<<10h>>2>>5678h	; results in segment 10h, offset 0
	ld	r3,<<(10h>>2)>>5678h	; results in segment 4h, offset 5678h
	ld	r3,<<(10h>>2)>>78h
	ld	r3,<<(10h>>2)>>78h(r4)
	ld	r3,|<<(10h>>2)>>78h|(r4)
