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
	movb	next,%r0
	movw	*next,%r0
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

	bccb	.			; 53 FE
	bccb	.+2			; 53 00
	bccb	.+129			; 53 7F
	expect	1370
	bccb	.+130
	endexpect
	bccb	.-126			; 53 80
	expect	1370
	bccb	.-127
	endexpect

	bcch	.			; 52 FD FF
	bcch	.+3			; 52 00 00
	bcch	.+32770			; 52 FF 7F
	expect	1370
	bcch	.+32771
	endexpect
	bcch	.-32765			; 52 00 80
	expect	1370
	bcch	.-32766
	endexpect

	bcc	.			; 53 FE
	bcc	.+2			; 53 00
	bcc	.+129			; 53 7F
	bcc	.+130			; 52 7F 00
	bcc	.+32770			; 52 FF 7F
	expect	1370
	bcc	.+32771
	endexpect
	bcc	.-126			; 53 80
	bcc	.-127			; 52 7E FF
	bcc	.-32765			; 52 00 80
	expect	1370
	bcc	.-32766
	endexpect

	bcsb	.			; 5B FE
	bcsb	.+2			; 5B 00
	bcsb	.+129			; 5B 7F
	expect	1370
	bcsb	.+130
	endexpect
	bcsb	.-126			; 5B 80
	expect	1370
	bcsb	.-127
	endexpect

	bcsh	.			; 5A FD FF
	bcsh	.+3			; 5A 00 00
	bcsh	.+32770			; 5A FF 7F
	expect	1370
	bcsh	.+32771
	endexpect
	bcsh	.-32765			; 5A 00 80
	expect	1370
	bcsh	.-32766
	endexpect

	bcs	.			; 5B FE
	bcs	.+2			; 5B 00
	bcs	.+129			; 5B 7F
	bcs	.+130			; 5A 7F 00
	bcs	.+32770			; 5A FF 7F
	expect	1370
	bcs	.+32771
	endexpect
	bcs	.-126			; 5B 80
	bcs	.-127			; 5A 7E FF
	bcs	.-32765			; 5A 00 80
	expect	1370
	bcs	.-32766
	endexpect

	beb	.			; 7F FE
	beb	.+2			; 7F 00
	beb	.+129			; 7F 7F
	expect	1370
	beb	.+130
	endexpect
	beb	.-126			; 7F 80
	expect	1370
	beb	.-127
	endexpect

	beh	.			; 7E FD FF
	beh	.+3			; 7E 00 00
	beh	.+32770			; 7E FF 7F
	expect	1370
	beh	.+32771
	endexpect
	beh	.-32765			; 7E 00 80
	expect	1370
	beh	.-32766
	endexpect

	be	.			; 7F FE
	be	.+2			; 7F 00
	be	.+129			; 7F 7F
	be	.+130			; 7E 7F 00
	be	.+32770			; 7E FF 7F
	expect	1370
	be	.+32771
	endexpect
	be	.-126			; 7F 80
	be	.-127			; 7E 7E FF
	be	.-32765			; 7E 00 80
	expect	1370
	be	.-32766
	endexpect

	beub	.			; 6F FE
	beub	.+2			; 6F 00
	beub	.+129			; 6F 7F
	expect	1370
	beub	.+130
	endexpect
	beub	.-126			; 6F 80
	expect	1370
	beub	.-127
	endexpect

	beuh	.			; 6E FD FF
	beuh	.+3			; 6E 00 00
	beuh	.+32770			; 6E FF 7F
	expect	1370
	beuh	.+32771
	endexpect
	beuh	.-32765			; 6E 00 80
	expect	1370
	beuh	.-32766
	endexpect

	beu	.			; 6F FE
	beu	.+2			; 6F 00
	beu	.+129			; 6F 7F
	beu	.+130			; 6E 7F 00
	beu	.+32770			; 6E FF 7F
	expect	1370
	beu	.+32771
	endexpect
	beu	.-126			; 6F 80
	beu	.-127			; 6E 7E FF
	beu	.-32765			; 6E 00 80
	expect	1370
	beu	.-32766
	endexpect

	bgb	.			; 47 FE
	bgb	.+2			; 47 00
	bgb	.+129			; 47 7F
	expect	1370
	bgb	.+130
	endexpect
	bgb	.-126			; 47 80
	expect	1370
	bgb	.-127
	endexpect

	bgh	.			; 46 FD FF
	bgh	.+3			; 46 00 00
	bgh	.+32770			; 46 FF 7F
	expect	1370
	bgh	.+32771
	endexpect
	bgh	.-32765			; 46 00 80
	expect	1370
	bgh	.-32766
	endexpect

	bg	.			; 47 FE
	bg	.+2			; 47 00
	bg	.+129			; 47 7F
	bg	.+130			; 46 7F 00
	bg	.+32770			; 46 FF 7F
	expect	1370
	bg	.+32771
	endexpect
	bg	.-126			; 47 80
	bg	.-127			; 46 7E FF
	bg	.-32765			; 46 00 80
	expect	1370
	bg	.-32766
	endexpect

	bgeb	.			; 43 FE
	bgeb	.+2			; 43 00
	bgeb	.+129			; 43 7F
	expect	1370
	bgeb	.+130
	endexpect
	bgeb	.-126			; 43 80
	expect	1370
	bgeb	.-127
	endexpect

	bgeh	.			; 42 FD FF
	bgeh	.+3			; 42 00 00
	bgeh	.+32770			; 42 FF 7F
	expect	1370
	bgeh	.+32771
	endexpect
	bgeh	.-32765			; 42 00 80
	expect	1370
	bgeh	.-32766
	endexpect

	bge	.			; 43 FE
	bge	.+2			; 43 00
	bge	.+129			; 43 7F
	bge	.+130			; 42 7F 00
	bge	.+32770			; 42 FF 7F
	expect	1370
	bge	.+32771
	endexpect
	bge	.-126			; 43 80
	bge	.-127			; 42 7E FF
	bge	.-32765			; 42 00 80
	expect	1370
	bge	.-32766
	endexpect

	bgeub	.			; 53 FE
	bgeub	.+2			; 53 00
	bgeub	.+129			; 53 7F
	expect	1370
	bgeub	.+130
	endexpect
	bgeub	.-126			; 53 80
	expect	1370
	bgeub	.-127
	endexpect

	bgeuh	.			; 52 FD FF
	bgeuh	.+3			; 52 00 00
	bgeuh	.+32770			; 52 FF 7F
	expect	1370
	bgeuh	.+32771
	endexpect
	bgeuh	.-32765			; 52 00 80
	expect	1370
	bgeuh	.-32766
	endexpect

	bgeu	.			; 53 FE
	bgeu	.+2			; 53 00
	bgeu	.+129			; 53 7F
	bgeu	.+130			; 52 7F 00
	bgeu	.+32770			; 52 FF 7F
	expect	1370
	bgeu	.+32771
	endexpect
	bgeu	.-126			; 53 80
	bgeu	.-127			; 52 7E FF
	bgeu	.-32765			; 52 00 80
	expect	1370
	bgeu	.-32766
	endexpect

	bgub	.			; 57 FE
	bgub	.+2			; 57 00
	bgub	.+129			; 57 7F
	expect	1370
	bgub	.+130
	endexpect
	bgub	.-126			; 57 80
	expect	1370
	bgub	.-127
	endexpect

	bguh	.			; 56 FD FF
	bguh	.+3			; 56 00 00
	bguh	.+32770			; 56 FF 7F
	expect	1370
	bguh	.+32771
	endexpect
	bguh	.-32765			; 56 00 80
	expect	1370
	bguh	.-32766
	endexpect

	bgu	.			; 57 FE
	bgu	.+2			; 57 00
	bgu	.+129			; 57 7F
	bgu	.+130			; 56 7F 00
	bgu	.+32770			; 56 FF 7F
	expect	1370
	bgu	.+32771
	endexpect
	bgu	.-126			; 57 80
	bgu	.-127			; 56 7E FF
	bgu	.-32765			; 56 00 80
	expect	1370
	bgu	.-32766
	endexpect

	bitb	%r0,{uhalf}%r1		; 3B 40 E2 41
	bith	*$0xff,%r3		; 3A EF FF 00 00 00 43
	bitw	bit(%r3),(%r0)		; 38 C3 07 50

	blb	.			; 4B FE
	blb	.+2			; 4B 00
	blb	.+129			; 4B 7F
	expect	1370
	blb	.+130
	endexpect
	blb	.-126			; 4B 80
	expect	1370
	blb	.-127
	endexpect

	blh	.			; 4A FD FF
	blh	.+3			; 4A 00 00
	blh	.+32770			; 4A FF 7F
	expect	1370
	blh	.+32771
	endexpect
	blh	.-32765			; 4A 00 80
	expect	1370
	blh	.-32766
	endexpect

	bl	.			; 4B FE
	bl	.+2			; 4B 00
	bl	.+129			; 4B 7F
	bl	.+130			; 4A 7F 00
	bl	.+32770			; 4A FF 7F
	expect	1370
	bl	.+32771
	endexpect
	bl	.-126			; 4B 80
	bl	.-127			; 4A 7E FF
	bl	.-32765			; 4A 00 80
	expect	1370
	bl	.-32766
	endexpect

	bleb	.			; 4F FE
	bleb	.+2			; 4F 00
	bleb	.+129			; 4F 7F
	expect	1370
	bleb	.+130
	endexpect
	bleb	.-126			; 4F 80
	expect	1370
	bleb	.-127
	endexpect

	bleh	.			; 4E FD FF
	bleh	.+3			; 4E 00 00
	bleh	.+32770			; 4E FF 7F
	expect	1370
	bleh	.+32771
	endexpect
	bleh	.-32765			; 4E 00 80
	expect	1370
	bleh	.-32766
	endexpect

	ble	.			; 4F FE
	ble	.+2			; 4F 00
	ble	.+129			; 4F 7F
	ble	.+130			; 4E 7F 00
	ble	.+32770			; 4E FF 7F
	expect	1370
	ble	.+32771
	endexpect
	ble	.-126			; 4F 80
	ble	.-127			; 4E 7E FF
	ble	.-32765			; 4E 00 80
	expect	1370
	ble	.-32766
	endexpect

	bleub	.			; 5F FE
	bleub	.+2			; 5F 00
	bleub	.+129			; 5F 7F
	expect	1370
	bleub	.+130
	endexpect
	bleub	.-126			; 5F 80
	expect	1370
	bleub	.-127
	endexpect

	bleuh	.			; 5E FD FF
	bleuh	.+3			; 5E 00 00
	bleuh	.+32770			; 5E FF 7F
	expect	1370
	bleuh	.+32771
	endexpect
	bleuh	.-32765			; 5E 00 80
	expect	1370
	bleuh	.-32766
	endexpect

	bleu	.			; 5F FE
	bleu	.+2			; 5F 00
	bleu	.+129			; 5F 7F
	bleu	.+130			; 5E 7F 00
	bleu	.+32770			; 5E FF 7F
	expect	1370
	bleu	.+32771
	endexpect
	bleu	.-126			; 5F 80
	bleu	.-127			; 5E 7E FF
	bleu	.-32765			; 5E 00 80
	expect	1370
	bleu	.-32766
	endexpect

	blub	.			; 5B FE
	blub	.+2			; 5B 00
	blub	.+129			; 5B 7F
	expect	1370
	blub	.+130
	endexpect
	blub	.-126			; 5B 80
	expect	1370
	blub	.-127
	endexpect

	bluh	.			; 5A FD FF
	bluh	.+3			; 5A 00 00
	bluh	.+32770			; 5A FF 7F
	expect	1370
	bluh	.+32771
	endexpect
	bluh	.-32765			; 5A 00 80
	expect	1370
	bluh	.-32766
	endexpect

	blu	.			; 5B FE
	blu	.+2			; 5B 00
	blu	.+129			; 5B 7F
	blu	.+130			; 5A 7F 00
	blu	.+32770			; 5A FF 7F
	expect	1370
	blu	.+32771
	endexpect
	blu	.-126			; 5B 80
	blu	.-127			; 5A 7E FF
	blu	.-32765			; 5A 00 80
	expect	1370
	blu	.-32766
	endexpect

	bneb	.			; 77 FE
	bneb	.+2			; 77 00
	bneb	.+129			; 77 7F
	expect	1370
	bneb	.+130
	endexpect
	bneb	.-126			; 77 80
	expect	1370
	bneb	.-127
	endexpect

	bneh	.			; 76 FD FF
	bneh	.+3			; 76 00 00
	bneh	.+32770			; 76 FF 7F
	expect	1370
	bneh	.+32771
	endexpect
	bneh	.-32765			; 76 00 80
	expect	1370
	bneh	.-32766
	endexpect

	bne	.			; 77 FE
	bne	.+2			; 77 00
	bne	.+129			; 77 7F
	bne	.+130			; 76 7F 00
	bne	.+32770			; 76 FF 7F
	expect	1370
	bne	.+32771
	endexpect
	bne	.-126			; 77 80
	bne	.-127			; 76 7E FF
	bne	.-32765			; 76 00 80
	expect	1370
	bne	.-32766
	endexpect

	bneub	.			; 67 FE
	bneub	.+2			; 67 00
	bneub	.+129			; 67 7F
	expect	1370
	bneub	.+130
	endexpect
	bneub	.-126			; 67 80
	expect	1370
	bneub	.-127
	endexpect

	bneuh	.			; 66 FD FF
	bneuh	.+3			; 66 00 00
	bneuh	.+32770			; 66 FF 7F
	expect	1370
	bneuh	.+32771
	endexpect
	bneuh	.-32765			; 66 00 80
	expect	1370
	bneuh	.-32766
	endexpect

	bneu	.			; 67 FE
	bneu	.+2			; 67 00
	bneu	.+129			; 67 7F
	bneu	.+130			; 66 7F 00
	bneu	.+32770			; 66 FF 7F
	expect	1370
	bneu	.+32771
	endexpect
	bneu	.-126			; 67 80
	bneu	.-127			; 66 7E FF
	bneu	.-32765			; 66 00 80
	expect	1370
	bneu	.-32766
	endexpect

	bpt				; 2E

	brb	.			; 7B FE
	brb	.+2			; 7B 00
	brb	.+129			; 7B 7F
	expect	1370
	brb	.+130
	endexpect
	brb	.-126			; 7B 80
	expect	1370
	brb	.-127
	endexpect

	brh	.			; 7A FD FF
	brh	.+3			; 7A 00 00
	brh	.+32770			; 7A FF 7F
	expect	1370
	brh	.+32771
	endexpect
	brh	.-32765			; 7A 00 80
	expect	1370
	brh	.-32766
	endexpect

	br	.			; 7B FE
	br	.+2			; 7B 00
	br	.+129			; 7B 7F
	br	.+130			; 7A 7F 00
	br	.+32770			; 7A FF 7F
	expect	1370
	br	.+32771
	endexpect
	br	.-126			; 7B 80
	br	.-127			; 7A 7E FF
	br	.-32765			; 7A 00 80
	expect	1370
	br	.-32766
	endexpect

	bsbb	.			; 37 FE
	bsbb	.+2			; 37 00
	bsbb	.+129			; 37 7F
	expect	1370
	bsbb	.+130
	endexpect
	bsbb	.-126			; 37 80
	expect	1370
	bsbb	.-127
	endexpect

	bsbh	.			; 36 FD FF
	bsbh	.+3			; 36 00 00
	bsbh	.+32770			; 36 FF 7F
	expect	1370
	bsbh	.+32771
	endexpect
	bsbh	.-32765			; 36 00 80
	expect	1370
	bsbh	.-32766
	endexpect

	bsb	.			; 37 FE
	bsb	.+2			; 37 00
	bsb	.+129			; 37 7F
	bsb	.+130			; 36 7F 00
	bsb	.+32770			; 36 FF 7F
	expect	1370
	bsb	.+32771
	endexpect
	bsb	.-126			; 37 80
	bsb	.-127			; 36 7E FF
	bsb	.-32765			; 36 00 80
	expect	1370
	bsb	.-32766
	endexpect

	bvcb	.			; 63 FE
	bvcb	.+2			; 63 00
	bvcb	.+129			; 63 7F
	expect	1370
	bvcb	.+130
	endexpect
	bvcb	.-126			; 63 80
	expect	1370
	bvcb	.-127
	endexpect

	bvch	.			; 62 FD FF
	bvch	.+3			; 62 00 00
	bvch	.+32770			; 62 FF 7F
	expect	1370
	bvch	.+32771
	endexpect
	bvch	.-32765			; 62 00 80
	expect	1370
	bvch	.-32766
	endexpect

	bvc	.			; 63 FE
	bvc	.+2			; 63 00
	bvc	.+129			; 63 7F
	bvc	.+130			; 62 7F 00
	bvc	.+32770			; 62 FF 7F
	expect	1370
	bvc	.+32771
	endexpect
	bvc	.-126			; 63 80
	bvc	.-127			; 62 7E FF
	bvc	.-32765			; 62 00 80
	expect	1370
	bvc	.-32766
	endexpect

	bvsb	.			; 6B FE
	bvsb	.+2			; 6B 00
	bvsb	.+129			; 6B 7F
	expect	1370
	bvsb	.+130
	endexpect
	bvsb	.-126			; 6B 80
	expect	1370
	bvsb	.-127
	endexpect

	bvsh	.			; 6A FD FF
	bvsh	.+3			; 6A 00 00
	bvsh	.+32770			; 6A FF 7F
	expect	1370
	bvsh	.+32771
	endexpect
	bvsh	.-32765			; 6A 00 80
	expect	1370
	bvsh	.-32766
	endexpect

	bvs	.			; 6B FE
	bvs	.+2			; 6B 00
	bvs	.+129			; 6B 7F
	bvs	.+130			; 6A 7F 00
	bvs	.+32770			; 6A FF 7F
	expect	1370
	bvs	.+32771
	endexpect
	bvs	.-126			; 6B 80
	bvs	.-127			; 6A 7E FF
	bvs	.-32765			; 6A 00 80
	expect	1370
	bvs	.-32766
	endexpect

	call	-(3*4)(%sp),func1	; 2C CC F4 CF 01
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

	extfb	&10,&4,L1,%r0		; CF 0A 04 CF 0E 40
	extfh	&10,&4,L1,%r0		; CE 0A 04 CF 08 40
	extfw	&10,&4,L1,%r0		; CC 0A 04 CF 02 40
L1:

	extop	0x2f			; 14 2F

	incb	4(%r2)			; 93 C2 04
	inch	%r0			; 92 40
	incw	(%r1)			; 90 51

	insfb	&11,&8,%r1,%r0		; CB 0B 08 41 40
	insfh	&11,&8,%r1,%r0		; CA 0B 08 41 40
	insfw	&11,&8,%r1,%r0		; C8 0B 08 41 40

L12:
	jmp	L12			; 24 CF FE

	jsb	error			; 34 CF 01
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

	movtrw	x,%r0			; 0C CF 02 04
x:

	retg				; 30 45

	; --------------
	; pseudo instructions just like DB, DW, DD:

	byte	1,2 dup(3), 4 dup(5)	; 01 03 03 05 05 05 05
	half	1,2 dup(3), 4 dup(5)	; 01 00 03 00 03 00 05 00 05 00 05 00 05 00
	word	1,2 dup(3), 4 dup(5)	; 01 00 00 00 03 00 00 00 03 00 00 00 05 00 00 00 05 00 00 00 05 00 00 00 05 00 00 00
