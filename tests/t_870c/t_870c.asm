#define B8Regs a,w,c,b,e,d,l,h
#define B16Regs wa,bc,de,hl,ix,iy,sp
#define SrcMem (12h),(1234h),(de),(hl),(ix),(iy),(ix-3),(4+iy),(sp+4),(hl-2),(hl+c),(+sp),(pc+a)
#define DestMem (12h),(1234h),(de),(hl),(ix),(iy),(ix-3),(4+iy),(sp+4),(hl-2),(hl+c),(sp-)

	cpu	tlcs-870/c
	page	0

	irp	reg1, B8Regs
	irp	reg2, B8Regs
	ld	reg1,reg2
	endm
	endm

	irp	reg1, B16Regs
	irp	reg2, B16Regs
	ld	reg1,reg2
	endm
	endm

	ld	a,(12h)
	ld	a,(hl)

	irp	reg, B8Regs
	irp	mem, SrcMem
	ld	reg,mem
	endm
	endm

	irp	reg, B8Regs
	ld	reg,12h
	endm

	irp	reg, B16Regs
	irp	mem, SrcMem
	ld	reg,mem
	endm
	endm

	irp	reg, B16Regs
	ld	reg,1234h
	endm

	ld	(hl),a
	ld	(12h),a

	irp	reg, B8Regs
	irp	mem, DestMem
	ld	mem,reg
	endm
	endm

	irp	reg, B16Regs
	irp	mem, DestMem
	ld	mem,reg
	endm
	endm

	irp	mem, DestMem
	ld	mem,0aah
	endm

	irp	reg, B16Regs
	ldw	reg,55aah
	endm
	ldw	(12h),55aah
	ldw	(hl),55aah

	irp	op,push,pop
	op	psw
	irp	reg,B16Regs
	op	reg
	endm
	endm

	ld	psw, 0ddh

	ld	rbs,0
	ld	rbs,1

	ld	sp,sp+2
	ld	sp,sp+0
	ld	sp,sp-5

	irp	reg1,B8Regs
 	irp	reg2,B8Regs
 	xch	reg1,reg2
	endm
	irp	mem,SrcMem
	xch	reg1,mem
	xch	mem,reg1
	endm
	endm

	irp	reg1,B16Regs
 	irp	reg2,B16Regs
 	xch	reg1,reg2
	endm
	irp	mem,SrcMem
	xch	reg1,mem
	xch	mem,reg1
	endm
	endm

	irp	instr,cmp,add,addc,sub,subb,and,or,xor
	 irp	 reg1,B8Regs
	  instr	  reg1,0aah
	  irp	  reg2,B8Regs
	   instr   reg1,reg2
          endm
	  irp	  mem,SrcMem
           instr   reg1,mem
	  endm
         endm
	 irp	 reg1,B16Regs
	  instr	  reg1,0aa55h
	  irp	  reg2,B16Regs
	   instr   reg1,reg2
          endm
	  irp	  mem,SrcMem
           instr   reg1,mem
	  endm
	 endm
	 irp	  mem,SrcMem
	  instr	   mem,66h
	 endm
	endm

	irp	instr,inc,dec
	 irp	 op,B8Regs,B16Regs
	  instr	  op
	 endm
	 irp	 op,SrcMem
	  instr	  op
	 endm
	endm

	irp	instr,daa,das,shlc,shrc,rolc,rorc,swap
	 irp	 reg,B8Regs
	  instr	  reg
	 endm
	endm

	mul	w,a
	mul	a,w
	mul	b,c
	mul	c,b
	mul	d,e
	mul	e,d
	mul	h,l
	mul	l,h

	div	wa,c
	div	de,c
	div	hl,c

	irp	reg, B16Regs
	 neg	 cs,reg
	endm

	irp	instr,shlca,shrca
	 irp	 reg,B16Regs
	  instr   reg
	 endm
	endm

	irp	instr,rold,rord
	 irp	 mem,SrcMem
	  instr   a,mem
	 endm
	endm

	irp	instr,set,clr,test,cpl
	 irp	 reg,B8Regs
	  instr	  reg.2
	 endm
	 irp	 mem,SrcMem
	  instr	  mem.5
	 endm
	 irp	 mem,SrcMem
	  instr	  mem.a
	 endm
	endm

	irp	reg,B8Regs
	 ld	 cf,reg.3
	endm
	irp	mem,SrcMem
	 ld	 cf,mem.4
	endm
	irp	mem,SrcMem
	 ld	 cf,mem.a
	endm

	irp	reg,B8Regs
	 ld	 reg.3,cf
	endm
	irp	mem,SrcMem
	 ld	 mem.4,cf
	endm
	irp	mem,SrcMem
	 ld	 mem.a,cf
	endm

	irp	reg,B8Regs
	 xor	 cf,reg.1
	endm
	irp	mem,SrcMem
	 xor	 cf,mem.6
	endm

	clr	cf
	cpl	cf
	set	cf

	ei
	di

	jr	targ
	irp	cond,t,f,eq,z,ne,nz,cs,lt,cc,ge,le,gt
	 jr	 cond,targ
	endm
	irp	cond,m,p,slt,sge,sle,sgt,vs,vc
	 jr	 cond,targ
	endm
targ:

	jrs	t,targ2
targ2:
	jrs	f,targ2

	jp	1234h
	irp	reg,B16Regs
	 jp	 reg
	endm
	irp	mem,SrcMem
	 jp	 mem
	endm

	j	$+10		; coded as JR
	j	$-10		; coded as JR
	j	$+100		; coded as JR
	j	$-100		; coded as JR
	j	$+1000		; coded as JP
	j	$-1000		; coded as JP

	j	t,$+10		; coded as JRS
	j	t,$-10		; coded as JRS
	j	t,$+100		; coded as JR
	j	t,$-100		; coded as JR
	j	t,$+1000	; coded as skipped JP
	j	t,$-1000	; coded as skipped JP

	j	f,$+10		; coded as JRS
	j	f,$-10		; coded as JRS
	j	f,$+100		; coded as JR
	j	f,$-100		; coded as JR
	j	f,$+1000	; coded as skipped JP
	j	f,$-1000	; coded as skipped JP

	j	slt,$+10	; coded as JR
	j	slt,$-10	; coded as JR
	j	slt,$+100	; coded as JR
	j	slt,$-100	; coded as JR
	j	slt,$+1000	; coded as skipped JP
	j	slt,$-1000	; coded as skipped JP

	callv	9

	call	1234h
	irp	reg,B16Regs
	 call	 reg
	endm
	irp	mem,SrcMem
	 call	 mem
	endm

	ret
	reti
	retn
	swi
	nop

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
