	cpu	mn1613alt
	page	0

	irp	instr,ld,std

	; X'00000'...X'0ffff' addressable
	assume	csbr:0,ssbr:0,tsr0:0,tsr1:0

	instr	r1,X'1000'
	instr	r1,X'ffff'
	expect	110
	instr	r1,X'10000'
	endexpect
	instr	r1,csbr,X'1000'
	instr	r1,csbr,X'ffff'
	expect	110
	instr	r1,csbr,X'10000'
	endexpect
	instr	r1,ssbr,X'1000'
	instr	r1,ssbr,X'ffff'
	expect	110
	instr	r1,ssbr,X'10000'
	endexpect
	instr	r1,tsr0,X'1000'
	instr	r1,tsr0,X'ffff'
	expect	110
	instr	r1,tsr0,X'10000'
	endexpect
	instr	r1,tsr1,X'1000'
	instr	r1,tsr1,X'ffff'
	expect	110
	instr	r1,tsr1,X'10000'
	endexpect

	; X'04000...X'13fff addressable
	assume	csbr:1
	expect	110
	instr	r1,X'1000'
	endexpect
	instr	r1,X'4000'
	instr	r1,X'ffff'
	instr	r1,X'10000'
	instr	r1,X'13fff'
	expect	110
	instr	r1,X'14000'
	endexpect

	; X'3c000...X'3ffff and X'00000...X'0bfff addressable
	assume	csbr:15
	instr	r1,X'0000'
	instr	r1,X'1000'
	instr	r1,X'bfff'
	expect	110
	instr	r1,X'c000'
	endexpect
	expect	110
	instr	r1,X'3bfff'
	endexpect
	instr	r1,X'3c000'
	endm

	irp	instr,lr,str
	instr	r2, ( r3 )
	instr	r2,ssbr,-(r4)
	instr	r2,tsr0,(r1)+
	endm

        irp	instr,mvwr,mvbr,bswr,dswr,awr,swr,cwr,cbr,ladr,andr,orr,eorr
	instr	r0,(r2)
	instr	r0,(r3),lm
	expect	1445
	instr	r1,(r2)
	endexpect
	expect	1350
	instr	r0,(r3)+
	endexpect
	endm

	irp	instr,mvwi,awi,swi,cwi,cbi,ladi,andi,ori,eori,ladi
	instr	r3,X'5678'
	instr	x1,X'8765',pz
	endm

	irp	instr,ad,sd
	instr	dr0,(r2)
	expect	1445
	instr	dr1,(r2)
	endexpect
	instr	dr0,(r2),c
	expect	1360
	instr	dr0,(r2),xyz
	endexpect
	instr	dr0,(r2),pz
	expect	1360
	instr	dr0,(r2),c,xyz
	endexpect
	instr	dr0,(r2),c,pz
	expect	1445
	instr	dr0,(r2),cc,pz
	endexpect
	endm

	irp	instr,m,d,fa,fs,fm,fd
	instr	dr0,(r2)
	expect	1445
	instr	dr1,(r2)
	endexpect
	expect	1360
	instr	dr0,(r2),xyz
	endexpect
	instr	dr0,(r2),pz
	endm

	irp	instr,daa,das
	instr	r0,(r2)
	expect	1445
	instr	r1,(r2)
	endexpect
	instr	r0,(r2),c
	expect	1360
	instr	r0,(r2),xyz
	endexpect
	instr	r0,(r2),pz
	expect	1360
	instr	r0,(r2),c,xyz
	endexpect
	instr	r0,(r2),c,pz
	expect	1445
	instr	r0,(r2),cc,pz
	endexpect
	endm

	irp	instr,rdr,wtr
	instr	r4,(r3)
	instr	x1,(x0)
	endm

	assume	csbr:0

	irp	instr,bd,bald
	instr	X'3456'
	expect	110
	instr	X'12345'
	endexpect
	endm

	irp	instr,bl,ball
	instr	(X'3456')
	expect	1350
	instr	X'3456'
	endexpect
	expect	110
	instr	(X'12345')
	endexpect
	endm

	irp	instr,br,balr
	instr	(r2)
	expect	1350
	instr	(r0)
	endexpect
	endm

	irp	instr,tset,trst
	instr	r4,X'2345'
	instr	r4,X'2345',nz
	expect	110
	instr	r4,X'12345',nz
	endexpect
	endm

	neg	r1
	neg	r1,c
	expect	1360
	neg	r1,xyz
	endexpect
	neg	r1,c,pz
	expect	1445
	neg	r1,x,pz
	endexpect
	expect	1360
	neg	r1,c,xyz
	endexpect

	fix	r0,dr0
	fix	r0,dr0,z
	flt	dr0,r0
	flt	dr0,r0,z

	srbt	r0,r2
	debp	r2,r0
	blk	(r2),(r1),r0
	expect	1350
	blk	(r3),(r1),r0
	endexpect
	expect	1350
	blk	(r2),(r3),r0
	endexpect
	expect	1445
	blk	(r2),(r1),r4
	endexpect

	irp	reg,ssbr,tsr0,tsr1,osr0,osr1,osr2,osr3
	lb	reg,X'2345'
	expect	110
	lb	reg,X'12345'
	endexpect
	endm

	irp	reg,csbr,ssbr,tsr0,tsr1,osr0,osr1,osr2,osr3
	stb	reg,X'2345'
	expect	110
	stb	reg,X'12345'
	endexpect
	endm

	irp	reg,sbrb,icb,npp
	irp	instr,ls,sts
	instr	reg,X'2345'
	expect	110
	instr	reg,X'12345'
	endexpect
	endm
	endm

	irp	reg,csbr,ssbr,tsr0,tsr1,osr0,osr1,osr2,osr3
	cpyb	r4,reg
	endm

	irp	reg,ssbr,tsr0,tsr1,osr0,osr1,osr2,osr3
	setb	r4,reg
	endm

	irp	reg,sbrb,icb,npp
	irp	instr,cpys,sets
	instr	r4,reg
	endm
	endm

	irp	reg,tcr,tir,tsr,scr,ssr,sir,iisr
	cpyh	r4,reg
	endm

	irp	reg,tcr,tir,tsr,scr,ssr,sor,iisr
	seth	r4,reg
	endm

	pshm
	popm

	retl

	; floating point support is experimental:

	dc	0.0		; 0000 0000 = 0.0000 * 16^(-64)
	dc	1.0		; 4110 0000 = 1/16 * 16^1
	dc	2.0		; 4120 0000 = 1/8 * 16^1
	dc	4.0		; 4140 0000 = 1/4 * 16^1
	dc	8.0		; 4180 0000 = 1/2 * 16^1
	dc	16.0		; 4210 0000 = 1/16 * 16^2
	dc	24.0		; 4218 0000 = 3/16 * 16^2
	dc	42.0		; 422A 0000 = 21/8 * 16^2
	dc	0.125		; 4020 0000 = 1/8 * 16^0
	dc	0.0078125	; 3F20 0000 = 1/8 * 16^-1
	dc	1000.0		; 433E 8000 = 0.24414062 * 16^3
	dc	-1000.0		; C33E 8000 = -0.24414062 * 16^3
;	dc	0.1		; 4019 999A = 0.1 * 16^0
;	dc	7.237005E75	; 7FFF FFFf (almost maximum)
;	dc	5.39761E-79	; 0010 0000 or 000F FFFF (smallest non-denormal number)
