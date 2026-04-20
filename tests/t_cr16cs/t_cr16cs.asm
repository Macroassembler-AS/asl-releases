	cpu	cr16c
	page	0
	assume	sr:1

	org	0x100000

	; In small mode, only the lower half of R12/R13/RA is available.
	; Instead, there are more 32-bit register pairs:

	movd	$4,(r1,r0)		; 5440
	movd	$4,(r2,r1)		; 5441
	movd	$4,(r3,r2)		; 5442
	movd	$4,(r4,r3)		; 5443
	movd	$4,(r5,r4)		; 5444
	movd	$4,(r6,r5)		; 5445
	movd	$4,(r7,r6)		; 5446
	movd	$4,(r8,r7)		; 5447
	movd	$4,(r9,r8)		; 5448
	movd	$4,(r10,r9)		; 5449
	movd	$4,(r11,r10)		; 544A
	movd	$4,(r12_l,r11)		; 544B
	movd	$4,(r13_l,r12_l)	; 544C
	movd	$4,(ra_l,r13_l)		; 544D
	movd	$4,(sp_l,ra_l)		; 544E
	movd	$4,(sp_h,sp_l)		; 544F
	expect	1445,1445
	movd	$4,r12
	movd	$4,(r12)
	endexpect
	expect	1445,1445
	movd	$4,r13
	movd	$4,(r13)
	endexpect
	expect	1445,1445
	movd	$4,ra
	movd	$4,(ra)
	endexpect
	expect	1445,1445
	movd	$4,sp
	movd	$4,(sp)
	endexpect

	loadb	0xc000(r1,r0),r4		; BF40 C000
	loadb	0xc000(r2,r1),r4		; BF41 C000
	loadb	0xc000(r3,r2),r4		; BF42 C000
	loadb	0xc000(r4,r3),r4		; BF43 C000
	loadb	0xc000(r5,r4),r4		; BF44 C000
	loadb	0xc000(r6,r5),r4		; BF45 C000
	loadb	0xc000(r7,r6),r4		; BF46 C000
	loadb	0xc000(r8,r7),r4		; BF47 C000
	loadb	0xc000(r9,r8),r4		; BF48 C000
	loadb	0xc000(r10,r9),r4		; BF49 C000
	loadb	0xc000(r11,r10),r4		; BF4A C000
	loadb	0xc000(r12_l,r11),r4		; BF4B C000
	loadb	0xc000(r13_l,r12_l),r4		; BF4C C000
	loadb	0xc000(ra_l,r13_l),r4		; BF4D C000
	loadb	0xc000(sp_l,ra_l),r4		; BF4E C000
	loadb	0xc000(sp_h,sp_l),r4		; BF4F C000
	expect	1438
	loadb	0xc000(r12),r4
	endexpect
	expect	1438
	loadb	0xc000(r13),r4
	endexpect
	expect	1438
	loadb	0xc000(ra),r4
	endexpect
	expect	1438
	loadb	0xc000(sp),r4
	endexpect

	; Since the upper halves of R12/R3 are unavailable, no index modes either:

	expect	1350
	loadb	[r12]0x1000,r4
	endexpect
	expect	1350
	loadb	[r13]0x1000(r1,r0),r4
	endexpect

	; in small mode, (prp) means (reg).  This means we have some
	; displacement length variants more for disp(reg):

	; disp0(prp),reg		
	loadb	(r12_l),r4		; BE4C
	; disp14(prp),reg
	loadb	1(r12_l),r4		; 864C 0041
	loadb	0x3fff(r12_l),r4	; 867C FF4F
	; disp20(reg),reg
	loadb	0x4000(r12_l),r4	; 0012 404C 4000

	; imm4,disp14(prp)
	storb	$7,(r12_l)		; 860C 0070
	storb	$7,1(r12_l)		; 860C 0071
	storb	$7,0x3fff(r12_l)	; 863C FF7F
	; imm4,disp20(reg)
	storb	$7,0x4000(r12_l)	; 0012 007C 4000

	; disp14(prp)
	tbitw	$7,(r13_l)		; 7ACD 0070
	tbitw	$7,1(r13_l)		; 7ACD 0071
	tbitw	$7,0x3fff(r13_l)	; 7AFD FF7F
	; disp20(reg)
	tbitw	$7,0x4000(r13_l)	; 0011 C07D 4000

