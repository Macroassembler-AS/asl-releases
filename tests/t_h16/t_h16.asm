	cpu		hd641016
	include		"regh16.inc"

	page		0
	supmode		on

	mov.b:g		r5,r12
	mov:g.w		r5,r12
	mov.l:g		r5,r12
	mov.b:r		r5,r12
	mov:r.w		r5,r12
	mov.l:r		r5,r12
	mov.b:g		r5,r0
	mov.w:g		r5,r0
	mov.l:g		r5,r0

	mov.l		#$2345.w,r13

	mov.l		@r4,r0
	mov.l		@r4,r13
	mov.l		@(r4),r0
	mov.l		@(r4),r13

	mov.l		@($23,r4),r0
	mov.l		@($23,r4),r13
	mov.l		@($23.b,r4),r0
	mov.l		@($23.b,r4),r13
	mov.l		@($23.w,r4),r0
	mov.l		@($23.w,r4),r13
	mov.l		@($23.l,r4),r0
	mov.l		@($23.l,r4),r13
	mov.l		@($2345,r4),r0
	mov.l		@($2345,r4),r13
	mov.l		@($2345.w,r4),r0
	mov.l		@($2345.w,r4),r13
	mov.l		@($2345.l,r4),r0
	mov.l		@($2345.l,r4),r13
	mov.l		@($234567,r4),r0
	mov.l		@($234567,r4),r13
	mov.l		@($234567.l,r4),r0
	mov.l		@($234567.l,r4),r13

	mov.l		@r4+,r0
	mov.l		@r4+,r13
	mov.l		@(r4+),r0
	mov.l		@(r4+),r13

	mov.l		@-r4,r0
	mov.l		@-r4,r13
	mov.l		@(-r4),r0
	mov.l		@(-r4),r13

	mov.l		@$34,r0
	mov.l		@$34,r13
	mov.l		@($34),r0
	mov.l		@($34),r13
	mov.l		@$34.b,r0
	mov.l		@$34.b,r13
	mov.l		@($34.b),r0
	mov.l		@($34.b),r13
	mov.l		@$34.w,r0
	mov.l		@$34.w,r13
	mov.l		@($34.w),r0
	mov.l		@($34.w),r13
	mov.l		@$34.l,r0
	mov.l		@$34.l,r13
	mov.l		@($34.l),r0
	mov.l		@($34.l),r13
	mov.l		@$ffff89,r0
	mov.l		@$ffff89,r13
	mov.l		@($ffff89),r0
	mov.l		@($ffff89),r13
	mov.l		@$3456,r0
	mov.l		@$3456,r13
	mov.l		@($3456),r0
	mov.l		@($3456),r13
	mov.l		@$3456.w,r0
	mov.l		@$3456.w,r13
	mov.l		@($3456.w),r0
	mov.l		@($3456.w),r13
	mov.l		@$3456.l,r0
	mov.l		@$3456.l,r13
	mov.l		@($3456.l),r0
	mov.l		@($3456.l),r13
	mov.l		@$ff89ab,r0
	mov.l		@$ff89ab,r13
	mov.l		@($ff89ab),r0
	mov.l		@($ff89ab),r13
	mov.l		@$345678,r0
	mov.l		@$345678,r13
	mov.l		@($345678),r0
	mov.l		@($345678),r13
	mov.l		@$345678.l,r0
	mov.l		@$345678.l,r13
	mov.l		@($345678.l),r0
	mov.l		@($345678.l),r13
	mov.l		@$89abcd,r0
	mov.l		@$89abcd,r13
	mov.l		@($89abcd),r0
	mov.l		@($89abcd),r13

	mov.l		@(r4*1),r0
	mov.l		@(r4*1),r13
	mov.l		@(-10,r4*1),r0
	mov.l		@(-10,r4*1),r13
	mov.l		@(-10.b,r4*1),r0
	mov.l		@(-10.b,r4*1),r13
	mov.l		@(-10.w,r4*1),r0
	mov.l		@(-10.w,r4*1),r13
	mov.l		@(-10.l,r4*1),r0
	mov.l		@(-10.l,r4*1),r13
	mov.l		@(-1000,r4*2),r0
	mov.l		@(-1000,r4*2),r13
	mov.l		@(-1000.w,r4*2),r0
	mov.l		@(-1000.w,r4*2),r13
	mov.l		@(-1000.l,r4*2),r0
	mov.l		@(-1000.l,r4*2),r13
	mov.l		@(-100000,r4*8),r0
	mov.l		@(-100000,r4*8),r13
	mov.l		@(-100000.l,r4*8),r0
	mov.l		@(-100000.l,r4*8),r13

	mov.l		@(r4,r2),r0
	mov.l		@(r4,r2),r13
	mov.l		@(r4,r2.w),r0
	mov.l		@(r4,r2.w),r13
	mov.l		@(r4,r2*4),r0
	mov.l		@(r4,r2*4),r13
	mov.l		@(r4,r2.w*4),r0
	mov.l		@(r4,r2.w*4),r13
	mov.l		@(10,r4,r2.l*8),r0
	mov.l		@(10,r4,r2.l*8),r13
	mov.l		@(10.b,r4,r2.l*8),r0
	mov.l		@(10.b,r4,r2.l*8),r13
	mov.l		@(10.w,r4,r2.l*8),r0
	mov.l		@(10.w,r4,r2.l*8),r13
	mov.l		@(10.l,r4,r2.l*8),r0
	mov.l		@(10.l,r4,r2.l*8),r13
	mov.l		@(1000,r4,r2.l*8),r0
	mov.l		@(1000,r4,r2.l*8),r13
	mov.l		@(1000.w,r4,r2.l*8),r0
	mov.l		@(1000.w,r4,r2.l*8),r13
	mov.l		@(1000.l,r4,r2.l*8),r0
	mov.l		@(1000.l,r4,r2.l*8),r13
	mov.l		@(100000,r4,r2.l*8),r0
	mov.l		@(100000,r4,r2.l*8),r13
	mov.l		@(100000.l,r4,r2.l*8),r0
	mov.l		@(100000.l,r4,r2.l*8),r13

	mov.l		@(r2,pc),r0
	mov.l		@(r2,pc),r13
	mov.l		@(r2.w,pc),r0
	mov.l		@(r2.w,pc),r13
	mov.l		@(r2*4,pc),r0
	mov.l		@(r2*4,pc),r13
	mov.l		@(r2.w*4,pc),r0
	mov.l		@(r2.w*4,pc),r13
	mov.l		@(*+20,r2.l*8,pc),r0
	mov.l		@(*+20,r2.l*8,pc),r13
	mov.l		@(*+20.b,r2.l*8,pc),r0
	mov.l		@(*+20.b,r2.l*8,pc),r13
	mov.l		@(*+20.w,r2.l*8,pc),r0
	mov.l		@(*+20.w,r2.l*8,pc),r13
	mov.l		@(*+20.l,r2.l*8,pc),r0
	mov.l		@(*+20.l,r2.l*8,pc),r13
	mov.l		@(*+2000,r2.l*8,pc),r0
	mov.l		@(*+2000,r2.l*8,pc),r13
	mov.l		@(*+2000.w,r2.l*8,pc),r0
	mov.l		@(*+2000.w,r2.l*8,pc),r13
	mov.l		@(*+2000.l,r2.l*8,pc),r0
	mov.l		@(*+2000.l,r2.l*8,pc),r13
	mov.l		@(*+200000,r2.l*8,pc),r0
	mov.l		@(*+200000,r2.l*8,pc),r13
	mov.l		@(*+200000.l,r2.l*8,pc),r0
	mov.l		@(*+200000.l,r2.l*8,pc),r13
	mov.l		@(10,r13),@(*+200000.l,r2.l*8,pc)

	mov.l		@pc,r0
	mov.l		@pc,r13
	mov.l		@(pc),r0
	mov.l		@(pc),r13
	mov.l		@(*+20,pc),r0
	mov.l		@(*+20,pc),r13
	mov.l		@(*+20.b,pc),r0
	mov.l		@(*+20.b,pc),r13
	mov.l		@(*+20.w,pc),r0
	mov.l		@(*+20.w,pc),r13
	mov.l		@(*+20.l,pc),r0
	mov.l		@(*+20.l,pc),r13
	mov.l		@(*+2000,pc),r0
	mov.l		@(*+2000,pc),r13
	mov.l		@(*+2000.w,pc),r0
	mov.l		@(*+2000.w,pc),r13
	mov.l		@(*+2000.l,pc),r0
	mov.l		@(*+2000.l,pc),r13
	mov.l		@(*+200000,pc),r0
	mov.l		@(*+200000,pc),r13
	mov.l		@(*+200000.l,pc),r0
	mov.l		@(*+200000.l,pc),r13
	mov.l		@(10,r13),@(*+200000.l,pc)

	mov.l		@@r4,r0
	mov.l		@@r4,r13
	mov.l		@(@(r4)),r0
	mov.l		@(@(r4)),r13
	mov.l		@(10,@(r4)),r0
	mov.l		@(10,@(r4)),r13
	mov.l		@(@(10,r4)),r0
	mov.l		@(@(10,r4)),r13
	mov.l		@(10,@(10,r4)),r0
	mov.l		@(10,@(10,r4)),r13
	mov.l		@(10.l,@(r4)),r0
	mov.l		@(10.l,@(r4)),r13
	mov.l		@(@(10.l,r4)),r0
	mov.l		@(@(10.l,r4)),r13
	mov.l		@(10.l,@(10.l,r4)),r0
	mov.l		@(10.l,@(10.l,r4)),r13

	mov.l		<crn>r5,r0
	mov.l		<crn>r5,r13
	mov.l		<prn>r5,r0
	mov.l		<prn>r5,r13
	mov.l		<prn><crn>r5,r0
	mov.l		<prn><crn>r5,r13

	mov.l		#30,@r13		; expect Q format
	mov.l		#-30,@r13
	mov.l		#$ffffffa0,@r13
	mov.w		#30,@r13
	mov.w		#-20,@r13
	mov.w		#$ffa0,@r13
	mov.b		#30,@r13
	mov.b		#-30,@r13
	mov.b		#$a0,@r13

	mov.l		#3,r13			; expect RQ format
	mov.l		#-3,r13
	mov.l		#$fffffffa,r13
	mov.w		#3,r13
	mov.w		#-3,r13
	mov.w		#$fffa,r13
	mov.b		#3,r13
	mov.b		#-3,r13
	mov.b		#$fa,r13

	mov:rq.l	#3,r13
	;mov:rq.l	#13,r13			; expect fail
	mov.l		#13,r13			; expect Q
	mov:g.l		#13,r13

	irp		instr,add,sub,cmp,mov
	instr:g.w	r0,@($23f5:16,r7)
	instr:g.w	#$a4.b,@$6bc.w
	instr:g.w	@($3e:8,r4*4),@($2436:16,r9.l*2,r13)
	instr:g.w	@r4+,@-r15
	instr:g.w	@((*+$2a):8,pc),@((*+$234a):16,r2.w*8,pc)
	instr:g.w	@($fe:8,@($0124:16,r4)),r7
	instr:g.w	cr0,@($23F5:16,pr7)
	endm

	irp		instr,adds,cmps,subs,movs
	irp		sz,b,w,l
	instr.sz	#4,r0
	instr.sz	#4,r13
	instr.sz	@r5,r0
	instr.sz	@r5,r13
	endm
	endm

	irp		instr,addx,subx,and,or,xor
	irp		sz,b,w,l
	instr.sz	#40.b,r0
	instr.sz	#40.b,r13
	instr.sz	@(r5,1000),r0
	instr.sz	@(r5,1000),r13
	endm
	endm

	irp		instr,dadd,dsub
	instr		#40.b,r0
	instr.b		#40.b,r0
	instr		#40.b,r13
	instr.b		#40.b,r13
	instr		@(r5,1000),r0
	instr.b		@(r5,1000),r0
	instr		@(r5,1000),r13
	instr.b		@(r5,1000),r13
	endm

	irp		instr,mulxs,mulxu,divxs,divxu
	irp		sz,b,w
	instr.sz	#40.b,r0
	instr.sz	#40.b,r13
	instr.sz	@(r5,1000),r0
	instr.sz	@(r5,1000),r13
	endm
	endm

	irp		instr,neg,not,negx,tst,clr
	irp		sz,b,w,l
	instr.sz	r0
	instr.sz	r5
	instr.sz	@$40.b
	instr.sz	@(r5,1000)
	endm
	endm
	irp		sz,b,w,l
	tst.sz		#$40
	endm
	irp		instr,dneg,tas
	instr.b		r0
	instr		r0
	instr.b		r5
	instr		r5
	instr.b		@$40.b
	instr		@$40.b
	instr.b		@(r5,1000)
	instr		@(r5,1000)
	endm

	irp		instr,exts,extu
	irp		sz,b,w,l
	instr.sz	r13
	endm
	endm

	irp		instr,andc,orc,xorc,ldc
	instr		#$55,ccr
	instr.b		#$55,ccr
	instr		#$55,vbnr
	instr.b		#$55,vbnr
	instr		#$55aa1234,cbnr
	instr.l		#$55aa1234,cbnr
	instr		#$55aa1234,bsp
	instr.l		#$55aa1234,bsp
	instr		#$55,bmr
	instr.b		#$55,bmr
	instr		#$55,gbnr
	instr.b		#$55,gbnr
	instr		#$55aa,sr
	instr.w		#$55aa,sr
	instr		#$55aa1234,ebr
	instr.l		#$55aa1234,ebr
	instr		#$55aa1234,rbr
	instr.l		#$55aa1234,rbr
	instr		#$55aa1234,usp
	instr.l		#$55aa1234,usp
	instr		#$55aa1234,ibr
	instr.l		#$55aa1234,ibr
	endm

	irp		instr,stc
	instr		ccr,@$55
	instr.b		ccr,@$55
	instr		vbnr,@$55
	instr.b		vbnr,@$55
	instr		cbnr,@$55aa1234
	instr.l		cbnr,@$55aa1234
	instr		bsp,@$55aa1234
	instr.l		bsp,@$55aa1234
	instr		bmr,@$55
	instr.b		bmr,@$55
	instr		gbnr,@$55
	instr.b		gbnr,@$55
	instr		sr,@$55aa
	instr.w		sr,@$55aa
	instr		ebr,@$55aa1234
	instr.l		ebr,@$55aa1234
	instr		rbr,@$55aa1234
	instr.l		rbr,@$55aa1234
	instr		usp,@$55aa1234
	instr.l		usp,@$55aa1234
	instr		ibr,@$55aa1234
	instr.l		ibr,@$55aa1234
	endm

	irp		instr,bra,bsr,bcc,bcc:g,bhs,bhs:g,bcs,bcs:g,blo,blo:g,bne,bne:g,beq,beq:g,bge,bge:g,blt,blt:g,bgt,bgt:g,ble,ble:g,bhi,bhi:g,bls,bls:g,bpl,bpl:g,bmi,bmi:g,bvc,bvc:g,bvs,bvs:g,bt,bt:g,bf,bf:g
	instr		*+10
	instr.b		*+10
	instr.w		*+10
	instr.l		*+10
	instr		*-10
	instr.b		*-10
	instr.w		*-10
	instr.l		*-10
	instr		*+1000
	;instr.b		*+1000
	instr.w		*+1000
	instr.l		*+1000
	instr		*-1000
	;instr.b		*-1000
	instr.w		*-1000
	instr.l		*-1000
	instr		*+100000
	;instr.b		*+100000
	;instr.w		*+100000
	instr.l		*+100000
	endm

	rts
	reset
	rte
	rtr
	sleep
	nop
	icbn
	dcbn

	irp		instr,rotl,rotr,rotxl,rotxr,shal,shar,shll,shlr
	irp		sz,b,w,l
	instr.sz	#7,r0
	instr.sz	#7,r13
	instr.sz	#7,@(100,r13)
	instr.sz	r4,r0
	instr.sz	r4,r13
	instr.sz	r4,@(100,r13)
	endm
	endm

	irp		cond,cc,hs,cs,lo,ne,eq,ge,lt,gt,le,hi,ls,pl,mi,vc,vs,t,f
	set/cond	r13
	set/cond.b	r13
	set/cond	@r13
	set/cond.b	@r13
	trap/cond
	scb/cond	r13,*+10
	scb/cond.b	r13,*+10
	scb/cond.w	r13,*+10
	scb/cond.l	r13,*+10
	scb/cond	r13,*+1000
	scb/cond.w	r13,*+1000
	scb/cond.l	r13,*+1000
	scb/cond	r13,*+100000
	scb/cond.l	r13,*+100000
	endm

	irp		instr,jmp,jsr
	instr		@r5
	instr		@(r4*1,r5)
	instr		@(r4*2,r5)
	endm

	irp		sz,b,w
	swap.sz		r13
	swap.sz		r0
	swap.sz		@r5+
	swap.sz		@(*,pc)
	endm

	xch		r4,r13
	xch		r13,r4
	xch.l		r4,r13
	xch.l		r13,r4

	link		r13,#-4
	link.b		r13,#-4
	link.w		r13,#-4
	link.l		r13,#-4
	link		r13,#-400
	link.w		r13,#-400
	link.l		r13,#-400
	link		r13,#-40000
	link.l		r13,#-40000
	unlk		r13
	trapa		#13
	rtd		#-4
	rtd.b		#-4
	rtd.w		#-4
	rtd.l		#-4
	rtd		#-400
	rtd.w		#-400
	rtd.l		#-400
	rtd		#-40000
	rtd.l		#-40000

	ldm.w		@(100,r13),r0,r3-r12,r15
	stm.l		r0,r3-r12,r15,@(100,r13)
	cgbn		r13
	cgbn		r13,r0-r3,r8
	cgbn		#13
	cgbn		#13,r0-r3,r8
	pgbn		r0-r3,r8
	pgbn
	pgbn.b		r0-r3,r8
	pgbn.b

	irp		instr,movfp,movfpe
	instr		@(100,r4),r13
	instr.w		@(100,r4),r13
	instr.l		@(100,r4),r13
	instr		@(100,r4),@(1000,r13)
	instr.w		@(100,r4),@(1000,r13)
	instr.l		@(100,r4),@(1000,r13)
	endm
	movfpe.b	@(100,r4),r13
	movfpe.b	@(100,r4),@(1000,r13)

	irp		instr,movtp,movtpe
	instr		r13,@(100,r4)
	instr.w		r13,@(100,r4)
	instr.l		r13,@(100,r4)
	instr		@(1000,r13),@(100,r4)
	instr.w		@(1000,r13),@(100,r4)
	instr.l		@(1000,r13),@(100,r4)
	endm
	movtpe.b	r13,@(100,r4)
	movtpe.b	@(1000,r13),@(100,r4)

	movf		r4
	movf.b		@r4
	movf.w		@(r4,100)
	movf.l		@(r4,100000)

	mova		@(1000,r4),r13
	mova.l		@(1000,r4),r13

	scmp/eq/f.b	r3,r4,r5,r6
	scmp/eq/f.w	r3,r4,r5,r6
	scmp/eq/f.b	@r3+,@r4+,r5,r6
	scmp/eq/f.w	@r3+,@r4+,r5,r6
	scmp/eq/b.b	r3,r4,r5,r6
	scmp/eq/b.w	r3,r4,r5,r6
	scmp/eq/b.b	@-r3,@-r4,r5,r6
	scmp/eq/b.w	@-r3,@-r4,r5,r6

	smov/f.b	r3,r4,r5
	smov/f.w	r3,r4,r5
	smov/f.b	@r3+,@r4+,r5
	smov/f.w	@r3+,@r4+,r5
	smov/b.b	r3,r4,r5
	smov/b.w	r3,r4,r5
	smov/b.b	@-r3,@-r4,r5
	smov/b.w	@-r3,@-r4,r5

	ssch/eq/f.b	r3,r4,r5,r6
	ssch/eq/f.w	r3,r4,r5,r6
	ssch/eq/f.b	@r3,@r4+,r5,r6
	ssch/eq/f.w	@r3,@r4+,r5,r6
	ssch/eq/b.b	r3,r4,r5,r6
	ssch/eq/b.w	r3,r4,r5,r6
	ssch/eq/b.b	@r3,@-r4,r5,r6
	ssch/eq/b.w	@r3,@-r4,r5,r6

	sstr/f.b	r3,r4,r5
	sstr/f.w	r3,r4,r5
	sstr/f.b	r3,@r4+,r5
	sstr/f.w	r3,@r4+,r5
	sstr/b.b	r3,r4,r5
	sstr/b.w	r3,r4,r5
	sstr/b.b	r3,@-r4,r5
	sstr/b.w	r3,@-r4,r5

	bfext		r3,r4,@r5,@$123456
	bfins		r3,r4,@r5,@$123456
	bfsch		r3,r4,@r5,@$123456
	bfmov		r3,r4,r5,r6,r7,r8
	bfmov		r3,r4,r5,r6,@r7,@r8

bbit	bit.b		7,$121
bbit2	bit.b		#7,@$121
wbit	bit.w		15,$122
wbit2	bit.w		#15,@$122
lbit	bit.l		31,$124
lbit2	bit.l		#31,@$124

	irp		instr,bclr,bnot,bset,btst
	instr		bbit
	instr.b		#7,@$121
	instr		wbit
	instr.w		#15,@$122
	instr		lbit
	instr.l		#31,@$124
	instr.b		r4,r7
	instr.w		r4,r7
	instr.l		r4,r7
	instr.b		r4,@(1000,r7)
	instr.w		r4,@(1000,r7)
	instr.w		r4,@(1000,r7)
	endm

	; register alias: R15 of global bank is SP:

	mov.l		@sp+,r4
	mov.l		@r15+,r4

	; optimization to MOVF:

	mov.l		r0,@r4
	mov.l		r1,@r4
	mov.l		r0,@(*+20,pc)
	mov.l		r1,@(*+20,pc)
	mov.l		r0,@(*+200,pc)
	mov.l		r1,@(*+200,pc)
	mov.l		r0,@(*+200000,r5,pc)
	mov.l		r1,@(*+200000,r5,pc)

	; convert non-available SUB:Q to ADD:Q :

	add.b		#-20,@r4
	sub.b		#20,@r4
	add.w		#30,@(1000,r13)
	sub.w		#-30,@(1000,r13)
	add.l		#-40,@(1000,r13)
	sub.l		#40,@(1000,r13)

	; cannot convert second because 128 cannot be added via ADD:Q:

	sub.b		#127,@r4
	sub.b		#128,@r4

	; things detected while working with disassembly:

	;  allow leading @ on destination address of branches

	bsr		@*+7
	bhi		@*+5

	;  immediate is allowed as destination for cmp(s):

	cmp.l		r5,#$123456
	cmps.w		r6,#$12.b

	; register aliases

reg_r0	equ		r0
reg_r1	equ		r1
reg_r2	equ		r2
reg_r3	equ		r3
reg_r4	equ		r4
reg_r5	equ		r5
reg_r6	equ		r6
reg_r7	equ		r7
reg_r8	equ		r8
reg_r9	equ		r9
reg_r10	equ		r10
reg_r11	equ		r11
reg_r12	equ		r12
reg_r13	equ		r13
reg_r14	equ		r14
reg_r15	equ		r15
reg_sp	reg		sp

reg_cr0		equ	cr0
reg_cr1		equ	cr1
reg_cr2		equ	cr2
reg_cr3		equ	cr3
reg_cr4		equ	cr4
reg_cr5		equ	cr5
reg_cr6		equ	cr6
reg_cr7		equ	cr7
reg_cr8		equ	cr8
reg_cr9		equ	cr9
reg_cr10	equ	cr10
reg_cr11	equ	cr11
reg_cr12	equ	cr12
reg_cr13	equ	cr13
reg_cr14	equ	cr14
reg_cr15	equ	cr15

reg_pr0		equ	pr0
reg_pr1		equ	pr1
reg_pr2		equ	pr2
reg_pr3		equ	pr3
reg_pr4		equ	pr4
reg_pr5		equ	pr5
reg_pr6		equ	pr6
reg_pr7		equ	pr7
reg_pr8		equ	pr8
reg_pr9		equ	pr9
reg_pr10	equ	pr10
reg_pr11	equ	pr11
reg_pr12	equ	pr12
reg_pr13	equ	pr13
reg_pr14	equ	pr14
reg_pr15	equ	pr15

		mov.l	@($234567,r4),r0
		mov.l	@($234567,reg_r4),reg_r0
		mov.l	@r4+,r13
		mov.l	@reg_r4+,reg_r13
		mov.l	@-r4,r13
		mov.l	@-reg_r4,reg_r13
		mov.l	@sp+,r13
		mov.l	@reg_sp+,reg_r13
		mov.l	@-sp,r13
		mov.l	@-reg_sp,reg_r13
		add:g.w	cr4,@($23F5:16,pr7)
		add:g.w	reg_cr4,@($23F5:16,reg_pr7)
