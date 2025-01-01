	page	0
	cpu 	8008

;-----------------------------------
; CPU Control Group

	hlt

;-----------------------------------
; Input and Output Group

	inp	5

	out	13h

;-----------------------------------
; Jump Group

	jmp	1234h

	jfc	1234h
	jfz	1234h
	jfs	1234h
	jfp	1234h
	jc	1234h
	jz	1234h
	js	1234h
	jp	1234h

;-----------------------------------
; Call and Return Group

        cal     1234h

        cfc     1234h
        cfz     1234h
        cfs     1234h
        cfp     1234h
        cc      1234h
        cz      1234h
        cs      1234h
        cp      1234h

	ret

        rfc
        rfz
        rfs
        rfp
        rc
        rz
        rs
        rp

	rst	38h

;-----------------------------------
; Load Group

	laa
	lab
	lac
	lad
	lae
	lah
	lal
	lba
	lbb
	lbc
	lbd
	lbe
	lbh
	lbl
	lca
	lcb
	lcc
	lcd
	lce
	lch
	lcl
	lda
	ldb
	ldc
	ldd
	lde
	ldh
	ldl
	lea
	leb
	lec
	led
	lee
	leh
	lel
	lha
	lhb
	lhc
	lhd
	lhe
	lhh
	lhl
	lla
	llb
	llc
	lld
	lle
	llh
	lll

	lam
	lbm
	lcm
	ldm
	lem
	lhm
	llm

	lma
	lmb
	lmc
	lmd
	lme
	lmh
	lml

	lai	55h
	lbi	55h
	lci	55h
	ldi	55h
	lei	55h
	lhi	55h
	lli	55h

	lmi	55h

;-----------------------------------
; Arithmetic Group

	ada
	adb
	adc
	add
	ade
	adh
	adl
	adm
	adi	55h

	aca
	acb
	acc
	acd
	ace
	ach
	acl
	acm
	aci	55h

	sua
	sub
	suc
	sud
	sue
	suh
	sul
	sum
	sui	55h

	sba
	sbb
	sbc
	sbd
	sbe
	sbh
	sbl
	sbm
	sbi	55h

	nda
	ndb
	ndc
	ndd
	nde
	ndh
	ndl
	ndm
	ndi	55h

	xra
	xrb
	xrc
	xrd
	xre
	xrh
	xrl
	xrm
	xri	55h

	ora
	orb
	orc
	ord
	ore
	orh
	orl
	orm
	ori	55h

	cpa
	cpb
	cpc
	cpd
	cpe
	cph
	cpl
	cpm
	cpi	55h

	inb
	inc
	ind
	ine
	inh
	inl

	dcb
	dcc
	dcd
	dce
	dch
	dcl

;-----------------------------------
; Rotate Group

	rlc
	rrc
	ral
	rar

;===================================

	cpu	8008new

;-----------------------------------
; CPU Control Group

        hlt

;-----------------------------------
; Input and Output Group

	in	5

	out	13h

;-----------------------------------
; Jump Group

	jmp	1234h

	jnc	1234h
	jnz	1234h
	jp	1234h
	jpo	1234h
	jc	1234h
	jz	1234h
	jm	1234h
	jpe	1234h

;-----------------------------------
; Call and Return Group

	call	1234h

        cnc     1234h
        cnz     1234h
	cp      1234h
        cpo     1234h
        cc      1234h
        cz      1234h
        cm      1234h
        cpe	1234h

	ret

        rnc
        rnz
	rp
        rpo
        rc
        rz
        rm
        rpe

	rst	38h

;-----------------------------------
; Load Group

	mov	a,a
	mov	a,b
	mov	a,c
	mov	a,d
	mov	a,e
	mov	a,h
	mov	a,l
	mov	b,a
	mov	b,b
	mov	b,c
	mov	b,d
	mov	b,e
	mov	b,h
	mov	b,l
	mov	c,a
	mov	c,b
	mov	c,c
	mov	c,d
	mov	c,e
	mov	c,h
	mov	c,l
	mov	d,a
	mov	d,b
	mov	d,c
	mov	d,d
	mov	d,e
	mov	d,h
	mov	d,l
	mov	e,a
	mov	e,b
	mov	e,c
	mov	e,d
	mov	e,e
	mov	e,h
	mov	e,l
	mov	h,a
	mov	h,b
	mov	h,c
	mov	h,d
	mov	h,e
	mov	h,h
	mov	h,l
	mov	l,a
	mov	l,b
	mov	l,c
	mov	l,d
	mov	l,e
	mov	l,h
	mov	l,l

	mov	a,m
	mov	b,m
	mov	c,m
	mov	d,m
	mov	e,m
	mov	h,m
	mov	l,m

	mov	m,a
	mov	m,b
	mov	m,c
	mov	m,d
	mov	m,e
	mov	m,h
	mov	m,l

	mvi	a,55h
	mvi	b,55h
	mvi	c,55h
	mvi	d,55h
	mvi	e,55h
	mvi	h,55h
	mvi	l,55h

	mvi	m,55h

	lxi	b,1234h		; convenience built-in macro:
	lxi	d,1234h		; LXI is assembled as 2 x MVI
	lxi	h,1234h

;-----------------------------------
; Arithmetic Group

	add	a
	add	b
	add	c
	add	d
	add	e
	add	h
	add	l
	add	m
	adi	55h

	adc	a
	adc	b
	adc	c
	adc	d
	adc	e
	adc	h
	adc	l
	adc	m
	aci	55h

	sub	a
	sub	b
	sub	c
	sub	d
	sub	e
	sub	h
	sub	l
	sub	m
	sui	55h

	sbb	a
	sbb	b
	sbb	c
	sbb	d
	sbb	e
	sbb	h
	sbb	l
	sbb	m
	sbi	55h

	ana	a
	ana	b
	ana	c
	ana	d
	ana	e
	ana	h
	ana	l
	ana	m
	ani	55h

	xra	a
	xra	b
	xra	c
	xra	d
	xra	e
	xra	h
	xra	l
	xra	m
	xri	55h

	ora	a
	ora	b
	ora	c
	ora	d
	ora	e
	ora	h
	ora	l
	ora	m
	ori	55h

	cmp	a
	cmp	b
	cmp	c
	cmp	d
	cmp	e
	cmp	h
	cmp	l
	cmp	m
	cpi	55h

	inr	b
	inr	c
	inr	d
	inr	e
	inr	h
	inr	l

	dcr	b
	dcr	c
	dcr	d
	dcr	e
	dcr	h
	dcr	l

;-----------------------------------
; Rotate Group

        rlc
        rrc
        ral
        rar

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
