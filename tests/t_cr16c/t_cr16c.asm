		cpu	cr16c
		page	0

		; NOTE: CFG.SR assumed to be 0, so (prp) = (rrp)

reg0		reg	r0
reg1		reg	r1
reg2		reg	r2
reg3		reg	r3
reg4		reg	r4
reg5		reg	r5
reg6		reg	r6
reg7		reg	r7
reg8		reg	r8
reg9		reg	r9
reg10		reg	r10
reg11		reg	r11
reg12_l		reg	r12_l
reg13_l		reg	r13_l
reg14_l		reg	ra_l
reg15_l		reg	sp_l
reg15_h		reg	sp_h

reg12		reg	r12
reg13		reg	r13
reg14		reg	ra
reg15		reg	sp

test_no_regname	macro	arg
		if	mompass>1
		expect	1010
		endif
false_reg	reg	arg
		if	mompass>1
		endexpect
		endif
		endm

		test_no_regname	rx
		test_no_regname	r14
		test_no_regname	r15
		test_no_regname	abc
		test_no_regname	xyz
		test_no_regname	lmaa
		test_no_regname	r5_l
		test_no_regname	r12_h
		test_no_regname	ra_h
		test_no_regname	sp_x
		test_no_regname	sp_y
		test_no_regname	r16
		test_no_regname	_r12
		test_no_regname	0123
num		equ	0123

		page	0

		supmode	off
		expect	50
		lpr	ra_l,dbs
		endexpect

		org	0x100000
		supmode	on

		expect	1445
		addw	sp_l,sp_h	; SP_H only allowed in combination with SP_L
		endexpect

		addb	$1,r7		; 3017
		addb	$8,r9		; 3089
		addb	$9,r3		; 30B3 0009
		addb	$10,r5		; 30A5
		addb	$11,r6		; 30B6 000B
		addb	$12,r4		; 30C4
		addb	$-1,r2		; 3092
		addb	$0xff,r2	;  "
		addb	$15,r1		; 30F1
		addb	$16,r8		; 30B8 0010
		addb	$-2,r10		; 30BA 00FE
		addb	$-128,r11	; 30BB 0080
		addb	$0xfe,r12_l	; 30BC 00FE
		addb	r4,r9		; 3149

		addcb	$1,r7		; 3417
		addcb	$8,r9		; 3489
		addcb	$9,r3		; 34B3 0009
		addcb	$10,r5		; 34A5
		addcb	$11,r6		; 34B6 000B
		addcb	$12,r4		; 34C4
		addcb	$-1,r2		; 3492
		addcb	$0xff,r2	;  "
		addcb	$15,r1		; 34F1
		addcb	$16,r8		; 34B8 0010
		addcb	$-2,r10		; 34BA 00FE
		addcb	$-128,r11	; 34BB 0080
		addcb	$0xfe,r12_l	; 34BC 00FE
		addcb	r4,r9		; 3549

		addcw	$1,r7		; 3617
		addcw	$8,r9		; 3689
		addcw	$9,r3		; 36B3 0009
		addcw	$10,r5		; 36A5
		addcw	$11,r6		; 36B6 000B
		addcw	$12,r4		; 36C4
		addcw	$-1,r2		; 3692
		addcw	$0xffff,r2	;  "
		addcw	$15,r1		; 36F1
		addcw	$16,r8		; 36B8 0010
		addcw	$-2,r10		; 36BA FFFE
		addcw	$-32768,r11	; 36BB 8000
		addcw	$0xfffe,r12_l	; 36BC FFFE
		addcw	r4,r9		; 3749

		; ADDD has imm4/16, imm20, and imm32 variant:
		expect	1350
		addd	$1,(r8,r6)		; register pair not consecutive
		endexpect
		expect	1350
		addd	$1,(r13,r12)		; no pair of 16 bit registers
		endexpect
		expect	1350
		addd	$1,(r12,r11)		; mixture of 32/16 bit registers
		endexpect
		addd	$1,(r8,r7)		; 6017
		addd	$8,(r10,r9)		; 6089
		addd	$9,(r4,r3)		; 60B3 0009
		addd	$10,(r6,r5)		; 60A5
		addd	$11,(r7,r6)		; 60B6 000B
		addd	$12,(r5,r4)		; 60C4
		addd	$-1,(r3,r2)		; 6092
		addd	$0xffffffff,(r3,r2)	;  "
		addd	$15,(r2,r1)		; 60F1
		addd	$16,(r9,r8)		; 60B8 0010
		addd	$-2,(r11,r10)		; 60BA FFFE
		addd	$-128,(r12_l,r11)	; 60BB FF80
		addd	$0xfe,r12		; 60BC 00FE
		addd	$-32768,(r12_l,r11)	; 60BB 8000
		addd	$0x7fff,r12		; 60BC 7FFF
		addd	$-32769,(r12_l,r11)	; 04BF 8001
		addd	$0x8000,r12		; 04C0 8000
		addd	$-524288,(r12_l,r11)	; 04B8 0000
		addd	$0x7FFFF,r12		; 04C7 FFFF
		addd	$-524289,(r12_l,r11)	; 002B FFFF FFF7
		addd	$0x80000,r12		; 002C 0000 0008
		addd	$-2147483648,(r12_l,r11); 002B 0000 8000
		addd	$0xffefffff,r12		; 002C FFFF FFEF
		addd	(r5,r4),(r10,r9)	; 6149

		addub	$1,r7		; 2C17
		addub	$8,r9		; 2C89
		addub	$9,r3		; 2CB3 0009
		addub	$10,r5		; 2CA5
		addub	$11,r6		; 2CB6 000B
		addub	$12,r4		; 2CC4
		addub	$-1,r2		; 2C92
		addub	$0xff,r2	;  "
		addub	$15,r1		; 2CF1
		addub	$16,r8		; 2CB8 0010
		addub	$-2,r10		; 2CBA 00FE
		addub	$-128,r11	; 2CBB 0080
		addub	$0xfe,r12_l	; 2CBC 00FE
		addub	r4,r9		; 2D49

		adduw	$1,r7		; 2E17
		adduw	$8,r9		; 2E89
		adduw	$9,r3		; 2EB3 0009
		adduw	$10,r5		; 2EA5
		adduw	$11,r6		; 2EB6 000B
		adduw	$12,r4		; 2EC4
		adduw	$-1,r2		; 2E92
		adduw	$0xffff,r2	;  "
		adduw	$15,r1		; 2EF1
		adduw	$16,r8		; 2EB8 0010
		adduw	$-2,r10		; 2EBA FFFE
		adduw	$-32768,r11	; 2EBB 8000
		adduw	$0xfffe,r12_l	; 2EBC FFFE
		adduw	r4,r9		; 2F49

		addw	$1,r7		; 3217
		addw	$8,r9		; 3289
		addw	$9,r3		; 32B3 0009
		addw	$10,r5		; 32A5
		addw	$11,r6		; 32B6 000B
		addw	$12,r4		; 32C4
		addw	$-1,r2		; 3292
		addw	$0xffff,r2	;  "
		addw	$15,r1		; 32F1
		addw	$16,r8		; 32B8 0010
		addw	$-2,r10		; 32BA FFFE
		addw	$-32768,r11	; 32BB 8000
		addw	$0xfffe,r12_l	; 32BC FFFE
		addw	r4,r9		; 3349

		andb	$1,r7		; 2017
		andb	$8,r9		; 2089
		andb	$9,r3		; 20B3 0009
		andb	$10,r5		; 20A5
		andb	$11,r6		; 20B6 000B
		andb	$12,r4		; 20C4
		andb	$-1,r2		; 2092
		andb	$0xff,r2	;  "
		andb	$15,r1		; 20F1
		andb	$16,r8		; 20B8 0010
		andb	$-2,r10		; 20BA 00FE
		andb	$-128,r11	; 20BB 0080
		andb	$0xfe,r12_l	; 20BC 00FE
		andb	r4,r9		; 2149

		; ANDD only has imm32 variant:
		expect	1350
		andd	$1,(r8,r6)		; register pair not consecutive
		endexpect
		expect	1350
		andd	$1,(r13,r12)		; no pair of 16 bit registers
		endexpect
		expect	1350
		andd	$1,(r12,r11)		; mixture of 32/16 bit registers
		endexpect
		andd	$1,(r8,r7)		; 0047 0001 0000
		andd	$8,(r10,r9)		; 0049 0008 0000
		andd	$9,(r4,r3)		; 0043 0009 0000
		andd	$10,(r6,r5)		; 0045 000A 0000
		andd	$11,(r7,r6)		; 0046 000B 0000
		andd	$12,(r5,r4)		; 0044 000C 0000
		andd	$-1,(r3,r2)		; 0042 FFFF FFFF
		andd	$0xffffffff,(r3,r2)	;  "
		andd	$15,(r2,r1)		; 0041 000F 0000
		andd	$16,(r9,r8)		; 0048 0010 0000
		andd	$-2,(r11,r10)		; 004A FFFE FFFF
		andd	$-128,(r12_l,r11)	; 004B FF80 FFFF
		andd	$0xfe,r12		; 004C 00FE 0000
		andd	$-32768,(r12_l,r11)	; 004B 8000 FFFF
		andd	$0x7fff,r12		; 004C 7FFF 0000
		andd	$-32769,(r12_l,r11)	; 004B 7FFF FFFF
		andd	$0x8000,r12		; 004C 8000 0000
		andd	$-524288,(r12_l,r11)	; 004B 0000 FFF8
		andd	$0x7FFFF,r12		; 004C FFFF 0007
		andd	$-524289,(r12_l,r11)	; 004B FFFF FFF7
		andd	$0x80000,r12		; 004C 0000 0008
		andd	$-2147483648,(r12_l,r11); 004B 0000 8000
		andd	$0xffefffff,r12		; 004C FFFF FFEF
		andd	(r5,r4),(r10,r9)	; 0014 B049

		andw	$1,r7		; 2217
		andw	$8,r9		; 2289
		andw	$9,r3		; 22B3 0009
		andw	$10,r5		; 22A5
		andw	$11,r6		; 22B6 000B
		andw	$12,r4		; 22C4
		andw	$-1,r2		; 2292
		andw	$0xffff,r2	;  "
		andw	$15,r1		; 22F1
		andw	$16,r8		; 22B8 0010
		andw	$-2,r10		; 22BA FFFE
		andw	$-32768,r11	; 22BB 8000
		andw	$0xfffe,r12_l	; 22BC FFFE
		andw	r4,r9		; 2349

		ashub	$0,r4		; 4004
		ashub	$1,r4		; 4014
		ashub	$7,r4		; 4074
		expect	1320
		ashub	$8,r4
		endexpect
		ashub	$-1,r4		; 40F4
		ashub	$-7,r4		; 4094
		expect	1315
		ashub	$-8,r4
		endexpect
		ashub	r6,r4		; 4164

		ashud	$0,(r5,r4)	; 4C04
		ashud	$1,(r5,r4)	; 4C14
		ashud	$31,(r5,r4)	; 4DF4
		expect	1320
		ashud	$32,(r5,r4)
		endexpect
		ashud	$-1,(r5,r4)	; 4FF4
		ashud	$-31,(r5,r4)	; 4E14
		expect	1315
		ashud	$-32,(r5,r4)
		endexpect
		ashud	r6,(r5,r4)	; 4864

		ashuw	$0,r4		; 4204
		ashuw	$1,r4		; 4214
		ashuw	$15,r4		; 42F4
		expect	1320
		ashuw	$16,r4
		endexpect
		ashuw	$-1,r4		; 43F4
		ashuw	$-15,r4		; 4314
		expect	1315
		ashuw	$-16,r4
		endexpect
		ashuw	r6,r4		; 4564

		bal	(ra),*		; C000 0000
		expect	1351
		bal	(ra),*+1
		endexpect
		bal	(ra),*+2	; C000 0002
		bal	(ra),*+30	; C000 001E
		bal	(ra),*+32	; C000 0020
		bal	(ra),*+254	; C000 00FE
		bal	(ra),*+256	; C000 0100
		bal	(ra),*+65534	; C000 FFFE
		bal	(ra),*+65536	; C001 0000
		bal	(ra),*+131070	; C001 FFFE
		bal	(ra),*+131072	; C002 0000
		expect	1351
		bal	(ra),*-1
		endexpect
		bal	(ra),*-2	; C0FF FFFF
		bal	(ra),*-32	; C0FF FFE1
		bal	(ra),*-34	; C0FF FFDF
		bal	(ra),*-254	; C0FF FF03
		bal	(ra),*-256	; C0FF FF01
		bal	(ra),*-65536	; C0FF 0001
		bal	(ra),*-65538	; C0FE FFFF
		bal	(ra),*-131072	; C0FE 0001
		bal	(ra),*-131074	; C0FD FFFF

		bal	(r12),*		; 0010 20C0 0000
		expect	1351
		bal	(r12),*+1
		endexpect
		bal	(r12),*+2	; 0010 20C0 0002
		bal	(r12),*+30	; 0010 20C0 001E
		bal	(r12),*+32	; 0010 20C0 0020
		bal	(r12),*+254	; 0010 20C0 00FE
		bal	(r12),*+256	; 0010 20C0 0100
		bal	(r12),*+65534	; 0010 20C0 FFFE
		bal	(r12),*+65536	; 0010 21C0 0000
		bal	(r12),*+131070	; 0010 21C0 FFFE
		bal	(r12),*+131072	; 0010 22C0 0000
		expect	1351
		bal	(r12),*-1
		endexpect
		bal	(r12),*-2	; 0010 2FCF FFFF
		bal	(r12),*-32	; 0010 2FCF FFE1
		bal	(r12),*-34	; 0010 2FCF FFDF
		bal	(r12),*-254	; 0010 2FCF FF03
		bal	(r12),*-256	; 0010 2FCF FF01
		bal	(r12),*-65536	; 0010 2FCF 0001
		bal	(r12),*-65538	; 0010 2ECF FFFF
		bal	(r12),*-131072	; 0010 2ECF 0001
		bal	(r12),*-131074	; 0010 2DCF FFFF

		beq	*		; 1000
		expect	1351
		beq	*+1
		endexpect
		beq	*+2		; 1001
		beq	*+30		; 100F
		beq	*+32		; 1100
		beq	*+254		; 170F
		beq	*+256		; 1800 0100
		beq	*+65534		; 1800 FFFE
		beq	*+65536		; 0010 0100 0000
		beq	*+131070	; 0010 0100 FFFE
		beq	*+131072	; 0010 0200 0000
		expect	1351
		beq	*-1
		endexpect
		beq	*-2		; 1F0F
		beq	*-32		; 1F00
		beq	*-34		; 1E0F
		beq	*-254		; 1801
		beq	*-256		; 1800 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		beq	*-65536		; 1800 0001
		beq	*-65538		; 0010 0E0F FFFF
		beq	*-131072	; 0010 0E0F 0001
		beq	*-131074	; 0010 0D0F FFFF

		bne	*		; 1010
		expect	1351
		bne	*+1
		endexpect
		bne	*+2		; 1011
		bne	*+30		; 101F
		bne	*+32		; 1110
		bne	*+254		; 171F
		bne	*+256		; 1810 0100
		bne	*+65534		; 1810 FFFE
		bne	*+65536		; 0010 0110 0000
		bne	*+131070	; 0010 0110 FFFE
		bne	*+131072	; 0010 0210 0000
		expect	1351
		bne	*-1
		endexpect
		bne	*-2		; 1F1F
		bne	*-32		; 1F10
		bne	*-34		; 1E1F
		bne	*-254		; 1811
		bne	*-256		; 1810 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bne	*-65536		; 1810 0001
		bne	*-65538		; 0010 0E1F FFFF
		bne	*-131072	; 0010 0E1F 0001
		bne	*-131074	; 0010 0D1F FFFF

		bcs	*		; 1020
		expect	1351
		bcs	*+1
		endexpect
		bcs	*+2		; 1021
		bcs	*+30		; 102F
		bcs	*+32		; 1120
		bcs	*+254		; 172F
		bcs	*+256		; 1820 0100
		bcs	*+65534		; 1820 FFFE
		bcs	*+65536		; 0010 0120 0000
		bcs	*+131070	; 0010 0120 FFFE
		bcs	*+131072	; 0010 0220 0000
		expect	1351
		bcs	*-1
		endexpect
		bcs	*-2		; 1F2F
		bcs	*-32		; 1F20
		bcs	*-34		; 1E2F
		bcs	*-254		; 1821
		bcs	*-256		; 1820 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bcs	*-65536		; 1820 0001
		bcs	*-65538		; 0010 0E2F FFFF
		bcs	*-131072	; 0010 0E2F 0001
		bcs	*-131074	; 0010 0D2F FFFF

		bcc	*		; 1030
		expect	1351
		bcc	*+1
		endexpect
		bcc	*+2		; 1031
		bcc	*+30		; 103F
		bcc	*+32		; 1130
		bcc	*+254		; 173F
		bcc	*+256		; 1830 0100
		bcc	*+65534		; 1830 FFFE
		bcc	*+65536		; 0010 0130 0000
		bcc	*+131070	; 0010 0130 FFFE
		bcc	*+131072	; 0010 0230 0000
		expect	1351
		bcc	*-1
		endexpect
		bcc	*-2		; 1F3F
		bcc	*-32		; 1F30
		bcc	*-34		; 1E3F
		bcc	*-254		; 1831
		bcc	*-256		; 1830 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bcc	*-65536		; 1830 0001
		bcc	*-65538		; 0010 0E3F FFFF
		bcc	*-131072	; 0010 0E3F 0001
		bcc	*-131074	; 0010 0D3F FFFF

		bhi	*		; 1040
		expect	1351
		bhi	*+1
		endexpect
		bhi	*+2		; 1041
		bhi	*+30		; 104F
		bhi	*+32		; 1140
		bhi	*+254		; 174F
		bhi	*+256		; 1840 0100
		bhi	*+65534		; 1840 FFFE
		bhi	*+65536		; 0010 0140 0000
		bhi	*+131070	; 0010 0140 FFFE
		bhi	*+131072	; 0010 0240 0000
		expect	1351
		bhi	*-1
		endexpect
		bhi	*-2		; 1F4F
		bhi	*-32		; 1F40
		bhi	*-34		; 1E4F
		bhi	*-254		; 1841
		bhi	*-256		; 1840 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bhi	*-65536		; 1840 0001
		bhi	*-65538		; 0010 0E4F FFFF
		bhi	*-131072	; 0010 0E4F 0001
		bhi	*-131074	; 0010 0D4F FFFF

		bls	*		; 1050
		expect	1351
		bls	*+1
		endexpect
		bls	*+2		; 1051
		bls	*+30		; 105F
		bls	*+32		; 1150
		bls	*+254		; 175F
		bls	*+256		; 1850 0100
		bls	*+65534		; 1850 FFFE
		bls	*+65536		; 0010 0150 0000
		bls	*+131070	; 0010 0150 FFFE
		bls	*+131072	; 0010 0250 0000
		expect	1351
		bls	*-1
		endexpect
		bls	*-2		; 1F5F
		bls	*-32		; 1F50
		bls	*-34		; 1E5F
		bls	*-254		; 1851
		bls	*-256		; 1850 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bls	*-65536		; 1850 0001
		bls	*-65538		; 0010 0E5F FFFF
		bls	*-131072	; 0010 0E5F 0001
		bls	*-131074	; 0010 0D5F FFFF

		bgt	*		; 1060
		expect	1351
		bgt	*+1
		endexpect
		bgt	*+2		; 1061
		bgt	*+30		; 106F
		bgt	*+32		; 1160
		bgt	*+254		; 176F
		bgt	*+256		; 1860 0100
		bgt	*+65534		; 1860 FFFE
		bgt	*+65536		; 0010 0160 0000
		bgt	*+131070	; 0010 0160 FFFE
		bgt	*+131072	; 0010 0260 0000
		expect	1351
		bgt	*-1
		endexpect
		bgt	*-2		; 1F6F
		bgt	*-32		; 1F60
		bgt	*-34		; 1E6F
		bgt	*-254		; 1861
		bgt	*-256		; 1860 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bgt	*-65536		; 1860 0001
		bgt	*-65538		; 0010 0E6F FFFF
		bgt	*-131072	; 0010 0E6F 0001
		bgt	*-131074	; 0010 0D6F FFFF

		ble	*		; 1070
		expect	1351
		ble	*+1
		endexpect
		ble	*+2		; 1071
		ble	*+30		; 107F
		ble	*+32		; 1170
		ble	*+254		; 177F
		ble	*+256		; 1870 0100
		ble	*+65534		; 1870 FFFE
		ble	*+65536		; 0010 0170 0000
		ble	*+131070	; 0010 0170 FFFE
		ble	*+131072	; 0010 0270 0000
		expect	1351
		ble	*-1
		endexpect
		ble	*-2		; 1F7F
		ble	*-32		; 1F70
		ble	*-34		; 1E7F
		ble	*-254		; 1871
		ble	*-256		; 1870 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		ble	*-65536		; 1870 0001
		ble	*-65538		; 0010 0E7F FFFF
		ble	*-131072	; 0010 0E7F 0001
		ble	*-131074	; 0010 0D7F FFFF

		bfs	*		; 1080
		expect	1351
		bfs	*+1
		endexpect
		bfs	*+2		; 1081
		bfs	*+30		; 108F
		bfs	*+32		; 1180
		bfs	*+254		; 178F
		bfs	*+256		; 1880 0100
		bfs	*+65534		; 1880 FFFE
		bfs	*+65536		; 0010 0180 0000
		bfs	*+131070	; 0010 0180 FFFE
		bfs	*+131072	; 0010 0280 0000
		expect	1351
		bfs	*-1
		endexpect
		bfs	*-2		; 1F8F
		bfs	*-32		; 1F80
		bfs	*-34		; 1E8F
		bfs	*-254		; 1881
		bfs	*-256		; 1880 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bfs	*-65536		; 1880 0001
		bfs	*-65538		; 0010 0E8F FFFF
		bfs	*-131072	; 0010 0E8F 0001
		bfs	*-131074	; 0010 0D8F FFFF

		bfc	*		; 1090
		expect	1351
		bfc	*+1
		endexpect
		bfc	*+2		; 1091
		bfc	*+30		; 109F
		bfc	*+32		; 1190
		bfc	*+254		; 179F
		bfc	*+256		; 1890 0100
		bfc	*+65534		; 1890 FFFE
		bfc	*+65536		; 0010 0190 0000
		bfc	*+131070	; 0010 0190 FFFE
		bfc	*+131072	; 0010 0290 0000
		expect	1351
		bfc	*-1
		endexpect
		bfc	*-2		; 1F9F
		bfc	*-32		; 1F90
		bfc	*-34		; 1E9F
		bfc	*-254		; 1891
		bfc	*-256		; 1890 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bfc	*-65536		; 1890 0001
		bfc	*-65538		; 0010 0E9F FFFF
		bfc	*-131072	; 0010 0E9F 0001
		bfc	*-131074	; 0010 0D9F FFFF

		blo	*		; 10A0
		expect	1351
		blo	*+1
		endexpect
		blo	*+2		; 10A1
		blo	*+30		; 10AF
		blo	*+32		; 11A0
		blo	*+254		; 17AF
		blo	*+256		; 18A0 0100
		blo	*+65534		; 18A0 FFFE
		blo	*+65536		; 0010 01A0 0000
		blo	*+131070	; 0010 01A0 FFFE
		blo	*+131072	; 0010 02A0 0000
		expect	1351
		blo	*-1
		endexpect
		blo	*-2		; 1FAF
		blo	*-32		; 1FA0
		blo	*-34		; 1EAF
		blo	*-254		; 18A1
		blo	*-256		; 18A0 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		blo	*-65536		; 18A0 0001
		blo	*-65538		; 0010 0EAF FFFF
		blo	*-131072	; 0010 0EAF 0001
		blo	*-131074	; 0010 0DAF FFFF

		bhs	*		; 10B0
		expect	1351
		bhs	*+1
		endexpect
		bhs	*+2		; 10B1
		bhs	*+30		; 10BF
		bhs	*+32		; 11B0
		bhs	*+254		; 17BF
		bhs	*+256		; 18B0 0100
		bhs	*+65534		; 18B0 FFFE
		bhs	*+65536		; 0010 01B0 0000
		bhs	*+131070	; 0010 01B0 FFFE
		bhs	*+131072	; 0010 02B0 0000
		expect	1351
		bhs	*-1
		endexpect
		bhs	*-2		; 1FBF
		bhs	*-32		; 1FB0
		bhs	*-34		; 1EBF
		bhs	*-254		; 18B1
		bhs	*-256		; 18B0 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bhs	*-65536		; 18B0 0001
		bhs	*-65538		; 0010 0EBF FFFF
		bhs	*-131072	; 0010 0EBF 0001
		bhs	*-131074	; 0010 0DBF FFFF

		blt	*		; 10C0
		expect	1351
		blt	*+1
		endexpect
		blt	*+2		; 10C1
		blt	*+30		; 10CF
		blt	*+32		; 11C0
		blt	*+254		; 17CF
		blt	*+256		; 18C0 0100
		blt	*+65534		; 18C0 FFFE
		blt	*+65536		; 0010 01C0 0000
		blt	*+131070	; 0010 01C0 FFFE
		blt	*+131072	; 0010 02C0 0000
		expect	1351
		blt	*-1
		endexpect
		blt	*-2		; 1FCF
		blt	*-32		; 1FC0
		blt	*-34		; 1ECF
		blt	*-254		; 18C1
		blt	*-256		; 18C0 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		blt	*-65536		; 18C0 0001
		blt	*-65538		; 0010 0ECF FFFF
		blt	*-131072	; 0010 0ECF 0001
		blt	*-131074	; 0010 0DCF FFFF

		bge	*		; 10D0
		expect	1351
		bge	*+1
		endexpect
		bge	*+2		; 10D1
		bge	*+30		; 10DF
		bge	*+32		; 11D0
		bge	*+254		; 17DF
		bge	*+256		; 18D0 0100
		bge	*+65534		; 18D0 FFFE
		bge	*+65536		; 0010 01D0 0000
		bge	*+131070	; 0010 01D0 FFFE
		bge	*+131072	; 0010 02D0 0000
		expect	1351
		bge	*-1
		endexpect
		bge	*-2		; 1FDF
		bge	*-32		; 1FD0
		bge	*-34		; 1EDF
		bge	*-254		; 18D1
		bge	*-256		; 18D0 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bge	*-65536		; 18D0 0001
		bge	*-65538		; 0010 0EDF FFFF
		bge	*-131072	; 0010 0EDF 0001
		bge	*-131074	; 0010 0DDF FFFF

		bge	*		; 10D0
		expect	1351
		bge	*+1
		endexpect
		bge	*+2		; 10D1
		bge	*+30		; 10DF
		bge	*+32		; 11D0
		bge	*+254		; 17DF
		bge	*+256		; 18D0 0100
		bge	*+65534		; 18D0 FFFE
		bge	*+65536		; 0010 01D0 0000
		bge	*+131070	; 0010 01D0 FFFE
		bge	*+131072	; 0010 02D0 0000
		expect	1351
		bge	*-1
		endexpect
		bge	*-2		; 1FDF
		bge	*-32		; 1FD0
		bge	*-34		; 1EDF
		bge	*-254		; 18D1
		bge	*-256		; 18D0 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		bge	*-65536		; 18D0 0001
		bge	*-65538		; 0010 0EDF FFFF
		bge	*-131072	; 0010 0EDF 0001
		bge	*-131074	; 0010 0DDF FFFF

		br	*		; 10E0
		expect	1351
		br	*+1
		endexpect
		br	*+2		; 10E1
		br	*+30		; 10EF
		br	*+32		; 11E0
		br	*+254		; 17EF
		br	*+256		; 18E0 0100
		br	*+65534		; 18E0 FFFE
		br	*+65536		; 0010 01E0 0000
		br	*+131070	; 0010 01E0 FFFE
		br	*+131072	; 0010 02E0 0000
		expect	1351
		br	*-1
		endexpect
		br	*-2		; 1FEF
		br	*-32		; 1FE0
		br	*-34		; 1EEF
		br	*-254		; 18E1
		br	*-256		; 18E0 FF01 (must be encoded as disp16 because disp8=0x80 is escape for disp16)
		br	*-65536		; 18E0 0001
		br	*-65538		; 0010 0EEF FFFF
		br	*-131072	; 0010 0EEF 0001
		br	*-131074	; 0010 0DEF FFFF

		expect	1130
		beq0b	r12,*+2
		endexpect
		expect	1370
		beq0b	r7,*
		endexpect
		beq0b	r7,*+2		; 0C07
		beq0b	r7,*+32		; 0CF7
		expect	1370
		beq0b	r7,*+34
		endexpect

		expect	1130
		beq0w	r12,*+2
		endexpect
		expect	1370
		beq0w	r12_l,*
		endexpect
		beq0w	r12_l,*+2	; 0E0C
		beq0w	r12_l,*+32	; 0EFC
		expect	1370
		beq0w	r12_l,*+34
		endexpect

		expect	1130
		beq1b	r12,*+2
		endexpect
		expect	1370
		beq1b	r7,*
		endexpect
		beq1b	r7,*+4		; 5017 1001 (built-in macro)
		cmpb	$1,r7		; expands to this
		beq	*+2
		beq1b	r7,*+34		; 5017 1100 (built-in macro)
		cmpb	$1,r7		; expands to this
		beq	*+32

		expect	1130
		beq1w	r12,*+2
		endexpect
		expect	1370
		beq1w	r7,*
		endexpect
		beq1w	r7,*+4		; 5217 1001 (built-in macro)
		cmpw	$1,r7		; expands to this
		beq	*+2
		beq1w	r7,*+34		; 5217 1100 (built-in macro)
		cmpw	$1,r7		; expands to this
		beq	*+32

		expect	1130
		bne0b	r13,*+2
		endexpect
		expect	1370
		bne0b	r9,*
		endexpect
		bne0b	r9,*+2		; 0D09
		bne0b	r9,*+32		; 0DF9
		expect	1370
		bne0b	r9,*+34
		endexpect

		expect	1130
		bne0b	r13,*+2
		endexpect
		expect	1370
		bne0w	r13_l,*
		endexpect
		bne0w	r13_l,*+2	; 0F0D
		bne0w	r13_l,*+32	; 0FFD
		expect	1370
		bne0w	r13_l,*+34
		endexpect

		expect	1130
		bne1b	r12,*+2
		endexpect
		expect	1370
		bne1b	r7,*
		endexpect
		bne1b	r7,*+4		; 5017 1011 (built-in macro)
		cmpb	$1,r7		; expands to this
		bne	*+2
		bne1b	r7,*+34		; 5017 1110 (built-in macro)
		cmpb	$1,r7		; expands to this
		bne	*+32

		expect	1130
		bne1w	r12,*+2
		endexpect
		expect	1370
		bne1w	r7,*
		endexpect
		bne1w	r7,*+4		; 5217 1011 (built-in macro)
		cmpw	$1,r7		; expands to this
		bne	*+2
		bne1w	r7,*+34		; 5217 1110 (built-in macro)
		cmpw	$1,r7		; expands to this
		bne	*+32

		; disp20(reg)
		cbitb	$7,(r4)		; 0010 4074 0000
		cbitb	$7,0(r4)	; 0010 4074 0000
		cbitb	$7,0x3fff(r4)	; 0010 4074 3FFF
		cbitb	$7,0x4000(r4)	; 0010 4074 4000
		cbitb	$7,0xfffff(r4)	; 0010 4F74 FFFF
		expect	1320
		cbitb	$7,0x100000(r4)
		endexpect
		expect	1315
		cbitb	$7,-1(r4)
		endexpect
		; disp0(rp)
		cbitb	$7,(r12)	; 6A7C
		cbitb	$7,0(r12_l,r11)	; 6A7B
		; disp16(rp)
		cbitb	$7,1(r12)	; 6B7C 0001
		cbitb	$7,13(r12)	; 6B7C 000D
		cbitb	$7,14(r12)	; 6B7C 000E
		cbitb	$7,15(r12)	; 6B7C 000F
		cbitb	$7,16(r12)	; 6B7C 0010
		cbitb	$7,65535(r12)	; 6B7C FFFF
		; disp20(rp)
		cbitb	$7,65536(r12)	; 0010 517C 0000
		cbitb	$7,0xfffff(r12)	; 0010 5F7C FFFF
		expect	1320
		cbitb	$7,0x100000(r12)
		endexpect
		expect	1315
		cbitb	$7,-1(r12)
		endexpect
		; disp14(rrp)
		cbitb	$7,[r12](r5,r4)		; 6A82 0070
		cbitb	$7,[r13](r5,r4)		; 6A8A 0070
		cbitb	$7,[r13]1(r6,r5)	; 6A8F 0071 (bits 0..3)
		cbitb	$7,[r13]15(r6,r5)	; 6A8F 007F
		cbitb	$7,[r13]16(r4,r3)	; 6A9E 0070 (additionally bits 5..4)
		cbitb	$7,[r13]63(r4,r3)	; 6ABE 007F
		cbitb	$7,[r13]64(r11,r10)	; 6A8D 4070 (additionally bits 7..6)
		cbitb	$7,[r13]255(r11,r10)	; 6ABD C07F
		cbitb	$7,[r13]256(r9,r8)	; 6A8C 0170 (additionally bits 13..8)
		cbitb	$7,[r13]16383(r9,r8)	; 6ABC FF7F
		; disp20(rrp)
		cbitb	$7,[r13]16384(r7,r6)	; 0010 607B 4000
		cbitb	$7,[r13]0x12345(r7,r6)	; 0010 617B 2345
		cbitb	$7,[r13]1048575(r7,r6)	; 0010 6F7B FFFF
		expect	1320
		cbitb	$7,[r13]1048576(r7,r6)
		endexpect
		expect	1315
		cbitb	$7,[r13]-1(r7,r6)
		endexpect
		; abs20
		cbitb	$7,0		; 6BF0 0000
		cbitb	$7,0x10000	; 6BF1 0000
		cbitb	$7,0x12345	; 6BF1 2345
		cbitb	$7,0xffffff	; 6BFF FFFF
		; abs24
		cbitb	$7,0x100000	; 0010 7071 0000
		cbitb	$7,0x123456	; 0010 7271 3456
		cbitb	$7,0xfeffff	; 0010 7E7F FFFF
		expect	1320
		cbitb	$7,0x1000000
		endexpect
		; abs20 rel
		cbitb	$7,[r12]0	; 6870 0000
		cbitb	$7,[r13]0x10000	; 68F1 0000
		cbitb	$7,[r12]0x12345	; 6871 2345
		cbitb	$7,[r13]0xfffff	; 68FF FFFF
		expect	1320
		cbitb	$7,[r12]0x100000
		endexpect
		expect	1130
		cbitb	$7,[r11]0x12345
		endexpect
		expect	1439
		cbitb	$7,[ra]0x12345
		endexpect

		; disp20(reg)
		cbitw	$7,(r4)		; 0011 4074 0000
		cbitw	$7,0(r4)	; 0011 4074 0000
		cbitw	$7,0x3fff(r4)	; 0011 4074 3FFF
		cbitw	$7,0x4000(r4)	; 0011 4074 4000
		cbitw	$7,0xfffff(r4)	; 0011 4F74 FFFF
		expect	1320
		cbitw	$7,0x100000(r4)
		endexpect
		expect	1315
		cbitw	$7,-1(r4)
		endexpect
		; disp0(rp)
		cbitw	$7,(r12)	; 6E7C
		cbitw	$7,0(r12_l,r11)	; 6E7B
		; disp16(rp)
		cbitw	$7,1(r12)	; 697C 0001
		cbitw	$7,13(r12)	; 697C 000D
		cbitw	$7,14(r12)	; 697C 000E
		cbitw	$7,15(r12)	; 697C 000F
		cbitw	$7,16(r12)	; 697C 0010
		cbitw	$7,65535(r12)	; 697C FFFF
		; disp20(rp)
		cbitw	$7,65536(r12)	; 0011 517C 0000
		cbitw	$7,0xfffff(r12)	; 0011 5F7C FFFF
		expect	1320
		cbitw	$7,0x100000(r12)
		endexpect
		expect	1315
		cbitw	$7,-1(r12)
		endexpect
		; disp14(rrp)
		cbitw	$7,[r12](r5,r4)		; 6AC2 0070
		cbitw	$7,[r13](r5,r4)		; 6ACA 0070
		cbitw	$7,[r13]1(r6,r5)	; 6ACF 0071 (bits 0..3)
		cbitw	$7,[r13]15(r6,r5)	; 6ACF 007F
		cbitw	$7,[r13]16(r4,r3)	; 6ADE 0070 (additionally bits 5..4)
		cbitw	$7,[r13]63(r4,r3)	; 6AFE 007F
		cbitw	$7,[r13]64(r11,r10)	; 6ACD 4070 (additionally bits 7..6)
		cbitw	$7,[r13]255(r11,r10)	; 6AFD C07F
		cbitw	$7,[r13]256(r9,r8)	; 6ACC 0170 (additionally bits 13..8)
		cbitw	$7,[r13]16383(r9,r8)	; 6AFC FF7F
		; disp20(rrp)
		cbitw	$7,[r13]16384(r7,r6)	; 0011 607B 4000
		cbitw	$7,[r13]0x12345(r7,r6)	; 0011 617B 2345
		cbitw	$7,[r13]1048575(r7,r6)	; 0011 6F7B FFFF
		expect	1320
		cbitw	$7,[r13]1048576(r7,r6)
		endexpect
		expect	1315
		cbitw	$7,[r13]-1(r7,r6)
		endexpect
		; abs20
		cbitw	$7,0		; 6F70 0000
		cbitw	$7,0x10000	; 6F71 0000
		cbitw	$7,0x12345	; 6F71 2345
		cbitw	$7,0xffffff	; 6F7F FFFF
		; abs24
		cbitw	$7,0x100000	; 0011 7071 0000
		cbitw	$7,0x123456	; 0011 7271 3456
		cbitw	$7,0xfeffff	; 0011 7E7F FFFF
		expect	1320
		cbitw	$7,0x1000000
		endexpect
		; abs20 rel
		cbitw	$7,[r12]0	; 6C70 0000
		cbitw	$7,[r13]0x10000	; 6D71 0000
		cbitw	$7,[r12]0x12345	; 6C71 2345
		cbitw	$7,[r13]0xfffff	; 6D7F FFFF
		expect	1320
		cbitw	$7,[r12]0x100000
		endexpect
		expect	1130
		cbitw	$7,[r11]0x12345
		endexpect
		expect	1439
		cbitw	$7,[ra]0x12345
		endexpect

		cinv	[i]		; 000A
		cinv	[i,u]		; 000B
		cinv	[d]		; 000C
		cinv	[d,u]		; 000D
		cinv	[d,i]		; 000E
		cinv	[d,i,u]		; 000F
		expect	1118
		cinv	[]
		endexpect
		expect	1116
		cinv	[x,u]
		endexpect
		expect	1116
		cinv	[,u]
		endexpect
		expect	1117
		cinv	[i,i]
		endexpect
		expect	1118
		cinv	[u]
		endexpect

		cmpb	$1,r7		; 5017
		cmpb	$8,r9		; 5089
		cmpb	$9,r3		; 50B3 0009
		cmpb	$10,r5		; 50A5
		cmpb	$11,r6		; 50B6 000B
		cmpb	$12,r4		; 50C4
		cmpb	$-1,r2		; 5092
		cmpb	$0xff,r2	;  "
		cmpb	$15,r1		; 50F1
		cmpb	$16,r8		; 50B8 0010
		cmpb	$-2,r10		; 50BA 00FE
		cmpb	$-128,r11	; 50BB 0080
		cmpb	$0xfe,r12_l	; 50BC 00FE
		cmpb	r4,r9		; 5149

		; CMPD has imm4/16 and imm32 variant:
		expect	1350
		cmpd	$1,(r8,r6)		; register pair not consecutive
		endexpect
		expect	1350
		cmpd	$1,(r13,r12)		; no pair of 16 bit registers
		endexpect
		expect	1350
		cmpd	$1,(r12,r11)		; mixture of 32/16 bit registers
		endexpect
		cmpd	$1,(r8,r7)		; 5617
		cmpd	$8,(r10,r9)		; 5689
		cmpd	$9,(r4,r3)		; 56B3 0009
		cmpd	$10,(r6,r5)		; 56A5
		cmpd	$11,(r7,r6)		; 56B6 000B
		cmpd	$12,(r5,r4)		; 56C4
		cmpd	$-1,(r3,r2)		; 5692
		cmpd	$0xffffffff,(r3,r2)	;  "
		cmpd	$15,(r2,r1)		; 56F1
		cmpd	$16,(r9,r8)		; 56B8 0010
		cmpd	$-2,(r11,r10)		; 56BA FFFE
		cmpd	$-128,(r12_l,r11)	; 56BB FF80
		cmpd	$0xfe,r12		; 56BC 00FE
		cmpd	$-32768,(r12_l,r11)	; 56BB 8000
		cmpd	$0x7fff,r12		; 56BC 7FFF
		cmpd	$-32769,(r12_l,r11)	; 009B 7FFF FFFF
		cmpd	$0x8000,r12		; 009C 8000 0000
		cmpd	$-524288,(r12_l,r11)	; 009B 0000 FFF8
		cmpd	$0x7FFFF,r12		; 009C FFFF 0007
		cmpd	$-524289,(r12_l,r11)	; 009B FFFF FFF7
		cmpd	$0x80000,r12		; 009C 0000 0008
		cmpd	$-2147483648,(r12_l,r11); 009B 0000 8000
		cmpd	$0xffefffff,r12		; 009C FFFF FFEF
		cmpd	(r5,r4),(r10,r9)	; 5749

		cmpw	$1,r7		; 5217
		cmpw	$8,r9		; 5289
		cmpw	$9,r3		; 52B3 0009
		cmpw	$10,r5		; 52A5
		cmpw	$11,r6		; 52B6 000B
		cmpw	$12,r4		; 52C4
		cmpw	$-1,r2		; 5292
		cmpw	$0xffff,r2	;  "
		cmpw	$15,r1		; 52F1
		cmpw	$16,r8		; 52B8 0010
		cmpw	$-2,r10		; 52BA FFFE
		cmpw	$-32768,r11	; 52BB 8000
		cmpw	$0xfffe,r12_l	; 52BC FFFE
		cmpw	r4,r9		; 5349

		di	   		; 0004

		ei	   		; 0005

		eiwait	   		; 0007

		excp	svc		; 00C5
		excp	dvz		; 00C6
		excp	flg		; 00C7
		excp	bpt		; 00C8
		excp	trc		; 00C9
		excp	und		; 00CA
		excp	iad		; 00CC
		excp	dbg		; 00CE
		excp	ise		; 00CF
		expect	1441
		excp	def
		endexpect

		jal	(ra),(r13)	; 00DD
		jal	(ra),(r6,r5)	; 00D5
		expect	1130
		jal	(ra),r6
		endexpect
		jal	(r12),(r13)	; 0014 80DC
		jal	(r4,r3),(r6,r5)	; 0014 8053
		expect	1350
		jal	(r4),(r13)
		endexpect

		jeq	(ra)		; 0A0E
		jne	(r13)		; 0A1D
		jge	r12		; 0ADC
		jcs	(r12_l,r11)	; 0A2B
		jcc	(r11,r10)	; 0A3A
		jhi	(r10,r9)	; 0A49
		jls	(r9,r8)		; 0A58
		jlo	(r8,r7)		; 0AA7
		jhs	(r7,r6)		; 0AB6
		jgt	(r6,r5)		; 0A65
		jle	(r5,r4)		; 0A74
		expect	1350
		jfs	(r11)
		endexpect
		jfs	(r4,r3)		; 0A83
		jfc	(r3,r2)		; 0A92
		jlt	(r2,r1)		; 0AC1
		jump	(r1,r0)		; 0AE0
		jusr	sp		; 0AFF

		; disp20(reg)
		loadb	(r4),r7		; 0012 4074 0000
		loadb	0(r4),r7	; 0012 4074 0000
		loadb	0x3fff(r4),r7	; 0012 4074 3FFF
		loadb	0x4000(r4),r7	; 0012 4074 4000
		loadb	0xfffff(r4),r7	; 0012 4F74 FFFF
		expect	1320
		loadb	0x100000(r4),r7
		endexpect
		; -disp20(reg)
		loadb	-1(r4),r7	; 0018 4F74 FFFF
		loadb	-0xfffff(r4),r7	; 0018 4074 0001
		loadb	-0x100000(r4),r7; 0018 4074 0000
		expect	1315
		loadb	-0x100001(r4),r7
		endexpect

		; disp4(reg)
		loadb	(r12),r7	; B07C
		loadb	0(r12_l,r11),r7	; B07B
		loadb	1(r12),r7	; B17C
		loadb	13(r12),r7	; BD7C
		; disp16(reg)
		loadb	14(r12),r7	; BF7C 000E
		loadb	15(r12),r7	; BF7C 000F
		loadb	16(r12),r7	; BF7C 0010
		loadb	65535(r12),r7	; BF7C FFFF
		; +disp20(reg)
		loadb	65536(r12),r7	; 0012 517C 0000
		loadb	0xfffff(r12),r7	; 0012 5F7C FFFF
		expect	1320
		loadb	0x100000(r12),r7
		endexpect
		; -disp20(reg)
		loadb	-1(r12),r7		; 0018 5F7C FFFF
		loadb	-0xfffff(r12),r7	; 0018 507C 0001
		loadb	-0x100000(r12),r7	; 0018 507C 0000
		expect	1315
		loadb	-0x100001(r12),r7
		endexpect

		; disp0(rrp)
		loadb	[r12](r1,r0),r7		; BE70
		loadb	[r12](r3,r2),r7		; BE71
		loadb	[r12](r5,r4),r7		; BE72
		loadb	[r12](r7,r6),r7		; BE73
		loadb	[r12](r9,r8),r7		; BE74
		loadb	[r12](r11,r10),r7	; BE75
		loadb	[r12](r4,r3),r7		; BE76
		loadb	[r12](r6,r5),r7		; BE77
		loadb	[r13](r1,r0),r7		; BE78
		loadb	[r13](r3,r2),r7		; BE79
		loadb	[r13](r5,r4),r7		; BE7A
		loadb	[r13](r7,r6),r7		; BE7B
		loadb	[r13](r9,r8),r7		; BE7C
		loadb	[r13](r11,r10),r7	; BE7D
		loadb	[r13](r4,r3),r7		; BE7E
		loadb	[r13](r6,r5),r7		; BE7F
		expect	1439
		loadb	[ra](r6,r5),r7
		endexpect
		expect	1350
		loadb	[r13](r2,r1),r7
		endexpect
		; disp14(rrp)
		loadb	[r13]1(r6,r5),r7	; 864F 0071 (bits 0..3)
		loadb	[r13]15(r6,r5),r7	; 864F 007F
		loadb	[r13]16(r4,r3),r7	; 865E 0070 (additionally bits 5..4)
		loadb	[r13]63(r4,r3),r7	; 867E 007F
		loadb	[r13]64(r11,r10),r7	; 864D 4070 (additionally bits 7..6)
		loadb	[r13]255(r11,r10),r7	; 867D C07F
		loadb	[r13]256(r9,r8),r7	; 864C 0170 (additionally bits 13..8)
		loadb	[r13]16383(r9,r8),r7	; 867C FF7F
		; disp20(rrp)
		loadb	[r13]16384(r7,r6),r7	; 0012 607B 4000
		loadb	[r13]0x12345(r7,r6),r7	; 0012 617B 2345
		loadb	[r13]1048575(r7,r6),r7	; 0012 6F7B FFFF
		expect	1320
		loadb	[r13]1048576(r7,r6),r7
		endexpect
		expect	1315
		loadb	[r13]-1(r7,r6),r7
		endexpect

		; abs20
		loadb	0,r7		; 8870 0000
		loadb	0x10000,r7	; 8871 0000
		loadb	0x12345,r7	; 8871 2345
		loadb	0xffffff,r7	; 887F FFFF
		; abs24
		loadb	0x100000,r7	; 0012 7071 0000
		loadb	0x123456,r7	; 0012 7271 3456
		loadb	0xfeffff,r7	; 0012 7E7F FFFF
		expect	1320
		loadb	0x1000000,r7
		endexpect

		; abs20 rel
		loadb	[r12]0,r7		; 8A70 0000
		loadb	[r13]0x10000,r7		; 8B71 0000
		loadb	[r12]0x12345,r7		; 8A71 2345
		loadb	[r13]0xfffff,r7		; 8B7F FFFF
		expect	1320
		loadb	[r12]0x100000,r7
		endexpect
		expect	1130
		loadb	[r11]0x12345,r7
		endexpect
		expect	1439
		loadb	[ra]0x12345,r7
		endexpect

		; disp20(reg)
		loadd	(r4),(r8,r7)		; 0012 8074 0000
		loadd	0(r4),(r8,r7)		; 0012 8074 0000
		loadd	0x3fff(r4),(r8,r7)	; 0012 8074 3FFF
		loadd	0x4000(r4),(r8,r7)	; 0012 8074 4000
		loadd	0xfffff(r4),(r8,r7)	; 0012 8F74 FFFF
		expect	1320
		loadd	0x100000(r4),(r8,r7)
		endexpect
		; -disp20(reg)
		loadd	-1(r4),(r8,r7)		; 0018 8F74 FFFF
		loadd	-0xfffff(r4),(r8,r7)	; 0018 8074 0001
		loadd	-0x100000(r4),(r8,r7)	; 0018 8074 0000
		expect	1315
		loadd	-0x100001(r4),(r8,r7)
		endexpect

		; disp4/16(rp)
		loadd	(r12),(r8,r7)		; A07C
		loadd	0(r12_l,r11),(r8,r7)	; A07B
		loadd	2(r12),(r8,r7)		; A17C
		loadd	26(r12),(r8,r7)		; AD7C
		loadd	27(r12),(r8,r7)		; AF7C 001B
		loadd	15(r12),(r8,r7)		; AF7C 000F
		loadd	16(r12),(r8,r7)		; A87C
		loadd	65535(r12),(r8,r7)	; AF7C FFFF
		; disp20(rp)
		loadd	65536(r12),(r8,r7)	; 0012 917C 0000
		loadd	0xfffff(r12),(r8,r7)	; 0012 9F7C FFFF
		expect	1320
		loadd	0x100000(r12),(r8,r7)
		endexpect
		; -disp20(rp)
		loadd	-1(r12),(r8,r7)		; 0018 9F7C FFFF
		loadd	-0xfffff(r12),(r8,r7)	; 0018 907C 0001
		loadd	-0x100000(r12),(r8,r7)	; 0018 907C 0000
		expect	1315
		loadd	-0x100001(r12),(r8,r7)
		endexpect

		; disp0(rrp)
		loadd	[r12](r1,r0),(r8,r7)		; AE70
		loadd	[r12](r3,r2),(r8,r7)		; AE71
		loadd	[r12](r5,r4),(r8,r7)		; AE72
		loadd	[r12](r7,r6),(r8,r7)		; AE73
		loadd	[r12](r9,r8),(r8,r7)		; AE74
		loadd	[r12](r11,r10),(r8,r7)		; AE75
		loadd	[r12](r4,r3),(r8,r7)		; AE76
		loadd	[r12](r6,r5),(r8,r7)		; AE77
		loadd	[r13](r1,r0),(r8,r7)		; AE78
		loadd	[r13](r3,r2),(r8,r7)		; AE79
		loadd	[r13](r5,r4),(r8,r7)		; AE7A
		loadd	[r13](r7,r6),(r8,r7)		; AE7B
		loadd	[r13](r9,r8),(r8,r7)		; AE7C
		loadd	[r13](r11,r10),(r8,r7)		; AE7D
		loadd	[r13](r4,r3),(r8,r7)		; AE7E
		loadd	[r13](r6,r5),(r8,r7)		; AE7F
		expect	1439
		loadd	[ra](r6,r5),(r8,r7)
		endexpect
		expect	1350
		loadd	[r13](r2,r1),(r8,r7)
		endexpect
		; disp14(rrp)
		loadd	[r13]1(r6,r5),(r8,r7)		; 868F 0071 (bits 0..3)
		loadd	[r13]15(r6,r5),(r8,r7)		; 868F 007F
		loadd	[r13]16(r4,r3),(r8,r7)		; 869E 0070 (additionally bits 5..4)
		loadd	[r13]63(r4,r3),(r8,r7)		; 86BE 007F
		loadd	[r13]64(r11,r10),(r8,r7)	; 868D 4070 (additionally bits 7..6)
		loadd	[r13]255(r11,r10),(r8,r7)	; 86BD C07F
		loadd	[r13]256(r9,r8),(r8,r7)		; 868C 0170 (additionally bits 13..8)
		loadd	[r13]16383(r9,r8),(r8,r7)	; 86BC FF7F
		; disp20(rrp)
		loadd	[r13]16384(r7,r6),(r8,r7)	; 0012 A07B 4000
		loadd	[r13]0x12345(r7,r6),(r8,r7)	; 0012 A17B 2345
		loadd	[r13]1048575(r7,r6),(r8,r7)	; 0012 AF7B FFFF
		expect	1320
		loadd	[r13]1048576(r7,r6),(r8,r7)
		endexpect
		expect	1315
		loadd	[r13]-1(r7,r6),(r8,r7)
		endexpect

		; abs20
		loadd	0,(r8,r7)		; 8770 0000
		loadd	0x10000,(r8,r7)		; 8771 0000
		loadd	0x12345,(r8,r7)		; 8771 2345
		loadd	0xffffff,(r8,r7)	; 877F FFFF
		; abs24
		loadd	0x100000,(r8,r7)	; 0012 B071 0000
		loadd	0x123456,(r8,r7)	; 0012 B271 3456
		loadd	0xfeffff,(r8,r7)	; 0012 BE7F FFFF
		expect	1320
		loadd	0x1000000,(r8,r7)
		endexpect

		; abs20 rel
		loadd	[r12]0,(r8,r7)		; 8C70 0000
		loadd	[r13]0x10000,(r8,r7)	; 8D71 0000
		loadd	[r12]0x12345,(r8,r7)	; 8C71 2345
		loadd	[r13]0xfffff,(r8,r7)	; 8D7F FFFF
		expect	1320
		loadd	[r12]0x100000,(r8,r7)
		endexpect
		expect	1130
		loadd	[r11]0x12345,(r8,r7)
		endexpect
		expect	1439
		loadd	[ra]0x12345,(r8,r7)
		endexpect

		expect	1315
		loadm	$0
		endexpect
		loadm	$1		; 00A0
		loadm	$2		; 00A1
		loadm	$8		; 00A7
		expect	1320
		loadm	$9
		endexpect

		expect	1315
		loadmp	$0
		endexpect
		loadmp	$1		; 00A8
		loadmp	$2		; 00A9
		loadmp	$8		; 00AF
		expect	1320
		loadmp	$9
		endexpect

		; disp20(rp)
		loadw	(r4),r7		; 0012 C074 0000
		loadw	0(r4),r7	; 0012 C074 0000
		loadw	0x3fff(r4),r7	; 0012 C074 3FFF
		loadw	0x4000(r4),r7	; 0012 C074 4000
		loadw	0xfffff(r4),r7	; 0012 CF74 FFFF
		expect	1320
		loadw	0x100000(r4),r7
		endexpect
		; -disp20(reg)
		loadw	-1(r4),r7	; 0018 CF74 FFFF
		loadw	-0xfffff(r4),r7	; 0018 C074 0001
		loadw	-0x100000(r4),r7; 0018 C074 0000
		expect	1315
		loadw	-0x100001(r4),r7
		endexpect

		; disp4/16(rp)
		loadw	(r12),r7	; 907C
		loadw	0(r12_l,r11),r7	; 907B
		loadw	2(r12),r7	; 917C
		loadw	26(r12),r7	; 9D7C
		loadw	14(r12),r7	; 977C
		loadw	15(r12),r7	; 9F7C 000F
		loadw	16(r12),r7	; 987C
		loadw	65535(r12),r7	; 9F7C FFFF
		; disp20(rp)
		loadw	65536(r12),r7	; 0012 D17C 0000
		loadw	0xfffff(r12),r7	; 0012 DF7C FFFF
		expect	1320
		loadw	0x100000(r12),r7
		endexpect
		; -disp20(rp)
		loadw	-1(r12),r7		; 0018 DF7C FFFF
		loadw	-0xfffff(r12),r7	; 0018 D07C 0001
		loadw	-0x100000(r12),r7	; 0018 D07C 0000
		expect	1315
		loadw	-0x100001(r12),r7
		endexpect

		; disp0(rrp)
		loadw	[r12](r1,r0),r7		; 9E70
		loadw	[r12](r3,r2),r7		; 9E71
		loadw	[r12](r5,r4),r7		; 9E72
		loadw	[r12](r7,r6),r7		; 9E73
		loadw	[r12](r9,r8),r7		; 9E74
		loadw	[r12](r11,r10),r7	; 9E75
		loadw	[r12](r4,r3),r7		; 9E76
		loadw	[r12](r6,r5),r7		; 9E77
		loadw	[r13](r1,r0),r7		; 9E78
		loadw	[r13](r3,r2),r7		; 9E79
		loadw	[r13](r5,r4),r7		; 9E7A
		loadw	[r13](r7,r6),r7		; 9E7B
		loadw	[r13](r9,r8),r7		; 9E7C
		loadw	[r13](r11,r10),r7	; 9E7D
		loadw	[r13](r4,r3),r7		; 9E7E
		loadw	[r13](r6,r5),r7		; 9E7F
		expect	1439
		loadw	[ra](r6,r5),r7
		endexpect
		expect	1350
		loadw	[r13](r2,r1),r7
		endexpect
		; disp14(rrp)
		loadw	[r13]1(r6,r5),r7	; 86CF 0071 (bits 0..3)
		loadw	[r13]15(r6,r5),r7	; 86CF 007F
		loadw	[r13]16(r4,r3),r7	; 86DE 0070 (additionally bits 5..4)
		loadw	[r13]63(r4,r3),r7	; 86FE 007F
		loadw	[r13]64(r11,r10),r7	; 86CD 4070 (additionally bits 7..6)
		loadw	[r13]255(r11,r10),r7	; 86FD C07F
		loadw	[r13]256(r9,r8),r7	; 86CC 0170 (additionally bits 13..8)
		loadw	[r13]16383(r9,r8),r7	; 86FC FF7F
		; disp20(rrp)
		loadw	[r13]16384(r7,r6),r7	; 0012 E07B 4000
		loadw	[r13]0x12345(r7,r6),r7	; 0012 E17B 2345
		loadw	[r13]1048575(r7,r6),r7	; 0012 EF7B FFFF
		expect	1320
		loadw	[r13]1048576(r7,r6),r7
		endexpect
		expect	1315
		loadw	[r13]-1(r7,r6),r7
		endexpect

		; abs20
		loadw	0,r7		; 8970 0000
		loadw	0x10000,r7	; 8971 0000
		loadw	0x12345,r7	; 8971 2345
		loadw	0xffffff,r7	; 897F FFFF
		; abs24
		loadw	0x100000,r7	; 0012 F071 0000
		loadw	0x123456,r7	; 0012 F271 3456
		loadw	0xfeffff,r7	; 0012 FE7F FFFF
		expect	1320
		loadw	0x1000000,r7
		endexpect

		; abs20 rel
		loadw	[r12]0,r7		; 8E70 0000
		loadw	[r13]0x10000,r7		; 8F71 0000
		loadw	[r12]0x12345,r7		; 8E71 2345
		loadw	[r13]0xfffff,r7		; 8F7F FFFF
		expect	1320
		loadw	[r12]0x100000,r7
		endexpect
		expect	1130
		loadw	[r11]0x12345,r7
		endexpect
		expect	1439
		loadw	[ra]0x12345,r7
		endexpect

		lpr	ra_l,dbs	; 0014 000E
		lpr	r13_l,dsr	; 0014 001D
		lpr	r12_l,dcrl	; 0014 002C
		lpr	r11,dcrh	; 0014 003B
		lpr	r10,car0l	; 0014 004A
		lpr	r9,car0h	; 0014 0059
		lpr	r8,car1l	; 0014 0068
		lpr	r7,car1h	; 0014 0077
		lpr	r6,cfg		; 0014 0086
		lpr	r5,psr		; 0014 0095
		lpr	r4,intbasel	; 0014 00A4
		lpr	r3,intbaseh	; 0014 00B3
		lpr	r2,ispl		; 0014 00C2
		lpr	r1,isph		; 0014 00D1
		lpr	r0,uspl		; 0014 00E0
		lpr	sp_l,usph	; 0014 00FF
		expect	1437
		lpr	r5,def		; unknown CPU register
		endexpect
		expect	1130
		lpr	r5,car0		; unknown 16 bit CPU register
		endexpect

		lprd	ra,dbs		; 0014 100E
		lprd	r13,dsr		; 0014 101D
		lprd	r12,dcr		; 0014 102C
		lprd	(r11,r10),car0	; 0014 104A
		lprd	(r9,r8),car1	; 0014 1068
		lprd	(r7,r6),cfg	; 0014 1086
		lprd	(r6,r5),psr	; 0014 1095
		lprd	(r5,r4),intbase	; 0014 10A4
		lprd	(r3,r2),isp	; 0014 10C2
		lprd	(r1,r0),usp	; 0014 10E0
		expect	1437
		lprd	(r6,r5),def	; unknown CPU register
		endexpect
		expect	1130
		lprd	r5,car0l	; unknown 32 bit CPU register
		endexpect

		lshb	$0,r4		; 4004 (uses ashub)
		lshb	$1,r4		; 4014 (uses ashub)
		lshb	$7,r4		; 4074 (uses ashub)
		expect	1320
		lshb	$8,r4
		endexpect
		lshb	$-1,r4		; 09F4
		lshb	$-7,r4		; 0994
		expect	1315
		lshb	$-8,r4
		endexpect
		lshb	r6,r4		; 4464

		lshd	$0,(r5,r4)	; 4C04 (uses ashud)
		lshd	$1,(r5,r4)	; 4C14 (uses ashud)
		lshd	$31,(r5,r4)	; 4DF4 (uses ashud)
		expect	1320
		lshd	$32,(r5,r4)
		endexpect
		lshd	$-1,(r5,r4)	; 4BF4
		lshd	$-31,(r5,r4)	; 4A14
		expect	1315
		lshd	$-32,(r5,r4)
		endexpect
		lshd	r6,(r5,r4)	; 4764

		lshw	$0,r4		; 4204 (uses ashuw)
		lshw	$1,r4		; 4214 (uses ashuw)
		lshw	$15,r4		; 42F4 (uses ashuw)
		expect	1320
		lshw	$16,r4
		endexpect
		lshw	$-1,r4		; 49F4
		lshw	$-15,r4		; 4914
		expect	1315
		lshw	$-16,r4
		endexpect
		lshw	r6,r4		; 4664

		macqw	r1,r2,r12	; 0014 DC12
		macuw	r1,r2,(r4,r3)	; 0014 E312
		macsw	r1,r2,r13	; 0014 FD12
		expect	1130
		macsw	r1,r2,r3
		endexpect

		movb	$1,r7		; 5817
		movb	$8,r9		; 5889
		movb	$9,r3		; 58B3 0009
		movb	$10,r5		; 58A5
		movb	$11,r6		; 58B6 000B
		movb	$12,r4		; 58C4
		movb	$-1,r2		; 5892
		movb	$0xff,r2	;  "
		movb	$15,r1		; 58F1
		movb	$16,r8		; 58B8 0010
		movb	$-2,r10		; 58BA 00FE
		movb	$-128,r11	; 58BB 0080
		movb	$0xfe,r12_l	; 58BC 00FE
		movb	r4,r9		; 5949

		; MOVD has imm4/16, imm20, and imm32 variant:
		expect	1350
		movd	$1,(r8,r6)		; register pair not consecutive
		endexpect
		expect	1350
		movd	$1,(r13,r12)		; no pair of 16 bit registers
		endexpect
		expect	1350
		movd	$1,(r12,r11)		; mixture of 32/16 bit registers
		endexpect
		movd	$1,(r8,r7)		; 5417
		movd	$8,(r10,r9)		; 5489
		movd	$9,(r4,r3)		; 54B3 0009
		movd	$10,(r6,r5)		; 54A5
		movd	$11,(r7,r6)		; 54B6 000B
		movd	$12,(r5,r4)		; 54C4
		movd	$-1,(r3,r2)		; 5492
		movd	$0xffffffff,(r3,r2)	;  "
		movd	$15,(r2,r1)		; 54F1
		movd	$16,(r9,r8)		; 54B8 0010
		movd	$-2,(r11,r10)		; 54BA FFFE
		movd	$-128,(r12_l,r11)	; 54BB FF80
		movd	$0xfe,r12		; 54BC 00FE
		movd	$-32768,(r12_l,r11)	; 54BB 8000
		movd	$0x7fff,r12		; 54BC 7FFF
		movd	$-32769,(r12_l,r11)	; 05BF 8001
		movd	$0x8000,r12		; 05C0 8000
		movd	$-524288,(r12_l,r11)	; 05B8 0000
		movd	$0x7FFFF,r12		; 05C7 FFFF
		movd	$-524289,(r12_l,r11)	; 007B FFFF FFF7
		movd	$0x80000,r12		; 007C 0000 0008
		movd	$-2147483648,(r12_l,r11); 007B 0000 8000
		movd	$0xffefffff,r12		; 007C FFFF FFEF
		movd	(r5,r4),(r10,r9)	; 5549

		movw	$1,r7		; 5A17
		movw	$8,r9		; 5A89
		movw	$9,r3		; 5AB3 0009
		movw	$10,r5		; 5AA5
		movw	$11,r6		; 5AB6 000B
		movw	$12,r4		; 5AC4
		movw	$-1,r2		; 5A92
		movw	$0xffff,r2	;  "
		movw	$15,r1		; 5AF1
		movw	$16,r8		; 5AB8 0010
		movw	$-2,r10		; 5ABA FFFE
		movw	$-32768,r11	; 5ABB 8000
		movw	$0xfffe,r12_l	; 5ABC FFFE
		movw	r4,r9		; 5B49

		movxb	r4,r7		; 5C47
		expect	1130
		movxb	r12,r7		; src is 32 instead of 8 bits
		endexpect
		expect	1130
		movxb	r4,r12		; dest is 32 instead of 16 bits
		endexpect
		movxw	r8,r13		; 5E8D
		movxw	r9,(r7,r6)	; 5E96
		expect	1130
		movxw	ra,sp		; src is 32 instead of 16 bits
		endexpect
		expect	1130
		movxw	r10,r4		; dest is 16 instead of 32 bits
		endexpect
		movzb	r4,r7		; 5D47
		expect	1130
		movzb	r12,r7		; src is 32 instead of 8 bits
		endexpect
		expect	1130
		movzb	r4,r12		; dest is 32 instead of 16 bits
		endexpect
		movzw	r8,r13		; 5F8D
		movzw	r9,(r7,r6)	; 5F96
		expect	1130
		movzw	ra,sp		; src is 32 instead of 16 bits
		endexpect
		expect	1130
		movzw	r10,r4		; dest is 16 instead of 32 bits
		endexpect

		mulb	$1,r7		; 6417
		mulb	$8,r9		; 6489
		mulb	$9,r3		; 64B3 0009
		mulb	$10,r5		; 64A5
		mulb	$11,r6		; 64B6 000B
		mulb	$12,r4		; 64C4
		mulb	$-1,r2		; 6492
		mulb	$0xff,r2	;  "
		mulb	$15,r1		; 64F1
		mulb	$16,r8		; 64B8 0010
		mulb	$-2,r10		; 64BA 00FE
		mulb	$-128,r11	; 64BB 0080
		mulb	$0xfe,r12_l	; 64BC 00FE
		mulb	r4,r9		; 6549

		mulw	$1,r7		; 6617
		mulw	$8,r9		; 6689
		mulw	$9,r3		; 66B3 0009
		mulw	$10,r5		; 66A5
		mulw	$11,r6		; 66B6 000B
		mulw	$12,r4		; 66C4
		mulw	$-1,r2		; 6692
		mulw	$0xffff,r2	;  "
		mulw	$15,r1		; 66F1
		mulw	$16,r8		; 66B8 0010
		mulw	$-2,r10		; 66BA FFFE
		mulw	$-32768,r11	; 66BB 8000
		mulw	$0xfffe,r12_l	; 66BC FFFE
		mulw	r4,r9		; 6749

		mulsb	r12_l,r13_l	; 0BCD
		mulsb	r4,reg7		; 0B47
		expect	1130
		mulsb	r12_l,r13
		endexpect
		mulsw	r12_l,r13	; 62CD
		mulsw	r5,(r7,r6)	; 6256
		expect	1130
		mulsw	r5,r1
		endexpect
		muluw	r12_l,r13	; 63CD
		muluw	r5,(r7,r6)	; 6356
		expect	1130
		muluw	r5,r1
		endexpect

		nop	   		; 2C00 (addub $0x0,r0)

		orb	$1,r7		; 2417
		orb	$8,r9		; 2489
		orb	$9,r3		; 24B3 0009
		orb	$10,r5		; 24A5
		orb	$11,r6		; 24B6 000B
		orb	$12,r4		; 24C4
		orb	$-1,r2		; 2492
		orb	$0xff,r2	;  "
		orb	$15,r1		; 24F1
		orb	$16,r8		; 24B8 0010
		orb	$-2,r10		; 24BA 00FE
		orb	$-128,r11	; 24BB 0080
		orb	$0xfe,r12_l	; 24BC 00FE
		orb	r4,r9		; 2549

		; ORD only has imm32 variant:
		expect	1350
		ord	$1,(r8,r6)		; register pair not consecutive
		endexpect
		expect	1350
		ord	$1,(r13,r12)		; no pair of 16 bit registers
		endexpect
		expect	1350
		ord	$1,(r12,r11)		; mixture of 32/16 bit registers
		endexpect
		ord	$1,(r8,r7)		; 0057 0001 0000
		ord	$8,(r10,r9)		; 0059 0008 0000
		ord	$9,(r4,r3)		; 0053 0009 0000
		ord	$10,(r6,r5)		; 0055 000A 0000
		ord	$11,(r7,r6)		; 0056 000B 0000
		ord	$12,(r5,r4)		; 0054 000C 0000
		ord	$-1,(r3,r2)		; 0052 FFFF FFFF
		ord	$0xffffffff,(r3,r2)	;  "
		ord	$15,(r2,r1)		; 0051 000F 0000
		ord	$16,(r9,r8)		; 0058 0010 0000
		ord	$-2,(r11,r10)		; 005A FFFE FFFF
		ord	$-128,(r12_l,r11)	; 005B FF80 FFFF
		ord	$0xfe,r12		; 005C 00FE 0000
		ord	$-32768,(r12_l,r11)	; 005B 8000 FFFF
		ord	$0x7fff,r12		; 005C 7FFF 0000
		ord	$-32769,(r12_l,r11)	; 005B 7FFF FFFF
		ord	$0x8000,r12		; 005C 8000 0000
		ord	$-524288,(r12_l,r11)	; 005B 0000 FFF8
		ord	$0x7FFFF,r12		; 005C FFFF 0007
		ord	$-524289,(r12_l,r11)	; 005B FFFF FFF7
		ord	$0x80000,r12		; 005C 0000 0008
		ord	$-2147483648,(r12_l,r11); 005B 0000 8000
		ord	$0xffefffff,r12		; 005C FFFF FFEF
		ord	(r5,r4),(r10,r9)	; 0014 9049

		orw	$1,r7		; 2617
		orw	$8,r9		; 2689
		orw	$9,r3		; 26B3 0009
		orw	$10,r5		; 26A5
		orw	$11,r6		; 26B6 000B
		orw	$12,r4		; 26C4
		orw	$-1,r2		; 2692
		orw	$0xffff,r2	;  "
		orw	$15,r1		; 26F1
		orw	$16,r8		; 26B8 0010
		orw	$-2,r10		; 26BA FFFE
		orw	$-32768,r11	; 26BB 8000
		orw	$0xfffe,r12_l	; 26BC FFFE
		orw	r4,r9		; 2749

		pop	ra		; 020E
		expect	1315
		pop	$0,r4
		endexpect
		pop	$1,r4		; 0204
		pop	$8,r4		; 0274
		expect	1320
		pop	$9,r4
		endexpect
		pop	$1,r4,ra	; 0284
		pop	$8,r4,ra	; 02F4
		expect	1410
		pop	$8,r9		; would extend beyond end of register block
		endexpect
		expect	1410
		pop	$8,r8,ra	; would restore RA twice
		endexpect

		popret	ra		; 030E
		expect	1315
		popret	$0,r4
		endexpect
		popret	$1,r4		; 0304
		popret	$8,r4		; 0374
		expect	1320
		popret	$9,r4
		endexpect
		popret	$1,r4,ra	; 0384
		popret	$8,r4,ra	; 03F4
		expect	1410
		popret	$8,r8		; would restore SP 
		endexpect
		expect	1410
		popret	$7,r8,ra	; would restore RA twice
		endexpect

		push	ra		; 010E
		expect	1315
		push	$0,r4
		endexpect
		push	$1,r4		; 0104
		push	$8,r4		; 0174
		expect	1320
		push	$9,r4
		endexpect
		push	$1,r4,ra	; 0184
		push	$8,r4,ra	; 01F4
		expect	1410
		push	$8,r9		; would extend beyond end of register block
		endexpect
		expect	1410
		push	$8,r8,ra	; would restore RA twice
		endexpect

		retx	   		; 0003

		; disp20(reg)
		sbitb	$7,(r4)		; 0010 8074 0000
		sbitb	$7,0(r4)	; 0010 8074 0000
		sbitb	$7,0x3fff(r4)	; 0010 8074 3FFF
		sbitb	$7,0x4000(r4)	; 0010 8074 4000
		sbitb	$7,0xfffff(r4)	; 0010 8F74 FFFF
		expect	1320
		sbitb	$7,0x100000(r4)
		endexpect
		expect	1315
		sbitb	$7,-1(r4)
		endexpect
		; disp0(rp)
		sbitb	$7,(r12)	; 727C
		sbitb	$7,0(r12_l,r11)	; 727B
		; disp16(rp)
		sbitb	$7,1(r12)	; 737C 0001
		sbitb	$7,13(r12)	; 737C 000D
		sbitb	$7,14(r12)	; 737C 000E
		sbitb	$7,15(r12)	; 737C 000F
		sbitb	$7,16(r12)	; 737C 0010
		sbitb	$7,65535(r12)	; 737C FFFF
		; disp20(rp)
		sbitb	$7,65536(r12)	; 0010 917C 0000
		sbitb	$7,0xfffff(r12)	; 0010 9F7C FFFF
		expect	1320
		sbitb	$7,0x100000(r12)
		endexpect
		expect	1315
		sbitb	$7,-1(r12)
		endexpect
		; disp14(rrp)
		sbitb	$7,[r12](r5,r4)		; 7282 0070
		sbitb	$7,[r13](r5,r4)		; 728A 0070
		sbitb	$7,[r13]1(r6,r5)	; 728F 0071 (bits 0..3)
		sbitb	$7,[r13]15(r6,r5)	; 728F 007F
		sbitb	$7,[r13]16(r4,r3)	; 729E 0070 (additionally bits 5..4)
		sbitb	$7,[r13]63(r4,r3)	; 72BE 007F
		sbitb	$7,[r13]64(r11,r10)	; 728D 4070 (additionally bits 7..6)
		sbitb	$7,[r13]255(r11,r10)	; 72BD C07F
		sbitb	$7,[r13]256(r9,r8)	; 728C 0170 (additionally bits 13..8)
		sbitb	$7,[r13]16383(r9,r8)	; 72BC FF7F
		; disp20(rrp)
		sbitb	$7,[r13]16384(r7,r6)	; 0010 A07B 4000
		sbitb	$7,[r13]0x12345(r7,r6)	; 0010 A17B 2345
		sbitb	$7,[r13]1048575(r7,r6)	; 0010 AF7B FFFF
		expect	1320
		sbitb	$7,[r13]1048576(r7,r6)
		endexpect
		expect	1315
		sbitb	$7,[r13]-1(r7,r6)
		endexpect
		; abs20
		sbitb	$7,0		; 73F0 0000
		sbitb	$7,0x10000	; 73F1 0000
		sbitb	$7,0x12345	; 73F1 2345
		sbitb	$7,0xffffff	; 73FF FFFF
		; abs24
		sbitb	$7,0x100000	; 0010 B071 0000
		sbitb	$7,0x123456	; 0010 B271 3456
		sbitb	$7,0xfeffff	; 0010 BE7F FFFF
		expect	1320
		sbitb	$7,0x1000000
		endexpect
		; abs20 rel
		sbitb	$7,[r12]0	; 7070 0000
		sbitb	$7,[r13]0x10000	; 70F1 0000
		sbitb	$7,[r12]0x12345	; 7071 2345
		sbitb	$7,[r13]0xfffff	; 70FF FFFF
		expect	1320
		sbitb	$7,[r12]0x100000
		endexpect
		expect	1130
		sbitb	$7,[r11]0x12345
		endexpect
		expect	1439
		sbitb	$7,[ra]0x12345
		endexpect

		; disp20(reg)
		sbitw	$7,(r4)		; 0011 8074 0000
		sbitw	$7,0(r4)	; 0011 8074 0000
		sbitw	$7,0x3fff(r4)	; 0011 8074 3FFF
		sbitw	$7,0x4000(r4)	; 0011 8074 4000
		sbitw	$7,0xfffff(r4)	; 0011 8F74 FFFF
		expect	1320
		sbitw	$7,0x100000(r4)
		endexpect
		expect	1315
		sbitw	$7,-1(r4)
		endexpect
		; disp0(rp)
		sbitw	$7,(r12)	; 767C
		sbitw	$7,0(r12_l,r11)	; 767B
		; disp16(rp)
		sbitw	$7,1(r12)	; 717C 0001
		sbitw	$7,13(r12)	; 717C 000D
		sbitw	$7,14(r12)	; 717C 000E
		sbitw	$7,15(r12)	; 717C 000F
		sbitw	$7,16(r12)	; 717C 0010
		sbitw	$7,65535(r12)	; 717C FFFF
		; disp20(rp)
		sbitw	$7,65536(r12)	; 0011 917C 0000
		sbitw	$7,0xfffff(r12)	; 0011 9F7C FFFF
		expect	1320
		sbitw	$7,0x100000(r12)
		endexpect
		expect	1315
		sbitw	$7,-1(r12)
		endexpect
		; disp14(rrp)
		sbitw	$7,[r12](r5,r4)		; 72C2 0070
		sbitw	$7,[r13](r5,r4)		; 72CA 0070
		sbitw	$7,[r13]1(r6,r5)	; 72CF 0071 (bits 0..3)
		sbitw	$7,[r13]15(r6,r5)	; 72CF 007F
		sbitw	$7,[r13]16(r4,r3)	; 72DE 0070 (additionally bits 5..4)
		sbitw	$7,[r13]63(r4,r3)	; 72FE 007F
		sbitw	$7,[r13]64(r11,r10)	; 72CD 4070 (additionally bits 7..6)
		sbitw	$7,[r13]255(r11,r10)	; 72FD C07F
		sbitw	$7,[r13]256(r9,r8)	; 72CC 0170 (additionally bits 13..8)
		sbitw	$7,[r13]16383(r9,r8)	; 72FC FF7F
		; disp20(rrp)
		sbitw	$7,[r13]16384(r7,r6)	; 0011 A07B 4000
		sbitw	$7,[r13]0x12345(r7,r6)	; 0011 A17B 2345
		sbitw	$7,[r13]1048575(r7,r6)	; 0011 AF7B FFFF
		expect	1320
		sbitw	$7,[r13]1048576(r7,r6)
		endexpect
		expect	1315
		sbitw	$7,[r13]-1(r7,r6)
		endexpect
		; abs20
		sbitw	$7,0		; 7770 0000
		sbitw	$7,0x10000	; 7771 0000
		sbitw	$7,0x12345	; 7771 2345
		sbitw	$7,0xffffff	; 777F FFFF
		; abs24
		sbitw	$7,0x100000	; 0011 B071 0000
		sbitw	$7,0x123456	; 0011 B271 3456
		sbitw	$7,0xfeffff	; 0011 BE7F FFFF
		expect	1320
		sbitw	$7,0x1000000
		endexpect
		; abs20 rel
		sbitw	$7,[r12]0		; 7470 0000
		sbitw	$7,[r13]0x10000		; 7571 0000
		sbitw	$7,[r12]0x12345		; 7471 2345
		sbitw	$7,[r13]0xfffff		; 757F FFFF
		expect	1320
		sbitw	$7,[r12]0x100000
		endexpect
		expect	1130
		sbitw	$7,[r11]0x12345
		endexpect
		expect	1439
		sbitw	$7,[ra]0x12345
		endexpect

		seq	r1		; 0801
		sne	r2		; 0812
		sge	r3		; 08D3
		scs	r4		; 0824
		scc	r5		; 0835
		shi	r6		; 0846
		sls	r7		; 0857
		slo	r8		; 08A8
		shs	r9		; 08B9
		sgt	r10		; 086A
		sle	r11		; 087B
		expect	1130
		sfs	r12
		endexpect
		sfs	r12_l		; 088C
		sfc	r13_l		; 089D
		slt	ra_l		; 08CE

		spr	dbs,ra_l	; 0014 200E
		spr	dsr,r13_l	; 0014 201D
		spr	dcrl,r12_l	; 0014 202C
		spr	dcrh,r11	; 0014 203B
		spr	car0l,r10	; 0014 204A
		spr	car0h,r9	; 0014 2059
		spr	car1l,r8	; 0014 2068
		spr	car1h,r7	; 0014 2077
		spr	cfg,r6		; 0014 2086
		spr	psr,r5		; 0014 2095
		spr	intbasel,r4	; 0014 20A4
		spr	intbaseh,r3	; 0014 20B3
		spr	ispl,r2		; 0014 20C2
		spr	isph,r1		; 0014 20D1
		spr	uspl,r0		; 0014 20E0
		spr	usph,sp_l	; 0014 20FF
		expect	1437
		spr	def,r5		; unknown CPU register
		endexpect
		expect	1130
		spr	car0,r5		; unknown 16 bit CPU register
		endexpect

		sprd	dbs,ra		; 0014 300E
		sprd	dsr,r13		; 0014 301D
		sprd	dcr,r12		; 0014 302C
		sprd	car0,(r11,r10)	; 0014 304A
		sprd	car1,(r9,r8)	; 0014 3068
		sprd	cfg,(r7,r6)	; 0014 3086
		sprd	psr,(r6,r5)	; 0014 3095
		sprd	intbase,(r5,r4)	; 0014 30A4
		sprd	isp,(r3,r2)	; 0014 30C2
		sprd	usp,(r1,r0)	; 0014 30E0
		expect	1437
		sprd	def,(r6,r5)	; unknown processor register
		endexpect
		expect	1130
		sprd	car0l,(r6,r5)	; unknown 32 bit processor register
		endexpect

		; disp20(reg)
		storb	r7,(r4)		; 0013 4074 0000
		storb	r7,0(r4)	; 0013 4074 0000
		storb	r7,0x3fff(r4)	; 0013 4074 3FFF
		storb	r7,0x4000(r4)	; 0013 4074 4000
		storb	r7,0xfffff(r4)	; 0013 4F74 FFFF
		expect	1320
		storb	r7,0x100000(r4)
		endexpect
		; -disp20(reg)
		storb	r7,-1(r4)	; 0019 4F74 FFFF
		storb	r7,-0xfffff(r4)	; 0019 4074 0001
		storb	r7,-0x100000(r4); 0019 4074 0000
		expect	1315
		storb	r7,-0x100001(r4)
		endexpect

		; disp4/16(rp)
		storb	r7,(r12)	; F07C
		storb	r7,0(r12_l,r11)	; F07B
		storb	r7,1(r12)	; F17C
		storb	r7,13(r12)	; FD7C
		storb	r7,14(r12)	; FF7C 000E
		storb	r7,15(r12)	; FF7C 000F
		storb	r7,16(r12)	; FF7C 0010
		storb	r7,65535(r12)	; FF7C FFFF
		; disp20(rp)
		storb	r7,65536(r12)	; 0013 517C 0000
		storb	r7,0xfffff(r12)	; 0013 5F7C FFFF
		expect	1320
		storb	r7,0x100000(r12)
		endexpect
		; -disp20(rp)
		storb	r7,-1(r12)		; 0019 5F7C FFFF
		storb	r7,-0xfffff(r12)	; 0019 507C 0001
		storb	r7,-0x100000(r12)	; 0019 507C 0000
		expect	1315
		storb	r7,-0x100001(r12)
		endexpect

		; disp0(rrp)
		storb	r7,[r12](r1,r0)		; FE70
		storb	r7,[r12](r3,r2)		; FE71
		storb	r7,[r12](r5,r4)		; FE72
		storb	r7,[r12](r7,r6)		; FE73
		storb	r7,[r12](r9,r8)		; FE74
		storb	r7,[r12](r11,r10)	; FE75
		storb	r7,[r12](r4,r3)		; FE76
		storb	r7,[r12](r6,r5)		; FE77
		storb	r7,[r13](r1,r0)		; FE78
		storb	r7,[r13](r3,r2)		; FE79
		storb	r7,[r13](r5,r4)		; FE7A
		storb	r7,[r13](r7,r6)		; FE7B
		storb	r7,[r13](r9,r8)		; FE7C
		storb	r7,[r13](r11,r10)	; FE7D
		storb	r7,[r13](r4,r3)		; FE7E
		storb	r7,[r13](r6,r5)		; FE7F
		expect	1439
		storb	r7,[ra](r6,r5)
		endexpect
		expect	1350
		storb	r7,[r13](r2,r1)
		endexpect
		; disp14(rrp)
		storb	r7,[r13]1(r6,r5)	; C64F 0071 (bits 0..3)
		storb	r7,[r13]15(r6,r5)	; C64F 007F
		storb	r7,[r13]16(r4,r3)	; C65E 0070 (additionally bits 5..4)
		storb	r7,[r13]63(r4,r3)	; C67E 007F
		storb	r7,[r13]64(r11,r10)	; C64D 4070 (additionally bits 7..6)
		storb	r7,[r13]255(r11,r10)	; C67D C07F
		storb	r7,[r13]256(r9,r8)	; C64C 0170 (additionally bits 13..8)
		storb	r7,[r13]16383(r9,r8)	; C67C FF7F
		; disp20(rrp)
		storb	r7,[r13]16384(r7,r6)	; 0013 607B 4000
		storb	r7,[r13]0x12345(r7,r6)	; 0013 617B 2345
		storb	r7,[r13]1048575(r7,r6)	; 0013 6F7B FFFF
		expect	1320
		storb	r7,[r13]1048576(r7,r6)
		endexpect
		expect	1315
		storb	r7,[r13]-1(r7,r6)
		endexpect

		; abs20
		storb	r7,0		; C870 0000
		storb	r7,0x10000	; C871 0000
		storb	r7,0x12345	; C871 2345
		storb	r7,0xffffff	; C87F FFFF
		; abs24
		storb	r7,0x100000	; 0013 7071 0000
		storb	r7,0x123456	; 0013 7271 3456
		storb	r7,0xfeffff	; 0013 7E7F FFFF
		expect	1320
		storb	r7,0x1000000
		endexpect

		; abs20 rel
		storb	r7,[r12]0	; CA70 0000
		storb	r7,[r13]0x10000	; CB71 0000
		storb	r7,[r12]0x12345	; CA71 2345
		storb	r7,[r13]0xfffff	; CB7F FFFF
		expect	1320
		storb	r7,[r12]0x100000
		endexpect
		expect	1130
		storb	r7,[r11]0x12345
		endexpect
		expect	1439
		storb	r7,[ra]0x12345
		endexpect

		; imm4,disp20(reg)
		storb	$7,(r4)			; 0012 0074 0000
		storb	$7,1(r4)		; 0012 0074 0001
		storb	$7,0x3fff(r4)		; 0012 0074 3FFF
		storb	$7,0x4000(r4)		; 0012 0074 4000
		storb	$7,0xffff(r4)		; 0012 0074 FFFF
		storb	$7,0x10000(r4)		; 0012 0174 0000
		storb	$7,0x12345(r4)		; 0012 0174 2345
		storb	$7,0xfffff(r4)		; 0012 0F74 FFFF
		expect	1320
		storb	$7,0x100000(r4)
		endexpect
		expect	1315
		storb	$7,-1(r4)
		endexpect

		; imm4,disp0(rp)
		storb	$7,(r12)		; 827C
		storb	$7,0(r12)		; 827C
		; imm4,disp16(rp)
		storb	$7,1(r12)		; 837C 0001
		storb	$7,0xffff(r12)		; 837C FFFF
		; imm4,disp20(rp)
		storb	$7,0x10000(r12)		; 0012 117C 0000
		storb	$7,0x12345(r12)		; 0012 117C 2345
		storb	$7,0xfffff(r12)		; 0012 1F7C FFFF
		expect	1320
		storb	$7,0x100000(r12)
		endexpect
		expect	1315
		storb	$7,-1(r12)
		endexpect

		; imm4,disp14(rrp)
		storb	$7,[r13](r3,r2)		; 8609 0070 (bits 0..3)
		storb	$7,[r13]0(r3,r2)	; 8609 0070
		storb	$7,[r13]15(r3,r2)	; 8609 007F
		storb	$7,[r13]16(r3,r2)	; 8619 0070 (additionally bits 4..5)
		storb	$7,[r13]63(r3,r2)	; 8639 007F
		storb	$7,[r13]64(r3,r2)	; 8609 4070 (additionally bits 6..7)
		storb	$7,[r13]255(r3,r2)	; 8639 C07F
		storb	$7,[r13]256(r3,r2)	; 8609 0170 (additionally bits 8..13)
		storb	$7,[r13]0x1234(r3,r2)	; 8639 1274
		storb	$7,[r13]0x3fff(r3,r2)	; 8639 FF7F
		; imm4,disp20(rrp)
		storb	$7,[r13]0x4000(r3,r2)	; 0012 2079 0000
		storb	$7,[r13]0x12345(r3,r2)	; 0012 2179 2345
		storb	$7,[r13]0xfffff(r3,r2)	; 0012 2F79 FFFF
		expect	1320
		storb	$7,[r13]0x100000(r3,r2)
		endexpect
		expect	1315
		storb	$7,[r13]-1(r3,r2)
		endexpect

		; imm4,abs20
		storb	$7,0		; 8170 0000
		storb	$7,0x10000	; 8171 0000
		storb	$7,0x12345	; 8171 2345
		storb	$7,0xffffff	; 817F FFFF
		; imm4,abs24
		storb	$7,0x100000	; 0012 3071 0000
		storb	$7,0x123456	; 0012 3271 3456
		storb	$7,0xfeffff	; 0012 3E7F FFFF
		expect	1320
		storb	$7,0x1000000
		endexpect

		; imm4,abs20 rel
		storb	$7,[r12]0	; 8470 0000
		storb	$7,[r13]0x10000	; 8571 0000
		storb	$7,[r12]0x12345	; 8471 2345
		storb	$7,[r13]0xfffff	; 857F FFFF
		expect	1320
		storb	$7,[r12]0x100000
		endexpect
		expect	1130
		storb	$7,[r11]0x12345
		endexpect
		expect	1439
		storb	$7,[ra]0x12345
		endexpect

		; disp20(reg)
		stord	(r8,r7),(r4)		; 0013 8074 0000
		stord	(r8,r7),0(r4)		; 0013 8074 0000
		stord	(r8,r7),0x3fff(r4)	; 0013 8074 3FFF
		stord	(r8,r7),0x4000(r4)	; 0013 8074 4000
		stord	(r8,r7),0xfffff(r4)	; 0013 8F74 FFFF
		expect	1320
		stord	(r8,r7),0x100000(r4)
		endexpect
		; -disp20(reg)
		stord	(r8,r7),-1(r4)		; 0019 8F74 FFFF
		stord	(r8,r7),-0xfffff(r4)	; 0019 8074 0001
		stord	(r8,r7),-0x100000(r4)	; 0019 8074 0000
		expect	1315
		stord	(r8,r7),-0x100001(r4)
		endexpect

		; disp4/16(rp)
		stord	(r8,r7),(r12)		; E07C
		stord	(r8,r7),0(r12_l,r11)	; E07B
		stord	(r8,r7),2(r12)		; E17C
		stord	(r8,r7),26(r12)		; ED7C
		stord	(r8,r7),27(r12)		; EF7C 001B
		stord	(r8,r7),15(r12)		; EF7C 000F
		stord	(r8,r7),16(r12)		; E87C
		stord	(r8,r7),65535(r12)	; EF7C FFFF
		; disp20(rp)
		stord	(r8,r7),65536(r12)	; 0013 917C 0000
		stord	(r8,r7),0xfffff(r12)	; 0013 9F7C FFFF
		expect	1320
		stord	(r8,r7),0x100000(r12)
		endexpect
		; -disp20(rp)
		stord	(r8,r7),-1(r12)		; 0019 9F7C FFFF
		stord	(r8,r7),-0xfffff(r12)	; 0019 907C 0001
		stord	(r8,r7),-0x100000(r12)	; 0019 907C 0000
		expect	1315
		stord	(r8,r7),-0x100001(r12)
		endexpect

		; disp0(rrp)
		stord	(r8,r7),[r12](r1,r0)		; EE70
		stord	(r8,r7),[r12](r3,r2)		; EE71
		stord	(r8,r7),[r12](r5,r4)		; EE72
		stord	(r8,r7),[r12](r7,r6)		; EE73
		stord	(r8,r7),[r12](r9,r8)		; EE74
		stord	(r8,r7),[r12](r11,r10)		; EE75
		stord	(r8,r7),[r12](r4,r3)		; EE76
		stord	(r8,r7),[r12](r6,r5)		; EE77
		stord	(r8,r7),[r13](r1,r0)		; EE78
		stord	(r8,r7),[r13](r3,r2)		; EE79
		stord	(r8,r7),[r13](r5,r4)		; EE7A
		stord	(r8,r7),[r13](r7,r6)		; EE7B
		stord	(r8,r7),[r13](r9,r8)		; EE7C
		stord	(r8,r7),[r13](r11,r10)		; EE7D
		stord	(r8,r7),[r13](r4,r3)		; EE7E
		stord	(r8,r7),[r13](r6,r5)		; EE7F
		expect	1439
		stord	(r8,r7),[ra](r6,r5)
		endexpect
		expect	1350
		stord	(r8,r7),[r13](r2,r1)
		endexpect
		; disp14(rrp)
		stord	(r8,r7),[r13]1(r6,r5)		; C68F 0071 (bits 0..3)
		stord	(r8,r7),[r13]15(r6,r5)		; C68F 007F
		stord	(r8,r7),[r13]16(r4,r3)		; C69E 0070 (additionally bits 5..4)
		stord	(r8,r7),[r13]63(r4,r3)		; C6BE 007F
		stord	(r8,r7),[r13]64(r11,r10)	; C68D 4070 (additionally bits 7..6)
		stord	(r8,r7),[r13]255(r11,r10)	; C6BD C07F
		stord	(r8,r7),[r13]256(r9,r8)		; C68C 0170 (additionally bits 13..8)
		stord	(r8,r7),[r13]16383(r9,r8)	; C6BC FF7F
		; disp20(rrp)
		stord	(r8,r7),[r13]16384(r7,r6)	; 0013 A07B 4000
		stord	(r8,r7),[r13]0x12345(r7,r6)	; 0013 A17B 2345
		stord	(r8,r7),[r13]1048575(r7,r6)	; 0013 AF7B FFFF
		expect	1320
		stord	(r8,r7),[r13]1048576(r7,r6)
		endexpect
		expect	1315
		stord	(r8,r7),[r13]-1(r7,r6)
		endexpect

		; abs20
		stord	(r8,r7),0		; C770 0000
		stord	(r8,r7),0x10000		; C771 0000
		stord	(r8,r7),0x12345		; C771 2345
		stord	(r8,r7),0xffffff	; C77F FFFF
		; abs24
		stord	(r8,r7),0x100000	; 0013 B071 0000
		stord	(r8,r7),0x123456	; 0013 B271 3456
		stord	(r8,r7),0xfeffff	; 0013 BE7F FFFF
		expect	1320
		stord	(r8,r7),0x1000000
		endexpect

		; abs20 rel
		stord	(r8,r7),[r12]0		; CC70 0000
		stord	(r8,r7),[r13]0x10000	; CD71 0000
		stord	(r8,r7),[r12]0x12345	; CC71 2345
		stord	(r8,r7),[r13]0xfffff	; CD7F FFFF
		expect	1320
		stord	(r8,r7),[r12]0x100000
		endexpect
		expect	1130
		stord	(r8,r7),[r11]0x12345
		endexpect
		expect	1439
		stord	(r8,r7),[ra]0x12345
		endexpect

		; stord imm4,... not implemented
		expect	1350
		stord	$7,(r13)
		endexpect

		expect	1315
		storm	$0
		endexpect
		storm	$1		; 00B0
		storm	$2		; 00B1
		storm	$8		; 00B7
		expect	1320
		storm	$9
		endexpect

		expect	1315
		stormp	$0
		endexpect
		stormp	$1		; 00B8
		stormp	$2		; 00B9
		stormp	$8		; 00BF
		expect	1320
		stormp	$9
		endexpect

		; disp20(reg)
		storw	r7,(r4)		; 0013 C074 0000
		storw	r7,0(r4)	; 0013 C074 0000
		storw	r7,0x3fff(r4)	; 0013 C074 3FFF
		storw	r7,0x4000(r4)	; 0013 C074 4000
		storw	r7,0xfffff(r4)	; 0013 CF74 FFFF
		expect	1320
		storw	r7,0x100000(r4)
		endexpect
		; -disp20(reg)
		storw	r7,-1(r4)	; 0019 CF74 FFFF
		storw	r7,-0xfffff(r4)	; 0019 C074 0001
		storw	r7,-0x100000(r4); 0019 C074 0000
		expect	1315
		storw	r7,-0x100001(r4)
		endexpect

		; disp4/16(rp)
		storw	r7,(r12)	; D07C
		storw	r7,0(r12_l,r11)	; D07B
		storw	r7,2(r12)	; D17C
		storw	r7,26(r12)	; DD7C
		storw	r7,14(r12)	; D77C
		storw	r7,15(r12)	; DF7C 000F
		storw	r7,16(r12)	; D87C
		storw	r7,65535(r12)	; DF7C FFFF
		; disp20(rp)
		storw	r7,65536(r12)	; 0013 D17C 0000
		storw	r7,0xfffff(r12)	; 0013 DF7C FFFF
		expect	1320
		storw	r7,0x100000(r12)
		endexpect
		; -disp20(rp)
		storw	r7,-1(r12)		; 0019 DF7C FFFF
		storw	r7,-0xfffff(r12)	; 0019 D07C 0001
		storw	r7,-0x100000(r12)	; 0019 D07C 0000
		expect	1315
		storw	r7,-0x100001(r12)
		endexpect

		; disp0(rrp)
		storw	r7,[r12](r1,r0)		; DE70
		storw	r7,[r12](r3,r2)		; DE71
		storw	r7,[r12](r5,r4)		; DE72
		storw	r7,[r12](r7,r6)		; DE73
		storw	r7,[r12](r9,r8)		; DE74
		storw	r7,[r12](r11,r10)	; DE75
		storw	r7,[r12](r4,r3)		; DE76
		storw	r7,[r12](r6,r5)		; DE77
		storw	r7,[r13](r1,r0)		; DE78
		storw	r7,[r13](r3,r2)		; DE79
		storw	r7,[r13](r5,r4)		; DE7A
		storw	r7,[r13](r7,r6)		; DE7B
		storw	r7,[r13](r9,r8)		; DE7C
		storw	r7,[r13](r11,r10)	; DE7D
		storw	r7,[r13](r4,r3)		; DE7E
		storw	r7,[r13](r6,r5)		; DE7F
		expect	1439
		storw	r7,[ra](r6,r5)
		endexpect
		expect	1350
		storw	r7,[r13](r2,r1)
		endexpect
		; disp14(rrp)
		storw	r7,[r13]1(r6,r5)	; C6CF 0071 (bits 0..3)
		storw	r7,[r13]15(r6,r5)	; C6CF 007F
		storw	r7,[r13]16(r4,r3)	; C6DE 0070 (additionally bits 5..4)
		storw	r7,[r13]63(r4,r3)	; C6FE 007F
		storw	r7,[r13]64(r11,r10)	; C6CD 4070 (additionally bits 7..6)
		storw	r7,[r13]255(r11,r10)	; C6FD C07F
		storw	r7,[r13]256(r9,r8)	; C6CC 0170 (additionally bits 13..8)
		storw	r7,[r13]16383(r9,r8)	; C6FC FF7F
		; disp20(rrp)
		storw	r7,[r13]16384(r7,r6)	; 0013 E07B 4000
		storw	r7,[r13]0x12345(r7,r6)	; 0013 E17B 2345
		storw	r7,[r13]1048575(r7,r6)	; 0013 EF7B FFFF
		expect	1320
		storw	r7,[r13]1048576(r7,r6)
		endexpect
		expect	1315
		storw	r7,[r13]-1(r7,r6)
		endexpect

		; abs20
		storw	r7,0		; C970 0000
		storw	r7,0x10000	; C971 0000
		storw	r7,0x12345	; C971 2345
		storw	r7,0xffffff	; C97F FFFF
		; abs24
		storw	r7,0x100000	; 0013 F071 0000
		storw	r7,0x123456	; 0013 F271 3456
		storw	r7,0xfeffff	; 0013 FE7F FFFF
		expect	1320
		storw	r7,0x1000000
		endexpect

		; abs20 rel
		storw	r7,[r12]0	; CE70 0000
		storw	r7,[r13]0x10000	; CF71 0000
		storw	r7,[r12]0x12345	; CE71 2345
		storw	r7,[r13]0xfffff	; CF7F FFFF
		expect	1320
		storw	r7,[r12]0x100000
		endexpect
		expect	1130
		storw	r7,[r11]0x12345
		endexpect
		expect	1439
		storw	r7,[ra]0x12345
		endexpect

		; imm4,disp20(reg)
		storw	$7,(r4)			; 0013 0074 0000
		storw	$7,1(r4)		; 0013 0074 0001
		storw	$7,0x3fff(r4)		; 0013 0074 3FFF
		storw	$7,0x4000(r4)		; 0013 0074 4000
		storw	$7,0xffff(r4)		; 0013 0074 FFFF
		storw	$7,0x10000(r4)		; 0013 0174 0000
		storw	$7,0x12345(r4)		; 0013 0174 2345
		storw	$7,0xfffff(r4)		; 0013 0F74 FFFF
		expect	1320
		storw	$7,0x100000(r4)
		endexpect
		expect	1315
		storw	$7,-1(r4)
		endexpect

		; imm4,disp0(rp)
		storw	$7,(r12)		; C27C
		storw	$7,0(r12)		; C27C
		; imm4,disp16(rp)
		storw	$7,1(r12)		; C37C 0001
		storw	$7,0xffff(r12)		; C37C FFFF
		; imm4,disp20(rp)
		storw	$7,0x10000(r12)		; 0013 117C 0000
		storw	$7,0x12345(r12)		; 0013 117C 2345
		storw	$7,0xfffff(r12)		; 0013 1F7C FFFF
		expect	1320
		storw	$7,0x100000(r12)
		endexpect
		expect	1315
		storw	$7,-1(r12)
		endexpect

		; imm4,disp14(rrp)
		storw	$7,[r13](r3,r2)		; C609 0070 (bits 0..3)
		storw	$7,[r13]0(r3,r2)	; C609 0070
		storw	$7,[r13]15(r3,r2)	; C609 007F
		storw	$7,[r13]16(r3,r2)	; C619 0070 (additionally bits 4..5)
		storw	$7,[r13]63(r3,r2)	; C639 007F
		storw	$7,[r13]64(r3,r2)	; C609 4070 (additionally bits 6..7)
		storw	$7,[r13]255(r3,r2)	; C639 C07F
		storw	$7,[r13]256(r3,r2)	; C609 0170 (additionally bits 8..13)
		storw	$7,[r13]0x1234(r3,r2)	; C639 1274
		storw	$7,[r13]0x3fff(r3,r2)	; C639 FF7F
		; imm4,disp20(rrp)
		storw	$7,[r13]0x4000(r3,r2)	; 0013 2079 0000
		storw	$7,[r13]0x12345(r3,r2)	; 0013 2179 2345
		storw	$7,[r13]0xfffff(r3,r2)	; 0013 2F79 FFFF
		expect	1320
		storw	$7,[r13]0x100000(r3,r2)
		endexpect
		expect	1315
		storw	$7,[r13]-1(r3,r2)
		endexpect

		; imm4,abs20
		storw	$7,0		; C170 0000
		storw	$7,0x10000	; C171 0000
		storw	$7,0x12345	; C171 2345
		storw	$7,0xffffff	; C17F FFFF
		; imm4,abs24
		storw	$7,0x100000	; 0013 3071 0000
		storw	$7,0x123456	; 0013 3271 3456
		storw	$7,0xfeffff	; 0013 3E7F FFFF
		expect	1320
		storw	$7,0x1000000
		endexpect

		; imm4,abs20 rel
		storw	$7,[r12]0	; C470 0000
		storw	$7,[r13]0x10000	; C571 0000
		storw	$7,[r12]0x12345	; C471 2345
		storw	$7,[r13]0xfffff	; C57F FFFF
		expect	1320
		storw	$7,[r12]0x100000
		endexpect
		expect	1130
		storw	$7,[r11]0x12345
		endexpect
		expect	1439
		storw	$7,[ra]0x12345
		endexpect

		subb	$1,r7		; 3817
		subb	$8,r9		; 3889
		subb	$9,r3		; 38B3 0009
		subb	$10,r5		; 38A5
		subb	$11,r6		; 38B6 000B
		subb	$12,r4		; 38C4
		subb	$-1,r2		; 3892
		subb	$0xff,r2	;  "
		subb	$15,r1		; 38F1
		subb	$16,r8		; 38B8 0010
		subb	$-2,r10		; 38BA 00FE
		subb	$-128,r11	; 38BB 0080
		subb	$0xfe,r12_l	; 38BC 00FE
		subb	r4,r9		; 3949

		subcb	$1,r7		; 3C17
		subcb	$8,r9		; 3C89
		subcb	$9,r3		; 3CB3 0009
		subcb	$10,r5		; 3CA5
		subcb	$11,r6		; 3CB6 000B
		subcb	$12,r4		; 3CC4
		subcb	$-1,r2		; 3C92
		subcb	$0xff,r2	;  "
		subcb	$15,r1		; 3CF1
		subcb	$16,r8		; 3CB8 0010
		subcb	$-2,r10		; 3CBA 00FE
		subcb	$-128,r11	; 3CBB 0080
		subcb	$0xfe,r12_l	; 3CBC 00FE
		subcb	r4,r9		; 3D49

		subcw	$1,r7		; 3E17
		subcw	$8,r9		; 3E89
		subcw	$9,r3		; 3EB3 0009
		subcw	$10,r5		; 3EA5
		subcw	$11,r6		; 3EB6 000B
		subcw	$12,r4		; 3EC4
		subcw	$-1,r2		; 3E92
		subcw	$0xffff,r2	;  "
		subcw	$15,r1		; 3EF1
		subcw	$16,r8		; 3EB8 0010
		subcw	$-2,r10		; 3EBA FFFE
		subcw	$-32768,r11	; 3EBB 8000
		subcw	$0xfffe,r12_l	; 3EBC FFFE
		subcw	r4,r9		; 3F49

		; SUBD only has imm32 variant:
		expect	1350
		subd	$1,(r8,r6)		; register pair not consecutive
		endexpect
		expect	1350
		subd	$1,(r13,r12)		; no pair of 16 bit registers
		endexpect
		expect	1350
		subd	$1,(r12,r11)		; mixture of 32/16 bit registers
		endexpect
		subd	$1,(r8,r7)		; 0037 0001 0000
		subd	$8,(r10,r9)		; 0039 0008 0000
		subd	$9,(r4,r3)		; 0033 0009 0000
		subd	$10,(r6,r5)		; 0035 000A 0000
		subd	$11,(r7,r6)		; 0036 000B 0000
		subd	$12,(r5,r4)		; 0034 000C 0000
		subd	$-1,(r3,r2)		; 0032 FFFF FFFF
		subd	$0xffffffff,(r3,r2)	;  "
		subd	$15,(r2,r1)		; 0031 000F 0000
		subd	$16,(r9,r8)		; 0038 0010 0000
		subd	$-2,(r11,r10)		; 003A FFFE FFFF
		subd	$-128,(r12_l,r11)	; 003B FF80 FFFF
		subd	$0xfe,r12		; 003C 00FE 0000
		subd	$-32768,(r12_l,r11)	; 003B 8000 FFFF
		subd	$0x7fff,r12		; 003C 7FFF 0000
		subd	$-32769,(r12_l,r11)	; 003B 7FFF FFFF
		subd	$0x8000,r12		; 003C 8000 0000
		subd	$-524288,(r12_l,r11)	; 003B 0000 FFF8
		subd	$0x7FFFF,r12		; 003C FFFF 0007
		subd	$-524289,(r12_l,r11)	; 003B FFFF FFF7
		subd	$0x80000,r12		; 003C 0000 0008
		subd	$-2147483648,(r12_l,r11); 003B 0000 8000
		subd	$0xffefffff,r12		; 003C FFFF FFEF
		subd	(r5,r4),(r10,r9)	; 0014 C049

		subw	$1,r7		; 3A17
		subw	$8,r9		; 3A89
		subw	$9,r3		; 3AB3 0009
		subw	$10,r5		; 3AA5
		subw	$11,r6		; 3AB6 000B
		subw	$12,r4		; 3AC4
		subw	$-1,r2		; 3A92
		subw	$0xffff,r2	;  "
		subw	$15,r1		; 3AF1
		subw	$16,r8		; 3AB8 0010
		subw	$-2,r10		; 3ABA FFFE
		subw	$-32768,r11	; 3ABB 8000
		subw	$0xfffe,r12_l	; 3ABC FFFE
		subw	r4,r9		; 3B49

		tbit	$0,r4		; 0604
		tbit	$1,r4		; 0614
		tbit	$15,r4		; 06F4
		expect	1320
		tbit	$16,r4
		endexpect
		expect	1130
		tbit	$12,r12
		endexpect
		tbit	r12_l,r4	; 07C4
		expect	1130
		tbit	r12,r4
		endexpect

		; disp20(reg)
		tbitb	$7,(r4)		; 0010 C074 0000
		tbitb	$7,0(r4)	; 0010 C074 0000
		tbitb	$7,0x3fff(r4)	; 0010 C074 3FFF
		tbitb	$7,0x4000(r4)	; 0010 C074 4000
		tbitb	$7,0xfffff(r4)	; 0010 CF74 FFFF
		expect	1320
		tbitb	$7,0x100000(r4)
		endexpect
		expect	1315
		tbitb	$7,-1(r4)
		endexpect
		; disp0(rp)
		tbitb	$7,(r12)	; 7A7C
		tbitb	$7,0(r12_l,r11)	; 7A7B
		; disp16(rp)
		tbitb	$7,1(r12)	; 7B7C 0001
		tbitb	$7,13(r12)	; 7B7C 000D
		tbitb	$7,14(r12)	; 7B7C 000E
		tbitb	$7,15(r12)	; 7B7C 000F
		tbitb	$7,16(r12)	; 7B7C 0010
		tbitb	$7,65535(r12)	; 7B7C FFFF
		; disp20(rp)
		tbitb	$7,65536(r12)	; 0010 D17C 0000
		tbitb	$7,0xfffff(r12)	; 0010 DF7C FFFF
		expect	1320
		tbitb	$7,0x100000(r12)
		endexpect
		expect	1315
		tbitb	$7,-1(r12)
		endexpect
		; disp14(rrp)
		tbitb	$7,[r12](r5,r4)		; 7A82 0070
		tbitb	$7,[r13](r5,r4)		; 7A8A 0070
		tbitb	$7,[r13]1(r6,r5)	; 7A8F 0071 (bits 0..3)
		tbitb	$7,[r13]15(r6,r5)	; 7A8F 007F
		tbitb	$7,[r13]16(r4,r3)	; 7A9E 0070 (additionally bits 5..4)
		tbitb	$7,[r13]63(r4,r3)	; 7ABE 007F
		tbitb	$7,[r13]64(r11,r10)	; 7A8D 4070 (additionally bits 7..6)
		tbitb	$7,[r13]255(r11,r10)	; 7ABD C07F
		tbitb	$7,[r13]256(r9,r8)	; 7A8C 0170 (additionally bits 13..8)
		tbitb	$7,[r13]16383(r9,r8)	; 7ABC FF7F
		; disp20(rrp)
		tbitb	$7,[r13]16384(r7,r6)	; 0010 E07B 4000
		tbitb	$7,[r13]0x12345(r7,r6)	; 0010 E17B 2345
		tbitb	$7,[r13]1048575(r7,r6)	; 0010 EF7B FFFF
		expect	1320
		tbitb	$7,[r13]1048576(r7,r6)
		endexpect
		expect	1315
		tbitb	$7,[r13]-1(r7,r6)
		endexpect
		; abs20
		tbitb	$7,0		; 7BF0 0000
		tbitb	$7,0x10000	; 7BF1 0000
		tbitb	$7,0x12345	; 7BF1 2345
		tbitb	$7,0xffffff	; 7BFF FFFF
		; abs24
		tbitb	$7,0x100000	; 0010 F071 0000
		tbitb	$7,0x123456	; 0010 F271 3456
		tbitb	$7,0xfeffff	; 0010 FE7F FFFF
		expect	1320
		tbitb	$7,0x1000000
		endexpect
		; abs20 rel
		tbitb	$7,[r12]0	; 7870 0000
		tbitb	$7,[r13]0x10000	; 78F1 0000
		tbitb	$7,[r12]0x12345	; 7871 2345
		tbitb	$7,[r13]0xfffff	; 78FF FFFF
		expect	1320
		tbitb	$7,[r12]0x100000
		endexpect
		expect	1130
		tbitb	$7,[r11]0x12345
		endexpect
		expect	1439
		tbitb	$7,[ra]0x12345
		endexpect

		; disp20(reg)
		tbitw	$7,(r4)		; 0011 C074 0000
		tbitw	$7,0(r4)	; 0011 C074 0000
		tbitw	$7,0x3fff(r4)	; 0011 C074 3FFF
		tbitw	$7,0x4000(r4)	; 0011 C074 4000
		tbitw	$7,0xfffff(r4)	; 0011 CF74 FFFF
		expect	1320
		tbitw	$7,0x100000(r4)
		endexpect
		expect	1315
		tbitw	$7,-1(r4)
		endexpect
		; disp0(rp)
		tbitw	$7,(r12)	; 7E7C
		tbitw	$7,0(r12_l,r11)	; 7E7B
		; disp16(rp)
		tbitw	$7,1(r12)	; 797C 0001
		tbitw	$7,13(r12)	; 797C 000D
		tbitw	$7,14(r12)	; 797C 000E
		tbitw	$7,15(r12)	; 797C 000F
		tbitw	$7,16(r12)	; 797C 0010
		tbitw	$7,65535(r12)	; 797C FFFF
		; disp20(rp)
		tbitw	$7,65536(r12)	; 0011 D17C 0000
		tbitw	$7,0xfffff(r12)	; 0011 DF7C FFFF
		expect	1320
		tbitw	$7,0x100000(r12)
		endexpect
		expect	1315
		tbitw	$7,-1(r12)
		endexpect
		; disp14(rrp)
		tbitw	$7,[r12](r5,r4)		; 7AC2 0070
		tbitw	$7,[r13](r5,r4)		; 7ACA 0070
		tbitw	$7,[r13]1(r6,r5)	; 7ACF 0071 (bits 0..3)
		tbitw	$7,[r13]15(r6,r5)	; 7ACF 007F
		tbitw	$7,[r13]16(r4,r3)	; 7ADE 0070 (additionally bits 5..4)
		tbitw	$7,[r13]63(r4,r3)	; 7AFE 007F
		tbitw	$7,[r13]64(r11,r10)	; 7ACD 4070 (additionally bits 7..6)
		tbitw	$7,[r13]255(r11,r10)	; 7AFD C07F
		tbitw	$7,[r13]256(r9,r8)	; 7ACC 0170 (additionally bits 13..8)
		tbitw	$7,[r13]16383(r9,r8)	; 7AFC FF7F
		; disp20(rrp)
		tbitw	$7,[r13]16384(r7,r6)	; 0011 E07B 4000
		tbitw	$7,[r13]0x12345(r7,r6)	; 0011 E17B 2345
		tbitw	$7,[r13]1048575(r7,r6)	; 0011 EF7B FFFF
		expect	1320
		tbitw	$7,[r13]1048576(r7,r6)
		endexpect
		expect	1315
		tbitw	$7,[r13]-1(r7,r6)
		endexpect
		; abs20
		tbitw	$7,0		; 7F70 0000
		tbitw	$7,0x10000	; 7F71 0000
		tbitw	$7,0x12345	; 7F71 2345
		tbitw	$7,0xffffff	; 7F7F FFFF
		; abs24
		tbitw	$7,0x100000	; 0011 F071 0000
		tbitw	$7,0x123456	; 0011 F271 3456
		tbitw	$7,0xfeffff	; 0011 FE7F FFFF
		expect	1320
		tbitw	$7,0x1000000
		endexpect
		; abs20 rel
		tbitw	$7,[r12]0	; 7C70 0000
		tbitw	$7,[r13]0x10000	; 7D71 0000
		tbitw	$7,[r12]0x12345	; 7C71 2345
		tbitw	$7,[r13]0xfffff	; 7D7F FFFF
		expect	1320
		tbitw	$7,[r12]0x100000
		endexpect
		expect	1130
		tbitw	$7,[r11]0x12345
		endexpect
		expect	1439
		tbitw	$7,[ra]0x12345
		endexpect

		wait	   		; 0006

		xorb	$1,r7		; 2817
		xorb	$8,r9		; 2889
		xorb	$9,r3		; 28B3 0009
		xorb	$10,r5		; 28A5
		xorb	$11,r6		; 28B6 000B
		xorb	$12,r4		; 28C4
		xorb	$-1,r2		; 2892
		xorb	$0xff,r2	;  "
		xorb	$15,r1		; 28F1
		xorb	$16,r8		; 28B8 0010
		xorb	$-2,r10		; 28BA 00FE
		xorb	$-128,r11	; 28BB 0080
		xorb	$0xfe,r12_l	; 28BC 00FE
		xorb	r4,r9		; 2949

		; XORD only has imm32 variant:
		expect	1350
		xord	$1,(r8,r6)		; register pair not consecutive
		endexpect
		expect	1350
		xord	$1,(r13,r12)		; no pair of 16 bit registers
		endexpect
		expect	1350
		xord	$1,(r12,r11)		; mixture of 32/16 bit registers
		endexpect
		xord	$1,(r8,r7)		; 0067 0001 0000
		xord	$8,(r10,r9)		; 0069 0008 0000
		xord	$9,(r4,r3)		; 0063 0009 0000
		xord	$10,(r6,r5)		; 0065 000A 0000
		xord	$11,(r7,r6)		; 0066 000B 0000
		xord	$12,(r5,r4)		; 0064 000C 0000
		xord	$-1,(r3,r2)		; 0062 FFFF FFFF
		xord	$0xffffffff,(r3,r2)	;  "
		xord	$15,(r2,r1)		; 0061 000F 0000
		xord	$16,(r9,r8)		; 0068 0010 0000
		xord	$-2,(r11,r10)		; 006A FFFE FFFF
		xord	$-128,(r12_l,r11)	; 006B FF80 FFFF
		xord	$0xfe,r12		; 006C 00FE 0000
		xord	$-32768,(r12_l,r11)	; 006B 8000 FFFF
		xord	$0x7fff,r12		; 006C 7FFF 0000
		xord	$-32769,(r12_l,r11)	; 006B 7FFF FFFF
		xord	$0x8000,r12		; 006C 8000 0000
		xord	$-524288,(r12_l,r11)	; 006B 0000 FFF8
		xord	$0x7FFFF,r12		; 006C FFFF 0007
		xord	$-524289,(r12_l,r11)	; 006B FFFF FFF7
		xord	$0x80000,r12		; 006C 0000 0008
		xord	$-2147483648,(r12_l,r11); 006B 0000 8000
		xord	$0xffefffff,r12		; 006C FFFF FFEF
		xord	(r5,r4),(r10,r9)	; 0014 A049

		xorw	$1,r7		; 2A17
		xorw	$8,r9		; 2A89
		xorw	$9,r3		; 2AB3 0009
		xorw	$10,r5		; 2AA5
		xorw	$11,r6		; 2AB6 000B
		xorw	$12,r4		; 2AC4
		xorw	$-1,r2		; 2A92
		xorw	$0xffff,r2	;  "
		xorw	$15,r1		; 2AF1
		xorw	$16,r8		; 2AB8 0010
		xorw	$-2,r10		; 2ABA FFFE
		xorw	$-32768,r11	; 2ABB 8000
		xorw	$0xfffe,r12_l	; 2ABC FFFE
		xorw	r4,r9		; 2B49

