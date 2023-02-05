	cpu	ipc-16
	page	0

	org	x'100

	boc	stfl,.+3	; 4002
	boc	req0,.+4	; 4103
	boc	psign,.+5	; 4204
	boc	bit0,.+6	; 4305
	boc	bit1,.+7	; 4406
	boc	nreq0,.+8	; 4507
	boc	bit2,.+9	; 4608
	boc	contin,.+10	; 4709
	boc	link,.-10	; 48f5
	boc	ien,.-9		; 49f6
	boc	carry,.-8	; 4af7
	boc	nsign,.-7	; 4bf8
	boc	ovf,.-6		; 4cf9
	boc	jc13,.-5	; 4dfa
	boc	jc14,.-4	; 4efb
	boc	jc15,.-3	; 4ffc
	expect	1370
	boc	link,.+130
	endexpect

	jmp	.+2		; 1901
	jmp	.-2		; 19fd
	jmp	(2)		; 1a00
	jmp	(3)		; 1b00
	jmp	-128(ac2)	; 1a80
	jmp	127(ac3)	; 1b7f
	expect	1315
	jmp	-129(ac2)
	endexpect
	expect	1320
	jmp	128(ac3)
	endexpect

	jmp	@.+2		; 9901
	jmp	@.-2		; 99fd
	jmp	@(2)		; 9a00
	jmp	@(3)		; 9b00
	jmp	@-128(ac2)	; 9a80
	jmp	@127(ac3)	; 9b7f
	expect	1315
	jmp	@-129(ac2)
	endexpect
	expect	1320
	jmp	@128(ac3)
	endexpect

	jsr	.+2		; 1501
	jsr	.-2		; 15fd
	jsr	(2)		; 1600
	jsr	(3)		; 1700
	jsr	-128(ac2)	; 1680
	jsr	127(ac3)	; 177f
	expect	1315
	jsr	-129(ac2)
	endexpect
	expect	1320
	jsr	128(ac3)
	endexpect

	jsr	@.+2		; 9501
	jsr	@.-2		; 95fd
	jsr	@(2)		; 9600
	jsr	@(3)		; 9700
	jsr	@-128(ac2)	; 9680
	jsr	@127(ac3)	; 977f
	expect	1315
	jsr	@-129(ac2)
	endexpect
	expect	1320
	jsr	@128(ac3)
	endexpect

	rts	2		; 8002
	expect	1315
	rts	-3
	endexpect
	expect	1320
	rts	256
	endexpect

	rti	2		; 7c02
	expect	1315
	rti	-3
	endexpect
	expect	1320
	rti	256
	endexpect

	; SKNE is one of the few instructions that
	; has same machine code as on IMP-16:

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

	skg	0,.+2		; 9d01
	skg	ac0,.-2		; 9dfd
	skg	0,(2)		; 9e00
	skg	ac0,(3)		; 9f00
	skg	0,-128(ac2)	; 9e80
	skg	ac0,127(ac3)	; 9f7f
	expect	1445
	skg	1,(2)
	endexpect
	expect	1445
	skg	ac2,(3)
	endexpect
	expect	1445
	skg	3,.+5
	endexpect
	expect	1315
	skg	0,-129(ac2)
	endexpect
	expect	1320
	skg	ac0,128(ac3)
	endexpect

	skaz	0,.+2		; b901
	skaz	ac0,.-2		; b9fd
	skaz	0,(2)		; ba00
	skaz	ac0,(3)		; bb00
	skaz	0,-128(ac2)	; ba80
	skaz	ac0,127(ac3)	; bb7f
	expect	1445
	skaz	1,(2)
	endexpect
	expect	1445
	skaz	ac2,(3)
	endexpect
	expect	1445
	skaz	3,.+5
	endexpect
	expect	1315
	skaz	0,-129(ac2)
	endexpect
	expect	1320
	skaz	ac0,128(ac3)
	endexpect

	isz	.+2		; 8d01
	isz	.-2		; 8dfd
	isz	(2)		; 8e00
	isz	(3)		; 8f00
	isz	-128(ac2)	; 8e80
	isz	127(ac3)	; 8f7f
	expect	1315
	isz	-129(ac2)
	endexpect
	expect	1320
	isz	128(ac3)
	endexpect

	dsz	.+2		; ad01
	dsz	.-2		; adfd
	dsz	(2)		; ae00
	dsz	(3)		; af00
	dsz	-128(ac2)	; ae80
	dsz	127(ac3)	; af7f
	expect	1315
	dsz	-129(ac2)
	endexpect
	expect	1320
	dsz	128(ac3)
	endexpect

	aisz	0,42		; 782a
	aisz	ac1,-42		; 79d6
	aisz	2,255		; 7aff
	aisz	ac3,-128	; 7b80
	expect	1320
	aisz	2,256
	endexpect
	expect	1315
	aisz	ac3,-129
	endexpect

	; untyped addresses (not from CODE segment)
	; result in using direct addressing if the
	; address is in the range 0..255:
	ld	0,127		; c07f

	ld	0,.+2		; c101
	ld	0,.+2(pc)	; c101
	ld	ac1,.-2		; c5fd
	ld	2,(2)		; ca00
	ld	ac3,(3)		; cf00
	ld	0,-128(ac2)	; c280
	ld	ac1,127(ac3)	; c77f
	expect	1315
	ld	2,-129(ac2)
	endexpect
	expect	1320
	ld	ac3,128(ac3)
	endexpect

	ld	0,@.+2		; a101
	ld	0,@.+2(pc)	; a101
	ld	ac0,@.-2	; a1fd
	ld	0,@(2)		; a200
	ld	ac0,@(3)	; a300
	ld	0,@-128(ac2)	; a280
	ld	ac0,@127(ac3)	; a37f
	expect	1445
	ld	1,@(2)
	endexpect
	expect	1445
	ld	ac2,@(3)
	endexpect
	expect	1445
	ld	3,@.+4
	endexpect
	expect	1315
	ld	0,@-129(ac2)
	endexpect
	expect	1320
	ld	0,@128(ac3)
	endexpect

	st	0,.+2		; d101
	st	ac1,.-2		; d5fd
	st	2,(2)		; da00
	st	ac3,(3)		; df00
	st	0,-128(ac2)	; d280
	st	ac1,127(ac3)	; d77f
	expect	1315
	st	2,-129(ac2)
	endexpect
	expect	1320
	st	ac3,128(ac3)
	endexpect

	st	0,@.+2		; b101
	st	ac0,@.-2	; b1fd
	st	0,@(2)		; b200
	st	ac0,@(3)	; b300
	st	0,@-128(ac2)	; b280
	st	ac0,@127(ac3)	; b37f
	expect	1445
	st	1,@(2)
	endexpect
	expect	1445
	st	ac2,@(3)
	endexpect
	expect	1445
	st	3,@.+4
	endexpect
	expect	1315
	st	0,@-129(ac2)
	endexpect
	expect	1320
	st	ac0,@128(ac3)
	endexpect

	lsex	0,.+2		; bd01
	lsex	ac0,.-2		; bdfd
	lsex	0,(2)		; be00
	lsex	ac0,(3)		; bf00
	lsex	0,-128(ac2)	; be80
	lsex	ac0,127(ac3)	; bf7f
	expect	1445
	lsex	1,(2)
	endexpect
	expect	1445
	lsex	ac2,(3)
	endexpect
	expect	1445
	lsex	3,.+5
	endexpect
	expect	1315
	lsex	0,-129(ac2)
	endexpect
	expect	1320
	lsex	ac0,128(ac3)
	endexpect

	and	0,.+2		; a901
	and	ac0,.-2		; a9fd
	and	0,(2)		; aa00
	and	ac0,(3)		; ab00
	and	0,-128(ac2)	; aa80
	and	ac0,127(ac3)	; ab7f
	expect	1445
	and	1,(2)
	endexpect
	expect	1445
	and	ac2,(3)
	endexpect
	expect	1445
	and	3,.+5
	endexpect
	expect	1315
	and	0,-129(ac2)
	endexpect
	expect	1320
	and	ac0,128(ac3)
	endexpect

	or	0,.+2		; a501
	or	ac0,.-2		; a5fd
	or	0,(2)		; a600
	or	ac0,(3)		; a700
	or	0,-128(ac2)	; a680
	or	ac0,127(ac3)	; a77f
	expect	1445
	or	1,(2)
	endexpect
	expect	1445
	or	ac2,(3)
	endexpect
	expect	1445
	or	3,.+5
	endexpect
	expect	1315
	or	0,-129(ac2)
	endexpect
	expect	1320
	or	ac0,128(ac3)
	endexpect

	add	0,.+2		; e101
	add	0,.+2(pc)	; e101
	add	ac1,.-2		; e5fd
	add	2,(2)		; ea00
	add	ac3,(3)		; ef00
	add	0,-128(ac2)	; e280
	add	ac1,127(ac3)	; e77f
	expect	1315
	add	2,-129(ac2)
	endexpect
	expect	1320
	add	ac3,128(ac3)
	endexpect

	subb	0,.+2		; 9101
	subb	ac0,.-2		; 91fd
	subb	0,(2)		; 9200
	subb	ac0,(3)		; 9300
	subb	0,-128(ac2)	; 9280
	subb	ac0,127(ac3)	; 937f
	expect	1445
	subb	1,(2)
	endexpect
	expect	1445
	subb	ac2,(3)
	endexpect
	expect	1445
	subb	3,.+5
	endexpect
	expect	1315
	subb	0,-129(ac2)
	endexpect
	expect	1320
	subb	ac0,128(ac3)
	endexpect

	deca	0,.+2		; 8901
	deca	ac0,.-2		; 89fd
	deca	0,(2)		; 8a00
	deca	ac0,(3)		; 8b00
	deca	0,-128(ac2)	; 8a80
	deca	ac0,127(ac3)	; 8b7f
	expect	1445
	deca	1,(2)
	endexpect
	expect	1445
	deca	ac2,(3)
	endexpect
	expect	1445
	deca	3,.+5
	endexpect
	expect	1315
	deca	0,-129(ac2)
	endexpect
	expect	1320
	deca	ac0,128(ac3)
	endexpect

	li	0,42		; 502a
	li	ac1,-42		; 51d6
	li	2,255		; 52ff
	li	ac3,-128	; 5380
	expect	1320
	li	2,256
	endexpect
	expect	1315
	li	ac3,-129
	endexpect

	rcpy	ac0,0		; 5c00
	rcpy	0,ac1		; 5d00
	rcpy	ac0,2		; 5e00
	rcpy	0,ac3		; 5f00
	rcpy	ac1,0		; 5c40
	rcpy	1,ac1		; 5d40
	rcpy	ac1,2		; 5e40
	rcpy	1,ac3		; 5f40
	rcpy	ac2,0		; 5c80
	rcpy	2,ac1		; 5d80
	rcpy	ac2,2		; 5e80
	rcpy	2,ac3		; 5f80
	rcpy	ac3,0		; 5cc0
	rcpy	3,ac1		; 5dc0
	rcpy	ac3,2		; 5ec0
	rcpy	3,ac3		; 5fc0

	rxch	ac0,0		; 6c00
	rxch	0,ac1		; 6d00
	rxch	ac0,2		; 6e00
	rxch	0,ac3		; 6f00
	rxch	ac1,0		; 6c40
	rxch	1,ac1		; 6d40
	rxch	ac1,2		; 6e40
	rxch	1,ac3		; 6f40
	rxch	ac2,0		; 6c80
	rxch	2,ac1		; 6d80
	rxch	ac2,2		; 6e80
	rxch	2,ac3		; 6f80
	rxch	ac3,0		; 6cc0
	rxch	3,ac1		; 6dc0
	rxch	ac3,2		; 6ec0
	rxch	3,ac3		; 6fc0

	xchrs	0		; 1c00
	xchrs	ac0		; 1c00
	xchrs	1		; 1d00
	xchrs	ac1		; 1d00
	xchrs	2		; 1e00
	xchrs	ac2		; 1e00
	xchrs	3		; 1f00
	xchrs	ac3		; 1f00

	cfr	0		; 0400
	cfr	ac0		; 0400
	cfr	1		; 0500
	cfr	ac1		; 0500
	cfr	2		; 0600
	cfr	ac2		; 0600
	cfr	3		; 0700
	cfr	ac3		; 0700

	crf	0		; 0800
	crf	ac0		; 0800
	crf	1		; 0900
	crf	ac1		; 0900
	crf	2		; 0a00
	crf	ac2		; 0a00
	crf	3		; 0b00
	crf	ac3		; 0b00

	push	0		; 6000
	push	ac0		; 6000
	push	1		; 6100
	push	ac1		; 6100
	push	2		; 6200
	push	ac2		; 6200
	push	3		; 6300
	push	ac3		; 6300

	pull	0		; 6400
	pull	ac0		; 6400
	pull	1		; 6500
	pull	ac1		; 6500
	pull	2		; 6600
	pull	ac2		; 6600
	pull	3		; 6700
	pull	ac3		; 6700

	pushf			; 0c00

	pullf			; 1000

	radd	ac0,0		; 6800
	radd	0,ac1		; 6900
	radd	ac0,2		; 6a00
	radd	0,ac3		; 6b00
	radd	ac1,0		; 6840
	radd	1,ac1		; 6940
	radd	ac1,2		; 6a40
	radd	1,ac3		; 6b40
	radd	ac2,0		; 6880
	radd	2,ac1		; 6980
	radd	ac2,2		; 6a80
	radd	2,ac3		; 6b80
	radd	ac3,0		; 68c0
	radd	3,ac1		; 69c0
	radd	ac3,2		; 6ac0
	radd	3,ac3		; 6bc0

	radc	ac0,0		; 7400
	radc	0,ac1		; 7500
	radc	ac0,2		; 7600
	radc	0,ac3		; 7700
	radc	ac1,0		; 7440
	radc	1,ac1		; 7540
	radc	ac1,2		; 7640
	radc	1,ac3		; 7740
	radc	ac2,0		; 7480
	radc	2,ac1		; 7580
	radc	ac2,2		; 7680
	radc	2,ac3		; 7780
	radc	ac3,0		; 74c0
	radc	3,ac1		; 75c0
	radc	ac3,2		; 76c0
	radc	3,ac3		; 77c0

	rand	ac0,0		; 5400
	rand	0,ac1		; 5500
	rand	ac0,2		; 5600
	rand	0,ac3		; 5700
	rand	ac1,0		; 5440
	rand	1,ac1		; 5540
	rand	ac1,2		; 5640
	rand	1,ac3		; 5740
	rand	ac2,0		; 5480
	rand	2,ac1		; 5580
	rand	ac2,2		; 5680
	rand	2,ac3		; 5780
	rand	ac3,0		; 54c0
	rand	3,ac1		; 55c0
	rand	ac3,2		; 56c0
	rand	3,ac3		; 57c0

	rxor	ac0,0		; 5800
	rxor	0,ac1		; 5900
	rxor	ac0,2		; 5a00
	rxor	0,ac3		; 5b00
	rxor	ac1,0		; 5840
	rxor	1,ac1		; 5940
	rxor	ac1,2		; 5a40
	rxor	1,ac3		; 5b40
	rxor	ac2,0		; 5880
	rxor	2,ac1		; 5980
	rxor	ac2,2		; 5a80
	rxor	2,ac3		; 5b80
	rxor	ac3,0		; 58c0
	rxor	3,ac1		; 59c0
	rxor	ac3,2		; 5ac0
	rxor	3,ac3		; 5bc0

	cai	0,42		; 702a
	cai	ac1,-42		; 71d6
	cai	2,255		; 72ff
	cai	ac3,-128	; 7380
	expect	1320
	cai	2,256
	endexpect
	expect	1315
	cai	ac3,-129
	endexpect

	shl	0,0		; 2800
	shl	ac1,1		; 2902
	shl	2,127		; 2afe
	shl	ac3,15		; 2b1e
	shl	0,0,0		; 2800
	shl	ac1,1,0		; 2902
	shl	2,127,0		; 2afe
	shl	ac3,15,0	; 2b1e
	shl	0,0,1		; 2801
	shl	ac1,1,1		; 2903
	shl	2,127,1		; 2aff
	shl	ac3,15,1	; 2b1f
	expect	1320
	shl	ac3,128
	endexpect
	expect	1320
	shl	ac2,10,2
	endexpect

	shr	0,0		; 2c00
	shr	ac1,1		; 2d02
	shr	2,127		; 2efe
	shr	ac3,15		; 2f1e
	shr	0,0,0		; 2c00
	shr	ac1,1,0		; 2d02
	shr	2,127,0		; 2efe
	shr	ac3,15,0	; 2f1e
	shr	0,0,1		; 2c01
	shr	ac1,1,1		; 2d03
	shr	2,127,1		; 2eff
	shr	ac3,15,1	; 2f1f
	expect	1320
	shr	ac3,128
	endexpect
	expect	1320
	shr	ac2,10,2
	endexpect

	rol	0,0		; 2000
	rol	ac1,1		; 2102
	rol	2,127		; 22fe
	rol	ac3,15		; 231e
	rol	0,0,0		; 2000
	rol	ac1,1,0		; 2102
	rol	2,127,0		; 22fe
	rol	ac3,15,0	; 231e
	rol	0,0,1		; 2001
	rol	ac1,1,1		; 2103
	rol	2,127,1		; 22ff
	rol	ac3,15,1	; 231f
	expect	1320
	rol	ac3,128
	endexpect
	expect	1320
	rol	ac2,10,2
	endexpect

	ror	0,0		; 2400
	ror	ac1,1		; 2502
	ror	2,127		; 26fe
	ror	ac3,15		; 271e
	ror	0,0,0		; 2400
	ror	ac1,1,0		; 2502
	ror	2,127,0		; 26fe
	ror	ac3,15,0	; 271e
	ror	0,0,1		; 2401
	ror	ac1,1,1		; 2503
	ror	2,127,1		; 26ff
	ror	ac3,15,1	; 271f
	expect	1320
	ror	ac3,128
	endexpect
	expect	1320
	ror	ac2,10,2
	endexpect

	halt			; 0000

	sflg	ie1		; 3180
	sflg	ie2		; 3280
	sflg	ie3		; 3380
	sflg	ie4		; 3480
	sflg	ie5		; 3580
	sflg	ov		; 3680
	sflg	cy		; 3780
	sflg	link		; 3880
	sflg	ien		; 3980
	sflg	byte		; 3a80
	sflg	f11		; 3b80
	sflg	f12		; 3c80
	sflg	f13		; 3d80
	sflg	f14		; 3e80

	pflg	ie1		; 3100
	pflg	ie2		; 3200
	pflg	ie3		; 3300
	pflg	ie4		; 3400
	pflg	ie5		; 3500
	pflg	ov		; 3600
	pflg	cy		; 3700
	pflg	link		; 3800
	pflg	ien		; 3900
	pflg	byte		; 3a00
	pflg	f11		; 3b00
	pflg	f12		; 3c00
	pflg	f13		; 3d00
	pflg	f14		; 3e00

	nop			; 5c00

	; PACE supports two different layouts of bottom page, selectable
	; via the BPS pin:

	assume	bps:1		; now first and last 128 bytes of 64K address space

	ld	ac2,127		; direct
	expect	1315
	ld	ac2,128		; no longer direct
	endexpect
	expect	1320
	ld	ac2,x'ff7f	; not direct
	endexpect
	ld	ac2,x'ff80	; direct
