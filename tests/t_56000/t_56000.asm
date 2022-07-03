		page	0

		cpu	56000

                include stddef56k.inc

		segment	code
		org	$300

		nop
		enddo
		illegal
		reset
		rti
		rts
		stop
		swi
		wait

		jmp	$214
		jsr	$889
		jmp	$3567
		jsr	$2340
		jmp	(r3)
		jsr	(r5)
		jmp	(r6)+
		jsr	(r1)+
		jmp	(r4)-
		jsr	(r2)-
		jmp	-(r5)
		jsr	-(r0)
		jmp	(r3+n3)
		jsr	(r7+n7)
		jmp	(r2)+n2
		jsr	(r6)+n6
		jmp	(r5)-n5
		jsr	(r3)-n3

		jcc	$123
		jge	$2345
		jne	(r7)
		jpl	(r6)-
		jnn	(r5)+
		jec	-(r4)
		jlc	(r3+n3)
		jgt	(r2)+n2
		jcs	(r1)-n1
		jlt	$123
		jeq	$2345
		jmi	(r0)
		jnr	(r1)-
		jes	(r2)+
		jls	-(r3)
		jle	(r4+n4)
		jhs	(r5)+n5
		jlo	(r6)-n6

		jsnn	$768
		jsle	(r0)


		move	(r3)+n3
		move	(r4)-n4
		move	(r2)-
		move	(r6)+

		move	n5,a2
		move	r4,b1
		move	a0,b2
		move	a,r4

		move	#$30,n5
		move	#$ff,r3

		move	x:#$123456,a0
		move	y:#$234567,y1
		move	  #$345678,r5
		move	y:#$456789,n1

		move	x:$12,b2
		move	y:$34,n6
		move	x:$1234,a
		move	y:$2345,b2
		move	b2,x:$12
		move	n6,y:$34
		move	a,x:$1234
		move	b2,y:$2345

		move	x:(r3),a0
		move	y:(r4)+n4,x1
		move	a0,x:(r3)
		move	x1,y:(r4)+n4

		move	l:$12,ab
		move	$3456,y
		move	ab,l:$12
		move	y,$3456

		move	b,x:(r1)+ x0,b
		move	y0,b b,y:(r1)+

		move	x1,x:(r2)+ a,y:(r5)+n5
		move	x:(r2)+,x1 a,y:(r5)+n5
		move	x:(r2)+,x1 y:(r5)+n5,a

		move	x:(r5),x1    a,y1
		move	a,x:-(r1)    b,y0
		move	b,x:$1234    a,y0
		move	#$234567,x0  b,y1

		move	b,x1 y:(r6)-n6,b

		abs	a #$123456,x0 a,y0
		asl	a (r3)-
		asr	b x:-(r3),r3
		clr	a #$7f,n0
		lsl	b #$7f,r0
		lsr	a a1,n4
		neg	b x1,x:(r3)+ y:(r6)-,a
		not	a ab,l:(r2)+
		rnd	a #$123456,x1 b,y1
		rol	a #$314,n2
		ror	b #$1234,r2
		tst	a #$345678,b
		adc	y,b a10,l:$4
		sbc	y,b a10,l:$4
		add	x0,a a,x1 a,y:(r1)+
		cmp	y0,b x0,x:(r6)+n6 y1,y:(r0)-
		cmpm	x1,a ba,l:-(r4)
		sub	x1,a x:(r2)+n2,r0
		addl	a,b #$0,r0
		addr	b,a x0,x:(r1)+n1 y0,y:(r4)-
		subl	a,b y:(r5+n5),r7
		subr	b,a n5,y:-(r5)
		and	x0,a (r5)-n5
		eor	y1,b (r2)+
		or	y1,b ba,l:$1234
		mac	x0,x0,a x:(r2)+n2,y1
		macr	x0,y0,b y:(r4)+n4,y0
		mpy	-x1,y1,a #$543210,y0
		mpyr	-y0,y0,b (r3)-n3

		bchg	#$7,x:$ffe2
		bclr	#$e,x:$ffe4
		bset	#$0,x:$ffe5
		btst	#$1,x:$ffee
		bclr    #$4,y:$ffe0
		bclr	#$5,x:$0020
		bclr	#$6,y:$0012
		bclr	#$7,x:$1234
		bclr	#$8,y:(r3)+
		bclr	#$9,r5
		bclr    #$a,m6
		bclr	#$b,omr

		div	x1,b

		do	x:(r3),$1234
		do	y:(r5+n5),$2345
		do	x:$12,$3456
		do	y:$23,$4567
		do	#$123,$5678
		do	n7,$6789

		jclr	#$5,x:$fff1,$1234
		jsclr	#$1,y:$ffe3,$1357
		jset	#12,x:$fff2,$4321
		jsset	#$17,y:$3f,$100
		jclr	#21,x:(r5),$6789
		jclr	#22,ssh,$5678

		lua	(r0)+n0,r1

		movec	m0,m2
		movec	m4,r2
		movec	n5,ssl
		movec	#0,omr
		movec	#123456,ssh
		movec	x:$12,m2
		movec	m2,x:$12
		movec	y:$23,m2
		movec	m2,y:$23
		movec	x:(r4),m5
		movec	m5,y:(r4)
		movec	y:(r4),m5
		movec	m5,x:(r4)

		movem	m4,$12
		movem	$12,m4
		movem	$123,m4
		movem	m4,$123

		andi	#2,ccr
		ori	#5,omr

		norm	r5,a
		norm	r2,b

		rep	r4
		rep	#$987
		rep	x:$12
		rep	y:$23
		rep	x:(r3)
		rep	y:$12

		movep	x:(r3),x:$ffe0
		movep	y:(r3),x:$ffe1
		movep	#$123456,x:$ffe2
		movep	x:$ffe3,x:(r3)
		movep	x:$ffe4,y:(r3)
		movep	x:(r3),y:$ffe5
		movep	y:(r3),y:$ffe6
		movep	#$123456,y:$ffe7
		movep	y:$ffe8,x:(r3)
		movep	y:$ffe9,y:(r3)
		movep	p:(r3),x:$ffea
		movep	x:$ffeb,p:(r3)
		movep	p:(r3),y:$ffec
		movep	y:$ffed,p:(r3)
		movep	a1,x:$ffef
		movep	x:$fff0,r3
		movep	n5,y:$fff1
		movep	y:$fff2,m1

		tfr	a,b a,x1 y:(r4+n4),y0
		tfr	y0,a

		tgt	x0,a r0,r1
		tne	y1,a

		; packed storage (3 chars per word)

		dc	"Hallo"
		dc	'1'
		dc	'12'
		dc	'123'
		dc	"1"
		dc	"12"
		dc	"123"
		dc	$123456
		dc	16777215
		dc	-8388608
		dc	"Dies ist ein Test, Leute" 0

		; unpacked storage (one char per word)

		packing	off
		dc	"Hallo"
		dc	'1'
		dc	'12'
		dc	'123'
		dc	"1"
		dc	"12"
		dc	"123"
		dc	$123456
		dc	16777215
		dc	-8388608
		dc	"Dies ist ein Test, Leute" 0

		segment	xdata

		org	$123
var1:   	ds	1

		segment	ydata

		org	$234
var2:		ds	1
