	cpu	ibm5100
	page	0

dest	reg	r5
mask	equ	r12
src	reg	r9
ptr	equ	r13

dev1	port	x'a'
dev2	port	b'1110'

var1	equ	x'aa'
var2	equ	x'ac'

	; basic jump instructions

	jle	dest,mask
	jlo	dest,mask
	jeq	dest,mask
	jno	r1
	jall	dest,mask
	jallm	dest,mask
	jnom	dest,mask
	jham	dest,mask
	jhi	dest,mask
	jhe	dest,mask
	jhl	dest,mask
	jsb	r1
	jsn	r5,r12
	jsnm	dest,mask
	jsm	dest,mask
	jhsnm	dest,mask

	; basic arithmetic

	mvm2	dest, src
	mvm1	dest, src
	mvp1	dest, src
	mvp2	dest, src
	move	dest, src
	and	dest, src
	or	dest, src
	orb	dest, src
	xor	dest, src
	add	dest, src
	sub	dest, src
	adds1	dest, src
	adds2	dest, src
	htl	dest, src
	lth	dest, src
	getr	dev1, dest
	geta	dev2, dest

	; basic shift & rotate

	shftr	dest
	rotr	dest
	srr3	dest
	srr4	dest

	; direct load/store

	ldhd	dest,var1
	expect	1351
	ldhd	dest,var1+1
	endexpect
	sthd	src,var2
	expect	1351
	sthd	src,var2-1
	endexpect

	; indirect load/store

	ldhi	dest,ptr
	ldhi	dest,ptr,0
	ldhi	dest,ptr,1
	ldhi	dest,ptr,2
	ldhi	dest,ptr,3
	ldhi	dest,ptr,4
	expect	1320
	ldhi	dest,ptr,5
	endexpect
	ldhi	dest,ptr,-1
	ldhi	dest,ptr,-2
	ldhi	dest,ptr,-3
	ldhi	dest,ptr,-4
	expect	1315
	ldhi	dest,ptr,-5
	endexpect

	sthi	dest,ptr
	sthi	dest,ptr,0
	sthi	dest,ptr,1
	sthi	dest,ptr,2
	sthi	dest,ptr,3
	sthi	dest,ptr,4
	expect	1320
	sthi	dest,ptr,5
	endexpect
	sthi	dest,ptr,-1
	sthi	dest,ptr,-2
	sthi	dest,ptr,-3
	sthi	dest,ptr,-4
	expect	1315
	sthi	dest,ptr,-5
	endexpect

	ldbi	dest,ptr
	ldbi	dest,ptr,0
	ldbi	dest,ptr,1
	ldbi	dest,ptr,2
	ldbi	dest,ptr,3
	ldbi	dest,ptr,4
	expect	1320
	ldbi	dest,ptr,5
	endexpect
	ldbi	dest,ptr,-1
	ldbi	dest,ptr,-2
	ldbi	dest,ptr,-3
	ldbi	dest,ptr,-4
	expect	1315
	ldbi	dest,ptr,-5
	endexpect

	stbi	dest,ptr
	stbi	dest,ptr,0
	stbi	dest,ptr,1
	stbi	dest,ptr,2
	stbi	dest,ptr,3
	stbi	dest,ptr,4
	expect	1320
	stbi	dest,ptr,5
	endexpect
	stbi	dest,ptr,-1
	stbi	dest,ptr,-2
	stbi	dest,ptr,-3
	stbi	dest,ptr,-4
	expect	1315
	stbi	dest,ptr,-5
	endexpect

	; immediate argument

	emit	dest,#x'aa'
	emit	dest,x'aa'
	clri	dest,#x'aa'
	clri	dest,x'aa'
	seti	dest,#x'aa'
	seti	dest,x'aa'

	addi	dest,#x'aa'
	addi	dest,x'aa'
	addi	dest,#1
	addi	dest,1
	addi	dest,#256
	addi	dest,256
	expect	1315
	addi	dest,#0
	endexpect
	expect	1320
	addi	dest,#257
	endexpect

	subi	dest,#x'aa'
	subi	dest,x'aa'
	subi	dest,#1
	subi	dest,1
	subi	dest,#256
	subi	dest,256
	expect	1315
	subi	dest,#0
	endexpect
	expect	1320
	subi	dest,#257
	endexpect

	; I/O

	ctl	dev2,#x'cf'
	ctl	dev2,b'11001111'

	putb	dev2,ptr
	putb	dev2,ptr,0
	putb	dev2,ptr,1
	putb	dev2,ptr,2
	putb	dev2,ptr,3
	putb	dev2,ptr,4
	expect	1320
	putb	dev2,ptr,5
	endexpect
	putb	dev2,ptr,-1
	putb	dev2,ptr,-2
	putb	dev2,ptr,-3
	putb	dev2,ptr,-4
	expect	1315
	putb	dev2,ptr,-5
	endexpect

	getb	dev1,ptr
	getb	dev1,ptr,0
	getb	dev1,ptr,1
	getb	dev1,ptr,2
	getb	dev1,ptr,3
	getb	dev1,ptr,4
	expect	1320
	getb	dev1,ptr,5
	endexpect
	getb	dev1,ptr,-1
	getb	dev1,ptr,-2
	getb	dev1,ptr,-3
	getb	dev1,ptr,-4
	expect	1315
	getb	dev1,ptr,-5
	endexpect

	; the same instructions in alphabetical order

	add	r13,r14		; 0DE8
	addi	r13,x'55'	; AD54 (!)
	adds1	r13,r14		; 0DEA
	adds2	r13,r14		; 0DEB
	and	r13,r14		; 0DE5
	clri	r13,x'55'	; 9D55
	ctl	13,#x'55'	; 1D55
	emit	r13,#x'55'	; 8D55
	geta	13,r14		; 0DEF
	getb	13,r14		; EDE8
	getr	13,r14		; 0DEE
	getrb	13,r14		; EDEF (not listed in IBM document)
	htl	r13,r14		; 0DEC
	jall	r13,r14		; CDE4
	jallm	r13,r14		; CDE5
	jeq	r13,r14		; CDE2
	jham	r13,r14		; CDE7
	jhe	r13,r14		; CDE9
	jhi	r13,r14		; CDE8
	jhl	r13,r14		; CDEA
	jhsnm	r13,r14		; CDEF
	jle	r13,r14		; CDE0
	jlo	r13,r14		; CDE1
	jno	r13		; CD03
	jnom	r13,r14		; CDE6
	jsb	r13		; CD0B
	jsm	r13,r14		; CDEE
	jsn	r13,r14		; CDEC
	jsnm	r13,r14		; CDED
	ldbi	r13,r14		; 6DE8
	ldhd    r13,x'aa'	; 2D55
	ldhi	r13,r14		; DDE8
	lth	r13,r14		; 0DED
	move	r13,r14		; 0DE4
	mvm1	r13,r14		; 0DE1
	mvm2	r13,r14		; 0DE0
	mvp1	r13,r14		; 0DE2
	mvp2	r13,r14		; 0DE3
	or	r13,r14		; 0DE6
	orb	r13,r14		; 0DE6
	putb    13,r14		; 4DE8
	rotr	r13		; E0DD
	seti	r13,x'55'	; BD55
	shftr	r13		; E0DC
	stbi	r13,r14		; 7DE8
	sthd	r13,x'aa'	; 3D55
	sthi    r13,r14		; 5DE8
	sub	r13,r14		; 0DE9
	subi	r13,x'55'	; FD54 (!)
	xor	r13,r14		; 0DE7

	db	1,2,3,4
	dw	1,2,3,4
