        cpu     tms9900
        page    0
	supmode off			; TMS9900 has no privileged mode, instructions shall not generate warnings

        a       r5,r3

        a       wr5,wr3
        ab      wr6,*wr10
        c       wr7,*wr2+
        cb      wr8,@1234h
        s       wr9,@1234h(wr6)

        sb      *wr10,wr9
        soc     *wr11,*wr6
        socb    *wr12,*wr1+
        szc     *wr13,@1234h
        szcb    *wr14,@1234h(wr14)

        mov     *wr15+,wr10
        movb    *wr0+,*wr6
        a       *wr1+,*wr6+
        ab      *wr2+,@1234h
        c       *wr3+,@1234h(wr7)

        cb      *wr4+,wr5
        s       *wr5+,*wr13
        sb      *wr6+,*wr13+
        soc     *wr7+,@1234h
        socb    *wr8+,@1234h(wr12)

        szc     @1234h,wr10
        szcb    @1234h,*wr2
        mov     @1234h,*wr3+
        movb    @1234h,@2345h
        a       @1234h,@2345h(wr8)

        ab      @1234h(wr9),wr5
        c       @1234h(wr10),*wr1
        cb      @1234h(wr11),*wr14+
        s       @1234h(wr12),@2345h
        sb      @1234h(wr13),@2345h(wr5)


        coc	wr12,wr5
        czc	*wr4,wr10
        xor	*wr12+,wr7
        mpy	@1234h,wr4
        div	@200(wr4),wr6
        xop	@2345h,wr5

        b	@1234h
        bl	*wr5
        blwp	*wr7+
        clr	wr4
        seto	@1234h(wr8)
        inv	wr7
        neg	*wr15
        abs	wr3
        swpb	wr9
        inc	*wr12+
        inct	*wr12+
        dec	*wr12+
        dect	*wr12+
        x	*wr6

        ldcr	wr5,10
        stcr	wr6,16

        sbo	10
        sbz	-33
        tb	34h

        jeq	lab
        jgt	lab
        jh	lab
        jhe	lab
        jl	lab
        jle	lab
lab:	jlt	lab
	jmp	lab
        jnc	lab
        jne	lab
        jno	lab
        joc	lab
        jop	lab

        sla	wr1,1
        sra	wr5,wr0
        src	wr6,10
        srl	wr10,15

        ai	wr5,1234h
        andi	wr10,2345h
        ci	wr15,3456h
        li	wr5,4567h
        ori	wr10,5678h

        lwpi	1234h
        limi	2345h

        stst	wr2
        stwp	wr6

        rtwp
        idle
        rset
        ckof
        ckon
        lrex

        padding on

        byte    1,2,3,4,5
	word	1,2,3,4,5	; inserts pad byte before
        byte    "Hello World"
        byte    1,2,3,4,5,6	; no pad needed, byte-oriented insn
	word	1,2,3,4,5	; two odd lengths add up to even length -> no padding needed
        byte    "Hello World!"
        word    1,2,3,4,5	; even number of bytes on previous insn -> no padding needed

        padding off

        byte    1,2,3,4,5
        byte    "Hello World"
        byte    1,2,3,4,5,6
        byte    "Hello World!"
        word    1,2,3,4,5


table	equ	1234h
oldval	equ	2345h
newval	equ	3456h
new	equ	4567h
loc	equ	5678h
count	equ	6789h
list	equ	789ah
tran	equ	89abh
testbits equ	9abch
testbit equ	0abcdh
ones	equ	0bcdeh
temp	equ	0cdefh
change	equ	0def0h
bits	equ	0ef01h
testva	equ	0f012h
prt	equ	r3

        a	r5,@table
        ab	3,*2+
	ai	6,0ch
        s	@oldval,@newval
	sb	*6+,1
        mpy	@new,5
        div	@loc,2
        inc	@count
        inct	5
        dec	@count
        dect	prt
	abs	@list(7)
        neg	5
        b	*3
        bl	@tran
        blwp	@tran
        rtwp
        jmp	$
        jh	$
        jl	$
        jhe	$
        jle	$
        jgt	$
        jlt	$
        jeq	$
        jne	$
        joc	$
        jnc	$
        jno	$
        jop	$
        x       @tran
        c	r5,@table
        cb	3,*2+
	ci	9,0f330h
	coc	@testbits,8
        czc	@testbit,8
        rset
        idle
        ckof
        ckon
        lrex
        sbo	20
        sbz	30
        tb	40
        ldcr	@1234h,7
        stcr	@1234h,7
        li	7,5
        limi	3
        lwpi	12h
        mov	7,7
        mov	@ones,9
        movb	@temp,3
        swpb	*0+
        stst	7
        stwp	r5
        andi	0,6d03h
        ori	5,60d3h
        xor	@change,2
        inv	11
        clr	*0bh
        seto	3
        soc	3,@new
        socb	5,8
        szc	5,7
        SZCB	@bits,@testva
        sra	5,0
        sla	10,5
        srl	0,3
        src	2,7
        xop	*4,wr4

	cpu	tms9995

        mpys	wr5
        divs	*wr9+
        lst	wr4
        lwp	wr8

	cpu	tms9940

	irp	instr,rset,lrex,ckon,ckof	; deleted on TMS9940
	expect	1500
	instr
	endexpect
	endm

	liim	2			; new on TMS9940
	dca	*wr5
        dcs	*wr7+

	cpu	tms99105

	; these instructions are privileged on 99xxx (and TI990 computer):

	irp	instr,idle,rset,ckon,ckof,lrex
	expect	50
	instr
	endexpect
	endm
	expect	50
	limi	1234h
	endexpect

	bind	@1234h(wr9)
	evad	*wr4

	blsk	wr5,1234h

	am	@1234h(wr13),@2345h(wr5)
	sm	@1234h(wr13),@2345h(wr5)

	slam	@1234h(wr13),10
	sram	*wr13,11

	tmb	*wr2,wr0
	tcmb	@1234h(wr13),10
	tsmb	@1234h(wr13),10

	rtwp	0
	rtwp	1
	rtwp	2
	expect	1985
	rtwp	3
	endexpect
	rtwp	4
	expect	1320
	rtwp	5
	endexpect

	cpu	tms99110

	; only on TI990 & TMS99110
	; LDS and LDD expect no argument on TMS99110 and act like a prefix?

	expect	50	
        lmf	r5,0
	endexpect
	expect	50
        lds
	endexpect
	expect	50
        ldd
	endexpect

	mm	@1234h(wr13),@2345h(wr5)
	cr	@1234h(wr13),@2345h(wr5)
	cer
	cre
	negr
	cri
	ar	@1234h(wr9)
	sr	*wr10
	mr	@1234h(wr9)
	dr	*wr10
	lr	@1234h(wr9)
	str	*wr10
	cir	@1234h(wr9)

	cpu	ti990/12

	; only on TI990/12
	; LDS and LDD have same opcode, but different arguments on TI990?

	expect	50	
        lmf	r5,0
	endexpect
	expect	50
        lds	@bits
	endexpect
	expect	50
        ldd	@bits
	endexpect

	; Type 11 instructions have a byte count as third argument.
        ; The TMS99xxx only implements AM and SM of those, with the
        ; additional restriction that the byte count is limited to 4,
        ; i.e. operand size is fixed to 32 bits.  This is also the
	; default for TI990/12 if the third argument is omitted:

	am	*WR7,@1234h(wr9),4
	am	*WR7,@1234h(wr9)
	sm	*WR7,@1234h(wr9),8
	nrm	*WR8,@1234h(wr10),7
	rto	*WR9,@1234h(wr11),6
	lto	*WR10,@1234h(wr12),5
	cnto	*WR11,@1234h(wr13),4
	bdc	*WR12,@1234h(wr14),3
	dbc	*WR13,@1234h(wr15),2
	swpm	*WR14,@1234h(wr1),1
	xorm	*WR15,@1234h(wr2),wr0
	orm	*WR1,@1234h(wr3),15
	andm	*WR2,@1234h(wr4),14

	emd
	eint
	dint
	cdi
	negd
	cde
	ced
	xit

	ad	*wr9
	cid	wr5
	dd	*wr12+
	ld	@1234h
	md	@1234h(wr12)
	sd	wr12
	std	@1234h(wr12)

	expect	2170
	crc	@1234h(wr12),@5678(wr13)
	endexpect
	crc	@1234h(wr12),@5678h(wr13),,wr15
	ckpt	wr15
	crc	@1234h(wr12),@5678h(wr13),,
	crc	@1234h(wr12),@5678h(wr13),wr0
	crc	@1234h(wr12),@5678h(wr13),12
	cs	@1234h(wr12),@5678h(wr13),12
	movs	@1234h(wr12),@5678h(wr13),12
	mvsk	@1234h(wr12),@5678h(wr13),12
	mvsr	@1234h(wr12),@5678h(wr13),12
	pops	@1234h(wr12),@5678h(wr13),12
	pshs	@1234h(wr12),@5678h(wr13),12
	seqb	@1234h(wr12),@5678h(wr13),12
	sneb	@1234h(wr12),@5678h(wr13),12
	ts	@1234h(wr12),@5678h(wr13),12
	ckpt	nothing
	expect	2170
	crc	@1234h(wr12),@5678(wr13)
	endexpect

	expect	2180,2180
	iof	@1234h,1
	iof	@1234h,(1)
	endexpect
	iof	@1234h,(1,8)

	insf	@0aa55h,@3366h,(1,8)
	xv	@0aa55h,@3366h,(1,8)
	xf	@0aa55h,@3366h,(1,8)

	arj	dest,,wr3
	arj	dest,wr3
	arj	dest,1,wr3
	arj	dest,8,wr3
	srj	dest,8,wr3
dest:

	stpc	r4
	lim	r5
	lcs	r6

	mova	@1234h(wr12),*wr4

	slsl	eq,@1234h,@5678h
	slsp	ne,@1234h,@5678h

	ep	@1234h,@5678h,6,10

	; leave out some values that may be subject of rounding errors:

	single	0.0		; 0000 0000 = 0.0000 * 16^(-64)
	single	1.0		; 4110 0000 = 1/16 * 16^1
	single	2.0		; 4120 0000 = 1/8 * 16^1
	single	4.0		; 4140 0000 = 1/4 * 16^1
	single	8.0		; 4180 0000 = 1/2 * 16^1
	single	16.0		; 4210 0000 = 1/16 * 16^2
	single	24.0		; 4218 0000 = 3/16 * 16^2
	single	42.0		; 422A 0000 = 21/8 * 16^2
	single	0.125		; 4020 0000 = 1/8 * 16^0
	single	0.0078125	; 3F20 0000 = 1/8 * 16^-1
	single	1000.0		; 433E 8000 = 0.24414062 * 16^3
	single	-1000.0		; C33E 8000 = -0.24414062 * 16^3
        single  -118.625        ; C276 A000 = -0.463... * 16^1
;	single	0.1		; 4019 999A = 0.1 * 16^0
;	single	7.23700528E75	; 7FFF FFFf (almost maximum)
;	single	5.397605346934E-79	; 0010 0000 or 000F FFFF (smallest non-denormal number)
