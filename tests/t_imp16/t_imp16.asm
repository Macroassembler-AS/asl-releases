	cpu	imp-16l
	page	0

	; for fun, we will this time iterate
        ; the instructions ordered by opcode
        ; ...and because that's the way they
	; are ordered in the instruction guide:

	halt			; 0000

	pushf			; 0080

	rti	2		; 0102
	expect	1315
	rti	-3
	endexpect
	expect	1320
	rti	256
	endexpect

	rts	2		; 0102
	expect	1315
	rts	-3
	endexpect
	expect	1320
	rts	256
	endexpect

	pullf			; 0280

	; accept address both with and without base:
	jsrp	x'7f		; 037f
	jsrp	x'17f		; 037f
	expect	1320,1320
	jsrp	x'80
	jsrp	x'180
	endexpect

	jsri	x'7f		; 03ff
	jsri	x'ffff		; 03ff
	expect	1315,1315
	jsri	x'80
	jsri	x'ff7f
	endexpect

	rin	x'12		; 0412
	rin	x'7f		; 047f
	expect	1320
	rin	x'80
	endexpect

	; untyped address preferrably uses absolute addressing mode:
	mpy	x'1000		; 0480 1000
	; force PC-relative address though address is untyped:
here	equ	.
	mpy	here(pc)	; 0580 ffff
	; CODE segment address preferrably uses PC-relative address:
	mpy	.+2		; 0580 0001
	mpy	.+2(pc)		; 0580 0001
	mpy	.-2		; 0580 fffd
	mpy	.-2(pc)		; 0580 fffd
	mpy	(2)		; 0680 0000
	mpy	(3)		; 0780 0000
	mpy	-128(ac2)	; 0680 ff80
	mpy	127(ac3)	; 0780 007f
	mpy	-129(ac2)	; 0680 ff7f
	mpy	128(ac3)	; 0780 0080

	expect	1350
	mpy	@.+2
	endexpect

	div	.+2		; 0590 0001
	div	.-2		; 0590 fffd
	div	(2)		; 0690 0000
	div	(3)		; 0790 0000
	div	-128(ac2)	; 0690 ff80
	div	127(ac3)	; 0790 007f
	div	-129(ac2)	; 0690 ff7f
	div	128(ac3)	; 0790 0080

	dadd	.+2		; 05a0 0001
	dadd	.-2		; 05a0 fffd
	dadd	(2)		; 06a0 0000
	dadd	(3)		; 07a0 0000
	dadd	-128(ac2)	; 06a0 ff80
	dadd	127(ac3)	; 07a0 007f
	dadd	-129(ac2)	; 06a0 ff7f
	dadd	128(ac3)	; 07a0 0080

	dsub	.+2		; 05b0 0001
	dsub	.-2		; 05b0 fffd
	dsub	(2)		; 06b0 0000
	dsub	(3)		; 07b0 0000
	dsub	-128(ac2)	; 06b0 ff80
	dsub	127(ac3)	; 07b0 007f
	dsub	-129(ac2)	; 06b0 ff7f
	dsub	128(ac3)	; 07b0 0080

	; no PC-relative addressing on LDB
	ldb	x'9000		; 04c0 9000
	ldb	(ac2)		; 06c0 0000
	ldb	(ac3)		; 07c0 0000
	ldb	x'7000(ac2)	; 06c0 7000
	ldb	x'7000(ac3)	; 07c0 7000

	; Not sure how to interprete this.  For me, it's the most
	; plausible to write the word address in the source,
	; and the assembler multiplies by two and sets the
	; LSB accordingly:
	llb	x'7000		; 04c0 e000
	llb	(ac2)		; 06c0 0000
	llb	(ac3)		; 07c0 0000
	llb	x'7000(ac2)	; 06c0 e000
	llb	x'7000(ac3)	; 07c0 e000

	lrb	x'7000		; 04c0 e001
	lrb	(ac2)		; 06c0 0001
	lrb	(ac3)		; 07c0 0001
	lrb	x'7000(ac2)	; 06c0 e001
	lrb	x'7000(ac3)	; 07c0 e001

	; no PC-relative addressing on STB
	stb	x'9000		; 04d0 9000
	stb	(ac2)		; 06d0 0000
	stb	(ac3)		; 07d0 0000
	ldb	x'7000(ac2)	; 06d0 7000
	ldb	x'7000(ac3)	; 07d0 7000

	; See remarks on LDB:
	slb	x'7000		; 04d0 e000
	slb	(ac2)		; 06d0 0000
	slb	(ac3)		; 07d0 0000
	slb	x'7000(ac2)	; 06d0 e000
	slb	x'7000(ac3)	; 07d0 e000

	srb	x'7000		; 04d0 e001
	srb	(ac2)		; 06d0 0001
	srb	(ac3)		; 07d0 0001
	srb	x'7000(ac2)	; 06d0 e001
	srb	x'7000(ac3)	; 07d0 e001

	jmpp	0		; 0500
	jmpp	15		; 050f
	jmpp	x'100		; 0500
	jmpp	x'10f		; 050f
	expect	1315
	jmpp	x'00ff
	endexpect
	expect	1320
	jmpp	x'0110
	endexpect

	iscan			; 0510

	jint	0		; 0520
	jint	15		; 052f
	jint	x'0120		; 0520
	jint	x'012f		; 052f
	expect	1315
	jint	x'011f
	endexpect
	expect	1320
	jint	x'0130
	endexpect

	rout	x'34		; 0634
	rout	x'7f		; 067f
	expect	1320
	rout	x'80
	endexpect

	setst	l		; 070f
	setst	ov		; 070e
	setst	cy		; 070d
	setst	0		; 0700
	setst	12		; 070c
	setst	15		; 070f
	expect	1320
	setst	16
	endexpect

	clrst	l		; 071f
	clrst	ov		; 071e
	clrst	cy		; 071d
	clrst	0		; 0710
	clrst	12		; 071c
	clrst	15		; 071f
	expect	1320
	clrst	16
	endexpect

	setbit	0		; 0720
	setbit	12		; 072c
	setbit	15		; 072f
	expect	1320
	setbit	16
	endexpect

	clrbit	0		; 0730
	clrbit	12		; 073c
	clrbit	15		; 073f
	expect	1320
	clrbit	16
	endexpect

	skstf	l		; 074f
	skstf	ov		; 074e
	skstf	cy		; 074d
	skstf	0		; 0740
	skstf	12		; 074c
	skstf	15		; 074f
	expect	1320
	skstf	16
	endexpect

	skbit	0		; 0750
	skbit	12		; 075c
	skbit	15		; 075f
	expect	1320
	skbit	16
	endexpect

	cmpbit	0		; 0760
	cmpbit	12		; 076c
	cmpbit	15		; 076f
	expect	1320
	cmpbit	16
	endexpect

	sflg	8,127		; 087f
	sflg	15,42		; 0f2a
	expect	1320
	sflg	16,42
	endexpect
	expect	1315
	sflg	4,42
	endexpect
	expect	1320
	sflg	12,130
	endexpect

	pflg	8,127		; 08ff
	pflg	15,42		; 0faa
	expect	1320
	pflg	16,42
	endexpect
	expect	1315
	pflg	4,42
	endexpect
	expect	1320
	pflg	12,130
	endexpect

	boc	int,.+4		; 1003
	boc	req0,.+5	; 1104
	boc	psign,.+6	; 1205
	boc	bit0,.+7	; 1306
	boc	bit1,.+8	; 1407
	boc	nreq0,.+9	; 1508
	boc	cpint,.+10	; 1609
	boc	start,.-10	; 17f5
	boc	stfl,.-9	; 18f6
	boc	inen,.-8	; 19f7
	boc	cy/ov,.-7	; 1af8
	boc	nsign,.-6	; 1bf9
	boc	poa,.-5		; 1cfa
	boc	sel,.-4		; 1dfb
	expect	1370
	boc	int,.+130
	endexpect

	jmp	.+2		; 2101
	jmp	.-2		; 21fd
	jmp	(2)		; 2200
	jmp	(3)		; 2300
	jmp	-128(ac2)	; 2280
	jmp	127(ac3)	; 237f
	expect	1315
	jmp	-129(ac2)
	endexpect
	expect	1320
	jmp	128(ac3)
	endexpect

	jmp	@.+2		; 2501
	jmp	@.-2		; 25fd
	jmp	@(2)		; 2600
	jmp	@(3)		; 2700
	jmp	@-128(ac2)	; 2680
	jmp	@127(ac3)	; 277f
	expect	1315
	jmp	@-129(ac2)
	endexpect
	expect	1320
	jmp	@128(ac3)
	endexpect

	jsr	.+2		; 2901
	jsr	.-2		; 29fd
	jsr	(2)		; 2a00
	jsr	(3)		; 2b00
	jsr	-128(ac2)	; 2a80
	jsr	127(ac3)	; 2b7f
	expect	1315
	jsr	-129(ac2)
	endexpect
	expect	1320
	jsr	128(ac3)
	endexpect

	jsr	@.+2		; 2d01
	jsr	@.-2		; 2dfd
	jsr	@(2)		; 2e00
	jsr	@(3)		; 2f00
	jsr	@-128(ac2)	; 2e80
	jsr	@127(ac3)	; 2f7f
	expect	1315
	jsr	@-129(ac2)
	endexpect
	expect	1320
	jsr	@128(ac3)
	endexpect

	radd	ac0,0		; 3000
	radd	0,ac1		; 3100
	radd	ac0,2		; 3200
	radd	0,ac3		; 3300
	radd	ac1,0		; 3400
	radd	1,ac1		; 3500
	radd	ac1,2		; 3600
	radd	1,ac3		; 3700
	radd	ac2,0		; 3800
	radd	2,ac1		; 3900
	radd	ac2,2		; 3a00
	radd	2,ac3		; 3b00
	radd	ac3,0		; 3c00
	radd	3,ac1		; 3d00
	radd	ac3,2		; 3e00
	radd	3,ac3		; 3f00

	rxch	ac0,0		; 3080
	rxch	0,ac1		; 3180
	rxch	ac0,2		; 3280
	rxch	0,ac3		; 3380
	rxch	ac1,0		; 3480
	rxch	1,ac1		; 3580
	rxch	ac1,2		; 3680
	rxch	1,ac3		; 3780
	rxch	ac2,0		; 3880
	rxch	2,ac1		; 3980
	rxch	ac2,2		; 3a80
	rxch	2,ac3		; 3b80
	rxch	ac3,0		; 3c80
	rxch	3,ac1		; 3d80
	rxch	ac3,2		; 3e80
	rxch	3,ac3		; 3f80

	rcpy	ac0,0		; 3081
	rcpy	0,ac1		; 3181
	rcpy	ac0,2		; 3281
	rcpy	0,ac3		; 3381
	rcpy	ac1,0		; 3481
	rcpy	1,ac1		; 3581
	rcpy	ac1,2		; 3681
	rcpy	1,ac3		; 3781
	rcpy	ac2,0		; 3881
	rcpy	2,ac1		; 3981
	rcpy	ac2,2		; 3a81
	rcpy	2,ac3		; 3b81
	rcpy	ac3,0		; 3c81
	rcpy	3,ac1		; 3d81
	rcpy	ac3,2		; 3e81
	rcpy	3,ac3		; 3f81

	nop			; 3081

	rxor	ac0,0		; 3082
	rxor	0,ac1		; 3182
	rxor	ac0,2		; 3282
	rxor	0,ac3		; 3382
	rxor	ac1,0		; 3482
	rxor	1,ac1		; 3582
	rxor	ac1,2		; 3682
	rxor	1,ac3		; 3782
	rxor	ac2,0		; 3882
	rxor	2,ac1		; 3982
	rxor	ac2,2		; 3a82
	rxor	2,ac3		; 3b82
	rxor	ac3,0		; 3c82
	rxor	3,ac1		; 3d82
	rxor	ac3,2		; 3e82
	rxor	3,ac3		; 3f82

	rand	ac0,0		; 3083
	rand	0,ac1		; 3183
	rand	ac0,2		; 3283
	rand	0,ac3		; 3383
	rand	ac1,0		; 3483
	rand	1,ac1		; 3583
	rand	ac1,2		; 3683
	rand	1,ac3		; 3783
	rand	ac2,0		; 3883
	rand	2,ac1		; 3983
	rand	ac2,2		; 3a83
	rand	2,ac3		; 3b83
	rand	ac3,0		; 3c83
	rand	3,ac1		; 3d83
	rand	ac3,2		; 3e83
	rand	3,ac3		; 3f83

	push	0		; 4000
	push	ac0		; 4000
	push	1		; 4100
	push	ac1		; 4100
	push	2		; 4200
	push	ac2		; 4200
	push	3		; 4300
	push	ac3		; 4300

	pull	0		; 4400
	pull	ac0		; 4400
	pull	1		; 4500
	pull	ac1		; 4500
	pull	2		; 4600
	pull	ac2		; 4600
	pull	3		; 4700
	pull	ac3		; 4700

	aisz	0,42		; 482a
	aisz	ac1,-42		; 49d6
	aisz	2,255		; 4aff
	aisz	ac3,-128	; 4b80
	expect	1320
	aisz	2,256
	endexpect
	expect	1315
	aisz	ac3,-129
	endexpect

	li	0,42		; 4c2a
	li	ac1,-42		; 4dd6
	li	2,255		; 4eff
	li	ac3,-128	; 4f80
	expect	1320
	li	2,256
	endexpect
	expect	1315
	li	ac3,-129
	endexpect

	cai	0,42		; 502a
	cai	ac1,-42		; 51d6
	cai	2,255		; 52ff
	cai	ac3,-128	; 5380
	expect	1320
	cai	2,256
	endexpect
	expect	1315
	cai	ac3,-129
	endexpect

	xchrs	0		; 5400
	xchrs	ac0		; 5400
	xchrs	1		; 5500
	xchrs	ac1		; 5500
	xchrs	2		; 5600
	xchrs	ac2		; 5600
	xchrs	3		; 5700
	xchrs	ac3		; 5700

	rol	0,0		; 5800
	rol	ac1,1		; 5901
	rol	2,127		; 5a7f
	rol	ac3,15		; 5b0f
	expect	1320
	rol	ac3,128
	endexpect

	ror	0,0		; 5800
	ror	ac1,1		; 59ff
	ror	2,127		; 5a81
	ror	ac3,15		; 5bf1
	expect	1320
	ror	ac3,128
	endexpect

	shl	0,0		; 5c00
	shl	ac1,1		; 5d01
	shl	2,127		; 5e7f
	shl	ac3,15		; 5f0f
	expect	1320
	shl	ac3,128
	endexpect

	shr	0,0		; 5c00
	shr	ac1,1		; 5dff
	shr	2,127		; 5e81
	shr	ac3,15		; 5ff1
	expect	1320
	shr	ac3,128
	endexpect

	and	0,.+2		; 6101
	and	ac1,.-2		; 65fd
	and	0,(2)		; 6200
	and	ac1,(3)		; 6700
	and	0,-128(ac2)	; 6280
	and	ac1,127(ac3)	; 677f
	expect	1315
	and	0,-129(ac2)
	endexpect
	expect	1320
	and	ac1,128(ac3)
	endexpect

	or	0,.+2		; 6901
	or	ac1,.-2		; 6dfd
	or	0,(2)		; 6a00
	or	ac1,(3)		; 6f00
	or	0,-128(ac2)	; 6a80
	or	ac1,127(ac3)	; 6f7f
	expect	1315
	or	0,-129(ac2)
	endexpect
	expect	1320
	or	ac1,128(ac3)
	endexpect

	skaz	0,.+2		; 7101
	skaz	ac1,.-2		; 75fd
	skaz	0,(2)		; 7200
	skaz	ac1,(3)		; 7700
	skaz	0,-128(ac2)	; 7280
	skaz	ac1,127(ac3)	; 777f
	expect	1315
	skaz	0,-129(ac2)
	endexpect
	expect	1320
	skaz	ac1,128(ac3)
	endexpect

	isz	.+2		; 7901
	isz	.-2		; 79fd
	isz	(2)		; 7a00
	isz	(3)		; 7b00
	isz	-128(ac2)	; 7a80
	isz	127(ac3)	; 7b7f
	expect	1315
	isz	-129(ac2)
	endexpect
	expect	1320
	isz	128(ac3)
	endexpect

	dsz	.+2		; 7d01
	dsz	.-2		; 7dfd
	dsz	(2)		; 7e00
	dsz	(3)		; 7f00
	dsz	-128(ac2)	; 7e80
	dsz	127(ac3)	; 7f7f
	expect	1315
	dsz	-129(ac2)
	endexpect
	expect	1320
	dsz	128(ac3)
	endexpect

	; untyped addresses (not from CODE segment)
	; result in using direct addressing if the
	; address is in the range 0..255:
	ld	0,127		; 807f

	ld	0,.+2		; 8101
	ld	0,.+2(pc)	; 8101
	ld	ac1,.-2		; 85fd
	ld	2,(2)		; 8a00
	ld	ac3,(3)		; 8f00
	ld	0,-128(ac2)	; 8280
	ld	ac1,127(ac3)	; 877f
	expect	1315
	ld	2,-129(ac2)
	endexpect
	expect	1320
	ld	ac3,128(ac3)
	endexpect

	ld	0,@.+2		; 9101
	ld	0,@.+2(pc)	; 9101
	ld	ac1,@.-2	; 95fd
	ld	2,@(2)		; 9a00
	ld	ac3,@(3)	; 9f00
	ld	0,@-128(ac2)	; 9280
	ld	ac1,@127(ac3)	; 977f
	expect	1315
	ld	2,@-129(ac2)
	endexpect
	expect	1320
	ld	ac3,@128(ac3)
	endexpect

	st	0,.+2		; a101
	st	ac1,.-2		; a5fd
	st	2,(2)		; aa00
	st	ac3,(3)		; af00
	st	0,-128(ac2)	; a280
	st	ac1,127(ac3)	; a77f
	expect	1315
	st	2,-129(ac2)
	endexpect
	expect	1320
	st	ac3,128(ac3)
	endexpect

	st	0,@.+2		; b101
	st	ac1,@.-2	; b5fd
	st	2,@(2)		; ba00
	st	ac3,@(3)	; bf00
	st	0,@-128(ac2)	; b280
	st	ac1,@127(ac3)	; b77f
	expect	1315
	st	2,@-129(ac2)
	endexpect
	expect	1320
	st	ac3,@128(ac3)
	endexpect

	add	0,.+2		; c101
	add	ac1,.-2		; c5fd
	add	2,(2)		; ca00
	add	ac3,(3)		; cf00
	add	0,-128(ac2)	; c280
	add	ac1,127(ac3)	; c77f
	expect	1315
	add	2,-129(ac2)
	endexpect
	expect	1320
	add	ac3,128(ac3)
	endexpect

	expect	1350
	add	0,@.+2
	endexpect

	sub	0,.+2		; d101
	sub	ac1,.-2		; d5fd
	sub	2,(2)		; da00
	sub	ac3,(3)		; df00
	sub	0,-128(ac2)	; d280
	sub	ac1,127(ac3)	; d77f
	expect	1315
	sub	2,-129(ac2)
	endexpect
	expect	1320
	sub	ac3,128(ac3)
	endexpect

	skg	0,.+2		; e101
	skg	ac1,.-2		; e5fd
	skg	2,(2)		; ea00
	skg	ac3,(3)		; ef00
	skg	0,-128(ac2)	; e280
	skg	ac1,127(ac3)	; e77f
	expect	1315
	skg	2,-129(ac2)
	endexpect
	expect	1320
	skg	ac3,128(ac3)
	endexpect

	skne	0,.+2		; f101
	skne	ac1,.-2		; f5fd
	skne	2,(2)		; fa00
	skne	ac3,(3)		; ff00
	skne	0,-128(ac2)	; f280
	skne	ac1,127(ac3)	; f77f
	expect	1315
	skne	2,-129(ac2)
	endexpect
	expect	1320
	skne	ac3,128(ac3)
	endexpect

	; we allow using immediate addressing via the
	; literal mechanism.  Note that using '@#addr'
	; effectively implements absolute addressing:

	ld	ac2,#x'1234
	ld	ac2,@#x'1234
	mpy	#x'1234
	expect	1350
	mpy	@#x'1234
	endexpect

	ascii	10,20,30,40
	expect	360
	ascii	10,20,30
	endexpect
	expect	360
	ascii	"Hello World"
	endexpect
	word	100,200,300
	word	"Hello World"

	ltorg

	; Skip instructions only skip one machine word,
	; and can therefore not skip extended instructions that
	; are two words long:

	isz	(ac2)
	expect	420
	mpy	.
	endexpect

	dsz	20
	expect	420
	div	(ac3)
	endexpect

	skg	ac0,(ac2)
	expect	420
	ldb	(ac3)
	endexpect

	skne	ac1,(ac2)
	expect	420
	stb	(ac3)
	endexpect

	skaz	0,x'80
	expect	420
	div	x'80
	endexpect

	aisz	ac2,1
	expect	420
	mpy	(ac3)
	endexpect
