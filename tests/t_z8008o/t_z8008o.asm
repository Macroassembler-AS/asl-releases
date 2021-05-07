	page	0
	cpu 	8008

	z80syntax on

;-----------------------------------
; CPU Control Group

	halt
	hlt

;-----------------------------------
; Input and Output Group

	inp	5
	in	a,5

	out	13h
	out	13h,a

;-----------------------------------
; Jump Group

	; in non-exclusive Z80 mode, JP with one
	; argument is interpreted as original 8008 mnemonic
        ; (jump if parity even)

	jmp	1234h
	jp	1234h		; as below

	jfc	1234h
	jp	nc,1234h
	jfz	1234h
	jp	nz,1234h
	jfs	1234h
	jp	p,1234h
	jfp	1234h
	jp	po,1234h
	jc	1234h
	jp	c,1234h
	jz	1234h
	jp	z,1234h
	js	1234h
	jp	m,1234h
	jp	1234h
	jp	pe,1234h

;-----------------------------------
; Call and Return Group

	; in non-exclusive Z80 mode, CP with one
        ; argument is interpreted as original 8008 mnemonic
        ; (call if parity even)

        cal     1234h
	call	1234h

        cfc     1234h
	call	nc,1234h
        cfz     1234h
	call	nz,1234h
        cfs     1234h
	call	p,1234h
        cfp     1234h
	call	po,1234h
        cc      1234h
	call	c,1234h
        cz      1234h
	call	z,1234h
        cs      1234h
	call	m,1234h
        cp      1234h
	call	pe,1234h

	ret

        rfc
	ret	nc
        rfz
	ret	nz
        rfs
	ret	p
        rfp
	ret	po
        rc
	ret	c
        rz
	ret	z
        rs
	ret	m
        rp
	ret	pe

	rst	38h

;-----------------------------------
; Load Group

	laa
	ld	a,a
	lab
	ld	a,b
	lac
	ld	a,c
	lad
	ld	a,d
	lae
	ld	a,e
	lah
	ld	a,h
	lal
	ld	a,l
	lba
	ld	b,a
	lbb
	ld	b,b
	lbc
	ld	b,c
	lbd
	ld	b,d
	lbe
	ld	b,e
	lbh
	ld	b,h
	lbl
	ld	b,l
	lca
	ld	c,a
	lcb
	ld	c,b
	lcc
	ld	c,c
	lcd
	ld	c,d
	lce
	ld	c,e
	lch
	ld	c,h
	lcl
	ld	c,l
	lda
	ld	d,a
	ldb
	ld	d,b
	ldc
	ld	d,c
	ldd
	ld	d,d
	lde
	ld	d,e
	ldh
	ld	d,h
	ldl
	ld	d,l
	lea
	ld	e,a
	leb
	ld	e,b
	lec
	ld	e,c
	led
	ld	e,d
	lee
	ld	e,e
	leh
	ld	e,h
	lel
	ld	e,l
	lha
	ld	h,a
	lhb
	ld	h,b
	lhc
	ld	h,c
	lhd
	ld	h,d
	lhe
	ld	h,e
	lhh
	ld	h,h
	lhl
	ld	h,l
	lla
	ld	l,a
	llb
	ld	l,b
	llc
	ld	l,c
	lld
	ld	l,d
	lle
	ld	l,e
	llh
	ld	l,h
	lll
	ld	l,l

	lam
	ld	a,(hl)
	lbm
	ld	b,(hl)
	lcm
	ld	c,(hl)
	ldm
	ld	d,(hl)
	lem
	ld	e,(hl)
	lhm
	ld	h,(hl)
	llm
	ld	l,(hl)

	lma
	ld	(hl),a
	lmb
	ld	(hl),b
	lmc
	ld	(hl),c
	lmd
	ld	(hl),d
	lme
	ld	(hl),e
	lmh
	ld	(hl),h
	lml
	ld	(hl),l

	lai	55h
	ld	a,55h
	lbi	55h
	ld	b,55h
	lci	55h
	ld	c,55h
	ldi	55h
	ld	d,55h
	lei	55h
	ld	e,55h
	lhi	55h
	ld	h,55h
	lli	55h
	ld	l,55h

	lmi	55h
	ld	(hl),55h

;-----------------------------------
; Arithmetic Group

	ada
	add	a,a
	adb
	add	a,b
	adc
	add	a,c
	add
	add	a,d
	ade
	add	a,e
	adh
	add	a,h
	adl
	add	a,l
	adm
	add	a,(hl)
	adi	55h
	add	a,55h

	aca
	adc	a,a
	acb
	adc	a,b
	acc
	adc	a,c
	acd
	adc	a,d
	ace
	adc	a,e
	ach
	adc	a,h
	acl
	adc	a,l
	acm
	adc	a,(hl)
	aci	55h
	adc	a,55h

	sua
	sub	a,a
	sub
	sub	a,b
	suc
	sub	a,c
	sud
	sub	a,d
	sue
	sub	a,e
	suh
	sub	a,h
	sul
	sub	a,l
	sum
	sub	a,(hl)
	sui	55h
	sub	a,55h

	sba
	sbc	a,a
	sbb
	sbc	a,b
	sbc
	sbc	a,c
	sbd
	sbc	a,d
	sbe
	sbc	a,e
	sbh
	sbc	a,h
	sbl
	sbc	a,l
	sbm
	sbc	a,(hl)
	sbi	55h
	sbc	a,55h

	nda
	and	a
	ndb
	and	b
	ndc
	and	c
	ndd
	and	d
	nde
	and	e
	ndh
	and	h
	ndl
	and	l
	ndm
	and	(hl)
	ndi	55h
	and	55h

	xra
	xor	a
	xrb
	xor	b
	xrc
	xor	c
	xrd
	xor	d
	xre
	xor	e
	xrh
	xor	h
	xrl
	xor	l
	xrm
	xor	(hl)
	xri	55h
	xor	55h

	ora
	or	a
	orb
	or	b
	orc
	or	c
	ord
	or	d
	ore
	or	e
	orh
	or	h
	orl
	or	l
	orm
	or	(hl)
	ori	55h
	or	55h

	; in non-exclusive Z80 syntax, CP with one
        ; argument is call-if-parity-even

	cpa
	if	mompass>1
	expect	1010
	endif
	cp	a
	if	mompass>1
	endexpect
	endif
	cpb
	if	mompass>1
	expect	1010
	endif
	cp	b
	if	mompass>1
	endexpect
	endif
	cpc
	if	mompass>1
	expect	1010
	endif
	cp	c
	if	mompass>1
	endexpect
	endif
	cpd
	if	mompass>1
	expect	1010
	endif
	cp	d
	if	mompass>1
	endexpect
	endif
	cpe
	if	mompass>1
	expect	1010
	endif
	cp	e
	if	mompass>1
	endexpect
	endif
	cph
	if	mompass>1
	expect	1010
	endif
	cp	h
	if	mompass>1
	endexpect
	endif
	cpl
	if	mompass>1
	expect	1010
	endif
	cp	l
	if	mompass>1
	endexpect
	endif
	cpm
	if	mompass>1
	expect	1010
	endif
	cp	(hl)
	if	mompass>1
	endexpect
	endif
	cpi	55h
	cp	55h		; -> call if parity even in non-exclusive mode

	inb
	inc	b
	inc
	inc	c
	ind
	inc	d
	ine
	inc	e
	inh
	inc	h
	inl
	inc	l

	dcb
	dec	b
	dcc
	dec	c
	dcd
	dec	d
	dce
	dec	e
	dch
	dec	h
	dcl
	dec	l

;-----------------------------------
; Rotate Group

        rlc
	rlca
        rrc
	rrca
        ral
	rla
        rar
	rra

;===================================
; we repeat only the instructions different in new 8008 syntax

	cpu	8008new

	z80syntax on

;-----------------------------------
; Input and Output Group

	in	5
	in	a,5

;-----------------------------------
; Jump Group

	; in non-exclusive Z80 mode, JP with one
	; argument is interpreted as original 8008
        ; mnemonic (jump if positive)

	jmp	1234h
	jp	1234h

	jnc	1234h
	jp	nc,1234h
	jnz	1234h
	jp	nz,1234h
	jp	1234h
	jp	p,1234h
	jpo	1234h
	jp	po,1234h
	jc	1234h
	jp	c,1234h
	jz	1234h
	jp	z,1234h
	jm	1234h
	jp	m,1234h
	jpe	1234h
	jp	pe,1234h

;-----------------------------------
; Call and Return Group

	call	1234h

        cnc     1234h
	call	nc,1234h
        cnz     1234h
	call	nz,1234h
	cp      1234h
	call	p,1234h
        cpo     1234h
	call	po,1234h
        cc      1234h
	call	c,1234h
        cz      1234h
	call	z,1234h
        cm      1234h
	call	m,1234h
        cpe	1234h
	call	pe,1234h

	ret

        rnc
	ret	nc
        rnz
	ret	nz
	rp
	ret	p
        rpo
	ret	po
        rc
	ret	c
        rz
	ret	z
        rm
	ret	m
        rpe
	ret	pe

	rst	38h

;-----------------------------------
; Load Group

	mov	a,a
	ld	a,a
	mov	a,b
	ld	a,b
	mov	a,c
	ld	a,c
	mov	a,d
	ld	a,d
	mov	a,e
	ld	a,e
	mov	a,h
	ld	a,h
	mov	a,l
	ld	a,l
	mov	b,a
	ld	b,a
	mov	b,b
	ld	b,b
	mov	b,c
	ld	b,c
	mov	b,d
	ld	b,d
	mov	b,e
	ld	b,e
	mov	b,h
	ld	b,h
	mov	b,l
	ld	b,l
	mov	c,a
	ld	c,a
	mov	c,b
	ld	c,b
	mov	c,c
	ld	c,c
	mov	c,d
	ld	c,d
	mov	c,e
	ld	c,e
	mov	c,h
	ld	c,h
	mov	c,l
	ld	c,l
	mov	d,a
	ld	d,a
	mov	d,b
	ld	d,b
	mov	d,c
	ld	d,c
	mov	d,d
	ld	d,d
	mov	d,e
	ld	d,e
	mov	d,h
	ld	d,h
	mov	d,l
	ld	d,l
	mov	e,a
	ld	e,a
	mov	e,b
	ld	e,b
	mov	e,c
	ld	e,c
	mov	e,d
	ld	e,d
	mov	e,e
	ld	e,e
	mov	e,h
	ld	e,h
	mov	e,l
	ld	e,l
	mov	h,a
	ld	h,a
	mov	h,b
	ld	h,b
	mov	h,c
	ld	h,c
	mov	h,d
	ld	h,d
	mov	h,e
	ld	h,e
	mov	h,h
	ld	h,h
	mov	h,l
	ld	h,l
	mov	l,a
	ld	l,a
	mov	l,b
	ld	l,b
	mov	l,c
	ld	l,c
	mov	l,d
	ld	l,d
	mov	l,e
	ld	l,e
	mov	l,h
	ld	l,h
	mov	l,l
	ld	l,l

	mov	a,m
	ld	a,(hl)
	mov	b,m
	ld	b,(hl)
	mov	c,m
	ld	c,(hl)
	mov	d,m
	ld	d,(hl)
	mov	e,m
	ld	e,(hl)
	mov	h,m
	ld	h,(hl)
	mov	l,m
	ld	l,(hl)

	mov	m,a
	ld	(hl),a
	mov	m,b
	ld	(hl),b
	mov	m,c
	ld	(hl),c
	mov	m,d
	ld	(hl),d
	mov	m,e
	ld	(hl),e
	mov	m,h
	ld	(hl),h
	mov	m,l
	ld	(hl),l

	mvi	a,55h
	ld	a,55h
	mvi	b,55h
	ld	b,55h
	mvi	c,55h
	ld	c,55h
	mvi	d,55h
	ld	d,55h
	mvi	e,55h
	ld	e,55h
	mvi	h,55h
	ld	h,55h
	mvi	l,55h
	ld	l,55h

	mvi	m,55h
	ld	(hl),55h

	lxi	b,1234h		; convenience built-in macro:
	ld	bc,1234h
	lxi	d,1234h		; LXI is assembled as 2 x MVI
	ld	de,1234h
	lxi	h,1234h
	ld	hl,1234h

;-----------------------------------
; Arithmetic Group

	add	a
	add	a,a
	add	b
	add	a,b
	add	c
	add	a,c
	add	d
	add	a,d
	add	e
	add	a,e
	add	h
	add	a,h
	add	l
	add	a,l
	add	m
	add	a,(hl)
	adi	55h
	add	a,55h

	adc	a
	adc	a,a
	adc	b
	adc	a,b
	adc	c
	adc	a,c
	adc	d
	adc	a,d
	adc	e
	adc	a,e
	adc	h
	adc	a,h
	adc	l
	adc	a,l
	adc	m
	adc	a,(hl)
	aci	55h
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
	sub	m
	sub	(hl)
	sub	a,(hl)
	sui	55h
	sub	a,55h

	sbb	a
	sbc	a,a
	sbb	b
	sbc	a,b
	sbb	c
	sbc	a,c
	sbb	d
	sbc	a,d
	sbb	e
	sbc	a,e
	sbb	h
	sbc	a,h
	sbb	l
	sbc	a,l
	sbb	m
	sbc	a,(hl)
	sbi	55h
	sbc	a,55h

	ana	a
	and	a,a
	ana	b
	and	a,b
	ana	c
	and	a,c
	ana	d
	and	a,d
	ana	e
	and	a,e
	ana	h
	and	a,h
	ana	l
	and	a,l
	ana	m
	and	a,(hl)
	ani	55h
	and	a,55h

	xra	a
	xor	a,a
	xra	b
	xor	a,b
	xra	c
	xor	a,c
	xra	d
	xor	a,d
	xra	e
	xor	a,e
	xra	h
	xor	a,h
	xra	l
	xor	a,l
	xra	m
	xor	a,(hl)
	xri	55h
	xor	a,55h

	ora	a
	or	a,a
	ora	b
	or	a,b
	ora	c
	or	a,c
	ora	d
	or	a,d
	ora	e
	or	a,e
	ora	h
	or	a,h
	ora	l
	or	a,l
	ora	m
	or	a,(hl)
	ori	55h
	or	a,55h

	cmp	a
	cp	a,a
	cmp	b
	cp	a,b
	cmp	c
	cp	a,c
	cmp	d
	cp	a,d
	cmp	e
	cp	a,e
	cmp	h
	cp	a,h
	cmp	l
	cp	a,l
	cmp	m
	cp	a,(hl)
	cpi	55h
	cp	a,55h

	inr	b
	inc	b
	inr	c
	inc	c
	inr	d
	inc	d
	inr	e
	inc	e
	inr	h
	inc	h
	inr	l
	inc	l

	dcr	b
	dec	b
	dcr	c
	dec	c
	dcr	d
	dec	d
	dcr	e
	dec	e
	dcr	h
	dec	h
	dcr	l
	dec	l
