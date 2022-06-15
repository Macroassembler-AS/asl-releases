	cpu	mn1610alt
	page	0

	nop				; = MV R0,R0

	irp	instr,l,st
	instr	r2,0
	instr	r2,20
	instr	r2,10+10
	instr	r2,(ic)
	instr	r2,10(ic)
	instr	r2,10 (ic)
	instr	r2,5 + 5 (ic)
	instr	r2,5+5(ic)
	instr	r2,-20(ic)
	instr	r2,-20(ic)
	instr	r2,-10-10(ic)
	instr	r2,(20)
	instr	r2,(10+10)
	instr	r2,((ic))
	instr	r2,(10(ic))
	instr	r2,(10 (ic))
	instr	r2,(5 + 5 (ic))
	instr	r2,(5+5(ic))
	instr	r2,(-20 (ic))
	instr	r2,(-20(ic))
	instr	r2,(-10-10(ic))
	instr	r2,(x0)
	instr	r2,10 (x0)
	instr	r2,5 + 5 (x0)
	instr	r2,10(x0)
	instr	r2,5+5(x0)
	instr	r2,(x1)
	instr	r2,10 (x1)
	instr	r2,5 + 5 (x1)
	instr	r2,10(x1)
	instr	r2,5+5(x1)
	instr	r2,(10)(x0)
	instr	r2,(5 + 5)(x0)
	instr	r2,(10) (x0)
	instr	r2,(10)(x1)
	instr	r2,(5 + 5)(x1)
	instr	r2,(10) (x1)
	endm

	irp	instr,b,bal,ims,dms
	instr	0
	instr	20
	instr	10+10
	instr	(ic)
	instr	10(ic)
	instr	+10(ic)
	instr	5 + 5 (ic)
	instr	5+5(ic)
	instr	-20(ic)
	instr	-20 (ic)
	instr	-10-10(ic)
	instr	(20)
	instr	(10+10)
	instr	((ic))
	instr	(10(ic))
	instr	(10 (ic))
	instr	(5 + 5 (ic))
	instr	(5+5(ic))
	instr	(-20(ic))
	instr	(-20 (ic))
	instr	(-10-10(ic))
	instr	(x0)
	instr	10(x0)
	instr	5 + 5 (x0)
	instr	10 (x0)
	instr	5+5(x0)
	instr	(x1)
	instr	10(x1)
	instr	5 + 5 (x1)
	instr	10 (x1)
	instr	5+5(x1)
	instr	(10)(x0)
	instr	(5 + 5)(x0)
	instr	(10) (x0)
	instr	(10)(x1)
	instr	(5 + 5)(x1)
	instr	(10) (x1)
	endm

	irp	instr,a,s,c,cb,mv,mvb,bswp,dswp,lad,and,or,eor
	irp	Rd,r0,r1,r2,r3,x0,r4,x1,sp,str
	irp	Rs,r0,r1,r2,r3,x0,r4,x1,sp,str
	instr	Rd,Rs
	irp	Skip,,skp,m,pz,z,nz,mz,p,ez,enz,oz,onz,lmz,lp,lpz,lm
	instr	Rd,Rs,Skip
	endm
	endm
	endm
	endm

	irp	instr,sl,sr
	instr	r4
	instr	r4,
	instr	r4,re
	instr	r4,se
	instr	r4,ce
	instr	r4,,
	instr	r4,re,
	instr	r4,se,
	instr	r4,ce,
	instr	r4,,z
	instr	r4,re,z
	instr	r4,se,z
	instr	r4,ce,z
	endm

	irp	instr,sbit,rbit,tbit,ai,si
	instr	r2,10
	instr	r3,15,nz
	endm

	lpsw	2

	h

	push	r0
	push	r1
	push	r2
	push	r3
	push	x0
	push	r4
	push	x1
	push	sp
	push	str

	pop	r0
	pop	r1
	pop	r2
	pop	r3
	pop	x0
	pop	r4
	pop	x1
	pop	sp
	pop	str

	ret

	rd	r0,X'12'
	rd	r1,X'23'
	rd	r2,X'34'
	rd	r3,X'45'
	rd	x0,X'45'
	rd	r4,X'56'
	rd	x1,X'56'
	rd	sp,X'67'
	rd	str,X'78'

	wt	r0,X'12'
	wt	r1,X'23'
	wt	r2,X'34'
	wt	r3,X'45'
	wt	x0,X'45'
	wt	r4,X'56'
	wt	x1,X'56'
	wt	sp,X'67'
	wt	str,X'78'

	mvi	r0,X'12'
	mvi	r1,X'23'
	mvi	r2,X'34'
	mvi	r3,X'45'
	mvi	x0,X'45'
	mvi	r4,X'56'
	mvi	x1,X'56'
	mvi	sp,X'67'
	mvi	str,X'78'

	; some addressing combinations that shall *NOT* work:

	expect	1330		; distance too big (for PC-relative addressing)
	l	r4,X'100'
	endexpect
	expect	1320
	l	r4,(X'100')
	endexpect

	expect	1320		; displacement under/overflow
	l	r4,130(ic)
	endexpect
	expect	1315
	l	r4,-129(ic)
	endexpect
	expect	1320
	l	r4,(130(ic))
	endexpect
	expect	1315
	l	r4,(-129(ic))
	endexpect

	expect	1320
	l	r4,X'100'(x0)
	endexpect
	expect	1320
	l	r4,X'100'(x1)
	endexpect
	expect	1320
	l	r4,(X'100')(x0)
	endexpect
	expect	1320
	l	r4,(X'100')(x1)
	endexpect

	expect	1350
	l	r4,((X'80')(x0))
	endexpect
	expect	1350
	l	r4,(X'80'(x0))
	endexpect
	expect	1350
	l	r4,((x'40')(ic))
	endexpect
	expect	1350
	l	r4,(x'40')(ic)
	endexpect

	packing	on

	dc	0
	expect	1320
	dc	65535
	endexpect
	expect	1315
	dc	-32768
	endexpect
	dc	'A'
	dc	'AB'
	dc	'ABC'	; treated like "ABC" due to length > 16 bit
	dc	"A"
	dc	"AB"
	dc	"ABC"
	dc	"ABCD"
	dc	"ABCDE"
	dc	"ABCDEF"

	packing	off

	dc	0
	dc	65535
	dc	-32768
	dc	'A'
	dc	'AB'
	dc	'ABC'	; treated like "ABC" due to length > 16 bit
	dc	"A"
	dc	"AB"
	dc	"ABC"
	dc	"ABCD"
	dc	"ABCDE"
	dc	"ABCDEF"

