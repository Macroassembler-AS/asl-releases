	cpu	we32100
	page	0

	org	0x8000

	; iterate through addressing modes

	; Absolute:
	movb	$0x100,%r0		; 87 7F 00 01 00 00 40
	movb	$0x12345678,r0		; 87 7F 78 56 34 12 40

	; Absolute Deferred:
	movb	*$0x2e00,%r1		; 87 EF 00 2E 00 00 41
	movb	*$0x12345678,r0		; 87 EF 78 56 34 12 40

	; Byte Displacement: 
	movb	6(%r1),%r0		; 87 C1 06 40
	movb	-128(r1),r0		; 87 C1 80 40
	movb	127(r1),r0		; 87 C1 7F 40

	; Byte Displacement Deferred:
	movb	*0x30(%r2),%r3		; 87 D2 30 43
	movb	*-128(r1),r0		; 87 D1 80 40
	movb	*127(r1),r0		; 87 D1 7F 40

	; Halfword Displacement:
	movb	0x1101(%r2),%r8		; 87 A2 01 11 48
	movb	-129(r1),r0		; 87 A1 7F FF 40
	movb	128(r1),r0		; 87 A1 80 00 40
	movb	-32768(r1),r0		; 87 A1 00 80 40
	movb	32767(r1),r0		; 87 A1 FF 7F 40[B (Halfword displacement)

	; Halfword Displacement Deferred:
	movb	*0x200(%r2),%r6		; 87 B2 00 02 46
	movb	*-129(r1),r0		; 87 B1 7F FF 40
	movb	*128(r1),r0		; 87 B1 80 00 40
	movb	*-32768(r1),r0		; 87 B1 00 80 40
	movb	*32767(r1),r0		; 87 B1 FF 7F 40

	; Word Displacement:
	movb	0x112234(%r2),%r4	; 87 82 34 22 11 00 44
	movb	-32769(r1),r0		; 87 81 FF 7F FF FF 40
	movb	32768(r1),r0		; 87 81 00 80 00 00 40
	movb	-2147483648(r1),r0	; 87 81 00 00 00 80 40
	movb	2147483647(r1),r0	; 87 81 FF FF FF 7F 40

	; Word Displacement Deferred:
	movb	*0x20304050(%r2),%r0	; 87 92 50 40 30 20 40
	movb	*-32769(r1),r0		; 87 91 FF 7F FF FF 40
	movb	*32768(r1),r0		; 87 91 00 80 00 00 40
	movb	*-2147483648(r1),r0	; 87 91 00 00 00 80 40
	movb	*2147483647(r1),r0	; 87 91 FF FF FF 7F 40

	; AP Short Offset:
	movw	4(%ap),%r3		; 84 74 43
	movb	0(ap),r0		; 87 70 40
	movb	14(ap),r0		; 87 7E 40
	movb	15(ap),r0		; 87 CA 0F 40 (use byte displacement instead)

	; FP Short Offset:
	movw	12(%fp),%r0		; 84 6C 40
	movb	0(fp),r0		; 87 60 40
	movb	14(fp),r0		; 87 6E 40
	movb	15(fp),r0		; 87 C9 0F 40 (use byte displacement instead)

	; Byte Immediate:
	movw	&0x40,%r6		; 84 6F 40 46
	movb	&0x78,r0		; 87 6F 78 40

	; Halfword Immediate:
	movw	&0x1234,%r2		; 84 5F 34 12 42
	movh	&0x5678,r0		; 86 5F 78 56 40

	; Word Immediate:
	movw	&0x12345678,%r3		; 84 4F 78 56 34 12 43
	movw	&0x12345678,r0		; 84 4F 78 56 34 12 40

	; Positive Literal:
	movb	&4,%r4			; 87 04 44
	movb	&0x0,r0			; 87 00 40
	movb	&0x3f,r0		; 87 3F 40
	movb	&0x40,r0		; 87 6F 40 40 (immediate, not literal)
	movh	&0x0,r0			; 86 00 40
	movh	&0x3f,r0		; 86 3F 40
	movh	&0x40,r0		; 86 6F 40 40 ((byte) immediate, not literal)
	movh	&0x7f,r0		; 86 6F 7F 40 ((byte) immediate, not literal)
	movh	&0x80,r0		; 86 5F 80 00 ((halfword) immediate, not literal)
	movw	&0x0,r0			; 84 00 40
	movw	&0x3f,r0		; 84 3F 40
	movw	&0x40,r0		; 84 6F 40 40 ((byte) immediate, not literal)
	movw	&0x7f,r0		; 84 6F 7F 40 ((byte) immediate, not literal)
	movw	&0x80,r0		; 84 5F 80 00 40 ((halfword) immediate, not literal)
	movw	&0x7fff,r0		; 84 5F FF 7F 40 ((halfword) immediate, not literal)
	movw	&0x8000,r0		; 84 4F 00 80 00 00 40 ((word) immediate, not literal)

	; Negative Literal:
	movb	&-1,%r0			; 87 FF 40
	movb	&-1,r0
	movb	&0xff,r0
	movb	&-16,r0			; 87 F0 40
	movb	&0xf0,r0
	movb	&-17,r0			; 87 6F EF 40 (immediate, not literal)
	movb	&0xef,r0
	movh	&-1,r0			; 86 FF 40
	movh	&0xffff,r0
	movh	&-16,r0			; 86 F0 40
	movh	&0xfff0,r0
	movh	&-17,r0			; 86 6F EF 40 ((byte) immediate, not literal)
	movh	&0xffef,r0
	movh	&-128,r0		; 86 6F 80 40 ((byte) immediate, not literal)
	movh	&0xff80,r0
	movh	&-129,r0		; 86 5F 7F FF 40 ((halfword) immediate, not literal)
	movh	&0xff7f,r0
	movw	&-1,r0			; 84 FF 40
	movw	&0xffffffff,r0
	movw	&-16,r0			; 84 F0 40
	movw	&0xfffffff0,r0
	movw	&-17,r0			; 84 6F EF 40 ((byte) immediate, not literal)
	movw	&0xffffffef,r0
	movw	&-128,r0		; 84 6F 80 40 ((byte) immediate, not literal)
	movw	&0xffffff80,r0
	movw	&-129,r0		; 84 5F 7F FF 40 ((halfword) immediate, not literal)
	movw	&0xffffff7f,r0
	movw	&-32768,r0		; 84 5F 00 80 40 ((halfword) immediate, not literal)
	movw	&0xffff8000,r0
	movw	&-32769,r0		; 84 4F FF 7F FF FF 40 ((word) immediate, not literal)
	movw	&0xffff7fff,r0

	; Register
	movb	%r0,%ap			; 87 40 4A
	movb	r1,r0			; 87 41 40

	; Register Deferred
	movh	(%r2),%r1		; 86 52 41
	movb	(r1),r0			; 87 51 40
	movb	(pc),r0			; 87 CF 00 40 (use byte displacement instead of 0x5f)

	; Expanded Operand
	movb	{sbyte}%r0,{uhalf}4(%r1)	; 87 E7 40 E2 C1 04
	movw	{sbyte}(r4),r0		; 84 E7 54 40
	movw	{half}(r4),r0		; 84 E6 54 40
	movw	{shalf}(r4),r0		; 84 E6 54 40
	movw	{word}(r4),r0		; 84 E4 54 40
	movw	{sword}(r4),r0		; 84 E4 54 40
	movw	{byte}(r4),r0		; 84 E3 54 40
	movw	{ubyte}(r4),r0		; 84 E3 54 40
	movw	{uhalf}(r4),r0		; 84 E2 54 40
	movw	{uword}(r4),r0		; 84 E0 54 40

	; Treat plain address as PC-relative:
	movb	next,%r0		; 87 CF 08 40
	movw	*next,%r0		; 87 DF 04 40
next:

	; dest operand must not be immediate
	expect	1350
	movw	r0,&0
	endexpect

	
	; -------------

result	label	0x12345678
resulth	label	0x1234567a
resultw	label	0x12345678
first	label	0x12345600
last	label	0x123456ff
real	label	0x123456ea
N1	label	4
N2	label	8
A	label	0x1000
bit	equ	7

	addb2	$0x100,%r0		; 9F 7F 00 01 00 00 40
	addh2	%r0,%r3			; 9E 40 43
	addw2	4(%r3),*$0x110		; 9C C3 04 EF 10 01 00 00

	addb3	%r0,%r3,%r5		; DF 40 43 45
	addh3	4(%r2),*$0x110,%r3	; DE C2 04 EF 10 01 00 00 43
	addw3	*$0x1f0,4(%r1),%r0	; DC EF F0 01 00 00 C1 04 40

	alsw3	&2,%r0,%r0		; C0 02 40 40

	andb2	&7,6(%r1)		; BB 07 C1 06
	andh2	%r0,*$result		; BA 40 EF 78 56 34 12
	andw2	(%r1),%r4		; B8 51 44

	andb3	&0x27,*$0x300,%r6	; FB 27 EF 00 03 00 00 46
	andh3	0x31(%r5),%r0,%r1	; FA C5 31 40 41
	andw3	%r2,%r1,%r0		; F8 42 41 40

	arsb3	&7,%r5,%r5		; C7 07 45 45
	arsh3	&2,%r0,%r0		; C6 02 40 40
	arsw3	&5,(%r3),%r3		; C4 05 53 43

	bccb	.-2			; 53 FE
	bccb	.			; 53 00
	bccb	.+127			; 53 7F
	expect	1370
	bccb	.+128
	endexpect
	bccb	.-128			; 53 80
	expect	1370
	bccb	.-129
	endexpect

	bcch	.-3			; 52 FD FF
	bcch	.			; 52 00 00
	bcch	.+32767			; 52 FF 7F
	expect	1370
	bcch	.+32768
	endexpect
	bcch	.-32768			; 52 00 80
	expect	1370
	bcch	.-32769
	endexpect

	bcc	.-2			; 53 FE
	bcc	.			; 53 00
	bcc	.+127			; 53 7F
	bcc	.+128			; 52 80 00
	bcc	.+32767			; 52 FF 7F
	expect	1370
	bcc	.+32768
	endexpect
	bcc	.-128			; 53 80
	bcc	.-129			; 52 7F FF
	bcc	.-32768			; 52 00 80
	expect	1370
	bcc	.-32769
	endexpect

	bcsb	.-2			; 5B FE
	bcsb	.			; 5B 00
	bcsb	.+127			; 5B 7F
	expect	1370
	bcsb	.+128
	endexpect
	bcsb	.-128			; 5B 80
	expect	1370
	bcsb	.-129
	endexpect

	bcsh	.-3			; 5A FD FF
	bcsh	.			; 5A 00 00
	bcsh	.+32767			; 5A FF 7F
	expect	1370
	bcsh	.+32768
	endexpect
	bcsh	.-32768			; 5A 00 80
	expect	1370
	bcsh	.-32769
	endexpect

	bcs	.-2			; 5B FE
	bcs	.			; 5B 00
	bcs	.+127			; 5B 7F
	bcs	.+128			; 5A 80 00
	bcs	.+32767			; 5A FF 7F
	expect	1370
	bcs	.+32768
	endexpect
	bcs	.-128			; 5B 80
	bcs	.-129			; 5A 7F FF
	bcs	.-32768			; 5A 00 80
	expect	1370
	bcs	.-32769
	endexpect

	beb	.-2			; 7F FE
	beb	.			; 7F 00
	beb	.+127			; 7F 7F
	expect	1370
	beb	.+128
	endexpect
	beb	.-128			; 7F 80
	expect	1370
	beb	.-129
	endexpect

	beh	.-3			; 7E FD FF
	beh	.			; 7E 00 00
	beh	.+32767			; 7E FF 7F
	expect	1370
	beh	.+32768
	endexpect
	beh	.-32768			; 7E 00 80
	expect	1370
	beh	.-32769
	endexpect

	be	.-2			; 7F FE
	be	.			; 7F 00
	be	.+127			; 7F 7F
	be	.+128			; 7E 80 00
	be	.+32767			; 7E FF 7F
	expect	1370
	be	.+32768
	endexpect
	be	.-128			; 7F 80
	be	.-129			; 7E 7F FF
	be	.-32768			; 7E 00 80
	expect	1370
	be	.-32769
	endexpect

	beub	.-2			; 6F FE
	beub	.			; 6F 00
	beub	.+127			; 6F 7F
	expect	1370
	beub	.+128
	endexpect
	beub	.-128			; 6F 80
	expect	1370
	beub	.-129
	endexpect

	beuh	.-3			; 6E FD FF
	beuh	.			; 6E 00 00
	beuh	.+32767			; 6E FF 7F
	expect	1370
	beuh	.+32768
	endexpect
	beuh	.-32768			; 6E 00 80
	expect	1370
	beuh	.-32769
	endexpect

	beu	.-2			; 6F FE
	beu	.			; 6F 00
	beu	.+127			; 6F 7F
	beu	.+128			; 6E 80 00
	beu	.+32767			; 6E FF 7F
	expect	1370
	beu	.+32771
	endexpect
	beu	.-128			; 6F 80
	beu	.-129			; 6E 7F FF
	beu	.-32768			; 6E 00 80
	expect	1370
	beu	.-32769
	endexpect

	bgb	.-2			; 47 FE
	bgb	.			; 47 00
	bgb	.+127			; 47 7F
	expect	1370
	bgb	.+128
	endexpect
	bgb	.-128			; 47 80
	expect	1370
	bgb	.-129
	endexpect

	bgh	.-3			; 46 FD FF
	bgh	.			; 46 00 00
	bgh	.+32767			; 46 FF 7F
	expect	1370
	bgh	.+32768
	endexpect
	bgh	.-32768			; 46 00 80
	expect	1370
	bgh	.-32769
	endexpect

	bg	.-2			; 47 FE
	bg	.			; 47 00
	bg	.+127			; 47 7F
	bg	.+128			; 46 80 00
	bg	.+32767			; 46 FF 7F
	expect	1370
	bg	.+32768
	endexpect
	bg	.-128			; 47 80
	bg	.-129			; 46 7F FF
	bg	.-32768			; 46 00 80
	expect	1370
	bg	.-32769
	endexpect

	bgeb	.-2			; 43 FE
	bgeb	.			; 43 00
	bgeb	.+127			; 43 7F
	expect	1370
	bgeb	.+128
	endexpect
	bgeb	.-128			; 43 80
	expect	1370
	bgeb	.-129
	endexpect

	bgeh	.-3			; 42 FD FF
	bgeh	.			; 42 00 00
	bgeh	.+32767			; 42 FF 7F
	expect	1370
	bgeh	.+32768
	endexpect
	bgeh	.-32768			; 42 00 80
	expect	1370
	bgeh	.-32769
	endexpect

	bge	.-2			; 43 FE
	bge	.			; 43 00
	bge	.+127			; 43 7F
	bge	.+128			; 42 80 00
	bge	.+32767			; 42 FF 7F
	expect	1370
	bge	.+32768
	endexpect
	bge	.-128			; 43 80
	bge	.-129			; 42 7F FF
	bge	.-32768			; 42 00 80
	expect	1370
	bge	.-32769
	endexpect

	bgeub	.-2			; 53 FE
	bgeub	.			; 53 00
	bgeub	.+127			; 53 7F
	expect	1370
	bgeub	.+128
	endexpect
	bgeub	.-128			; 53 80
	expect	1370
	bgeub	.-129
	endexpect

	bgeuh	.-3			; 52 FD FF
	bgeuh	.			; 52 00 00
	bgeuh	.+32767			; 52 FF 7F
	expect	1370
	bgeuh	.+32768
	endexpect
	bgeuh	.-32768			; 52 00 80
	expect	1370
	bgeuh	.-32769
	endexpect

	bgeu	.-2			; 53 FE
	bgeu	.			; 53 00
	bgeu	.+127			; 53 7F
	bgeu	.+128			; 52 80 00
	bgeu	.+32767			; 52 FF 7F
	expect	1370
	bgeu	.+32768
	endexpect
	bgeu	.-128			; 53 80
	bgeu	.-129			; 52 7F FF
	bgeu	.-32768			; 52 00 80
	expect	1370
	bgeu	.-32769
	endexpect

	bgub	.-2			; 57 FE
	bgub	.			; 57 00
	bgub	.+127			; 57 7F
	expect	1370
	bgub	.+128
	endexpect
	bgub	.-128			; 57 80
	expect	1370
	bgub	.-129
	endexpect

	bguh	.-3			; 56 FD FF
	bguh	.			; 56 00 00
	bguh	.+32767			; 56 FF 7F
	expect	1370
	bguh	.+32768
	endexpect
	bguh	.-32768			; 56 00 80
	expect	1370
	bguh	.-32769
	endexpect

	bgu	.-2			; 57 FE
	bgu	.			; 57 00
	bgu	.+127			; 57 7F
	bgu	.+128			; 56 80 00
	bgu	.+32767			; 56 FF 7F
	expect	1370
	bgu	.+32768
	endexpect
	bgu	.-128			; 57 80
	bgu	.-129			; 56 7F FF
	bgu	.-32768			; 56 00 80
	expect	1370
	bgu	.-32769
	endexpect

	bitb	%r0,{uhalf}%r1		; 3B 40 E2 41
	bith	*$0xff,%r3		; 3A EF FF 00 00 00 43
	bitw	bit(%r3),(%r0)		; 38 C3 07 50

	blb	.-2			; 4B FE
	blb	.			; 4B 00
	blb	.+127			; 4B 7F
	expect	1370
	blb	.+128
	endexpect
	blb	.-128			; 4B 80
	expect	1370
	blb	.-129
	endexpect

	blh	.-3			; 4A FD FF
	blh	.			; 4A 00 00
	blh	.+32767			; 4A FF 7F
	expect	1370
	blh	.+32768
	endexpect
	blh	.-32768			; 4A 00 80
	expect	1370
	blh	.-32769
	endexpect

	bl	.-2			; 4B FE
	bl	.			; 4B 00
	bl	.+127			; 4B 7F
	bl	.+128			; 4A 80 00
	bl	.+32767			; 4A FF 7F
	expect	1370
	bl	.+32768
	endexpect
	bl	.-128			; 4B 80
	bl	.-129			; 4A 7F FF
	bl	.-32768			; 4A 00 80
	expect	1370
	bl	.-32769
	endexpect

	bleb	.-2			; 4F FE
	bleb	.			; 4F 00
	bleb	.+127			; 4F 7F
	expect	1370
	bleb	.+128
	endexpect
	bleb	.-128			; 4F 80
	expect	1370
	bleb	.-129
	endexpect

	bleh	.-3			; 4E FD FF
	bleh	.			; 4E 00 00
	bleh	.+32767			; 4E FF 7F
	expect	1370
	bleh	.+32768
	endexpect
	bleh	.-32768			; 4E 00 80
	expect	1370
	bleh	.-32769
	endexpect

	ble	.-2			; 4F FE
	ble	.			; 4F 00
	ble	.+127			; 4F 7F
	ble	.+128			; 4E 80 00
	ble	.+32767			; 4E FF 7F
	expect	1370
	ble	.+32768
	endexpect
	ble	.-128			; 4F 80
	ble	.-129			; 4E 7F FF
	ble	.-32768			; 4E 00 80
	expect	1370
	ble	.-32769
	endexpect

	bleub	.-2			; 5F FE
	bleub	.			; 5F 00
	bleub	.+127			; 5F 7F
	expect	1370
	bleub	.+128
	endexpect
	bleub	.-128			; 5F 80
	expect	1370
	bleub	.-129
	endexpect

	bleuh	.-3			; 5E FD FF
	bleuh	.			; 5E 00 00
	bleuh	.+32767			; 5E FF 7F
	expect	1370
	bleuh	.+32768
	endexpect
	bleuh	.-32768			; 5E 00 80
	expect	1370
	bleuh	.-32769
	endexpect

	bleu	.-2			; 5F FE
	bleu	.			; 5F 00
	bleu	.+127			; 5F 7F
	bleu	.+128			; 5E 80 00
	bleu	.+32767			; 5E FF 7F
	expect	1370
	bleu	.+32768
	endexpect
	bleu	.-128			; 5F 80
	bleu	.-129			; 5E 7F FF
	bleu	.-32768			; 5E 00 80
	expect	1370
	bleu	.-32769
	endexpect

	blub	.-2			; 5B FE
	blub	.			; 5B 00
	blub	.+127			; 5B 7F
	expect	1370
	blub	.+128
	endexpect
	blub	.-128			; 5B 80
	expect	1370
	blub	.-129
	endexpect

	bluh	.-3			; 5A FD FF
	bluh	.			; 5A 00 00
	bluh	.+32767			; 5A FF 7F
	expect	1370
	bluh	.+32768
	endexpect
	bluh	.-32768			; 5A 00 80
	expect	1370
	bluh	.-32769
	endexpect

	blu	.-2			; 5B FE
	blu	.			; 5B 00
	blu	.+127			; 5B 7F
	blu	.+128			; 5A 8 00
	blu	.+32767			; 5A FF 7F
	expect	1370
	blu	.+32768
	endexpect
	blu	.-128			; 5B 80
	blu	.-129			; 5A 7F FF
	blu	.-32768			; 5A 00 80
	expect	1370
	blu	.-32769
	endexpect

	bneb	.-2			; 77 FE
	bneb	.			; 77 00
	bneb	.+127			; 77 7F
	expect	1370
	bneb	.+128
	endexpect
	bneb	.-128			; 77 80
	expect	1370
	bneb	.-129
	endexpect

	bneh	.-3			; 76 FD FF
	bneh	.			; 76 00 00
	bneh	.+32767			; 76 FF 7F
	expect	1370
	bneh	.+32768
	endexpect
	bneh	.-32768			; 76 00 80
	expect	1370
	bneh	.-32769
	endexpect

	bne	.-2			; 77 FE
	bne	.			; 77 00
	bne	.+127			; 77 7F
	bne	.+128			; 76 80 00
	bne	.+32767			; 76 FF 7F
	expect	1370
	bne	.+32768
	endexpect
	bne	.-128			; 77 80
	bne	.-129			; 76 7F FF
	bne	.-32768			; 76 00 80
	expect	1370
	bne	.-32769
	endexpect

	bneub	.-2			; 67 FE
	bneub	.			; 67 00
	bneub	.+127			; 67 7F
	expect	1370
	bneub	.+128
	endexpect
	bneub	.-128			; 67 80
	expect	1370
	bneub	.-129
	endexpect

	bneuh	.-3			; 66 FD FF
	bneuh	.			; 66 00 00
	bneuh	.+32767			; 66 FF 7F
	expect	1370
	bneuh	.+32768
	endexpect
	bneuh	.-32768			; 66 00 80
	expect	1370
	bneuh	.-32769
	endexpect

	bneu	.-2			; 67 FE
	bneu	.			; 67 00
	bneu	.+127			; 67 7F
	bneu	.+128			; 66 80 00
	bneu	.+32767			; 66 FF 7F
	expect	1370
	bneu	.+32768
	endexpect
	bneu	.-128			; 67 80
	bneu	.-129			; 66 7F FF
	bneu	.-32768			; 66 00 80
	expect	1370
	bneu	.-32769
	endexpect

	bpt				; 2E

	brb	.-2			; 7B FE
	brb	.			; 7B 00
	brb	.+127			; 7B 7F
	expect	1370
	brb	.+128
	endexpect
	brb	.-128			; 7B 80
	expect	1370
	brb	.-129
	endexpect

	brh	.-3			; 7A FD FF
	brh	.			; 7A 00 00
	brh	.+32767			; 7A FF 7F
	expect	1370
	brh	.+32768
	endexpect
	brh	.-32768			; 7A 00 80
	expect	1370
	brh	.-32769
	endexpect

	br	.-2			; 7B FE
	br	.			; 7B 00
	br	.+127			; 7B 7F
	br	.+128			; 7A 80 00
	br	.+32767			; 7A FF 7F
	expect	1370
	br	.+32768
	endexpect
	br	.-128			; 7B 80
	br	.-129			; 7A 7F FF
	br	.-32768			; 7A 00 80
	expect	1370
	br	.-32769
	endexpect

	bsbb	.-2			; 37 FE
	bsbb	.			; 37 00
	bsbb	.+127			; 37 7F
	expect	1370
	bsbb	.+128
	endexpect
	bsbb	.-128			; 37 80
	expect	1370
	bsbb	.-129
	endexpect

	bsbh	.-3			; 36 FD FF
	bsbh	.			; 36 00 00
	bsbh	.+32767			; 36 FF 7F
	expect	1370
	bsbh	.+32768
	endexpect
	bsbh	.-32768			; 36 00 80
	expect	1370
	bsbh	.-32769
	endexpect

	bsb	.-2			; 37 FE
	bsb	.			; 37 00
	bsb	.+127			; 37 7F
	bsb	.+128			; 36 80 00
	bsb	.+32767			; 36 FF 7F
	expect	1370
	bsb	.+32768
	endexpect
	bsb	.-128			; 37 80
	bsb	.-129			; 36 7F FF
	bsb	.-32768			; 36 00 80
	expect	1370
	bsb	.-32769
	endexpect

	bvcb	.-2			; 63 FE
	bvcb	.			; 63 00
	bvcb	.+127			; 63 7F
	expect	1370
	bvcb	.+128
	endexpect
	bvcb	.-128			; 63 80
	expect	1370
	bvcb	.-129
	endexpect

	bvch	.-3			; 62 FD FF
	bvch	.			; 62 00 00
	bvch	.+32767			; 62 FF 7F
	expect	1370
	bvch	.+32768
	endexpect
	bvch	.-32768			; 62 00 80
	expect	1370
	bvch	.-32769
	endexpect

	bvc	.-2			; 63 FE
	bvc	.			; 63 00
	bvc	.+127			; 63 7F
	bvc	.+128			; 62 80 00
	bvc	.+32767			; 62 FF 7F
	expect	1370
	bvc	.+32768
	endexpect
	bvc	.-128			; 63 80
	bvc	.-129			; 62 7F FF
	bvc	.-32768			; 62 00 80
	expect	1370
	bvc	.-32769
	endexpect

	bvsb	.-2			; 6B FE
	bvsb	.			; 6B 00
	bvsb	.+127			; 6B 7F
	expect	1370
	bvsb	.+128
	endexpect
	bvsb	.-128			; 6B 80
	expect	1370
	bvsb	.-129
	endexpect

	bvsh	.-3			; 6A FD FF
	bvsh	.			; 6A 00 00
	bvsh	.+32767			; 6A FF 7F
	expect	1370
	bvsh	.+32768
	endexpect
	bvsh	.-32768			; 6A 00 80
	expect	1370
	bvsh	.-32769
	endexpect

	bvs	.-2			; 6B FE
	bvs	.			; 6B 00
	bvs	.+127			; 6B 7F
	bvs	.+128			; 6A 80 00
	bvs	.+32767			; 6A FF 7F
	expect	1370
	bvs	.+32768
	endexpect
	bvs	.-128			; 6B 80
	bvs	.-129			; 6A 7F FF
	bvs	.-32768			; 6A 00 80
	expect	1370
	bvs	.-32769
	endexpect

	call	-(3*4)(%sp),func1	; 2C CC F4 CF 05
func1:

	cflush				; 27

	clrb	*$0x300			; 83 EF 00 03 00 00
	clrh	%r1			; 82 41
	clrw	(%r0)			; 80 50

	cmpb	&10,%r0			; 3F 0A 40
	cmph	(%r0),(%r1)		; 3E 50 51
	cmpw	*$0x12F7,%r2		; 3C EF F7 12 00 00 42

	decb	4(%fp)			; 97 64
	dech	$result			; 96 7F 78 56 34 12
	decw	*$last			; 94 EF FF 56 34 12

	divb2	&40,%r6			; AF 28 46
	divh2	4(%r3),(%r4)		; AE C3 04 54
	divw2	$first,$last		; AC 7F 00 56 34 12 7F FF 56 34 12

	divb3	&0x30,%r3,12(%ap)	; EF 30 43 7C
	divh3	&0x3030,(%r2),5(%r2)	; EE 5F 30 30 52 C2 05
	divw3	&0x304050,(%r1),4(%r1)	; EC 4F 50 40 30 00 51 C1 04 

	extfb	&10,&4,L1,%r0		; CF 0A 04 CF 12 40
	extfh	&10,&4,L1,%r0		; CE 0A 04 CF 0C 40
	extfw	&10,&4,L1,%r0		; CC 0A 04 CF 06 40
L1:

	extop	0x2f			; 14 2F

	incb	4(%r2)			; 93 C2 04
	inch	%r0			; 92 40
	incw	(%r1)			; 90 51

	insfb	&11,&8,%r1,%r0		; CB 0B 08 41 40
	insfh	&11,&8,%r1,%r0		; CA 0B 08 41 40
	insfw	&11,&8,%r1,%r0		; C8 0B 08 41 40

L12:
	jmp	L12			; 24 CF 00

	jsb	error			; 34 CF 03
error:
	expect	1350
	jsb	{uhalf}error		; no expanded addressing modes
	endexpect

	llsb3	&5,%r5,%r5		; D3 05 45 45
	llsh3	&2,%r0,%r0		; D2 02 40 40
	llsw3	&7,(%r6),%r4		; D0 07 56 44

	lrsw3	&0x11,%r0,%r0		; D4 11 40 40

	mcomb	%r0,%r1			; 8B 40 41
	mcomh	%r0,%r1			; 8A 40 41
	mcomw	%r0,%r1			; 88 40 41

	mnegb	%r0,%r1			; 8F 40 41
	mnegh	%r0,%r1			; 8E 40 41
	mnegw	%r0,%r1			; 8C 40 41

	modb2	&40,%r3			; A7 28 43
	modh2	4(%r3),%r3		; A6 C3 04 43
	modw2	%r0,*$result		; A4 40 EF 78 56 34 12

	modb3	&40,%r3,0x1101(%r2)	; E7 28 43 A2 01 11 
	modh3	%r3,$real,%r3		; E6 43 7F EA 56 34 12 43
	modw3	4(%r2),*$0x34,%r0	; E4 C2 04 EF 34 00 00 00 40

	movb	%r0,%r1			; 87 40 41
	movh	%r0,%r1			; 86 40 41
	movw	%r0,%r1			; 84 40 41

	movaw	4(%r0),r1		; 04 C0 04 41

	movblw				; 30 19

	mulb2	%r2,{sbyte}4(%r6)	; AB 42 E7 C6 04
	mulh2	%r2,{sbyte}4(%r6)	; AA 42 E7 C6 04
	mulw2	%r2,{sbyte}4(%r6)	; A8 42 E7 C6 04

	mulb3	%r3,*$0x1004,%r4	; EB 43 EF 04 10 00 00 44
	mulh3	%r3,*$0x1004,%r4	; EA 43 EF 04 10 00 00 44
	mulw3	%r3,*$0x1004,%r4	; E8 43 EF 04 10 00 00 44

	mverno				; 30 09

	nop				; 70
	nop2	0x12			; 73 12
	nop3	0x12,0x34		; 72 12 34

	orb2	&12,4(%fp)		; B3 0C 64
	orh2	%r0,4(%r0)		; B2 40 C0 04
	orw2	%r3,$result		; B0 43 7F 78 56 34 12

	orb3	&16,*$0x304,%r0		; F3 10 EF 04 03 00 00 40
	orh3	%r1,4(%r1),%r1		; F2 41 C1 04 41
	orw3	%r2,%r3,%r1		; F0 42 43 41

	popw	(%r2)			; 20 52
	expect	1350
	popw	{sbyte}(%r2)		; no expanded addressing modes
	endexpect

	pusha	0x14(%r6)		; E0 C6 14
	expect	1350
	pusha	{ubyte}0x14(%r6)	; no expanded addressing modes
	endexpect

	pushw	(%r2)			; A0 52
	expect	1350
	pushw	{uhalf}(%r2)		; no expanded addressing modes
	endexpect

	rcc				; 50

	rcs				; 58

	reql				; 7C
	reqlu				; 6C

	restore	%r3			; 18 43

	ret				; 18

	rgeq				; 40

	rgequ				; 50

	rgtr				; 44

	rgtru				; 54

	rleq				; 4C

	rlequ				; 5C

	rlss				; 48

	rlssu				; 58

	rneq				; 74
	rnequ				; 64

	rotw	&0x404,%r0,%r0		; D8 5F 04 04 40 40

	rsb				; 78

	rvc				; 60

	rvs				; 68

	save	%r3			; 10 43

	spop	0xffffffff		; 32 FF FF FF FF

	spoprs	0xf379ffff,*$0xff37	; 22 FF FF 79 F3 EF 37 FF 00 00
	spoprd	0xffffffff,(%r3)	; 02 FF FF FF FF 53
	spoprt	0x00000000,(%r4)	; 06 00 00 00 00 54

	spops2	0xff,4(%r0),12(%r6)	; 23 FF 00 00 00 C0 04 C6 0C
	spopd2	0xfff,(%r3),12(%r6)	; 03 FF 0F 00 00 53 C6 0C
	spopt2	0xfe,(%r0),12(%r6)	; 07 FE 00 00 00 50 C6 0C

	spopws	0x00,(%r0)		; 33 00 00 00 00 50
	spopwd	0x0f,(%r1)		; 13 0F 00 00 00 51
	spopwt	0x1000,4(%r2)		; 17 00 10 00 00 C2 04

	strcpy				; 30 35

	strend				; 30 1F

	subb2	%r6,*0x30(%r2)		; BF 46 D2 30
	subh2	%r0,$resulth		; BE 40 7F 7A 56 34 12
	subw2	%r3,$resultw		; BC 43 7F 78 56 34 12

	subb3	%r3,*$0x1005,%r2	; FF 43 EF 05 10 00 00 42
	subh3	%r1,%r3,%r0		; FE 41 43 40
	subw3	$N1,$N2,$result		; FC 7F 04 00 00 00 7F 08 00 00 00 7F 78 56 34 12

	swapbi	A			; 1F AF .. ..
	swaphi	A			; 1E AF .. ..
	swapwi	A			; 1C AF .. ..
	expect	1350
	swapwi	{shalf}A		; no expanded addressing modes
	endexpect

	tstb	r2			; 2B 42
	tsth	14(%r2)			; 2A C2 0E
	tstw	&0x1234			; 28 5F 34 12

	xorb2	&40,4(%r4)		; B7 28 C4 04
	xorh2	%r1,$result		; B6 41 7F 78 56 34 12
	xorw2	4(%r1),$result		; B4 C1 04 7F 78 56 34 12

	xorb3	&4,*12(%r3),*$0x400	; F7 04 D3 0C EF 00 04 00 00
	xorh3	%r1,4(%r1),%r0		; F6 41 C1 04 40
	xorw3	%r0,%r1,%r3		; F4 40 41 43

	; -------------
	; registers not writable in non-kernel mode

	movw	%psw,r0			; 84 4B 40
	movw	%pcbp,r0		; 84 4D 40
	movw	%isp,r0			; 84 4E 40
	expect	1443
	movw	%r0,%psw
	endexpect
	expect	1443
	movw	(%r0),isp
	endexpect
	expect	1443
	movw	&0x12345678,pcbp
	endexpect

	; -------------

	expect	50
	callps				; 30 AC
	endexpect

	execmode kernel
	
	disvjmp				; 30 13

	enbvjmp				; 30 0D

	intack				; 30 2F

	retps				; 30 C8

	wait				; 2F

	gate				; 30 61

	movtrw	x,%r0			; 0C CF 04 04
x:

	retg				; 30 45

	; --------------
	; Pseudo instructions just like DB, DW, DD:
        ; Note that data is stored in big-endian order, while operands
	; in addressing modes are little endian:

	byte	1,2 dup(3), 4 dup(5)	; 01 03 03 05 05 05 05
	half	1,2 dup(3), 4 dup(5)	; 00 01 00 03 00 03 00 05 00 05 00 05 00 05
	word	1,2 dup(3), 4 dup(5)	; 00 00 00 01 00 00 00 03 00 00 00 03 00 00 00 05 00 00 00 05 00 00 00 05 00 00 00 05
