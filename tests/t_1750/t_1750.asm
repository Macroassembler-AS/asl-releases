	cpu	1750
	page	0

	aisp	r3,2
	aim	r1,100
	ar	r0,r1
	a	r0,10
	a	r0,20,r1
	andr	r1,r2
	and	r1,30
	and	r1,40,r2
	andm	r2,200
	abs	r2,r3
	ab	b12,10
	andb	r13,20
	abx	r12,r1
	andx	b13,r2
	bez	$+1
	bnz	$+2
	bgt	$+3
	ble	$+4
	bge	$+5
	blt	$+6
	br	$+7
	bex	7
	bpt
	bif	9
	cisp	r4,3
	cim	r3,300
	cr	r3,r4
	c	r2,50
	c	r2,60,r3
	cisn	r5,4
	cb	b14,30
	cbl	r2,50
	cbl	r2,60,r3
	cbx	r14,r3
	disp	r6,5
	dim	r4,400
	disn	r7,6
	dvim	r5,500
	dlr	r4,r5
	dl	r3,70
	dl	r3,80,r4
	dst	r4,90
	dst	r4,100,r5
	dsll	r7,5
	dsrl	r8,6
	dsra	r9,7
	dslc	r10,8
	dslr	r5,r6
	dsar	r6,r7
	dscr	r7,r8
	decm	1,1000
	decm	2,2000,r4
	dar	r8,r9
	da	r5,110
	da	r5,120,r6
	dsr	r9,r10
	ds	r6,130
	ds	r6,140,r7
	dmr	r10,r11
	dm	r7,150
	dm	r7,160,r8
	ddr	r11,r12
	dd	r8,170
	dd	r8,180,r9
	dcr	r12,r13
	dc	r9,190
	dc	r9,200,r10
	dlb	r15,40
	dstb	b12,50
	dneg	r13,r14
	dabs	r14,r15
	dr	r15,r0
	d	r10,210
	d	r10,220,r11
	dvr	r0,r1
	dv	r11,230
	dv	r11,240,r12
	dli	r12,250
	dli	r12,260,r13
	dsti	r13,270
	dsti	r13,280,r14
	db	b14,60
        dbx	b15,r4
	dlbx	r12,r5
	dstx	b13,r6
	dle	r9,100
	dle	r9,100,r10
	dste	r10,200
	dste	r10,200,r11
	efl	r14,290
	efl	r14,300,r15
	efst	r15,310
	efst	r15,320,r1	; r0 not allowed as index
	efcr	r1,r2
	efc	r0,330
	efc	r0,340,r1
	efar	r2,r3
	efa	r1,350
	efa	r1,360,r2
	efsr	r3,r4
	efs	r2,370
	efs	r2,380,r3
	efmr	r4,r5
	efm	r3,390
	efm	r3,400,r4
	efdr	r5,r6
	efd	r4,410
	efd	r4,420,r5
	eflt	r6,r7
	efix	r7,r8
	far	r8,r9
	fa	r5,430
	fa	r5,440,r6
	fsr	r9,r10
	fs	r6,450
	fs	r6,460,r7
	fmr	r10,r11
	fm	r7,470
	fm	r7,480,r8
	fdr	r11,r12
	fd	r8,490
	fd	r8,500,r9
	fcr	r12,r13
	fc	r9,510
	fc	r9,520,r10
	fabs	r13,r14
	fix	r14,r15
	flt	r15,r0
	fneg	r0,r1
	fab	r15,70
	fabx    r14,r7
	fsb	b12,80
	fsbx	b15,r8
	fmb	r13,90
	fmbx	r12,r9
	fdb	b14,100
	fdbx	b13,r10
	fcb	r15,110
	fcbx	r14,r11
	incm	1,3000
	incm	2,4000,r4
	jc	lt,1234h
	jc	eq,1234h,r4
	j	$-1
	jez	$-2
	jle	$-3
	jgt	$-4
	jnz	$-5
	jge	$-6
	jlt	$-7
	jci	le,1234h
	jci	gt,1234h,r5
	js	r10,530
	js	r10,540,r11
	lisp	r8,7
	lim	r11,550
	lim	r11,560,r12
	lr	r1,r2
	l	r12,570
	l	r12,580,r13
	lisn	r9,8
	lb	b12,120
	lbx	b15,r12
	lsti	1000
	lsti	1000,r5
	lst	2000
	lst	2000,r6
	li	r13,590
	li	r13,600,r14
	lm	6,23h
	lm	6,23h,r7
	lub	r13,590
	lub	r13,600,r14
	llb	r14,610
	llb	r14,620,r14
	lubi	r15,630
	lubi	r15,640,r1	; r0 not allowed as index
	llbi	r0,650
	llbi	r0,660,r1
	le	r13,300
	le	r13,300,r14
	misp	r10,9
	msim	r6,600
	msr	r2,r3
	ms	r1,670
	ms	r1,680,r2
	misn	r11,10
	mim	r7,700
	mr	r3,r4
	m	r2,690
	m	r2,700,r3
	mov	r4,r5
        mb	r13,130
	mbx	r12,r13
	neg	r5,r6
	nop
	nim	r8,800
	nr	r6,r7
	n	r3,710
	n	r3,720,r4
	orim	r9,900
	orr	r7,r8
	or	r4,730
	or	r4,740,r5
	orb	b14,140
	orbx	b13,r14
	pshm	r8,r9
	popm	r9,r10
	rbr	4,r12
	rvbr	r10,r11
	rb	7,34h
	rb	7,34h,r8
	rbi	8,45h
	rbi	8,45h,r9
	st	r5,750
	st	r5,760,r6
	stc	9,56h
	stc	9,56h,r10
	sisp	r12,11
	sim	r10,1000
	sr	r11,r12
	s	r6,770
	s	r6,780,r7
	sll	r7,5
	srl	r8,6
	sra	r9,7
	slc	r10,8
	slr	r12,r13
	sar	r13,r14
	scr	r14,r15
	sjs	r7,790
	sjs	r7,800,r8
	stb	r15,150
	sbr	5,r13
	sb	10,67h
	sb	10,67h,r11
	svbr	r15,r0
	soj	r8,810
	soj	r8,820,r9
	sbb	b12,160
	stbx	b13,r14
	sbbx	r14,r15
	sbi	11,78h
	sbi	11,78h,r12
	stz	5000
	stz	5000,r6
	stci	12,89h
	stci	12,89h,r13
	sti	r9,830
	sti	r9,840,r10
	sfbs	r0,r1
	srm	r10,850
	srm	r10,860,r11
	stm	13,9ah
	stm	13,9ah,r14
	stub	r11,870
	stub	r11,880,r12
	stlb	r12,890
	stlb	r12,900,r13
	subi	r13,910
	subi	r13,920,r14
	slbi	r14,930
	slbi	r14,940,r15
	ste	r14,500
	ste	r14,500,r15
	tbr	9,r4
	tb	14,0abh
	tb	14,0abh,r15
	tbi	15,0bch
	tbi	15,0bch,r1
	tsb	1,0cdh
	tsb	1,0cdh,r2
	tvbr	r1,r2
	urs	r5
	uar	r2,r3
	ua	r15,950
	ua	r15,960,r1	; r0 not alloed as index
	usr	r3,r4
	us	r0,970
	us	r0,980,r1
	ucim	r11,1100
	ucr	r4,r5
	uc	r1,990
	uc	r1,1000,r2
	vio	r2,1010
	vio	r2,1020,r3
	xorr	r5,r6
	xorm	r12,1200
	xor	r3,1030
	xor	r3,1030,r4
	xwr	r6,r7
	xbr	r7
	xio	r4,wopr
	xio	r5,tbh,r2
	xio	r6,200
	xio	r7,300,r3

; These are some examples for the floating point format used by MIL STD 1750.
; Since conversion from the host's FP format to this format is always dependent
; on rounding issues, I leave out the majority of them.  You may remove the
; comments to see how precisely conversion works on your system...

testall	equ	0

	if	testall
	float	1.7014118346046921e+38	; 0.999999880791 * 2^127 = 7fff ff 7f
	float	8.5070591730234615e+37	; 0.5 * 2^127            = 4000 00 7f
	endif
	float	10.0			; 0.625 * 2^4            = 5000 00 04
	float	1.0			; 0.5 * 2^1              = 4000 00 01
	float	0.5			; 0.5 * 2^0              = 4000 00 00
	float	0.25			; 0.5 * 2^-1             = 4000 00 ff
	if	testall
	float	1.46936793853e-39	; 0.5 * 2^-128           = 4000 00 80
	endif
	float	0.0			; 0.0 * 2^0		 = 0000 00 00
	float	-1.0			; -1.0 * 2^0             = 8000 00 00
	if	testall
	float	-1.4693682888524e-39	; -0.5000001 * 2^-128    = bfff ff 80
	float	-12.000001		; -0.7500001 * 2^4       = 9fff ff 04
	endif

	expect	1320
	float	1.0e50			; too large
	endexpect

; not sure whether 1750 supports denormalized numbers. For the
; moment, I assume yes.  If no, all these should result in 0:

	if	testall
	float	7.3468396926393e-40	; 2^-2  * 2^-128 (denorm) = 2000 00 80
	float	4.59177480789957e-41	; 2^-6  * 2^-128 (denorm) = 0200 00 80
	float	2.86985925493723e-42	; 2^-10 * 2^-128 (denorm) = 0020 00 80
	float	1.79366203433577e-43	; 2^-14 * 2^-128 (denorm) = 0002 00 80
	float	1.12103877145986e-44	; 2^-18 * 2^-128 (denorm) = 0000 20 80
	float	7.00649232162409e-46	; 2^-22 * 2^-128 (denorm) = 0000 02 80
	float	3.50324616081205e-46    ; 2^-23 * 2^-128 (denorm) = 0000 01 80
	float	1.75162308040603e-46	; 2^-24 * 2^-128 (uflo)   = 0000 00 00
	endif

	if		testall
	extended	8.5070591730234615e+37	; 0.5 * 2^127        ; = 400000 7f 0000
	endif
	extended	0.5			; 0.5 * 2^0          = 400000 00 0000
	extended	0.25			; 0.5 * 2^-1         = 400000 ff 0000
	if		testall
	extended	1.46936793853e-39	; 0.5 * 2^-128       ; = 400000 80 0000
	extended	-1.70141183460469e+38	; -1.0 * 2^127       ; = 800000 7f 0000
	endif
	extended	-1.0			; -1.0 * 2^0         = 800000 00 0000
	extended	-0.5			; -1.0 * 2^-1        = 800000 ff 0000
	if		testall
	extended	-2.93873587705571e-39	; -1.0 * 2^-128      ; = 800000 80 0000
	endif
	extended	0.0			; 0.0 * 2^0          = 000000 00 0000
	extended	-0.375			; -0.75 * 2^-1       : a00000 ff 0000

; due to more mantissa bits, we can go a bit further with denormalized numbers:

	if		testall
	extended	7.3468396926393e-40	; 2^-2 * 2^-128 (denorm) = 2000 00 80 0000
	extended	4.59177480789957e-41	; 2^-6 * 2^-128 (denorm) = 0200 00 80 0000
	extended	2.86985925493723e-42	; 2^-10 * 2^-128 (denorm)= 0020 00 80 0000
	extended	1.79366203433577e-43	; 2^-14 * 2^-128 (denorm)= 0002 00 80 0000
	extended	1.12103877145986e-44	; 2^-18 * 2^-128 (denorm)= 0000 20 80 0000
	extended	7.00649232162409e-46	; 2^-22 * 2^-128 (denorm)= 0000 02 80 0000
	extended	4.37905770101506e-47    ; 2^-26 * 2^-128 (denorm)= 0000 00 80 2000
	extended	2.73691106313441e-48	; 2^-30 * 2^-128 (denorm)= 0000 00 80 0200
	extended	1.71056941445901e-49	; 2^-34 * 2^-128 (denorm)= 0000 00 80 0020
	extended	1.06910588403688e-50	; 2^-38 * 2^-128 (denorm)= 0000 00 80 0002
	extended	5.34552942018440e-51	; 2^-39 * 2^-128 (denorm)= 0000 00 80 0001
	extended	2.6727647100922e-51	; 2^-40 * 2^-128 (uflo)  = 0000 00 00 0000
	endif
