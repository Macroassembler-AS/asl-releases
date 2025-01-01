	cpu	xs1

	; 3r format

	irp	op,add,and,eq,ld16s,ld8u,ldw,lss,lsu,or,shl,shr,sub,tsetr
	op	r1,r4,r11
	op	r8,r0,r9
	endm

	; 2rus format

	irp	op,addi,eqi,ldwi,shli,shri,stwi,subi
	op	r1,r4,11
	op	r8,r0,9
	endm

	; 2r format

	irp	op,andnot,chkct,eef,eet,endin,getst,getts,in,inct,inshr,int,mkmsk,neg,not,outct,peek,sext
	op	r1,r11
	op	r3,r8
	op	r1,r10
	endm
	irp	op,testct,testwct,tinitcp,tinitdp,tinitpc,tinitsp,tsetmr,zext
	op	r1,r11
	op	r3,r8
	op	r1,r10
	endm

	;l3r format

	irp	op,ashr,lda16f,remu,crc,ldawb,st16,divs,ldawf,st8,divu,mul,stw,lda16b,rems,xor
        op      r1,r4,r11
        op      r8,r0,r9
	endm

	; l2rus format

	irp     op,ashri,ldawbi,outpw,inpw,ldawfi
        op      r1,r4,11
        op      r8,r0,9
        endm

	; 1r format

	irp 	op,bau,eeu,setsp,bla,freer,setv,bru,kcall,syncr,clrpt,mjoin,tstart,dgetreg
	op	r0
	op	r5
	op	r11
	endm
	irp 	op,dgetreg,msync,waitef,ecallf,setcp,waitet,ecallt,setdp,edu,setev
	op	r0
	op	r5
	op	r11
	endm

	; l2r format

	irp 	op,bitrev,getd,setc,byterev,getn,testlcl,clz,getps,tinitlr
        op      r1,r11
        op      r3,r8
        op      r1,r10
        endm

	; u10 format

	irp	op,blacp,blrf,ldapf,blrb,ldapb,ldwcpl
	op	10
	op	1023
	endm

	; lu10 format

	irp	op,blacp,blrf,ldapf,blrb,ldapb,ldwcpl
	op	1024
	op	1048575
	endm

	; u6 format

	irp	op,blat,extdp,krestsp,brbu,extsp,ldawcp,brfu,getsr,retsp,clrsr,kcalli,setsr,entsp,kentsp
	op	7
	op	63
	endm

	; lu6 format
        irp     op,blat,extdp,krestsp,brbu,extsp,ldawcp,brfu,getsr,retsp,clrsr,kcalli,setsr,entsp,kentsp
	op	64
	op	65535
	endm

	; ru6 format
	irp	op,brbf,ldawsp,setci,brbt,ldc,stwdp,brff,ldwcp,stwsp,brft,ldwdp,ldawdp,ldwsp
	op	r2,7
	op	r11,63
	endm

	; lru6 format
	irp	op,brbf,ldawsp,setci,brbt,ldc,stwdp,brff,ldwcp,stwsp,brft,ldwdp,ldawdp,ldwsp
	op	r2,64
	op	r11,65535
	endm

	; convenience instructions for branching

	brf	r1,$-10
	brf	r1,$+10
	brt	r1,$-10
	brt	r1,$+10
	bru	$-10
	bru	$+10
	brf	r1,$-1000
	brf	r1,$+1000
	brt	r1,$-1000
	brt	r1,$+1000
	bru	$-1000
	bru	$+1000

	; rus format

	irp	op,chkcti,mkmski,sexti,getr,outcti,zexti
	op	r5,3
	op	r10,10
	endm

	; 0r format

	irp 	op,clre,getid,setkep,dcall,getkep,ssync,dentsp,getksp,stet
	op
	endm

	irp 	op,drestsp,kret,stsed,dret,ldet,stspc,freet,ldsed,stssr,geted,ldspc,waiteu,getet,ldssr
	op
	endm

	; 4r format

	irp	op,crc8,maccs,maccu
	op	r1,r2,r3,r4
	op	r11,r10,r9,r8
	endm

	; lr2r format

	irp 	op,setclk,setps,settw,setn,setrdy
	op	r1,r11
	op	r4,r7
	endm

	; r2r format

	irp	op,out,outt,setpsc,outshr,setd,setpt
	op	r1,r11
        op      r4,r7
        endm

	; l5r format

	irp	op,ladd,ldivu,lsub
	op	r1,r5,r3,r4,r10
	op	r11,r0,r2,r7,r6
	endm

	; l6r format

	irp	op,lmul
	op	r1,r5,r3,r4,r10,r7
	op	r11,r0,r2,r7,r6,r1
	endm

	; register aliases

myreg1e		equ	r11
myreg2e		equ	r10
myreg3e		equ	r9
myreg1r		reg	r11
myreg2r		reg	r10
myreg3r		reg	r9
myreg1re	reg	myreg1e
myreg2re	reg	myreg2e
myreg3re	reg	myreg3e

		add	r11,r10,r9
		add	myreg1e,myreg2e,myreg3e
		add	myreg1r,myreg2r,myreg3r
		add	myreg1re,myreg2re,myreg3re

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
