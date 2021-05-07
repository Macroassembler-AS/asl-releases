	page	0
	cpu 	8008

	z80syntax exclusive

;-----------------------------------
; CPU Control Group

	halt
	expect	1506
	hlt
	endexpect

;-----------------------------------
; Input and Output Group

	expect	1506
	inp	5
	endexpect
	in	a,5

	expect	1110
	out	13h
	endexpect
	out	13h,a

;-----------------------------------
; Jump Group

	; in exclusive Z80 mode, JP with one
	; argument is interpreted as unconditional jump

	expect	1506
	jmp	1234h
	endexpect
	jp	1234h

	expect	1506
	jfc	1234h
	endexpect
	jp	nc,1234h
	expect	1506
	jfz	1234h
	endexpect
	jp	nz,1234h
	expect	1506
	jfs	1234h
	endexpect
	jp	p,1234h
	expect	1506
	jfp	1234h
	endexpect
	jp	po,1234h
	expect	1506
	jc	1234h
	endexpect
	jp	c,1234h
	expect	1506
	jz	1234h
	endexpect
	jp	z,1234h
	expect	1506
	js	1234h
	endexpect
	jp	m,1234h
	jp	1234h
	jp	pe,1234h

;-----------------------------------
; Call and Return Group

	expect	1506
        cal     1234h
	endexpect
	call	1234h

	expect	1506
        cfc     1234h
	endexpect
	call	nc,1234h
	expect	1506
        cfz     1234h
	endexpect
	call	nz,1234h
	expect	1506
        cfs     1234h
	endexpect
	call	p,1234h
	expect	1506
        cfp     1234h
	endexpect
	call	po,1234h
	expect	1506
        cc      1234h
	endexpect
	call	c,1234h
	expect	1506
        cz      1234h
	endexpect
	call	z,1234h
	expect	1506
        cs      1234h
	endexpect
	call	m,1234h
	expect	1320		; is treated as compare -> 8 bit value overflow
        cp      1234h
	endexpect
	call	p,1234h

	ret

	expect	1506
        rfc
	endexpect
	ret	nc
	expect	1506
        rfz
	endexpect
	ret	nz
	expect	1506
        rfs
	endexpect
	ret	p
	expect	1506
        rfp
	endexpect
	ret	po
	expect	1506
        rc
	endexpect
	ret	c
	expect	1506
        rz
	endexpect
	ret	z
	expect	1506
        rs
	endexpect
	ret	m
	expect	1506
        rp
	endexpect
	ret	pe

	rst	38h

;-----------------------------------
; Load Group

	expect	1506
	laa
	endexpect
	ld	a,a
	expect	1506
	lab
	endexpect
	ld	a,b
	expect	1506
	lac
	endexpect
	ld	a,c
	expect	1506
	lad
	endexpect
	ld	a,d
	expect	1506
	lae
	endexpect
	ld	a,e
	expect	1506
	lah
	endexpect
	ld	a,h
	expect	1506
	lal
	endexpect
	ld	a,l
	expect	1506
	lba
	endexpect
	ld	b,a
	expect	1506
	lbb
	endexpect
	ld	b,b
	expect	1506
	lbc
	endexpect
	ld	b,c
	expect	1506
	lbd
	endexpect
	ld	b,d
	expect	1506
	lbe
	endexpect
	ld	b,e
	expect	1506
	lbh
	endexpect
	ld	b,h
	expect	1506
	lbl
	endexpect
	ld	b,l
	expect	1506
	lca
	endexpect
	ld	c,a
	expect	1506
	lcb
	endexpect
	ld	c,b
	expect	1506
	lcc
	endexpect
	ld	c,c
	expect	1506
	lcd
	endexpect
	ld	c,d
	expect	1506
	lce
	endexpect
	ld	c,e
	expect	1506
	lch
	endexpect
	ld	c,h
	expect	1506
	lcl
	endexpect
	ld	c,l
	expect	1506
	lda
	endexpect
	ld	d,a
	expect	1506
	ldb
	endexpect
	ld	d,b
	expect	1506
	ldc
	endexpect
	ld	d,c
	expect	1506
	ldd
	endexpect
	ld	d,d
	expect	1506
	lde
	endexpect
	ld	d,e
	expect	1506
	ldh
	endexpect
	ld	d,h
	expect	1506
	ldl
	endexpect
	ld	d,l
	expect	1506
	lea
	endexpect
	ld	e,a
	expect	1506
	leb
	endexpect
	ld	e,b
	expect	1506
	lec
	endexpect
	ld	e,c
	expect	1506
	led
	endexpect
	ld	e,d
	expect	1506
	lee
	endexpect
	ld	e,e
	expect	1506
	leh
	endexpect
	ld	e,h
	expect	1506
	lel
	endexpect
	ld	e,l
	expect	1506
	lha
	endexpect
	ld	h,a
	expect	1506
	lhb
	endexpect
	ld	h,b
	expect	1506
	lhc
	endexpect
	ld	h,c
	expect	1506
	lhd
	endexpect
	ld	h,d
	expect	1506
	lhe
	endexpect
	ld	h,e
	expect	1506
	lhh
	endexpect
	ld	h,h
	expect	1506
	lhl
	endexpect
	ld	h,l
	expect	1506
	lla
	endexpect
	ld	l,a
	expect	1506
	llb
	endexpect
	ld	l,b
	expect	1506
	llc
	endexpect
	ld	l,c
	expect	1506
	lld
	endexpect
	ld	l,d
	expect	1506
	lle
	endexpect
	ld	l,e
	expect	1506
	llh
	endexpect
	ld	l,h
	expect	1506
	lll
	endexpect
	ld	l,l

	expect	1506
	lam
	endexpect
	ld	a,(hl)
	expect	1506
	lbm
	endexpect
	ld	b,(hl)
	expect	1506
	lcm
	endexpect
	ld	c,(hl)
	expect	1506
	ldm
	endexpect
	ld	d,(hl)
	expect	1506
	lem
	endexpect
	ld	e,(hl)
	expect	1506
	lhm
	endexpect
	ld	h,(hl)
	expect	1506
	llm
	endexpect
	ld	l,(hl)

	expect	1506
	lma
	endexpect
	ld	(hl),a
	expect	1506
	lmb
	endexpect
	ld	(hl),b
	expect	1506
	lmc
	endexpect
	ld	(hl),c
	expect	1506
	lmd
	endexpect
	ld	(hl),d
	expect	1506
	lme
	endexpect
	ld	(hl),e
	expect	1506
	lmh
	endexpect
	ld	(hl),h
	expect	1506
	lml
	endexpect
	ld	(hl),l

	expect	1506
	lai	55h
	endexpect
	ld	a,55h
	expect	1506
	lbi	55h
	endexpect
	ld	b,55h
	expect	1506
	lci	55h
	endexpect
	ld	c,55h
	expect	1506
	ldi	55h
	endexpect
	ld	d,55h
	expect	1506
	lei	55h
	endexpect
	ld	e,55h
	expect	1506
	lhi	55h
	endexpect
	ld	h,55h
	expect	1506
	lli	55h
	endexpect
	ld	l,55h

	expect	1506
	lmi	55h
	endexpect
	ld	(hl),55h

;-----------------------------------
; Arithmetic Group

	expect	1506
	ada
	endexpect
	add	a,a
	expect	1506
	adb
	endexpect
	add	a,b
	expect	1110
	adc
	endexpect
	add	a,c
	expect	1110
	add
	endexpect
	add	a,d
	expect	1506
	ade
	endexpect
	add	a,e
	expect	1506
	adh
	endexpect
	add	a,h
	expect	1506
	adl
	endexpect
	add	a,l
	expect	1506
	adm
	endexpect
	add	a,(hl)
	expect	1506
	adi	55h
	endexpect
	add	a,55h

	expect	1506
	aca
	endexpect
	adc	a,a
	expect	1506
	acb
	endexpect
	adc	a,b
	expect	1506
	acc
	endexpect
	adc	a,c
	expect	1506
	acd
	endexpect
	adc	a,d
	expect	1506
	ace
	endexpect
	adc	a,e
	expect	1506
	ach
	endexpect
	adc	a,h
	expect	1506
	acl
	endexpect
	adc	a,l
	expect	1506
	acm
	endexpect
	adc	a,(hl)
	expect	1506
	aci	55h
	endexpect
	adc	a,55h

	expect	1506
	sua
	endexpect
	sub	a,a
	expect	1110
	sub
	endexpect
	sub	a,b
	expect	1506
	suc
	endexpect
	sub	a,c
	expect	1506
	sud
	endexpect
	sub	a,d
	expect	1506
	sue
	endexpect
	sub	a,e
	expect	1506
	suh
	endexpect
	sub	a,h
	expect	1506
	sul
	endexpect
	sub	a,l
	expect	1506
	sum
	endexpect
	sub	a,(hl)
	expect	1506
	sui	55h
	endexpect
	sub	a,55h

	expect	1506
	sba
	endexpect
	sbc	a,a
	expect	1506
	sbb
	endexpect
	sbc	a,b
	expect	1110
	sbc
	endexpect
	sbc	a,c
	expect	1506
	sbd
	endexpect
	sbc	a,d
	expect	1506
	sbe
	endexpect
	sbc	a,e
	expect	1506
	sbh
	endexpect
	sbc	a,h
	expect	1506
	sbl
	endexpect
	sbc	a,l
	expect	1506
	sbm
	endexpect
	sbc	a,(hl)
	expect	1506
	sbi	55h
	endexpect
	sbc	a,55h

	expect	1506
	nda
	endexpect
	and	a
	expect	1506
	ndb
	endexpect
	and	b
	expect	1506
	ndc
	endexpect
	and	c
	expect	1506
	ndd
	endexpect
	and	d
	expect	1506
	nde
	endexpect
	and	e
	expect	1506
	ndh
	endexpect
	and	h
	expect	1506
	ndl
	endexpect
	and	l
	expect	1506
	ndm
	endexpect
	and	(hl)
	expect	1506
	ndi	55h
	endexpect
	and	55h

	expect	1506
	xra
	endexpect
	xor	a
	expect	1506
	xrb
	endexpect
	xor	b
	expect	1506
	xrc
	endexpect
	xor	c
	expect	1506
	xrd
	endexpect
	xor	d
	expect	1506
	xre
	endexpect
	xor	e
	expect	1506
	xrh
	endexpect
	xor	h
	expect	1506
	xrl
	endexpect
	xor	l
	expect	1506
	xrm
	endexpect
	xor	(hl)
	expect	1506
	xri	55h
	endexpect
	xor	55h

	expect	1506
	ora
	endexpect
	or	a
	expect	1506
	orb
	endexpect
	or	b
	expect	1506
	orc
	endexpect
	or	c
	expect	1506
	ord
	endexpect
	or	d
	expect	1506
	ore
	endexpect
	or	e
	expect	1506
	orh
	endexpect
	or	h
	expect	1506
	orl
	endexpect
	or	l
	expect	1506
	orm
	endexpect
	or	(hl)
	expect	1506
	ori	55h
	endexpect
	or	55h

	expect	1506
	cpa
	endexpect
	cp	a
	expect	1506
	cpb
	endexpect
	cp	b
	expect	1506
	cpc
	endexpect
	cp	c
	expect	1506
	cpd
	endexpect
	cp	d
	expect	1506
	cpe
	endexpect
	cp	e
	expect	1506
	cph
	endexpect
	cp	h
	expect	1506
	cpl
	endexpect
	cp	l
	expect	1506
	cpm
	endexpect
	cp	(hl)
	expect	1506
	cpi	55h
	endexpect
	cp	55h

	expect	1506
	inb
	endexpect
	inc	b
	expect	1110
	inc
	endexpect
	inc	c
	expect	1506
	ind
	endexpect
	inc	d
	expect	1506
	ine
	endexpect
	inc	e
	expect	1506
	inh
	endexpect
	inc	h
	expect	1506
	inl
	endexpect
	inc	l

	expect	1506
	dcb
	endexpect
	dec	b
	expect	1506
	dcc
	endexpect
	dec	c
	expect	1506
	dcd
	endexpect
	dec	d
	expect	1506
	dce
	endexpect
	dec	e
	expect	1506
	dch
	endexpect
	dec	h
	expect	1506
	dcl
	endexpect
	dec	l

;-----------------------------------
; Rotate Group

	rlca
	rrca
	rla
	rra
	expect	1506,1506,1506,1506
        rlc
        rrc
        ral
        rar
	endexpect

;===================================
; we repeat only the instructions different in new 8008 syntax

	cpu	8008new

	z80syntax exclusive

;-----------------------------------
; Input and Output Group

	expect	1110
	in	5
	endexpect
	in	a,5

;-----------------------------------
; Jump Group

	; in exclusive Z80 mode, JP with one
	; argument is interpreted as unconditional jump

	expect	1506
	jmp	1234h
	endexpect
	jp	1234h

	expect	1506
	jnc	1234h
	endexpect
	jp	nc,1234h
	expect	1506
	jnz	1234h
	endexpect
	jp	nz,1234h
	jp	1234h
	jp	p,1234h
	expect	1506
	jpo	1234h
	endexpect
	jp	po,1234h
	expect	1506
	jc	1234h
	endexpect
	jp	c,1234h
	expect	1506
	jz	1234h
	endexpect
	jp	z,1234h
	expect	1506
	jm	1234h
	endexpect
	jp	m,1234h
	expect	1506
	jpe	1234h
	endexpect
	jp	pe,1234h

;-----------------------------------
; Call and Return Group

	call	1234h

	expect	1506
        cnc     1234h
	endexpect
	call	nc,1234h
	expect	1506
        cnz     1234h
	endexpect
	call	nz,1234h
	expect	1320		; is treated as compare in pure Z80 mode -> 8 bit range overflow
	cp      1234h
	endexpect
	call	p,1234h
	expect	1506
        cpo     1234h
	endexpect
	call	po,1234h
	expect	1506
        cc      1234h
	endexpect
	call	c,1234h
	expect	1506
        cz      1234h
	endexpect
	call	z,1234h
	expect	1506
        cm      1234h
	endexpect
	call	m,1234h
	expect	1506
        cpe	1234h
	endexpect
	call	pe,1234h

	ret

	expect	1506
        rnc
	endexpect
	ret	nc
	expect	1506
        rnz
	endexpect
	ret	nz
	expect	1506
	rp
	endexpect
	ret	p
	expect	1506
        rpo
	endexpect
	ret	po
	expect	1506
        rc
	endexpect
	ret	c
	expect	1506
        rz
	endexpect
	ret	z
	expect	1506
        rm
	endexpect
	ret	m
	expect	1506
        rpe
	endexpect
	ret	pe

	rst	38h

;-----------------------------------
; Load Group

	expect	1506
	mov	a,a
	endexpect
	ld	a,a
	expect	1506
	mov	a,b
	endexpect
	ld	a,b
	expect	1506
	mov	a,c
	endexpect
	ld	a,c
	expect	1506
	mov	a,d
	endexpect
	ld	a,d
	expect	1506
	mov	a,e
	endexpect
	ld	a,e
	expect	1506
	mov	a,h
	endexpect
	ld	a,h
	expect	1506
	mov	a,l
	endexpect
	ld	a,l
	expect	1506
	mov	b,a
	endexpect
	ld	b,a
	expect	1506
	mov	b,b
	endexpect
	ld	b,b
	expect	1506
	mov	b,c
	endexpect
	ld	b,c
	expect	1506
	mov	b,d
	endexpect
	ld	b,d
	expect	1506
	mov	b,e
	endexpect
	ld	b,e
	expect	1506
	mov	b,h
	endexpect
	ld	b,h
	expect	1506
	mov	b,l
	endexpect
	ld	b,l
	expect	1506
	mov	c,a
	endexpect
	ld	c,a
	expect	1506
	mov	c,b
	endexpect
	ld	c,b
	expect	1506
	mov	c,c
	endexpect
	ld	c,c
	expect	1506
	mov	c,d
	endexpect
	ld	c,d
	expect	1506
	mov	c,e
	endexpect
	ld	c,e
	expect	1506
	mov	c,h
	endexpect
	ld	c,h
	expect	1506
	mov	c,l
	endexpect
	ld	c,l
	expect	1506
	mov	d,a
	endexpect
	ld	d,a
	expect	1506
	mov	d,b
	endexpect
	ld	d,b
	expect	1506
	mov	d,c
	endexpect
	ld	d,c
	expect	1506
	mov	d,d
	endexpect
	ld	d,d
	expect	1506
	mov	d,e
	endexpect
	ld	d,e
	expect	1506
	mov	d,h
	endexpect
	ld	d,h
	expect	1506
	mov	d,l
	endexpect
	ld	d,l
	expect	1506
	mov	e,a
	endexpect
	ld	e,a
	expect	1506
	mov	e,b
	endexpect
	ld	e,b
	expect	1506
	mov	e,c
	endexpect
	ld	e,c
	expect	1506
	mov	e,d
	endexpect
	ld	e,d
	expect	1506
	mov	e,e
	endexpect
	ld	e,e
	expect	1506
	mov	e,h
	endexpect
	ld	e,h
	expect	1506
	mov	e,l
	endexpect
	ld	e,l
	expect	1506
	mov	h,a
	endexpect
	ld	h,a
	expect	1506
	mov	h,b
	endexpect
	ld	h,b
	expect	1506
	mov	h,c
	endexpect
	ld	h,c
	expect	1506
	mov	h,d
	endexpect
	ld	h,d
	expect	1506
	mov	h,e
	endexpect
	ld	h,e
	expect	1506
	mov	h,h
	endexpect
	ld	h,h
	expect	1506
	mov	h,l
	endexpect
	ld	h,l
	expect	1506
	mov	l,a
	endexpect
	ld	l,a
	expect	1506
	mov	l,b
	endexpect
	ld	l,b
	expect	1506
	mov	l,c
	endexpect
	ld	l,c
	expect	1506
	mov	l,d
	endexpect
	ld	l,d
	expect	1506
	mov	l,e
	endexpect
	ld	l,e
	expect	1506
	mov	l,h
	endexpect
	ld	l,h
	expect	1506
	mov	l,l
	endexpect
	ld	l,l

	expect	1506
	mov	a,m
	endexpect
	ld	a,(hl)
	expect	1506
	mov	b,m
	endexpect
	ld	b,(hl)
	expect	1506
	mov	c,m
	endexpect
	ld	c,(hl)
	expect	1506
	mov	d,m
	endexpect
	ld	d,(hl)
	expect	1506
	mov	e,m
	endexpect
	ld	e,(hl)
	expect	1506
	mov	h,m
	endexpect
	ld	h,(hl)
	expect	1506
	mov	l,m
	endexpect
	ld	l,(hl)

	expect	1506
	mov	m,a
	endexpect
	ld	(hl),a
	expect	1506
	mov	m,b
	endexpect
	ld	(hl),b
	expect	1506
	mov	m,c
	endexpect
	ld	(hl),c
	expect	1506
	mov	m,d
	endexpect
	ld	(hl),d
	expect	1506
	mov	m,e
	endexpect
	ld	(hl),e
	expect	1506
	mov	m,h
	endexpect
	ld	(hl),h
	expect	1506
	mov	m,l
	endexpect
	ld	(hl),l

	expect	1506
	mvi	a,55h
	endexpect
	ld	a,55h
	expect	1506
	mvi	b,55h
	endexpect
	ld	b,55h
	expect	1506
	mvi	c,55h
	endexpect
	ld	c,55h
	expect	1506
	mvi	d,55h
	endexpect
	ld	d,55h
	expect	1506
	mvi	e,55h
	endexpect
	ld	e,55h
	expect	1506
	mvi	h,55h
	endexpect
	ld	h,55h
	expect	1506
	mvi	l,55h
	endexpect
	ld	l,55h

	expect	1506
	mvi	m,55h
	endexpect
	ld	(hl),55h

	expect	1506
	lxi	b,1234h		; convenience built-in macro:
	endexpect
	ld	bc,1234h
	expect	1506
	lxi	d,1234h		; LXI is assembled as 2 x MVI
	endexpect
	ld	de,1234h
	expect	1506
	lxi	h,1234h
	endexpect
	ld	hl,1234h

;-----------------------------------
; Arithmetic Group

	expect	1110
	add	a
	endexpect
	add	a,a
	expect	1110
	add	b
	endexpect
	add	a,b
	expect	1110
	add	c
	endexpect
	add	a,c
	expect	1110
	add	d
	endexpect
	add	a,d
	expect	1110
	add	e
	endexpect
	add	a,e
	expect	1110
	add	h
	endexpect
	add	a,h
	expect	1110
	add	l
	endexpect
	add	a,l
	expect	1110
	add	m
	endexpect
	add	a,(hl)
	expect	1506
	adi	55h
	endexpect
	add	a,55h

	expect	1110
	adc	a
	endexpect
	adc	a,a
	expect	1110
	adc	b
	endexpect
	adc	a,b
	expect	1110
	adc	c
	endexpect
	adc	a,c
	expect	1110
	adc	d
	endexpect
	adc	a,d
	expect	1110
	adc	e
	endexpect
	adc	a,e
	expect	1110
	adc	h
	endexpect
	adc	a,h
	expect	1110
	adc	l
	endexpect
	adc	a,l
	expect	1110
	adc	m
	endexpect
	adc	a,(hl)
	expect	1506
	aci	55h
	endexpect
	adc	a,55h

	; note that A as destination is optional in Z80 syntax,
	; since Z80 cannot use anything else as dest for SUB.
	; However, M as source or SUI remains unallowed in
        ; pure Z80 mode.  The 'unknown symbol' error for M will
        ; not occur in pass 1:

	sub	a
	sub	a,a
	sub	b
	sub	a,b
	sub	c
	sub	a,c
	sub	d
	sub	a,d
	sub	e
	sub	a,e
	sub	h
	sub	a,h
	sub	l
	sub	a,l
	if	mompass>1
	expect	1010
	endif
	sub	m
	if	mompass>1
	endexpect
	endif
	sub	(hl)
	sub	a,(hl)
	expect	1506
	sui	55h
	endexpect
	sub	a,55h

	expect	1506
	sbb	a
	endexpect
	sbc	a,a
	expect	1506
	sbb	b
	endexpect
	sbc	a,b
	expect	1506
	sbb	c
	endexpect
	sbc	a,c
	expect	1506
	sbb	d
	endexpect
	sbc	a,d
	expect	1506
	sbb	e
	endexpect
	sbc	a,e
	expect	1506
	sbb	h
	endexpect
	sbc	a,h
	expect	1506
	sbb	l
	endexpect
	sbc	a,l
	expect	1506
	sbb	m
	endexpect
	sbc	a,(hl)
	expect	1506
	sbi	55h
	endexpect
	sbc	a,55h

	expect	1506
	ana	a
	endexpect
	and	a,a
	expect	1506
	ana	b
	endexpect
	and	a,b
	expect	1506
	ana	c
	endexpect
	and	a,c
	expect	1506
	ana	d
	endexpect
	and	a,d
	expect	1506
	ana	e
	endexpect
	and	a,e
	expect	1506
	ana	h
	endexpect
	and	a,h
	expect	1506
	ana	l
	endexpect
	and	a,l
	expect	1506
	ana	m
	endexpect
	and	a,(hl)
	expect	1506
	ani	55h
	endexpect
	and	a,55h

	expect	1506
	xra	a
	endexpect
	xor	a,a
	expect	1506
	xra	b
	endexpect
	xor	a,b
	expect	1506
	xra	c
	endexpect
	xor	a,c
	expect	1506
	xra	d
	endexpect
	xor	a,d
	expect	1506
	xra	e
	endexpect
	xor	a,e
	expect	1506
	xra	h
	endexpect
	xor	a,h
	expect	1506
	xra	l
	endexpect
	xor	a,l
	expect	1506
	xra	m
	endexpect
	xor	a,(hl)
	expect	1506
	xri	55h
	endexpect
	xor	a,55h

	expect	1506
	ora	a
	endexpect
	or	a,a
	expect	1506
	ora	b
	endexpect
	or	a,b
	expect	1506
	ora	c
	endexpect
	or	a,c
	expect	1506
	ora	d
	endexpect
	or	a,d
	expect	1506
	ora	e
	endexpect
	or	a,e
	expect	1506
	ora	h
	endexpect
	or	a,h
	expect	1506
	ora	l
	endexpect
	or	a,l
	expect	1506
	ora	m
	endexpect
	or	a,(hl)
	expect	1506
	ori	55h
	endexpect
	or	a,55h

	expect	1506
	cmp	a
	endexpect
	cp	a,a
	expect	1506
	cmp	b
	endexpect
	cp	a,b
	expect	1506
	cmp	c
	endexpect
	cp	a,c
	expect	1506
	cmp	d
	endexpect
	cp	a,d
	expect	1506
	cmp	e
	endexpect
	cp	a,e
	expect	1506
	cmp	h
	endexpect
	cp	a,h
	expect	1506
	cmp	l
	endexpect
	cp	a,l
	expect	1506
	cmp	m
	endexpect
	cp	a,(hl)
	expect	1506
	cpi	55h
	endexpect
	cp	a,55h

	expect	1506
	inr	b
	endexpect
	inc	b
	expect	1506
	inr	c
	endexpect
	inc	c
	expect	1506
	inr	d
	endexpect
	inc	d
	expect	1506
	inr	e
	endexpect
	inc	e
	expect	1506
	inr	h
	endexpect
	inc	h
	expect	1506
	inr	l
	endexpect
	inc	l

	expect	1506
	dcr	b
	endexpect
	dec	b
	expect	1506
	dcr	c
	endexpect
	dec	c
	expect	1506
	dcr	d
	endexpect
	dec	d
	expect	1506
	dcr	e
	endexpect
	dec	e
	expect	1506
	dcr	h
	endexpect
	dec	h
	expect	1506
	dcr	l
	endexpect
	dec	l

