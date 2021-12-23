	cpu	z380
        include regz380

        extmode on

        page    0
        relaxed on

	ddir	w
	ddir	ib,w
	ddir	iw,w
	ddir	ib
	ddir	lw
	ddir	ib,lw
	ddir	iw,lw
	ddir	iw

	cpl
	cpl	a
	neg
	neg	a

	ei
	ei	$40
	di
	di	$bf

	btest
	exall
	exxx
	exxy
	indw
	indrw
	iniw
	inirw
	lddw
	lddrw
	ldiw
	ldirw
	mtest
	otdrw
	otirw
	outdw
	outiw
	retb

	cplw
        cplw	hl
        negw
        negw	hl

        ret
        ret	nz
        ret	z
        ret	nc
        ret	c
        ret	po
        ret	nv
        ret	pe
        ret	v
        ret	p
        ret	ns
        ret	m
        ret	s

        jp	1234h
        jp	123456h
        ddir	ib
        jp	123456h
        ddir	iw
        jp	123456h
	ddir	w
        jp	123456h
        ddir	lw
        jp	123456h
        ddir	w,iw
        jp	123456h
        jp	12345678h
        ddir	lw
        jp	12345678h
        jp	z,4321h
        jp	nc,654321h
        jp	pe,87654321h
        jp	(hl)
        jp	(ix)
        jp	(iy)

        call	$1234
	call	$123456
        call	$12345678
        call	nz,$4321
        call	m,$654321
        call	po,$87654321

        jr	$+20
        jr	c,$-20
        jr	$-200
        jr	z,$+200
        jr	$+$200000
        jr	nc,$-$200000

        calr	$+20
        calr	c,$-20
        calr	$-200
        calr	z,$+200
        calr	$+$200000
        calr	nc,$-$200000

        djnz	$+20
        djnz	$-200
        djnz	$+$200000

        exts	a
        exts
        extsw	hl
        extsw

        and	a
        and	a,b
        and	a,c
	and	a,d
        and	a,e
        and	a,h
        and	a,l
        and	a,ixl
        and	a,ixu
        and	a,iyl
        and	a,iyu
        and	a,$55
        and	a,(hl)
        and	a,(ix+20)
        and	a,(iy-300)
        and	a,(ix+100000)

        andw	ix
        andw	hl,ix
        andw	hl,iy
        andw	hl,bc
        andw	hl,de
        andw	hl,hl
        andw	hl,(ix+5)
        andw	hl,(iy-200)
	andw	hl,55aah

        cp	a
        cp	a,b
        cp	a,c
	cp	a,d
        cp	a,e
        cp	a,h
        cp	a,l
        cp	a,ixl
        cp	a,ixu
        cp	a,iyl
        cp	a,iyu
        cp	a,$34
        cp	a,(hl)
        cp	a,(ix-20)
        cp	a,(iy+$300)
        cp	a,(ix+100000h)

        cpw	ix
        cpw	hl,ix
        cpw	hl,iy
        cpw	hl,bc
        cpw	hl,de
        cpw	hl,hl
        cpw	hl,(ix+17)
        cpw	hl,(iy-200)
	cpw	hl,$aa55

        or	a
        or	a,b
        or	a,c
	or	a,d
        or	a,e
        or	a,h
        or	a,l
        or	a,ixl
        or	a,ixu
        or	a,iyl
        or	a,iyu
        or	a,$34
        or	a,(hl)
        or	a,(ix-20)
        or	a,(iy+$300)
        or	a,(ix+100000h)

        orw	ix
        orw	hl,ix
        orw	hl,iy
        orw	hl,bc
        orw	hl,de
        orw	hl,hl
        orw	hl,(ix+17)
        orw	hl,(iy-200)
	orw	hl,$aa55

        xor	a
        xor	a,b
        xor	a,c
	xor	a,d
        xor	a,e
        xor	a,h
        xor	a,l
        xor	a,ixl
        xor	a,ixu
        xor	a,iyl
        xor	a,iyu
        xor	a,$34
        xor	a,(hl)
        xor	a,(ix-20)
        xor	a,(iy+$300)
        xor	a,(ix+100000h)

        xorw	ix
        xorw	hl,ix
        xorw	hl,iy
        xorw	hl,bc
        xorw	hl,de
        xorw	hl,hl
        xorw	hl,(ix+17)
        xorw	hl,(iy-200)
	xorw	hl,$aa55

        sub	a
        sub	a,b
        sub	a,c
	sub	a,d
        sub	a,e
        sub	a,h
        sub	a,l
        sub	a,ixl
        sub	a,ixu
        sub	a,iyl
        sub	a,iyu
        sub	a,$34
        sub	a,(hl)
        sub	a,(ix-20)
        sub	a,(iy+$300)
        sub	a,(ix+100000h)

        sub	hl,(1234h)
        sub	hl,(123456h)
        sub	hl,(12345678h)
        sub	sp,3412o

	subw	ix
        subw	hl,ix
        subw	hl,iy
        subw	hl,bc
        subw	hl,de
        subw	hl,hl
        subw	hl,(ix+17)
	subw	hl,(iy-200)
	subw	hl,$aa55

	add	a,b
        add	a,iyu
        add	a,' '
        add	a,(hl)
	add	a,(ix+10)
        add	a,(ix+1000)
	add	hl,bc
	add	ix,de
	add	iy,iy
	add	ix,sp
	add	hl,(12345678h)
	add	sp,3412o

	addw	bc
	addw	hl,hl
	addw	hl,iy
	addw	hl,2314h
	addw	hl,(ix+128)

	adc	a,h
	adc	a,ixu
	adc     a,20
	adc	a,(hl)
	adc	a,(ix-500)
	adc	hl,sp

	adcw	hl,bc
	adcw	hl,iy
	adcw	hl,$abcd
	adcw	hl,(iy-30)

	sbc	a,d
	sbc	a,iyl
	sbc     a,20h
	sbc	a,(hl)
	sbc	a,(ix+500)
	sbc	hl,sp

	sbcw	hl,bc
	sbcw	hl,iy
	sbcw	hl,$abcd
	sbcw	hl,(iy-30)

	dec	a
	dec	(hl)
	dec	ixu
	dec	(ix+35)

	decw	de
	dec	iy

	inc	a
	inc	(hl)
	inc	ixu
	inc	(ix+35)

	incw	de
	inc	iy

	rl	d
	rl	(hl)
	rl	(ix+200)
	rlw	ix
	rlw	iy
	rlw	de
	rlw	hl
	rlw	(hl)
	rlw	(iy+$100000)

	rlc	d
	rlc	(hl)
	rlc	(ix+200)
	rlcw	ix
	rlcw	iy
	rlcw	de
	rlcw	hl
	rlcw	(hl)
	rlcw	(iy+$100000)

	rr	d
	rr	(hl)
	rr	(ix+200)
	rrw	ix
	rrw	iy
	rrw	de
	rrw	hl
	rrw	(hl)
	rrw	(iy+$100000)

	rrc	d
	rrc	(hl)
	rrc	(ix+200)
	rrcw	ix
	rrcw	iy
	rrcw	de
	rrcw	hl
	rrcw	(hl)
	rrcw	(iy+$100000)

	sla	d
	sla	(hl)
	sla	(ix+200)
	slaw	ix
	slaw	iy
	slaw	de
	slaw	hl
	slaw	(hl)
	slaw	(iy+$100000)

	sra	d
	sra	(hl)
	sra	(ix+200)
	sraw	ix
	sraw	iy
	sraw	de
	sraw	hl
	sraw	(hl)
	sraw	(iy+$100000)

	srl	d
	srl	(hl)
	srl	(ix+200)
	srlw	ix
	srlw	iy
	srlw	de
	srlw	hl
	srlw	(hl)
	srlw	(iy+$100000)

	bit	5,a
	bit	6,(hl)
	bit	3,(ix+67)

	res	5,a
	res	6,(hl)
	res	3,(ix+67)

	set	5,a
	set	6,(hl)
	set	3,(ix+67)

	mlt	bc
	mlt	hl
	mlt	sp

	ld	a,c
	ld	a,h
	ld	a,iyu
	ld      a,ixl
	ld	a,(hl)
	ld	a,(ix+20)
	ld	a,(iy-300)
	ld	a,(bc)
	ld	a,(de)
	ld	a,'A'
	ld	a,(2000h)
	ld	a,(10000h)
	ld	a,r
	ld	a,i
	ld	d,a
	ld	d,e
	ld	d,ixl
	ld	d,(hl)
	ld	d,(iy+15)
	ld	d,'D'
	ld	ixl,a
	ld	iyu,'I'
	ld	iyl,iyu
	ld	ixu,ixl
	ld	ixl,e
	ld	(hl),a
	ld	(hl),c
	ld	(ix+100),a
	ld	(iy-200),d
	ld	(hl),'H'
	ld	(ix),'X'
	ld	(hl),hl
	ld	(hl),de
	ld	(hl),bc
	ld	(hl),ix
	ld	(hl),iy
	ld	(ix),hl
	ld	(ix),de
	ld	(ix),bc
	ld	(iy),hl
	ld	(iy),de
	ld	(iy),bc
	ld	(iy),ix
	ld	(ix+123456h),iy
	ld	sp,hl
	ld	sp,iy
	ddir	lw
	ld	sp,123456h
	ld	sp,(6)
	ld	bc,(hl)
	ld	de,(hl)
	ld	hl,(hl)
	ld	bc,(ix)
	ld	de,(ix)
	ld	hl,(ix)
	ld	bc,(iy)
	ld	de,(iy)
	ld	hl,(iy)
	ld	bc,hl
	ld	de,bc
	ld	de,ix
	ld	hl,iy
	ld	de,(bc)
	ld	hl,(de)
	ld	hl,2000h
	ddir	lw
	ld	hl,12345687h
	ld	hl,(2000h)
	ld	de,(20000h)
	ld	hl,(sp+5)
	ld	de,(sp-200)
	ld	ix,(hl)
	ld	iy,(hl)
	ld	ix,(iy)
	ld	iy,(ix)
	ld	iy,hl
	ld	ix,bc
	ld	ix,iy
	ld	iy,ix
	ld	ix,(bc)
	ld	iy,(de)
	ddir	lw
	ld	ix,123456h
	ld	iy,0
	ld	ix,(2000h)
	ld	iy,(87654321h)
	ld	ix,(sp)
	ld	(bc),a
	ld	(de),a
	ld	(bc),de
	ld	(de),hl
	ld	(de),iy
	ld	($20001),a
	ld	(123456h),hl
	ld	(123456h),ix
	ld	(123456h),de
        ld      (123456h),sp
        ld      i,a
	ld	i,hl
	ld	r,a
	ld	hl,i
	ld	(sp),de
	ld	(sp),ix
	ld	(hl),10
	ldw	(hl),1000
	ddir	lw
	ldw	(hl),100000
	ldw	(bc),30
	ldw	(de),40

	pop	af
	pop	sr
	pop	bc
	pop	de
	pop	hl
	pop	ix
	pop	iy
	push	af
	push	sr
	push	300
	push	bc
	push	de
	push	hl
	push	ix
	push	iy

	; The AF' syntax for the alternate register bank is
        ; really nasty.  This requires a quotation qualifier
        ; for the parser:

	ex	af,af'		; will comment be recognized?
	ex	af',af		; will comment be recognized?

	ex	(sp),hl
	ex	hl,(sp)
	ex	(sp),ix
	ex	ix,(sp)
	ex	(sp),iy
	ex	iy,(sp)
	ex	de,hl
	ex	hl,de
	ex	a,a'
	ex	c,c'
	ex	a,h
	ex	d,a
	ex	a,(hl)
	ex	(hl),a
	ex	bc,de
	ex	bc,hl
	ex	bc,ix
	ex	bc,iy
	ex	de,bc
	ex	de,hl
	ex	de,ix
	ex	de,iy
	ex	hl,bc
	ex	hl,de
	ex	hl,ix
	ex	hl,iy
	ex	ix,bc
	ex	ix,de
	ex	ix,hl
	ex	ix,iy
	ex	iy,bc
	ex	iy,de
	ex	iy,hl
	ex	iy,ix
	ex	bc,bc'
	ex	de,de'
	ex	hl,hl'
	ex	ix,ix'
	ex	iy,iy'

	im	0
	im	1
	im	2
	im	3

	in	a,(12h)
	out	(12h),a
	in	c,(c)
	out	(c),c
	out	(c),12h

	inw	bc,(c)
	outw	(c),bc
	inw	de,(c)
	outw	(c),de
	inw	hl,(c)
	outw	(c),hl
	outw	(c),$2002

	in0	d,(20h)
	in0	(20h)
	out0	(20h),e

	ina	a,(12h)
	inaw	hl,(1234h)
	outa	(123456h),a
	outaw	(12345678h),hl

	tstio	1<<7

	tst	a
	tst	(hl)
	tst	33h

	divuw	(ix+5)
	multw	hl,(iy-3)
	multuw	hl,(iy+100)
	divuw	hl,bc
	multw	hl,de
	multuw	hl,hl
	divuw	hl,ix
	multw	hl,iy
	multuw	hl,ix
	divuw	hl,10
	multw	hl,100
	multuw	hl,1000

        ldctl	sr,a
        ldctl	xsr,a
        ldctl	a,xsr
        ldctl	dsr,a
        ldctl	a,dsr
        ldctl	ysr,a
        ldctl	a,ysr
        ldctl	sr,20h
        ldctl	xsr,31h
        ldctl	dsr,42h
        ldctl	ysr,53h
        ldctl	sr,hl
        ldctl	hl,sr

        resc	lw
        setc	lw
        resc	lck
        setc	lck
        setc	xm

        swap	bc
        swap	de
        swap	hl
        swap	ix
        swap	iy

        out     (c),0

        cpu     z80undoc

        slia    d
        slia    (ix+5)
        slia    (hl)
        slia    a
        inc     ixl
        inc     iyu
        dec     ixu
        dec     iyl
        ld      iyl,'a'
        ld      b,ixl
        ld      ixu,c
        ld      iyl,iyu
        add     a,ixl
        adc     a,ixu
        sub     a,iyl
        sbc     a,iyu
        and     a,ixl
        xor     a,ixu
        or      a,iyl
        cp      a,iyu
        rlc     (ix+3)
        rrc     b,(iy-3)	; extra dest reg may be first or last arg
	rrc	(iy-3),b
        slia    a,(ix-100)	; several namings in use (no standard from Zilog!)
	sli	a,(ix-100)
	sl1	a,(ix-100)
	sls	a,(ix-100)
        res     5,h
        set     6,(ix+6)
        bit     3,(hl)
        res     c,4,(ix-1)	; extra dest reg may be first or last arg
	res	4,(ix-1),c
        set     l,6,(iy+17)
	set	6,(iy+17),l
        out     (c),0
        in      (c)
        tsti
