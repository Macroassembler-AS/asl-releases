	cpu	s912zvh128f2clq
	page	0
	padding	off

bstruct	struct	dots
value	ds.b	1
bflg	defbit	value,5
bfld	defbitfield value,3:1
	ends
	org	$123
myb	bstruct

wstruct	struct	dots
value	ds.w	1
wflg	defbit	value,12
wfld	defbitfield value,5:7
	ends
	org	$240
myw	wstruct

lstruct	struct	dots
value	ds.l	1
lflg	defbit	value,25
lfld	defbitfield value,5:26
	ends
	org	$280
myl	lstruct

pstruct	struct	dots
value	ds.p	1
pfld	defbitfield value,5:18
	ends
	org	$380
myp	pstruct

	org	$1000

	nop
	bgnd
	clc
	cli
	clv
	rti
	rts
	sec
	sei
	sev
	stop
	swi
	sys
	wai

	irp	op,bcc,bcs,beq,bge,bgt,bhi,bhs,ble,blo,bls,blt,bmi,bne,bpl,bra,bsr,bvc,bvs
	op	*+10
	op	*+100
	op.s	*+10
	op.l	*+10
	op	*+100
	endm

	abs	d0
	abs.b	d0
	abs	d1
	abs.b	d1
	abs	d2
	abs.w	d2
	abs	d3
	abs.w	d3
	abs	d4
	abs.w	d4
	abs	d5
	abs.w	d5
	abs	d6
	abs.l	d6
	abs	d7
	abs.l	d7

	irp	instr,adc,add,and,bit,eor,or,sbc,sub,cmp
	instr	d0,d1
	instr	d0,#-1
	instr	d0,#1
	instr	d0,#2
	instr	d0,#3
	instr	d0,#4
	instr	d0,#5
	instr	d0,#6
	instr	d0,#7
	instr	d0,#8
	instr	d0,#9
	instr	d0,#10
	instr	d0,#11
	instr	d0,#12
	instr	d0,#13
	instr	d0,#14
	instr	d0,#15
	instr	d0,#$ff
	instr	d0,#$fe
	instr	d0,#$40
	instr	d3,d5
	instr	d3,#-1
	instr	d3,#1
	instr	d3,#2
	instr	d3,#3
	instr	d3,#4
	instr	d3,#5
	instr	d3,#6
	instr	d3,#7
	instr	d3,#8
	instr	d3,#9
	instr	d3,#10
	instr	d3,#11
	instr	d3,#12
	instr	d3,#13
	instr	d3,#14
	instr	d3,#15
	instr	d3,#$ffff
	instr	d3,#$ff
	instr	d3,#$fffe
	instr	d3,#$3040
	instr	d6,d7
	instr	d6,#-1
	instr	d6,#1
	instr	d6,#2
	instr	d6,#3
	instr	d6,#4
	instr	d6,#5
	instr	d6,#6
	instr	d6,#7
	instr	d6,#8
	instr	d6,#9
	instr	d6,#10
	instr	d6,#11
	instr	d6,#12
	instr	d6,#13
	instr	d6,#14
	instr	d6,#15
	instr	d6,#$ffffffff
	instr	d6,#$ffffff
	instr	d6,#$ffff
	instr	d6,#$ff
	instr	d6,#$fffffffe
	instr	d6,#$30405060
	instr	d6,( -x)
	instr	d6,(-y )
	instr	d6,(-s)
	instr	d6,(x-)
	instr	d6,(y-)
	instr	d6,(+x)
	instr	d6,(+y)
	instr	d6,(x+)
	instr	d6,(y+)
	instr	d6,(s+)
	instr	d6,(2 ,x)
	instr	d6,(y)
	instr	d6,(s,7)
	instr	d6,(20,x)
	instr	d6,(-100,y)
	instr	d6,(255,s)
	instr	d6,(*+2,pc)
	instr	d6,(10000,x)
	instr	d6,(-10000,y)
	instr	d6,(100000,s)
	instr	d6,(*+100000,pc)
	instr	d6,(d0 ,x)
	instr	d6,(d1, y)
	instr	d6,(s,d6)
	instr	d6,(d6)
	instr	d6,(100,d7)
	instr	d6,(100000,d0)
	instr	d6,(1000000,d6)
	instr	d6,(1000000,d7)
	instr	d6,(1000000,d0)
	instr	d6,[d0,x]
        instr	d6,[y,d1]
	instr	d6,[x]
	instr	d6,[y,2]
	instr	d6,[-25,s]
	instr	d6,[*-10,pc]
	instr	d6,[10000,x]
	instr	d6,[y,10000]
	instr	d6,[10000,s]
	instr	d6,[*+10000,pc]
	instr	d6,[0]
	instr	d6,[10000000]
	instr	d6,$123
	instr	d6,$1234
	instr	d6,$12345
	instr	d6,$22345
	instr	d6,$32345
	instr	d6,$123456
	endm

	; additional forms of SUB

	sub	d6,x,y
	sub	d6,y,x
	;sub	d5,y,x
	;sub	d6,y,y

	; additional forms of CMP

	cmp	x,y
	irp	reg,x,y,s
	cmp	reg,d3
	cmp	reg,#-1
	cmp	reg,#1
	cmp	reg,#2
	cmp	reg,#3
	cmp	reg,#4
	cmp	reg,#5
	cmp	reg,#6
	cmp	reg,#7
	cmp	reg,#8
	cmp	reg,#9
	cmp	reg,#10
	cmp	reg,#11
	cmp	reg,#12
	cmp	reg,#13
	cmp	reg,#14
	cmp	reg,#15
	cmp	reg,#$ffffff
	cmp	reg,#$ffff
	cmp	reg,#$ff
	cmp	reg,#$fffffe
	cmp	reg,#$304050
	cmp	reg,(-x)
	cmp	reg,(-y)
	cmp	reg,(-s)
	cmp	reg,(x-)
	cmp	reg,(y-)
	cmp	reg,(+x)
	cmp	reg,(+y)
	cmp	reg,(x+)
	cmp	reg,(y+)
	cmp	reg,(s+)
	cmp	reg,(2,x)
	cmp	reg,(y)
	cmp	reg,(s,7)
	cmp	reg,(20,x)
	cmp	reg,(-100,y)
	cmp	reg,(255,s)
	cmp	reg,(*+2,pc)
	cmp	reg,(10000,x)
	cmp	reg,(-10000,y)
	cmp	reg,(100000,s)
	cmp	reg,(*+100000,pc)
	cmp	reg,(d0,x)
	cmp	reg,(d1,y)
	cmp	reg,(s,d6)
	cmp	reg,(x)
	cmp	reg,(100,d7)
	cmp	reg,(100000,d0)
	cmp	reg,(1000000,x)
	cmp	reg,(1000000,d7)
	cmp	reg,(1000000,d0)
	cmp	reg,[d0,x]
        cmp	reg,[y,d1]
	cmp	reg,[x]
	cmp	reg,[y,2]
	cmp	reg,[-25,s]
	cmp	reg,[*-10,pc]
	cmp	reg,[10000,x]
	cmp	reg,[y,10000]
	cmp	reg,[10000,s]
	cmp	reg,[*+10000,pc]
	cmp	reg,[0]
	cmp	reg,[10000000]
	cmp	reg,$123
	cmp	reg,$1234
	cmp	reg,$12345
	cmp	reg,$22345
	cmp	reg,$32345
	cmp	reg,$123456
	endm

	andcc	#$55
	orcc	#$aa

	irp	instr,asl,asr,lsl,lsr
	instr	d6,d5,d0	; REG-REG
	instr	d6,d5,#5	; REG-IMM (general)
	instr	d6,d5,#1	; REG-IMM (n=1..2)
	instr	d6,d5,#2	; REG-IMM (n=1..2)
	instr	d6,d5,(-x)	; REG-OPR1/2/3 (ASL) OPR/1/2/3-OPR/1/2/3 (others)
	instr	d6,d5,$123456	; REG-OPR1/2/3 (ASL) OPR/1/2/3-OPR/1/2/3 (others)
	instr.b	d6,(x),#2	; OPR/1/2/3-IMM
	instr.w	d6,(x),#2	; OPR/1/2/3-IMM
	instr.p	d6,(x),#2	; OPR/1/2/3-IMM
	instr.l	d6,(x),#2	; OPR/1/2/3-IMM
	instr.b	d6,$1234,#5	; OPR/1/2/3-IMM
	instr.w	d6,$1234,#6	; OPR/1/2/3-IMM
	instr.p	d6,$1234,#7	; OPR/1/2/3-IMM
	instr.l	d6,$1234,#8	; OPR/1/2/3-IMM
	instr.b	d6,(100,x),(200,y)	; OPR/1/2/3-OPR/1/2/3
	instr.w	d6,(1000,x),(2000,y)	; OPR/1/2/3-OPR/1/2/3
	instr.p	d6,(10000,x),(20000,y)	; OPR/1/2/3-OPR/1/2/3
	instr.l	d6,(100000,x),(200000,y)	; OPR/1/2/3-OPR/1/2/3
	instr	d6,d6,d0	; REG-REG
	instr	d6,d0
	instr	d6,d6,#5	; REG-IMM (general)
	instr	d6,#5
	instr	d6,d6,#1	; REG-IMM (n=1..2)
	instr	d6,#1
	instr	d6,d6,#2	; REG-IMM (n=1..2)
	instr	d6,#2
	instr	d6,d6,(-x)	; REG-OPR1/2/3 (ASL) OPR/1/2/3-OPR/1/2/3 (others)
	instr	d6,(-x)
	instr	d6,d6,$123456	; REG-OPR1/2/3 (ASL) OPR/1/2/3-OPR/1/2/3 (others)
	instr	d6,$123456
	instr.b	(x),#1		; OPR/1/2/3-IMM (n=1..2)
	instr.w	(y),#2
	instr.p	(x+),#1
	instr.l	(y+),#2
	endm

	irp	instr,bclr,bset,btgl
	instr	d1,#7		; REG-IMM
	instr	d5,#15
	instr	d7,#31
	instr	d1,d0		; REG-REG
	instr	d5,d4
	instr	d7,d6
	instr.b	(100,x),#7	; OPR/1/2/3-IMM
	instr.w	(100,x),#15
	instr.l	(100,x),#31
	instr.b	$123456,d0	; OPR/1/2/3-REG
	instr.w	$123456,d4
	instr.l	$123456,d6
	endm

	irp	instr,bfext,bfins
	instr	d3,d4,d5	; REG-REG-REG
	instr	d3,d4,#10:3	; REG-REG-IMM
	instr.b	d6,(10,x),d5	; REG-OPR1/2/3-REG
	instr.w	d6,(10,x),d5
	instr.p	d6,(10,x),d5
	instr.l	d6,(10,x),d5
	instr.b	d6,(10,s),#10:3	; REG-OPR1/2/3-IMM
	instr.w	d6,(10,s),#10:3
	instr.p	d6,(10,s),#10:3
	instr.l	d6,(10,s),#10:3
	instr.b	(10,x),d6,d5	; OPR1/2/3-REG-REG
	instr.w	(10,x),d6,d5
	instr.p	(10,x),d6,d5
	instr.l	(10,x),d6,d5
	instr.b	(10,s),d6,#10:3	; OPR1/2/3-REG-IMM
	instr.w	(10,s),d6,#10:3
	instr.p	(10,s),d6,#10:3
	instr.l	(10,s),d6,#10:3
	endm

	irp	instr,brclr,brset
	instr	d1,#7,*+20		; REG-IMM-REL
	instr	d5,#15,*-30
	instr	d7,#31,*+40
	instr	d1,d0,*-50		; REG-REG-REL
	instr	d5,d4,*+60
	instr	d7,d6,*+70
	instr.b	(100,x),#7,*-80		; OPR/1/2/3-IMM-REL
	instr.w	(100,x),#15,*+90
	instr.l	(100,x),#31,*-100
	instr.b	$123456,d0,*+110	; OPR/1/2/3-REG-REL
	instr.w	$123456,d4,*-120
	instr.l	$123456,d6,*+130
	endm

	clb	d7,d0

	clr	d1
	clr.b	d1
	clr	d3
	clr.w	d3
	clr	d6
	clr.l	d6
	clr	x
	clr.p	x
	clr	y
	clr.p	y
	clr.b	(10,x)
	clr.w	$123
	clr.p	(-y)
	clr.l	($123456,x)

	irp	instr,com,neg
	instr	d1
	instr.b	d1
	instr	d3
	instr.w	d3
	instr	d6
	instr.l	d6
	instr.b	(10,x)
	instr.w	$123
	instr.l	($123456,x)
	endm

	irp	instr,dbne,dbeq,dbpl,dbmi,dbgt,dble
	instr	d1,*+10
	instr.b	d1,*+10
	instr	d3,*-20
	instr.w	d3,*-20
	instr	d6,*+30
	instr.l	d6,*+30
	instr	x,*-40
	instr.p	x,*-40
	instr	y,*+50
	instr.p	y,*+50
	instr.b	(10,x),*-60
	instr.w	$123,*+70
	instr.p	(-y),*-80
	instr.l	($123456,x),*+90
	endm

	irp	instr,dec,inc
	instr   d1
	instr.b d1
	instr   d3
	instr.w d3
	instr   d6
	instr.l d6
	instr.b (10,x)
	instr.w $123
	instr.l ($123456,x)
	endm

	irp	instr,divs,divu,mods,modu,macs,macu,muls,mulu,qmuls,qmulu
	instr	d7,d6,d3
	instr.b	d7,d6,#30
	instr.w	d7,d6,#30
	instr.l	d7,d6,#30
	instr.b	d7,d6,(16,s)
	instr.w	d7,d6,(16,s)
	instr.l	d7,d6,(16,s)
	instr.bb d7,(100,x),(16,s)
	instr.bw d7,(100,x),(16,s)
	instr.bl d7,(100,x),(16,s)
	instr.wb d7,(100,x),(16,s)
	instr.ww d7,(100,x),(16,s)
	instr.wl d7,(100,x),(16,s)
	instr.lb d7,(100,x),(16,s)
	instr.lw d7,(100,x),(16,s)
	instr.ll d7,(100,x),(16,s)
	endm

	irp	src,d2,d3,d4,d5,d0,d1,d6,d7,x,y,s,cch,ccl,ccw
	irp	dest,d2,d3,d4,d5,d0,d1,d6,d7,x,y,s,cch,ccl,ccw
	exg	src,dest
	tfr	src,dest
	endm
	endm

	irp	instr,jmp,jsr
	instr	$12
	instr	$123
	instr	$1234
	instr	$12345
	instr	$123456
	instr	(100,x)
	endm

	ld	d1,#3		; REG-IMM and REG-OPR1/2/3 equally good
	ld.b	d1,#3
	ld	d1,#$23		; REG-IMM
	ld.b	d1,#$23	
	ld	d3,#$1234
	ld.w	d3,#$1234
	ld	d6,#$12345678
	ld.l	d6,#$12345678
	ld	d1,$123456	; REG-EXT3
	ld.b	d1,$123456
	ld	d3,$123456
	ld.w	d3,$123456
	ld	d6,$123456
	ld.l	d6,$123456
	ld	d1,(100,x)	; REG-OPR1/2/3
	ld.b	d1,(100,x)
	ld	d3,(100,y)
	ld.w	d3,(100,y)
	ld	d6,(100,s)
	ld.l	d6,(100,s)
	ld	x,#$12345	; XREG-IMM2
	ld.p	x,#$12345
	ld	y,#$12345
	ld.p	y,#$12345
	ld	x,#$123456	; XREG-IMM3
	ld.p	x,#$123456
	ld	y,#$123456
	ld.p	y,#$123456
	ld	x,$123456	; XREG-EXT3
	ld	y,$123456
	ld	x,#3		; XREG-OPR1/2/3
	ld.p	x,#3
	ld	y,#3
	ld.p	y,#3
	ld	x,(100,y)
	ld.p	x,(100,y)
	ld	y,(100,x)
	ld.p	y,(100,x)
	ld	s,#$123456	; S-IMM3
	ld.p	s,#$123456
	ld	s,#3		; S-OPR1/2/3
	ld.p	s,#3
	ld	s,(100,x)
	ld.p	s,(100,x)

	lea	d6,(x,100)
	lea	d7,$123456
	lea	s,(100,s)
	lea	s,(-100,s)
	lea	x,(100,x)
	lea	x,(-100,x)
	lea	y,(100,y)
	lea	y,(-100,y)
	lea	s,(*+100,pc)
	lea	s,(*-100,pc)
	lea	x,(100,y)
	lea	x,(-100,y)
	lea	y,(100,x)
	lea	y,(-100,x)

	irp	instr,mins,minu,maxs,maxu
	instr	d1,(x)
	instr.b	d1,(x)
	instr	d4,(100,x)
	instr.w	d4,(100,x)
	instr	d6,$123456
	instr.l	d6,$123456
	endm

	mov	d0,d1		; REG-REG (may also use TFR)
	mov.b	d0,d1
	mov	d4,d5
	mov.w	d4,d5
	mov	d6,d7
	mov.l	d6,d7
	mov	#$aa,d0		; IMM-REG (no simm4 possible)
	mov.b	#$aa,d0
	mov	#$55aa,d5
	mov.w	#$55aa,d5
	mov	#$113355aa,d6
	mov.l	#$113355aa,d6
	mov.b	#$aa,(100,x)	; IMM-OPR1/2/3 (no simm4 possible)
	mov.w	#$55aa,(100,x)
	mov.p	#$3355aa,(100,x)
	mov.l	#$113355aa,(100,x)
	mov	#$ff,d0		; IMM-OPR1/2/3 (simm4 possible)
	mov.b	#$ff,d0
	mov	#$ffff,d5
	mov.w	#$ffff,d5
	mov	#$ffffffff,d6
	mov.l	#$ffffffff,d6
	mov.b	#$ff,(100,x)	; IMM-OPR1/2/3 (simm4 possible)
	mov.w	#$ffff,(100,x)
	mov.p	#$ffffff,(100,x)
	mov.l	#$ffffffff,(100,x)
	mov.b	(100,y),(100,x)	; OPR1/2/3-OPR1/2/3
	mov.w	(100,y),(100,x)
	mov.p	(100,y),(100,x)
	mov.b	(100,y),(100,x)

	irp	instr,psh,pul
	instr	all
	instr	cch,ccl,d0,d1,d2,d3,d4,d5,d6,d7,x,y
	instr	all16b
	instr	d2,d3,d4,d5
	instr	cch
	instr	ccl
	instr	d0
	instr	d1
	instr	d2
	instr	d3
	instr	d4
	instr	d5
	instr	d6
	instr	d7
	instr	x
	instr	y
	instr	cch,ccl,d0,d1,d2,d3
	instr	d4,d5,d6,d7,x,y
	;instr	s		; not possible
	;instr	d3,d4		; needs two separate instructions
	endm

	irp	instr,rol,ror
	instr	d1
	instr.b	d1
	instr	d3
	instr.w	d3
	instr	d6
	instr.l	d6
	instr.b	(200,x)
	instr.w	(200,x)
	instr.p	(200,x)
	instr.l	(200,x)
	instr.b	$123456
	instr.w	$123456
	instr.p	$123456
	instr.l	$123456
	endm

	sat	d1
	sat	d4
	sat	d7

	irp	instr,sex,zex
	instr	d0,d3
	instr	d0,y
	instr	d0,d6
	instr	d3,y
	instr	d3,d6
	instr	y,d6
	endm

	st	d1,$123456	; REG-EXT3
	st.b	d1,$123456
	st	d3,$123456
	st.w	d3,$123456
	st	d6,$123456
	st.l	d6,$123456
	st	d1,(100,x)	; REG-OPR1/2/3
	st.b	d1,(100,x)
	st	d3,(100,y)
	st.w	d3,(100,y)
	st	d6,(100,s)
	st.l	d6,(100,s)
	st	x,$123456	; XREG-EXT3
	st.p	y,$123456
	st	x,(100,y)
	st.p	x,(100,y)
	st	y,(100,x)
	st.p	y,(100,x)
	st	s,(100,x)
	st.p	s,(100,x)

	irp	instr,tbne,tbeq,tbpl,tbmi,tbgt,tble
	instr	d1,*+20
	instr.b	d1,*+20
	instr	d1,*+200
	instr.b	d1,*+200
	instr.bl d1,*+20
	instr	d3,*+20
	instr.w	d3,*+20
	instr	d3,*+200
	instr.w	d3,*+200
	instr.wl d3,*+20
	instr	d6,*+20
	instr.l	d6,*+20
	instr	d6,*+200
	instr.l	d6,*+200
	instr.ll d6,*+20
	instr	y,*+20
	instr.p	y,*+20
	instr	y,*+200
	instr.p	y,*+200
	instr.pl y,*+20
	instr.b	(50,x),*+20
	instr.bl (50,x),*+20
	instr.w	(50,x),*+20
	instr.wl (50,x),*+20
	instr.p	(50,x),*+20
	instr.pl (50,x),*+20
	instr.l	(50,x),*+20
	instr.ll (50,x),*+20
	endm

	trap	#$ff
	spare

	; define some bit symbols and see if using them results in the same
        ; code as writing register address & bit position explicitly

myreg	equ		$123
mybit	defbit.b	myreg,5
myfld	defbitfield.b	myreg,3:1
	bclr.b		$123,#5
	bclr.b		myreg,#5
	bclr		mybit
	bclr		myb.bflg
	bfins.b		$123,d4,#3:1
	bfins.b		myreg,d4,#3:1
	bfins		myfld,d4
	bfins		myb.bfld,d4
	bfext.b		d4,$123,#3:1
	bfext.b		d4,myreg,#3:1
	bfext		d4,myfld
	bfext		d4,myb.bfld

mywreg	equ		$240
mywbit	defbit.w	mywreg,12
mywfld	defbitfield.w	mywreg,5:7
	bset.w		$240,#12
	bset.w		mywreg,#12
	bset		mywbit
	bset		myw.wflg
	bfins.w		$240,d4,#5:7
	bfins.w		mywreg,d4,#5:7
	bfins		mywfld,d4
	bfins		myw.wfld,d4
	bfext.w		d4,$240,#5:7
	bfext.w		d4,mywreg,#5:7
	bfext		d4,mywfld
	bfext		d4,myw.wfld

mylreg	equ		$280
mylbit	defbit.l	mylreg,25
mylfld	defbitfield.l	mylreg,5:26
	bset.l		$280,#25
	bset.l		mylreg,#25
	bset		mylbit
	bset		myl.lflg
	bfins.l		$280,d4,#5:26
	bfins.l		mylreg,d4,#5:26
	bfins		mylfld,d4
	bfins		myl.lfld,d4
	bfext.l		d4,$280,#5:26
	bfext.l		d4,mylreg,#5:26
	bfext		d4,mylfld
	bfext		d4,myl.lfld

mypreg	equ		$380
mypfld	defbitfield.p	mypreg,5:18
	bfins.p		$380,d4,#5:18
	bfins.p		mypreg,d4,#5:18
	bfins		mypfld,d4
	bfins		myp.pfld,d4
	bfext.p		d4,$380,#5:18
	bfext.p		d4,mypreg,#5:18
	bfext		d4,mypfld
	bfext		d4,myp.pfld
