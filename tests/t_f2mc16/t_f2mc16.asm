	cpu	mb90500
	page	0

;----------------------------------------------------------------
; some preliminaries to keep the source code in acceptable size
; you didn't see this - you didn't see this - ignore it

#define R8MODES r0,r1,r2,r3,r4,r5,r6,r7
#define R16MODES rw0,rw1,rw2,rw3,rw4,rw5,rw6,rw7
#define R32MODES rl0,(rl0),rl1,(rl1),rl2,(rl2),rl3,(rl3)
#define MEM1MODES @rw0,@rw1,@rw2,@rw3
#define MEM1AMODES @rw0+,@rw1+,@rw2+,@rw3+
#define MEM2MODES @rw0+10,@rw1-10,@rw2+20,@rw3-30,@rw4+40,@rw5-50,@rw6+60,@rw7-70,@rw0+1000,@rw1-1000,@rw2+2000,@rw3-3000,@rw0+rw7,@rw1+rw7,@pc+5,1234h

enum8_x macro memo
	irp	var,R8MODES
	memo	var
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	var
	endm
	irp	var,MEM2MODES
	memo	var
	endm
	endm

enum16_x macro memo
	irp	var,R16MODES
	memo	var
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	var
	endm
	irp	var,MEM2MODES
	memo	var
	endm
	endm

enum32_x macro memo
	irp	var,R32MODES
	memo	var
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	var
	endm
	irp	var,MEM2MODES
	memo	var
	endm
	endm

enum8_px macro memo,par
	irp	var,R8MODES
	memo	par,var
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	par,var
	endm
	irp	var,MEM2MODES
	memo	par,var
	endm
	endm

enum16_px macro memo,par
	irp	var,R16MODES
	memo	par,var
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	par,var
	endm
	irp	var,MEM2MODES
	memo	par,var
	endm
	endm

enum32_px macro memo,par
	irp	var,R32MODES
	memo	par,var
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	par,var
	endm
	irp	var,MEM2MODES
	memo	par,var
	endm
	endm

enum8_xp macro memo,par
	irp	var,R8MODES
	memo	var,par
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	var,par
	endm
	irp	var,MEM2MODES
	memo	var,par
	endm
	endm

enum16_xp macro memo,par
	irp	var,R16MODES
	memo	var,par
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	var,par
	endm
	irp	var,MEM2MODES
	memo	var,par
	endm
	endm

enum32_xp macro memo,par
	irp	var,R32MODES
	memo	var,par
	endm
	irp	var,MEM1MODES,MEM1AMODES
	memo	var,par
	endm
	irp	var,MEM2MODES
	memo	var,par
	endm
	endm

enum8_xpp macro memo,par1,par2
	irp	var,R8MODES
	memo	var,par1,par2
	endm
	irp	var,MEM1MODES
	memo	var,par1,par2
	endm
	irp	var,MEM2MODES
	memo	var,par1,par2
	endm
	endm

enum16_xpp macro memo,par1,par2
	irp	var,R16MODES
	memo	var,par1,par2
	endm
	irp	var,MEM1MODES
	memo	var,par1,par2
	endm
	irp	var,MEM2MODES
	memo	var,par1,par2
	endm
	endm

enumseg macro 	memo
	irp	var,pcb,dtb,adb,spb
	memo	var
	endm
	endm

;----------------------------------------------------------------
; let's go...

	add	a,#12h
	add	a,123h
	enum8_px add,a
	enum8_xp add,a

	addc	a
	enum8_px addc,a

	enum16_px addcw,a

	adddc	a

	addl	a,#12345678h
	enum32_px addl,a

	addsp	#20
	addsp	#-20
	addsp	#200
	addsp	#-200

	addw	a
	addw	a,#1234h
	enum16_px addw,a
	enum16_xp addw,a

	and	a,#12h
	and	a,123h		; translates to 16 bit!
	enum8_px and,a
	enum8_px and,a
	and	ccr,#12h

	enum32_px andl,a

	andw	a
	andw	a,#1234h
	enum16_px andw,a
	enum16_xp andw,a

	asr	a,r0
	asrl	a,r0
	asrw	a
	asrw	a,r0

	bbc	80h:4,$+2
	bbc	180h:4,$+2
	bbc	280h:4,$+2

	bbs	80h:4,$+2
	bbs	180h:4,$+2
	bbs	280h:4,$+2

	bz	$+2
	beq	$+2
	bnz	$+2
	bne	$+2
        bc	$+2
	blo	$+2
	bnc	$+2
	bhs	$+2
	bn	$+2
        bp	$+2
	bv	$+2
	bnv	$+2
	bt	$+2
	bnt	$+2
        blt	$+2
	bge	$+2
	ble	$+2
	bgt	$+2
	bls	$+2
        bhi	$+2
	bra	$+2

	call	0ff1234h
	call	@rw0
	call	@rw1
	call	@rw2
	call	@rw3
	call	@rw4
	call	@rw5
	call	@rw6
	call	@rw7
	call	@@rw0
	call	@@rw1
	call	@@rw2
	call	@@rw3
	call	@@rw0+
	call	@@rw1+
	call	@@rw2+
	call	@@rw3+
	call	@@rw0+10
	call	@@rw1-10
	call	@@rw2+20
	call	@@rw3-30
	call	@@rw4+40
	call	@@rw5-50
	call	@@rw6+60
	call	@@rw7-70
	call	@@rw0+1000
	call	@@rw1-1000
	call	@@rw2+2000
	call	@@rw3-3000
	call	@@rw0+rw7
	call	@@rw1+rw7
	call	@@pc+5
	call	@1234h

	callp	0123456h
	callp	@rl0
	callp	@(rl0)
	callp	@rl1
	callp	@(rl1)
	callp	@rl2
	callp	@(rl2)
	callp	@rl3
	callp	@(rl3)
	callp	@@rw0
	callp	@@rw1
	callp	@@rw2
	callp	@@rw3
	callp	@@rw0+
	callp	@@rw1+
	callp	@@rw2+
	callp	@@rw3+
	callp	@@rw0+10
	callp	@@rw1-10
	callp	@@rw2+20
	callp	@@rw3-30
	callp	@@rw4+40
	callp	@@rw5-50
	callp	@@rw6+60
	callp	@@rw7-70
	callp	@@rw0+1000
	callp	@@rw1-1000
	callp	@@rw2+2000
	callp	@@rw3-3000
	callp	@@rw0+rw7
	callp	@@rw1+rw7
	callp	@@pc+5
	callp	@1234h

	callv	#10

	cbne	a,#12,$
	enum8_xpp cbne,#12,$

	clrb	80h:4
	clrb	180h:4
	clrb	280h:4

	cmp	a
	cmp	a,#12h
	enum8_px cmp,a

	cmpl	a,#12345678h
	enum32_px cmpl,a

	cmpw	a
	cmpw	a,#1234h
	enum16_px cmpw,a

	cmr

	cwbne	a,#1234,$
	enum16_xpp cwbne,#1234,$

	enum8_xp dbnz,$

	enum8_x dec

	enum32_x decl

	enum16_x decw

	div	a
	enum8_px div,a

	enum16_px divw,a

	divu	a
	enum8_px divu,a

	enum16_px divuw,a

	enum16_xp dwbnz,$

	ext
	extw

	enum8_x inc

	enum32_x incl

	enum16_x incw

	enumseg fils
	enumseg filsi
	enumseg filsw
	enumseg filswi

	int	0ff1234h
	int	#11

	int9

	intp	123456h

	jctx	@a

	jmp	@a
	jmp	0ff1234h
	jmp	@rw0
	jmp	@rw1
	jmp	@rw2
	jmp	@rw3
	jmp	@rw4
	jmp	@rw5
	jmp	@rw6
	jmp	@rw7
	jmp	@@rw0
	jmp	@@rw1
	jmp	@@rw2
	jmp	@@rw3
	jmp	@@rw0+
	jmp	@@rw1+
	jmp	@@rw2+
	jmp	@@rw3+
	jmp	@@rw0+10
	jmp	@@rw1-10
	jmp	@@rw2+20
	jmp	@@rw3-30
	jmp	@@rw4+40
	jmp	@@rw5-50
	jmp	@@rw6+60
	jmp	@@rw7-70
	jmp	@@rw0+1000
	jmp	@@rw1-1000
	jmp	@@rw2+2000
	jmp	@@rw3-3000
	jmp	@@rw0+rw7
	jmp	@@rw1+rw7
	jmp	@@pc+5
	jmp	@1234h

	jmpp	0123456h
	jmpp	@rl0
	jmpp	@(rl0)
	jmpp	@rl1
	jmpp	@(rl1)
	jmpp	@rl2
	jmpp	@(rl2)
	jmpp	@rl3
	jmpp	@(rl3)
	jmpp	@@rw0
	jmpp	@@rw1
	jmpp	@@rw2
	jmpp	@@rw3
	jmpp	@@rw0+
	jmpp	@@rw1+
	jmpp	@@rw2+
	jmpp	@@rw3+
	jmpp	@@rw0+10
	jmpp	@@rw1-10
	jmpp	@@rw2+20
	jmpp	@@rw3-30
	jmpp	@@rw4+40
	jmpp	@@rw5-50
	jmpp	@@rw6+60
	jmpp	@@rw7-70
	jmpp	@@rw0+1000
	jmpp	@@rw1-1000
	jmpp	@@rw2+2000
	jmpp	@@rw3-3000
	jmpp	@@rw0+rw7
	jmpp	@@rw1+rw7
	jmpp	@@pc+5
	jmpp	@1234h

	link	#10

	lsl	a,r0
	lsll	a,r0
	lslw	a
	lslw	a,r0

	lsr	a,r0
	lsrl	a,r0
	lsrw	a
	lsrw	a,r0

	mov	a,#123
	mov	a,@a
	mov	a,@rl0+10
	mov	a,@rl1-20
	mov	a,@rl2+30
	mov	a,@rl3-40
	mov	a,12h
	mov	a,123h
	mov	a,dtb
	mov	a,adb
	mov	a,ssb
	mov	a,usb
	mov	a,dpr
	mov	a,pcb
	enum8_px mov,a
	mov	123h,a
	mov	@rl0+10,a
	mov	@rl1-20,a
	mov	@rl2+30,a
	mov	@rl3-40,a
	mov	12h,a
	mov	dtb,a
	mov	adb,a
	mov	ssb,a
	mov	usb,a
	mov	dpr,a
        enum8_xp mov,a
	mov	rp,#12
	mov	ilm,#12
	mov	123h,#12
	mov	23h,#12
	enum8_xp mov,#12
	enum8_px mov,r2
	enum8_xp mov,r2
	mov	@a,t
	mov	@al,ah

	movb	a,123h:4
	movb	a,23h:4
	movb	a,1234h:4
	movb	123h:4,a
	movb	23h:4,a
	movb	1234h:4,a

	enum16_px movea,a
	enum16_px movea,rw2

	movl	a,#12345678h
	enum32_px movl,a
	enum32_xp movl,a

	movn	a,#4

	irp	reg2,pcb,dtb,adb,spb
	 irp	 reg1,pcb,dtb,adb,spb
	  movs	  reg1,reg2
	  movsi	  reg1,reg2
	  movsd	  reg1,reg2
	  movsw	  reg1,reg2
	  movswi  reg1,reg2
	  movswd  reg1,reg2
	 endm
	endm

	movw	a,#1234h
	movw	a,@a
	movw	a,sp
	movw	a,@rl0+10
	movw	a,@rl1-20
	movw	a,@rl2+30
	movw	a,@rl3-40
	movw	a,23h
	movw	a,123h
	enum16_px movw,a
	movw	@rl0+10,a
	movw	@rl1-20,a
	movw	@rl2+30,a
	movw	@rl3-40,a
	movw	sp,a
	movw	23h,a
	movw	123h,a
	enum16_xp movw,a
	enum16_xp movw,#1234h
	enum16_px movw,rw2
	enum16_xp movw,rw2
	movw	23h,#1234h
        movw    @a,t
        movw    @al,ah

	movx	a,#12h
	movx	a,@a
	movx	a,@rl0+10
	movx	a,@rl1-20
	movx	a,@rl2+30
	movx	a,@rl3-40
	movx	a,23h
	movx	a,123h
	enum8_px movx,a

	mul	a
	enum8_px mul,a

	mulw	a
	enum16_px mulw,a

	mulu	a
	enum8_px mulu,a

	muluw	a
	enum16_px muluw,a

	ncc

	neg	a
	enum8_x	neg

	negw	a
	enum16_x negw

	nop

	not	a
	enum8_x	not

	notw	a
	enum16_x notw

	nrml	a,r0

	or	a,#12h
	or	a,123h		; translates to 16 bit!
	enum8_px or,a
	enum8_xp or,a
	or	ccr,#12h

	enum32_px orl,a

	orw	a
	orw	a,#1234h
	enum16_px orw,a
	enum16_xp orw,a

	popw	a
	popw	ah
	popw	ps
	popw	rw4
	popw	rw4,rw6
	popw	rw1,rw3-rw4
	popw	rw6-rw2,rw4

	pushw	a
	pushw	ah
	pushw	ps
	pushw	rw4
	pushw	rw4,rw6
	pushw	rw1,rw3-rw4
	pushw	rw6-rw2,rw4

	ret
	reti
	retp

	rolc	a
	enum8_x rolc

	rorc	a
	enum8_x rorc

	sbbs	1234h:4,$

	enumseg sceqd
	enumseg sceqi
	enumseg scweqd
	enumseg scweqi

	setb	80h:4
	setb	180h:4
	setb	280h:4

	sub	a,#12h
	sub	a,123h
	enum8_px sub,a
	enum8_xp sub,a

	subc	a
	enum8_px subc,a

	enum16_px subcw,a

	subdc	a

	subl	a,#12345678h
	enum32_px subl,a

	subw	a
	subw	a,#1234h
	enum16_px subw,a
	enum16_xp subw,a

	swap
	swapw

	unlink

	wbtc	34h:7

	wbts	34h:7

	enum8_px xch,a
	enum8_xp xch,a
	enum8_px xch,r2
	enum8_xp xch,r2

	enum16_px xchw,a
	enum16_xp xchw,a
	enum16_px xchw,rw2
	enum16_xp xchw,rw2

	xor	a,#12h
	xor	a,123h		; translates to 16 bit!
	enum8_px xor,a
	enum8_xp xor,a

	enum8_px xorl,a

	xorw	a
	xorw	a,#1234h
	enum16_px xorw,a
	enum16_xp xorw,a

	zext
	zextw

;-------------------------------------------------------------------------
; automatic extension

	mov	1234h,r4	; normal
	mov	123h,r4		; extend dir->abs
	mov	12h,r4		; extend io->abs

	movw	1234h,rw4	; normal
	movw	123h,rw4	; extend dir->abs
	movw	12h,rw4		; extend io->abs

	movw	1234h,#5678h	; normal
	movw	123h,#5678h	; extend dir->abs
	movw	12h,#5678h	; normal, exception!

;-------------------------------------------------------------------------
; check banking

	assume	pcb:0
	assume	dtb:1
	assume	adb:2
	assume	usb:3
	assume	ssb:4

	pcb
	mov	a,02000h
	mov	a,12000h
	adb
	mov	a,22000h
	spb
	mov	a,32000h
	supmode on
	spb
	mov	a,42000h
