	cpu	z88c00
	page	0
	include	"regz8.inc"


	adc	r3,r5	  	; 12 35
	adc	r3,@r5  	; 13 35
	adc	r3,64   	; 14 40 c3
	adc	32,r5   	; 14 c5 20
	adc	32,64   	; 14 40 20
	adc	r3,@64  	; 15 40 c3
	adc	32,@r5  	; 15 c5 20
	adc	32,@64  	; 15 40 20
	adc	r3,#64	 	; 16 c3 40
	adc	32,#64  	; 16 20 40

	add	r3,r5		; 02 35
	add	r3,@r5		; 03 35
	add	r3,64		; 04 40 c3
	add	32,r5		; 04 c5 20
	add	32,64		; 04 40 20
	add	r3,@64		; 05 40 c3
	add	32,@r5		; 05 c5 20
	add	32,@64		; 05 40 20
	add	r3,#64		; 06 c3 40
	add	32,#64		; 06 20 40

	and	r3,r5		; 52 35
	and	r3,@r5		; 53 35
	and	r3,64		; 54 40 c3
	and	32,r5		; 54 c5 20
	and	32,64		; 54 40 20
	and	r3,@64		; 55 40 c3
	and	32,@r5		; 55 c5 20
	and	32,@64		; 55 40 20
	and	r3,#64		; 56 c3 40
	and	32,#64		; 56 20 40

	band	r3,r5,#7	; 67 3e c5
	band	r3,64,#7	; 67 3e 40
	band	r3,#7,r5	; 67 5f c3
	band	32,#7,r5	; 67 5f 20

	bcp	r3,r5,#7	; 17 3e c5
	bcp	r3,64,#7	; 17 3e 40

	bitc	r3,#7		; 57 3e

	bitr	r3,#7		; 77 3e

	bits	r3,#7		; 77 3f

	bor	r3,r5,#7	; 07 3e c5
	bor	r3,64,#7	; 07 3e 40
	bor	r3,#7,r5	; 07 5f c3
	bor	32,#7,r5	; 07 5f 20

	btjrf	$,r5,#7 	; 37 5e fd
	btjrt	$,r5,#7 	; 37 5f fd

	bxor 	r3,r5,#7	; 27 3e c5
	bxor 	r3,64,#7	; 27 3e 40
	bxor 	r3,#7,r5	; 27 5f c3
	bxor 	32,#7,r5	; 27 5f 20

	call 	#32		; d4 20 (actually, 'call (32)' would better describe what happens)
	expect	1351
	call	#33
	endexpect
	call 	@rr2		; f4 c2
	call 	@32		; f4 20
	call 	64		; f6 00 40

	ccf			; ef

	clr	r3 		; b0 c3
	clr	32 		; b0 20	
	clr	@r3		; b1 c3
	clr	@32		; b1 20

	com	r3 		; 60 c3
	com   	32 		; 60 20
	com 	@r3		; 61 c3
	com   	@32		; 61 20

	cp	r3,r5		; a2 35
	cp	r3,@r5		; a3 35
	cp	r3,64		; a4 40 c3
	cp	32,r5		; a4 c5 20
	cp	32,64		; a4 40 20
	cp	r3,@64		; a5 40 c3
	cp	32,@r5		; a5 c5 20
	cp	32,@64		; a5 40 20
	cp	r3,#64		; a6 c3 40
	cp	32,#64		; a6 20 40

	cpijne	r3,@r5,$	; d2 53 fd

	cpije	r3,@r5,$	; c2 53 fd

	da	r3		; 40 c3
	da	32		; 40 20
	da	@r3		; 41 c3
	da	@32		; 41 20

	dec	r3		; 00 c3
	dec	32		; 00 20
	dec	@r3		; 01 c3
	dec	@32		; 01 20

	decw	rr2		; 80 c2
	decw	32		; 80 20
	decw	@r3		; 81 c3
	decw	@32		; 81 20

	di			; 8f

	div	rr2,r5		; 94 c5 c2
	div	rr2,64		; 94 40 c2
	div	32,r5		; 94 c5 20
	div	32,64		; 94 40 20
	div	rr2,@r5		; 95 c5 c2
	div	rr2,@64		; 95 40 c2
	div	32,@r5		; 95 c5 20
	div	32,@64		; 95 40 20
	div	rr2,#64		; 96 40 c2
	div	32,#64		; 96 40 20

	djnz	r3,$		; 3a fe

	ei			; 9f

	enter			; 1f

	exit			; 2f

	inc	r3		; 3e
	inc	32		; 20 20
	inc	@r3		; 21 c3
	inc	@32		; 21 20

	incw	rr2		; a0 c2
	incw	32		; a0 20
	incw	@r3		; a1 c3
	incw	@32		; a1 20

	iret			; bf

	jp	1024		; 8d 04 00
	jp	nz,1024		; ed 04 00
	jp	@rr2		; 30 c2
	jp	@32		; 30 20

	jr	$		; 8b fe
	jr	nz,$		; eb fe

	ld	r3,#64		; 3c 40
	ld	r3,r5		; 38 c5
	ld	r3,64		; 38 40
	ld	32,r5		; 59 20
	ld	r3,@r5		; c7 35
	ld	@r3,r5		; d7 35
	ld	32,64		; e4 40 20
	ld	r3,@r5		; c7 35
	ld	r3,@64		; e5 40 c3
	ld	32,@r5		; e5 c5 20
	ld	32,@64		; e5 40 20
	ld	r3,#64		; 3c 40
	ld	32,#64		; e6 20 40
	ld	@r3,#64		; d6 c3 40
	ld	@32,#64		; d6 20 40
	ld	@r3,r5		; d7 35
	ld	@r3,64		; f5 40 c3
	ld	@32,r5		; f5 c5 20
	ld	@32,64		; f5 40 20
	ld	r3,64(r5)	; 87 35 40
	ld	64(r3),r5	; 97 53 40

	ldb	r3,r5,#7	; 47 3e c5
	ldb	r3,64,#7	; 47 3e 40
	ldb	r3,#7,r5	; 47 5f c3
	ldb	32,#7,r5	; 47 5f 20

	ldc	r3,1024(rr4)	; a7 34 00 04
	ldc	r3,64(rr4)	; e7 34 40
	ldc	1024(rr2),r5	; b7 52 00 04
	ldc	64(rr2),r5	; f7 52 40
	ldc	32,r5		; b7 50 20 00
	ldc	r5,64		; a7 50 40 00
	ldc	r3,@rr4		; c3 34
	ldc	@rr2,r5		; d3 52

	ldcd	r3,@rr4		; e2 34

	ldci	r3,@rr4		; e3 34

	ldcpd	@rr2,r5		; f2 52

	ldcpi	@rr2,r5		; f3 52

	lde	r3,1024(rr4)	; a7 35 00 04
	lde	r3,64(rr4)	; e7 35 40
	lde	1024(rr2),r5	; b7 53 00 04
	lde	64(rr2),r5	; f7 53 40
	lde	32,r5		; b7 51 20 00
	lde	r5,64		; a7 51 40 00
	lde	r3,@rr4		; c3 35
	lde	@rr2,r5		; d3 53

	lded	r3,@rr4		; e2 35

	ldei	r3,@rr4		; e3 35

	ldepd	@rr2,r5		; f2 53

	ldepi	@rr2,r5		; f3 53

	ldw	rr2,rr4		; c4 c4 c2
	ldw	rr2,64		; c4 40 c2
	ldw	32,rr4		; c4 c4 20
	ldw	32,64		; c4 40 20
	ldw	rr2,@r4		; c5 c4 c2
	ldw	rr2,@64		; c5 40 c2
	ldw	32,@r4		; c5 c4 20
	ldw	32,@64		; c5 40 20
	ldw	rr2,#1024	; c6 c2 04 00
	ldw	32,#1024	; c6 20 04 00

	mult	rr2,r5		; 84 c5 c2
	mult	rr2,64		; 84 40 c2
	mult	32,r5		; 84 c5 20
	mult	32,64		; 84 40 20
	mult	rr2,@r5		; 85 c5 c2
	mult	rr2,@64		; 85 40 c2
	mult	32,@r5		; 85 c5 20
	mult	32,@64		; 85 40 20
	mult	rr2,#64		; 86 40 c2
	mult	32,#64		; 86 40 20

	next			; 0f

	nop			; ff

	or	r3,r5		; 42 35
	or	r3,@r5		; 43 35
	or	r3,64		; 44 40 c3
	or	32,r5		; 44 c5 20
	or	32,64		; 44 40 20
	or	r3,@64		; 45 40 c3
	or	32,@r5		; 45 c5 20
	or	32,@64		; 45 40 20
	or	r3,#64		; 46 c3 40
	or	32,#64		; 46 20 40

	pop	r3		; 50 c3
	pop	32		; 50 20
	pop	@r3		; 51 c3
	pop	@32		; 51 20

	popud	r3,@r5		; 92 c5 c3
	popud	r3,@64		; 92 40 c3
	popud	32,@r5		; 92 c5 20
	popud	32,@64		; 92 40 20

	popui	r3,@r5		; 93 c5 c3
	popui	r3,@64		; 93 40 c3
	popui	32,@r5		; 93 c5 20
	popui	32,@64		; 93 40 20

	push	r3 		; 70 c3
	push	32 		; 70 20
	push	@r3		; 71 c3
	push	@32		; 71 20

	pushud	@r3,r5		; 82 c3 c5
	pushud	@r3,64		; 82 c3 40
	pushud	@32,r5		; 82 20 c5
	pushud	@32,64		; 82 20 40

	pushui	@r3,r5		; 83 c3 c5
	pushui	@r3,64		; 83 c3 40
	pushui	@32,r5		; 83 20 c5
	pushui	@32,64		; 83 20 40

	rcf			; cf

	rdr	#0a5h		; d5 a5

	ret			; af

	rl	r3		; 90 c3
	rl	32		; 90 20
	rl	@r3		; 91 c3
	rl	@32		; 91 20

	rlc	r3		; 10 c3
	rlc	32		; 10 20
	rlc	@r3		; 11 c3
	rlc	@32		; 11 20

	rr	r3		; e0 c3
	rr	32		; e0 20
	rr	@r3		; e1 c3
	rr	@32		; e1 20

	rrc	r3		; c0 c3
	rrc	32		; c0 20
	rrc	@r3		; c1 c3
	rrc	@32		; c1 20

	sb0			; 4f

	sb1			; 5f

	sbc	r3,r5		; 32 35
	sbc	r3,@r5		; 33 35
	sbc	r3,64		; 34 40 c3
	sbc	32,r5		; 34 c5 20
	sbc	32,64		; 34 40 20
	sbc	r3,@64		; 35 40 c3
	sbc	32,@r5		; 35 c5 20
	sbc	32,@64		; 35 40 20
	sbc	r3,#64		; 36 c3 40
	sbc	32,#64		; 36 20 40

	scf			; df

	sra	r3		; d0 c3
	sra	32		; d0 20
	sra	@r3		; d1 c3
	sra	@32		; d1 20

	srp	#128		; 31 80
	srp1	#128		; 31 81
	srp0	#128+8		; 31 8a

	sub	r3,r5		; 22 35
	sub	r3,@r5		; 23 35
	sub	r3,64		; 24 40 c3
	sub	32,r5		; 24 c5 20
	sub	32,64		; 24 40 20
	sub	r3,@64		; 25 40 c3
	sub	32,@r5		; 25 c5 20
	sub	32,@64		; 25 40 20
	sub	r3,#64		; 26 c3 40
	sub	32,#64		; 26 20 40

	swap	r3		; f0 c3
	swap	32		; f0 20
	swap	@r3		; f1 c3
	swap	@32		; f1 20

	tcm	r3,r5		; 62 35
	tcm	r3,@r5		; 63 35
	tcm	r3,64		; 64 40 c3
	tcm	32,r5		; 64 c5 20
	tcm	32,64		; 64 40 20
	tcm	r3,@64		; 65 40 c3
	tcm	32,@r5		; 65 c5 20
	tcm	32,@64		; 65 40 20
	tcm	r3,#64		; 66 c3 40
	tcm	32,#64		; 66 20 40

	tm	r3,r5		; 72 35
	tm	r3,@r5		; 73 35
	tm	r3,64		; 74 40 c3
	tm	32,r5		; 74 c5 20
	tm	32,64		; 74 40 20
	tm	r3,@64		; 75 40 c3
	tm	32,@r5		; 75 c5 20
	tm	32,@64		; 75 40 20
	tm	r3,#64		; 76 c3 40
	tm	32,#64		; 76 20 40

	xor	r3,r5		; b2 35
	xor	r3,@r5		; b3 35
	xor	r3,64		; b4 40 c3
	xor	32,r5		; b4 c5 20
	xor	32,64		; b4 40 20
	xor	r3,@64		; b5 40 c3
	xor	32,@r5		; b5 c5 20
	xor	32,@64		; b5 40 20
	xor	r3,#64		; b6 c3 40
	xor	32,#64		; b6 20 40

	wfi			; 3f

	; test for condition codes

	jp	f,128		; 0d 00 80
	jp	z,128		; 6d 00 80
	jp	nz,128		; ed 00 80
	jp	eq,128		; 6d 00 80
	jp	ne,128		; ed 00 80
	jp	c,128		; 7d 00 80
	jp	nc,128		; fd 00 80
	jp	gt,128		; ad 00 80
	jp	lt,128		; 1d 00 80
	jp	ge,128		; 9d 00 80
	jp	le,128		; 2d 00 80
	jp	pl,128		; dd 00 80
	jp	mi,128		; 5d 00 80
	jp	nov,128		; cd 00 80
	jp	ov,128		; 4d 00 80
	jp	ugt,128		; bd 00 80
	jp	ult,128		; 7d 00 80
	jp	uge,128		; fd 00 80
	jp	ule,128		; 3d 00 80

	; defined register names

	ld	r3,sym		; 38 de
	ld	r3,imr		; 38 dd
	ld	r3,irr		; 38 dc
	ldw	rr2,ip		; c4 da c2
	ld	r3,ipl		; 38 db
	ld	r3,iph		; 38 da
	ldw	rr2,sp		; c4 d8 c2
	ld	r3,spl		; 38 d9
	ld	r3,sph		; 38 da
	ld	r3,rp1		; 38 d7
	ld	r3,rp0		; 38 d6
	ld	r3,flags	; 38 d5
	ld	r3,p4		; 38 d4
	ld	r3,p3		; 38 d3
	ld	r3,p2		; 38 d2
	ld	r3,p1		; 38 d1
	ld	r3,p0		; 38 d0

	; Bank 0 Special Registers

	ld	r3,ipr		; 38 ff
	ld	r3,emt		; 38 fe
	ld	r3,p2bip	; 38 fd
	ld	r3,p2aip	; 38 fc
	ld	r3,p2dm		; 38 fb
	ld	r3,p2cm		; 38 fa
	ld	r3,p2bm		; 38 f9
	ld	r3,p2am		; 38 f8
	ld	r3,p4od		; 38 f7
	ld	r3,p4d		; 38 f6
	ld	r3,h1c		; 38 f5
	ld	r3,h0c		; 38 f4
	ld	r3,pm		; 38 f1
	ld	r3,p1		; 38 d1
	ld	r3,p0m		; 38 f0
	ld	r3,uie		; 38 ed
	ld	r3,urc		; 38 ec
	ld	r3,utc		; 38 eb
	if	0
	ld	r3,sio		; 38 ea
	ld	r3,sie		; 38 e9
	ld	r3,srcb		; 38 e8
	ld	r3,srca		; 38 e7
	ld	r3,stc		; 38 e6
	endif
	ldw	rr2,c1c		; c4 e4 c2
	ld	r3,c1cl		; 38 e5
	ld	r3,c1ch		; 38 e4
	ldw	rr2,c0c		; c4 e2 c2
	ld	r3,c0cl		; 38 e3
	ld	r3,c0ch		; 38 e2
	ld	r3,c1ct		; 38 e1
	ld	r3,c0ct		; 38 e0

	; Bank 1 Special Registers

	ld	r3,wumsk	; 38 ff
	ld	r3,wumch	; 38 fe
	ld	r3,umb		; 38 fb
	ld	r3,uma		; 38 fa
	ldw	rr2,ubg		; c4 f8 c2
	ld	r3,ubgl		; 38 f9
	ld	r3,ubgh		; 38 f8
	ldw	rr2,dc		; c4 f0 c2
	ld	r3,dcl		; 38 f1
	ld	r3,dch		; 38 f0
	if	0
	ldw	rr2,syn		; c4 ee c2
	ld	r3,synh		; 38 ef
	ld	r3,synl		; 38 ee
	ld	r3,smd		; 38 ed
	ld	r3,smc		; 38 ec
	ld	r3,smb		; 38 eb
	ld	r3,sma		; 38 ea
	ldw	rr2,sbg		; c4 e8 c2
	ld	r3,sbgl		; 38 e9
	ld	r3,sbgh		; 38 e8
	endif
	ldw	rr2,c1tc	; c4 e4 c2
	ld	r3,c1tcl	; 38 e5
	ld	r3,c1tch	; 38 e4
	ldw	rr2,c0tc	; c4 e2 c2
	ld	r3,c0tcl	; 38 e3
	ld	r3,c0tch	; 38 e2
	ld	r3,c1m		; 38 e1
	ld	r3,c0m		; 38 e0

	; upper case test

	ld	r3,SYM		; 38 de
	ld	r3,IMR		; 38 dd
	ld	r3,IRR		; 38 dc
	ldw	rr2,IP		; c4 da c2
	ld	r3,IPL		; 38 db
	ld	r3,IPH		; 38 da
	ldw	rr2,SP		; c4 d8 c2
	ld	r3,SPL		; 38 d9
	ld	r3,SPH		; 38 d8
	ld	r3,RP1		; 38 d7
	ld	r3,RP0		; 38 d6
	ld	r3,FLAGS	; 38 d5
	ld	r3,P4		; 38 d4
	ld	r3,P3		; 38 d3
	ld	r3,P2		; 38 d2
	ld	r3,P1		; 38 d1
	ld	r3,P0		; 38 d0

	; Bank 0 Special Registers

	ld	r3,IPR		; 38 ff
	ld	r3,P2BIP	; 38 fd
	ld	r3,P2AIP	; 38 fc
	ld	r3,P2DM		; 38 fb
	ld	r3,P2CM		; 38 fa
	ld	r3,P2BM		; 38 f9
	ld	r3,P2AM		; 38 f8
	ld	r3,P4OD		; 38 f7
	ld	r3,P4D		; 38 f6
	ld	r3,H1C		; 38 f5
	ld	r3,H0C		; 38 f4
	ld	r3,PM		; 38 f1
	ld	r3,P1		; 38 d1
	ld	r3,P0M		; 38 f0
	ld	r3,UIE		; 38 ed
	ld	r3,URC		; 38 ec
	ld	r3,UTC		; 3S eb
	if	0
	ld	r3,SIO		; 38 ea
	ld	r3,SIE		; 38 e9
	ld	r3,SRCB		; 38 e8
	ld	r3,SRCA		; 38 e7
	ld	r3,STC		; 38 e6
	endif
	ldw	rr2,C1C		; c4 e4 c2
	ld	r3,C1CL		; 38 e5
	ld	r3,C1CH		; 38 e4
	ldw	rr2,C0C		; c4 e2 c2
	ld	r3,C0CL		; 38 e3
	ld	r3,C0CH		; 38 e2
	ld	r3,C1CT		; 38 e1
	ld	r3,C0CT		; 38 e0

	; Bank 1 Special Registers

	ld	r3,WUMSK	; 38 ff
	ld	r3,WUMCH	; 38 fe
	ld	r3,UMB		; 38 fb
	ld	r3,UMA		; 38 fa
	ldw	rr2,UBG		; c4 f0 c2
	ld	r3,UBGL		; 38 f9
	ld	r3,UBGH		; 38 f0
	ldw	rr2,DC		; c4 f0c 2
	ld	r3,DCL		; 38 f1
	ld	r3,DCH		; 38 f0
	if	0
	ldw	rr2,SYN		; c4 ee c2
	ld	r3,SYNH		; 38 ef
	ld	r3,SYNL		; 38 ee
	ld	r3,SMD		; 38 ed
	ld	r3,SMC		; 38 ec
	ld	r3,SMB		; 38 eb
	ld	r3,SMA		; 38 ea
	ldw	rr2,SBG		; c4 e0 c2
	ld	r3,SBGL		; 38 e9
	ld	r3,SBGH		; 38 e0
	endif
	ldw	rr2,C1TC	; c4 e4 c2
	ld	r3,C1TCL	; 38 e5
	ld	r3,C1TCH	; 38 e4
	ldw	rr2,C0TC	; c4 e2 c2
	ld	r3,C0TCL	; 38 e3
	ld	r3,C0TCH	; 38 e2
	ld	r3,C1M		; 38 e1
	ld	r3,C0M		; 38 e0

	; test for condition codes

	jp	f,128		; 0d 00 80

	jp	z,128		; 6d 00 80
	jp	nz,128		; ed 00 80
	jp	eq,128		; 6d 00 80
	jp	ne,128		; ed 00 80

	jp	c,128		; 7d 00 80
	jp	nc,128		; fd 00 80

	jp	gt,128		; ad 00 80
	jp	lt,128		; 1d 00 80
	jp	ge,128		; 9d 00 80
	jp	le,128		; 2d 00 80

	jp	pl,128		; dd 00 80
	jp	mi,128		; 5d 00 80

	jp	nov,128		; cd 00 80
	jp	ov,128		; 4d 00 80

	jp	ugt,128		; bd 00 80
	jp	ult,128		; 7d 00 80
	jp	uge,128		; fd 00 80
	jp	ule,128		; 3d 00 80

	; symbolic bits

	ldb	r3,0feh,#6	; 47 3c fe
	ldb	r3,emt,#6	; 47 3c fe
	ldb	r3,slow		; 47 3c fe

	band	0edh,#5,r3	; 67 3b ed
	band	UIE,#5,r3	; 67 3b ed
	band	UBRKIE,r3	; 67 3b ed

regbit	defbit	r3,4
	bitc	r3,#4
	bitc	regbit
	btjrf	$,r3,#4
	btjrf	$,regbit

	; register pointers

	assume	rp:20h		; sets RP0=20h, RP1=28h
	ld	24h,#0aah	; translates to 'ld r4,...'
	ld	2ch,#0aah	; translates to 'ld r12,...'
	assume	rp1:30h
	ld	24h,#0aah	; translates to 'ld r4,...'
	ld	2ch,#0aah	; no mapping to work register
	ld	34h,#0aah	; translates to 'ld r12,...'

	end
