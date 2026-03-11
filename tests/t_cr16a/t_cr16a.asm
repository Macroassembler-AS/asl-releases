		cpu	cr16a

		page	0

		org	0x10000

		addb	r5,r7		; 40EB
		; ADDB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		addb	$0,r7		; 00E0
		addb	$1,r7		; 00E1
		addb	$15,r7		; 00EF
		addb	$16,r7		; 00F1 0010
		addb	$127,r7		; 00F1 007F
		addb	$0xef,r7	; 00F1 00EF
		addb	$0xf0,r7	; 00F0
		addb	$0xf1,r7	; 00F1 00F1
		addb	$0xf2,r7	; 00F2
		addb	$0xff,r7	; 00FF
		expect	1320
		addb	$0x100,r7
		endexpect
		addb	$-1,r7		; 00FF
		addb	$-14,r7		; 00F2
		addb	$-15,r7		; 00F1 00F1
		addb	$-16,r7		; 00F0
		addb	$-17,r7		; 00F1 00EF
		addb	$-128,r7	; 00F1 0080
		expect	1315
		addb	$-129,r7
		endexpect

		addw	r5,r7		; 60EB
		; ADDW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		addw	$0,r7		; 20E0
		addw	$1,r7		; 20E1
		addw	$15,r7		; 20EF
		addw	$16,r7		; 20F1 0010
		addw	$32767,r7	; 20F1 7FFF
		addw	$0xffef,r7	; 20F1 FFEF
		addw	$0xfff0,r7	; 20F0
		addw	$0xfff1,r7	; 20F1 FFF1
		addw	$0xfff2,r7	; 20F2
		addw	$0xffff,r7	; 20FF
		expect	1320
		addw	$0x10000,r7
		endexpect
		addw	$-1,r7		; 20FF
		addw	$-14,r7		; 20F2
		addw	$-15,r7		; 20F1 FFF1
		addw	$-16,r7		; 20F0
		addw	$-17,r7		; 20F1 FFEF
		addw	$-32768,r7	; 20F1 8000
		expect	1315
		addw	$-32769,r7
		endexpect

		addcb	r5,r7		; 52EB
		; ADDCB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		addcb	$0,r7		; 12E0
		addcb	$1,r7		; 12E1
		addcb	$15,r7		; 12EF
		addcb	$16,r7		; 12F1 0010
		addcb	$127,r7		; 12F1 007F
		addcb	$0xef,r7	; 12F1 00EF
		addcb	$0xf0,r7	; 12F0
		addcb	$0xf1,r7	; 12F1 00F1
		addcb	$0xf2,r7	; 12F2
		addcb	$0xff,r7	; 12FF
		expect	1320
		addcb	$0x100,r7
		endexpect
		addcb	$-1,r7		; 12FF
		addcb	$-14,r7		; 12F2
		addcb	$-15,r7		; 12F1 00F1
		addcb	$-16,r7		; 12F0
		addcb	$-17,r7		; 12F1 00EF
		addcb	$-128,r7	; 12F1 0080
		expect	1315
		addcb	$-129,r7
		endexpect

		addcw	r5,r7		; 72EB
		; ADDCW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		addcw	$0,r7		; 32E0
		addcw	$1,r7		; 32E1
		addcw	$15,r7		; 32EF
		addcw	$16,r7		; 32F1 0010
		addcw	$32767,r7	; 32F1 7FFF
		addcw	$0xffef,r7	; 32F1 FFEF
		addcw	$0xfff0,r7	; 32F0
		addcw	$0xfff1,r7	; 32F1 FFF1
		addcw	$0xfff2,r7	; 32F2
		addcw	$0xffff,r7	; 32FF
		expect	1320
		addcw	$0x10000,r7
		endexpect
		addcw	$-1,r7		; 32FF
		addcw	$-14,r7		; 32F2
		addcw	$-15,r7		; 32F1 FFF1
		addcw	$-16,r7		; 32F0
		addcw	$-17,r7		; 32F1 FFEF
		addcw	$-32768,r7	; 32F1 8000
		expect	1315
		addcw	$-32769,r7
		endexpect

		addub	r5,r7		; 42EB
		; ADDUB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		addub	$0,r7		; 02E0
		addub	$1,r7		; 02E1
		addub	$15,r7		; 02EF
		addub	$16,r7		; 02F1 0010
		addub	$127,r7		; 02F1 007F
		addub	$0xef,r7	; 02F1 00EF
		addub	$0xf0,r7	; 02F0
		addub	$0xf1,r7	; 02F1 00F1
		addub	$0xf2,r7	; 02F2
		addub	$0xff,r7	; 02FF
		expect	1320
		addub	$0x100,r7
		endexpect
		addub	$-1,r7		; 02FF
		addub	$-14,r7		; 02F2
		addub	$-15,r7		; 02F1 00F1
		addub	$-16,r7		; 02F0
		addub	$-17,r7		; 02F1 00EF
		addub	$-128,r7	; 02F1 0080
		expect	1315
		addub	$-129,r7
		endexpect

		adduw	r5,r7		; 62EB
		; ADDUW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		adduw	$0,r7		; 22E0
		adduw	$1,r7		; 22E1
		adduw	$15,r7		; 22EF
		adduw	$16,r7		; 22F1 0010
		adduw	$32767,r7	; 22F1 7FFF
		adduw	$0xffef,r7	; 22F1 FFEF
		adduw	$0xfff0,r7	; 22F0
		adduw	$0xfff1,r7	; 22F1 FFF1
		adduw	$0xfff2,r7	; 22F2
		adduw	$0xffff,r7	; 22FF
		expect	1320
		adduw	$0x10000,r7
		endexpect
		adduw	$-1,r7		; 22FF
		adduw	$-14,r7		; 22F2
		adduw	$-15,r7		; 22F1 FFF1
		adduw	$-16,r7		; 22F0
		adduw	$-17,r7		; 22F1 FFEF
		adduw	$-32768,r7	; 22F1 8000
		expect	1315
		adduw	$-32769,r7
		endexpect

		andb	r5,r7		; 50EB
		; ANDB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		andb	$0,r7		; 10E0
		andb	$1,r7		; 10E1
		andb	$15,r7		; 10EF
		andb	$16,r7		; 10F1 0010
		andb	$127,r7		; 10F1 007F
		andb	$0xef,r7	; 10F1 00EF
		andb	$0xf0,r7	; 10F0
		andb	$0xf1,r7	; 10F1 00F1
		andb	$0xf2,r7	; 10F2
		andb	$0xff,r7	; 10FF
		expect	1320
		andb	$0x100,r7
		endexpect
		andb	$-1,r7		; 10FF
		andb	$-14,r7		; 10F2
		andb	$-15,r7		; 10F1 00F1
		andb	$-16,r7		; 10F0
		andb	$-17,r7		; 10F1 00EF
		andb	$-128,r7	; 10F1 0080
		expect	1315
		andb	$-129,r7
		endexpect

		andw	r5,r7		; 70EB
		; ANDW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		andw	$0,r7		; 30E0
		andw	$1,r7		; 30E1
		andw	$15,r7		; 30EF
		andw	$16,r7		; 30F1 0010
		andw	$32767,r7	; 30F1 7FFF
		andw	$0xffef,r7	; 30F1 FFEF
		andw	$0xfff0,r7	; 30F0
		andw	$0xfff1,r7	; 30F1 FFF1
		andw	$0xfff2,r7	; 30F2
		andw	$0xffff,r7	; 30FF
		expect	1320
		andw	$0x10000,r7
		endexpect
		andw	$-1,r7		; 30FF
		andw	$-14,r7		; 30F2
		andw	$-15,r7		; 30F1 FFF1
		andw	$-16,r7		; 30F0
		andw	$-17,r7		; 30F1 FFEF
		andw	$-32768,r7	; 30F1 8000
		expect	1315
		andw	$-32769,r7
		endexpect

		ashub	r5,r7		; 48EB
		; ASHUB: 5 bit immediate value sign-extended to 8 bits, may be used as -8...-1
                ; 0xf0...0xff does not make sense, and -8 has principally undefined behaviour
		ashub	$0,r7		; 08E0
		ashub	$1,r7		; 08E1
		ashub	$7,r7		; 08E7
		expect	1320
		ashub	$8,r7
		endexpect
		expect	1320
		ashub	$0xef,r7
		endexpect
		expect	1320
		ashub	$0xf0,r7
		endexpect
		expect	1320
		ashub	$0xff,r7
		endexpect
		ashub	$-1,r7		; 08FF
		ashub	$-8,r7		; 08F8
		expect	1315
		ashub	$-9,r7
		endexpect
		expect	1315
		ashub	$-128,r7
		endexpect

		ashuw	r5,r7		; 68EB
		; ASHUW: 5 bit immediate value sign-extended to 16 bits, may be used as -16...-1
                ; 0xfff0...0xffff does not make sense, and -16 has principally undefined behaviour
		ashuw	$0,r7		; 28E0
		ashuw	$1,r7		; 28E1
		ashuw	$15,r7		; 28EF
		expect	1320
		ashuw	$16,r7
		endexpect
		expect	1320
		ashuw	$0xffef,r7
		endexpect
		expect	1320
		ashuw	$0xfff0,r7
		endexpect
		expect	1320
		ashuw	$0xffff,r7
		endexpect
		ashuw	$-1,r7		; 28FF
		ashuw	$-16,r7		; 28F0
		expect	1315
		ashuw	$-17,r7
		endexpect
		expect	1315
		ashuw	$-32768,r7
		endexpect

		bal	r13,*		; 35BE FFFC
		bal	r13,*+4		; 35AE 0000
		expect	1351
		bal	r13,*+255
		endexpect
		bal	r13,*+256	; 35AE 00FC
		bal	r13,*+258	; 35AE 00FE
		expect	1351
		bal	r13,*-253
		endexpect
		bal	r13,*-254	; 35BE FEFE
		bal	r13,*-256	; 35BE FEFC

		beq	*		; 5E1E
		beq	*+2		; 4000
		expect	1351
		beq	*+255
		endexpect
		beq	*+256		; 4E1E
		beq	*+258		; 140E 00FE
		expect	1351
		beq	*-253
		endexpect
		beq	*-254		; 5000
		beq	*-256		; 141E FEFC

		bne	*		; 5E3E
		bne	*+2		; 4020
		expect	1351
		bne	*+255
		endexpect
		bne	*+256		; 4E3E
		bne	*+258		; 142E 00FE
		expect	1351
		bne	*-253
		endexpect
		bne	*-254		; 5020
		bne	*-256		; 143E FEFC

		bge	*		; 5FBE
		bge	*+2		; 41A0
		expect	1351
		bge	*+255
		endexpect
		bge	*+256		; 4FBE
		bge	*+258		; 15AE 00FE
		expect	1351
		bge	*-253
		endexpect
		bge	*-254		; 51A0
		bge	*-256		; 15BE FEFC

		bcs	*		; 5E5E
		bcs	*+2		; 4040
		expect	1351
		bcs	*+255
		endexpect
		bcs	*+256		; 4E5E
		bcs	*+258		; 144E 00FE
		expect	1351
		bcs	*-253
		endexpect
		bcs	*-254		; 5040
		bcs	*-256		; 145E FEFC

		bcc	*		; 5E7E
		bcc	*+2		; 4060
		expect	1351
		bcc	*+255
		endexpect
		bcc	*+256		; 4E7E
		bcc	*+258		; 146E 00FE
		expect	1351
		bcc	*-253
		endexpect
		bcc	*-254		; 5060
		bcc	*-256		; 147E FEFC

		bhi	*		; 5E9E
		bhi	*+2		; 4080
		expect	1351
		bhi	*+255
		endexpect
		bhi	*+256		; 4E9E
		bhi	*+258		; 148E 00FE
		expect	1351
		bhi	*-253
		endexpect
		bhi	*-254		; 5080
		bhi	*-256		; 149E FEFC

		bls	*		; 5EBE
		bls	*+2		; 40A0
		expect	1351
		bls	*+255
		endexpect
		bls	*+256		; 4EBE
		bls	*+258		; 14AE 00FE
		expect	1351
		bls	*-253
		endexpect
		bls	*-254		; 50A0
		bls	*-256		; 14BE FEFC

		blo	*		; 5F5E
		blo	*+2		; 4140
		expect	1351
		blo	*+255
		endexpect
		blo	*+256		; 4F5E
		blo	*+258		; 154E 00FE
		expect	1351
		blo	*-253
		endexpect
		blo	*-254		; 5140
		blo	*-256		; 155E FEFC

		bhs	*		; 5F7E
		bhs	*+2		; 4160
		expect	1351
		bhs	*+255
		endexpect
		bhs	*+256		; 4F7E
		bhs	*+258		; 156E 00FE
		expect	1351
		bhs	*-253
		endexpect
		bhs	*-254		; 5160
		bhs	*-256		; 157E FEFC

		bgt	*		; 5EDE
		bgt	*+2		; 40C0
		expect	1351
		bgt	*+255
		endexpect
		bgt	*+256		; 4EDE
		bgt	*+258		; 14CE 00FE
		expect	1351
		bgt	*-253
		endexpect
		bgt	*-254		; 50C0
		bgt	*-256		; 14DE FEFC

		ble	*		; 5EFE
		ble	*+2		; 40E0
		expect	1351
		ble	*+255
		endexpect
		ble	*+256		; 4EFE
		ble	*+258		; 14EE 00FE
		expect	1351
		ble	*-253
		endexpect
		ble	*-254		; 50E0
		ble	*-256		; 14FE FEFC

		bfs	*		; 5F1E
		bfs	*+2		; 4100
		expect	1351
		bfs	*+255
		endexpect
		bfs	*+256		; 4F1E
		bfs	*+258		; 150E 00FE
		expect	1351
		bfs	*-253
		endexpect
		bfs	*-254		; 5100
		bfs	*-256		; 151E FEFC

		bfc	*		; 5F3E
		bfc	*+2		; 4120
		expect	1351
		bfc	*+255
		endexpect
		bfc	*+256		; 4F3E
		bfc	*+258		; 152E 00FE
		expect	1351
		bfc	*-253
		endexpect
		bfc	*-254		; 5120
		bfc	*-256		; 153E FEFC

		br	*		; 5FDE
		br	*+2		; 41C0
		expect	1351
		br	*+255
		endexpect
		br	*+256		; 4FDE
		br	*+258		; 15CE 00FE
		expect	1351
		br	*-253
		endexpect
		br	*-254		; 51C0
		br	*-256		; 15DE FEFC

		blt	*		; 5F9E
		blt	*+2		; 4180
		expect	1351
		blt	*+255
		endexpect
		blt	*+256		; 4F9E
		blt	*+258		; 158E 00FE
		expect	1351
		blt	*-253
		endexpect
		blt	*-254		; 5180
		blt	*-256		; 159E FEFC

		cmpb	r5,r7		; 4EEB
		; CMPB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		cmpb	$0,r7		; 0EE0
		cmpb	$1,r7		; 0EE1
		cmpb	$15,r7		; 0EEF
		cmpb	$16,r7		; 0EF1 0010
		cmpb	$127,r7		; 0EF1 007F
		cmpb	$0xef,r7	; 0EF1 00EF
		cmpb	$0xf0,r7	; 0EF0
		cmpb	$0xf1,r7	; 0EF1 00F1
		cmpb	$0xf2,r7	; 0EF2
		cmpb	$0xff,r7	; 0EFF
		expect	1320
		cmpb	$0x100,r7
		endexpect
		cmpb	$-1,r7		; 0EFF
		cmpb	$-14,r7		; 0EF2
		cmpb	$-15,r7		; 0EF1 00F1
		cmpb	$-16,r7		; 0EF0
		cmpb	$-17,r7		; 0EF1 00EF
		cmpb	$-128,r7	; 0EF1 0080
		expect	1315
		cmpb	$-129,r7
		endexpect

		cmpw	r5,r7		; 6EEB
		; CMPW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		cmpw	$0,r7		; 2EE0
		cmpw	$1,r7		; 2EE1
		cmpw	$15,r7		; 2EEF
		cmpw	$16,r7		; 2EF1 0010
		cmpw	$32767,r7	; 2EF1 7FFF
		cmpw	$0xffef,r7	; 2EF1 FFEF
		cmpw	$0xfff0,r7	; 2EF0
		cmpw	$0xfff1,r7	; 2EF1 FFF1
		cmpw	$0xfff2,r7	; 2EF2
		cmpw	$0xffff,r7	; 2EFF
		expect	1320
		cmpw	$0x10000,r7
		endexpect
		cmpw	$-1,r7		; 2EFF
		cmpw	$-14,r7		; 2EF2
		cmpw	$-15,r7		; 2EF1 FFF1
		cmpw	$-16,r7		; 2EF0
		cmpw	$-17,r7		; 2EF1 FFEF
		cmpw	$-32768,r7	; 2EF1 8000
		expect	1315
		cmpw	$-32769,r7
		endexpect

		di			; 7DDE

		ei			; 7DFE

		excp	svc		; 7BEA
		excp	dvz		; 7BEC
		excp	flg		; 7BEE
		excp	bpt		; 7BF0
		excp	und		; 7BF4
		expect	1441
		excp	non
		endexpect

		jal	r7,r5		; 74EB

		jump	r3		; 55C7
		jump	r13		; 55DB
		jump	ra		; 55DD
		jump	sp		; 55DF

		jeq	r1		; 5403
		jne	r2		; 5425
		jge	r3		; 55A7
		jcs	r4		; 5449
		jcc	r5		; 546B
		jhi	r6		; 548D
		jls	r7		; 54AF
		jlo	r8		; 5551
		jhs	r9		; 5573
		jgt	r10		; 54D5
		jle	r11		; 54F7
		jfs	r12		; 5519
		jfc	r13		; 553B
		jlt	ra		; 559D

		loadb	(r12),r9	; 8138
		loadb	0(r12),r9	; 8138
		loadb	15(r12),r9	; 8F39
		loadb	16(r12),r9	; 9139 0010
		loadb	0x3ffff(r12),r9	; 9739 FFFF
		expect	1320
		loadb	0x40000(r12),r9
		endexpect
		loadb	-16(r12),r9	; 9138
		loadb	-17(r12),r9	; 9739 FFEF
		loadb	-0x20000(r12),r9; 9539 0000
		expect	1315
		loadb	-0x20001(r12),r9
		endexpect
		loadb	(r8,r7),r9	; 992F 0000
		loadb	0(r8,r7),r9	; 992F 0000
		loadb	15(r8,r7),r9	; 992F 000F
		loadb	16(r8,r7),r9	; 992F 0010
		loadb	0x3ffff(r8,r7),r9; 9F2F FFFF
		expect	1320
		loadb	0x40000(r8,r7),r9
		endexpect
		loadb	-16(r8,r7),r9	; 9F2F FFF0
		loadb	-17(r8,r7),r9	; 9F2F FFEF
		loadb	-0x20000(r8,r7),r9; 9D2F 0000
		expect	1315
		loadb	-0x20001(r8,r7),r9
		endexpect
		expect	1350
		loadb	16(r7,r8),r9
		endexpect
		expect	1350
		loadb	16(r0,sp),r9
		endexpect
		loadb	0x34567,r9	; 9F3F 4567

		loadw	(r12),r9	; A138
		loadw	0(r12),r9	; A138
		loadw	15(r12),r9	; AF39
		loadw	16(r12),r9	; B139 0010
		loadw	0x3ffff(r12),r9	; B739 FFFF
		expect	1320
		loadw	0x40000(r12),r9
		endexpect
		loadw	-16(r12),r9	; B138
		loadw	-17(r12),r9	; B739 FFEF
		loadw	-0x20000(r12),r9; B539 0000
		expect	1315
		loadw	-0x20001(r12),r9
		endexpect
		loadw	(r8,r7),r9	; B92F 0000
		loadw	0(r8,r7),r9	; B92F 0000
		loadw	15(r8,r7),r9	; B92F 000F
		loadw	16(r8,r7),r9	; B92F 0010
		loadw	0x3ffff(r8,r7),r9; BF2F FFFF
		expect	1320
		loadw	0x40000(r8,r7),r9
		endexpect
		loadw	-16(r8,r7),r9	; BF2F FFF0
		loadw	-17(r8,r7),r9	; BF2F FFEF
		loadw	-0x20000(r8,r7),r9; BD2F 0000
		expect	1315
		loadw	-0x20001(r8,r7),r9
		endexpect
		expect	1350
		loadw	16(r7,r8),r9
		endexpect
		expect	1350
		loadw	16(r0,sp),r9
		endexpect
		loadw	0x34567,r9	; BF3F 4567

		lpr	r1,psr		; 7022
		lpr	r7,intbase	; 706E
		lpr	r13,isp		; 717A
		expect	1440
		lpr	sp,nix
		endexpect

		lshb	r5,r7		; 4AEB
		; LSHB: 5 bit immediate value sign-extended to 8 bits, may be used as -8...-1
                ; 0xf0...0xff does not make sense, and -8 has principally undefined behaviour
		lshb	$0,r7		; 0AE0
		lshb	$1,r7		; 0AE1
		lshb	$7,r7		; 0AE7
		expect	1320
		lshb	$8,r7
		endexpect
		expect	1320
		lshb	$0xef,r7
		endexpect
		expect	1320
		lshb	$0xf0,r7
		endexpect
		expect	1320
		lshb	$0xff,r7
		endexpect
		lshb	$-1,r7		; 0AFF
		lshb	$-8,r7		; 0AF8
		expect	1315
		lshb	$-9,r7
		endexpect
		expect	1315
		lshb	$-128,r7
		endexpect

		lshw	r5,r7		; 6AEB
		; LSHW: 5 bit immediate value sign-extended to 16 bits, may be used as -16...-1
                ; 0xfff0...0xffff does not make sense, and -16 has principally undefined behaviour
		lshw	$0,r7		; 2AE0
		lshw	$1,r7		; 2AE1
		lshw	$15,r7		; 2AEF
		expect	1320
		lshw	$16,r7
		endexpect
		expect	1320
		lshw	$0xffef,r7
		endexpect
		expect	1320
		lshw	$0xfff0,r7
		endexpect
		expect	1320
		lshw	$0xffff,r7
		endexpect
		lshw	$-1,r7		; 2AFF
		lshw	$-16,r7		; 2AF0
		expect	1315
		lshw	$-17,r7
		endexpect
		expect	1315
		lshw	$-32768,r7
		endexpect

		movb	r5,r7		; 58EB
		; MOVB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		movb	$0,r7		; 18E0
		movb	$1,r7		; 18E1
		movb	$15,r7		; 18EF
		movb	$16,r7		; 18F1 0010
		movb	$127,r7		; 18F1 007F
		movb	$0xef,r7	; 18F1 00EF
		movb	$0xf0,r7	; 18F0
		movb	$0xf1,r7	; 18F1 00F1
		movb	$0xf2,r7	; 18F2
		movb	$0xff,r7	; 18FF
		expect	1320
		movb	$0x100,r7
		endexpect
		movb	$-1,r7		; 18FF
		movb	$-14,r7		; 18F2
		movb	$-15,r7		; 18F1 00F1
		movb	$-16,r7		; 18F0
		movb	$-17,r7		; 18F1 00EF
		movb	$-128,r7	; 18F1 0080
		expect	1315
		movb	$-129,r7
		endexpect

		movw	r5,r7		; 78EB
		; MOVW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		movw	$0,r7		; 38E0
		movw	$1,r7		; 38E1
		movw	$15,r7		; 38EF
		movw	$16,r7		; 38F1 0010
		movw	$32767,r7	; 38F1 7FFF
		movw	$0xffef,r7	; 38F1 FFEF
		movw	$0xfff0,r7	; 38F0
		movw	$0xfff1,r7	; 38F1 FFF1
		movw	$0xfff2,r7	; 38F2
		movw	$0xffff,r7	; 38FF
		expect	1320
		movw	$0x10000,r7
		endexpect
		movw	$-1,r7		; 38FF
		movw	$-14,r7		; 38F2
		movw	$-15,r7		; 38F1 FFF1
		movw	$-16,r7		; 38F0
		movw	$-17,r7		; 38F1 FFEF
		movw	$-32768,r7	; 38F1 8000
		expect	1315
		movw	$-32769,r7
		endexpect

		movxb	r5,r7		; 68EA

		movzb	r5,r7		; 6AEA

		mulb	r5,r7		; 46EB
		; MULB: 5 bit immediate value sign-extended to 8 bits, 0xf0...0xff does not make sense here
		mulb	$0,r7		; 06E0
		mulb	$1,r7		; 06E1
		mulb	$15,r7		; 06EF
		mulb	$16,r7		; 06F1 0010
		mulb	$127,r7		; 06F1 007F
		expect	1320
		mulb	$128,r7
		endexpect
		expect  1320
		mulb	$0xf0,r7	; 06F0
		endexpect
		expect  1320
		mulb	$0xff,r7	; 06FF
		endexpect
		mulb	$-1,r7		; 06FF
		mulb	$-15,r7		; 06F1 00F1
		mulb	$-16,r7		; 06F0
		mulb	$-17,r7		; 06F1 00EF
		mulb	$-128,r7	; 06F1 0080
		expect	1315
		mulb	$-129,r7
		endexpect

		mulw	r5,r7		; 66EB
		; MULW: 5 bit immediate value sign-extended to 16 bits, 0xfff0...0xffff does not make sense here
		mulw	$0,r7		; 26E0
		mulw	$1,r7		; 26E1
		mulw	$15,r7		; 26EF
		mulw	$16,r7		; 26F1 0010
		mulw	$32767,r7	; 26F1 7FFF
		expect	1320
		mulw	$32768,r7
		endexpect
		expect	1320
		mulw	$0xfff0,r7	; 26F0
		endexpect
		expect	1320
		mulw	$0xffff,r7	; 26FF
		endexpect
		mulw	$-1,r7		; 26FF
		mulw	$-15,r7		; 26F1 FFF1
		mulw	$-16,r7		; 26F0
		mulw	$-17,r7		; 26F1 FFEF
		mulw	$-32768,r7	; 26F1 8000
		expect	1315
		mulw	$-32769,r7
		endexpect

		nop			; 0200

		orb	r5,r7		; 5CEB
		; ORB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		orb	$0,r7		; 1CE0
		orb	$1,r7		; 1CE1
		orb	$15,r7		; 1CEF
		orb	$16,r7		; 1CF1 0010
		orb	$127,r7		; 1CF1 007F
		orb	$0xef,r7	; 1CF1 00EF
		orb	$0xf0,r7	; 1CF0
		orb	$0xf1,r7	; 1CF1 00F1
		orb	$0xf2,r7	; 1CF2
		orb	$0xff,r7	; 1CFF
		expect	1320
		orb	$0x100,r7
		endexpect
		orb	$-1,r7		; 1CFF
		orb	$-14,r7		; 1CF2
		orb	$-15,r7		; 1CF1 00F1
		orb	$-16,r7		; 1CF0
		orb	$-17,r7		; 1CF1 00EF
		orb	$-128,r7	; 1CF1 0080
		expect	1315
		orb	$-129,r7
		endexpect

		orw	r5,r7		; 7CEB
		; ORW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		orw	$0,r7		; 3CE0
		orw	$1,r7		; 3CE1
		orw	$15,r7		; 3CEF
		orw	$16,r7		; 3CF1 0010
		orw	$32767,r7	; 3CF1 7FFF
		orw	$0xffef,r7	; 3CF1 FFEF
		orw	$0xfff0,r7	; 3CF0
		orw	$0xfff1,r7	; 3CF1 FFF1
		orw	$0xfff2,r7	; 3CF2
		orw	$0xffff,r7	; 3CFF
		expect	1320
		orw	$0x10000,r7
		endexpect
		orw	$-1,r7		; 3CFF
		orw	$-14,r7		; 3CF2
		orw	$-15,r7		; 3CF1 FFF1
		orw	$-16,r7		; 3CF0
		orw	$-17,r7		; 3CF1 FFEF
		orw	$-32768,r7	; 3CF1 8000
		expect	1315
		orw	$-32769,r7
		endexpect

		seq	r1		; 6E02
		sne	r2		; 6E24
		sge	r3		; 6FA6
		scs	r4		; 6E48
		scc	r5		; 6E6A
		shi	r6		; 6E8C
		sls	r7		; 6EAE
		slo	r8		; 6F50
		shs	r9		; 6F72
		sgt	r10		; 6ED4
		sle	r11		; 6EF6
		sfs	r12		; 6F18
		sfc	r13		; 6F3A
		slt	ra		; 6F9C

		spr	psr,r1		; 7222
		spr	intbase,r7	; 726E
		spr	isp,r13		; 737A
		expect	1440
		spr	nix,sp
		endexpect

		storb	r9,(r12)	; C138
		storb	r9,0(r12)	; C138
		storb	r9,15(r12)	; CF39
		storb	r9,16(r12)	; D139 0010
		storb	r9,0x3ffff(r12)	; D739 FFFF
		expect	1320
		storb	r9,0x40000(r12)
		endexpect
		storb	r9,-16(r12)	; D138
		storb	r9,-17(r12)	; D739 FFEF
		storb	r9,-0x20000(r12); D539 0000
		expect	1315
		storb	r9,-0x20001(r12)
		endexpect
		storb	r9,(r8,r7)	; D92F 0000
		storb	r9,0(r8,r7)	; D92F 0000
		storb	r9,15(r8,r7)	; D92F 000F
		storb	r9,16(r8,r7)	; D92F 0010
		storb	r9,0x3ffff(r8,r7); DF2F FFFF
		expect	1320
		storb	r9,0x40000(r8,r7)
		endexpect
		storb	r9,-16(r8,r7)	; DF2F FFF0
		storb	r9,-17(r8,r7)	; DF2F FFEF
		storb	r9,-0x20000(r8,r7); DD2F 0000
		expect	1315
		storb	r9,-0x20001(r8,r7)
		endexpect
		expect	1350
		storb	r9,16(r7,r8)
		endexpect
		expect	1350
		storb	r9,16(r0,sp)
		endexpect
		storb	r9,0x34567	; DF3F 4567

		storw	r9,(r12)	; E138
		storw	r9,0(r12)	; E138
		storw	r9,15(r12)	; EF39
		storw	r9,16(r12)	; F139 0010
		storw	r9,0x3ffff(r12)	; F739 FFFF
		expect	1320
		storw	r9,0x40000(r12)
		endexpect
		storw	r9,-16(r12)	; F138
		storw	r9,-17(r12)	; F739 FFEF
		storw	r9,-0x20000(r12); F539 0000
		expect	1315
		storw	r9,-0x20001(r12)
		endexpect
		storw	r9,(r8,r7)	; F92F 0000
		storw	r9,0(r8,r7)	; F92F 0000
		storw	r9,15(r8,r7)	; F92F 000F
		storw	r9,16(r8,r7)	; F92F 0010
		storw	r9,0x3ffff(r8,r7); FF2F FFFF
		expect	1320
		storw	r9,0x40000(r8,r7)
		endexpect
		storw	r9,-16(r8,r7)	; FF2F FFF0
		storw	r9,-17(r8,r7)	; FF2F FFEF
		storw	r9,-0x20000(r8,r7); FD2F 0000
		expect	1315
		storw	r9,-0x20001(r8,r7)
		endexpect
		expect	1350
		storw	r9,16(r7,r8)
		endexpect
		expect	1350
		storw	r9,16(r0,sp)
		endexpect
		storw	r9,0x34567	; FF3F 4567

		subb	r5,r7		; 5EEB
		; SUBB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		subb	$0,r7		; 1EE0
		subb	$1,r7		; 1EE1
		subb	$15,r7		; 1EEF
		subb	$16,r7		; 1EF1 0010
		subb	$127,r7		; 1EF1 007F
		subb	$0xef,r7	; 1EF1 00EF
		subb	$0xf0,r7	; 1EF0
		subb	$0xf1,r7	; 1EF1 00F1
		subb	$0xf2,r7	; 1EF2
		subb	$0xff,r7	; 1EFF
		expect	1320
		subb	$0x100,r7
		endexpect
		subb	$-1,r7		; 1EFF
		subb	$-14,r7		; 1EF2
		subb	$-15,r7		; 1EF1 00F1
		subb	$-16,r7		; 1EF0
		subb	$-17,r7		; 1EF1 00EF
		subb	$-128,r7	; 1EF1 0080
		expect	1315
		subb	$-129,r7
		endexpect

		subw	r5,r7		; 7EEB
		; SUBW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		subw	$0,r7		; 3EE0
		subw	$1,r7		; 3EE1
		subw	$15,r7		; 3EEF
		subw	$16,r7		; 3EF1 0010
		subw	$32767,r7	; 3EF1 7FFF
		subw	$0xffef,r7	; 3EF1 FFEF
		subw	$0xfff0,r7	; 3EF0
		subw	$0xfff1,r7	; 3EF1 FFF1
		subw	$0xfff2,r7	; 3EF2
		subw	$0xffff,r7	; 3EFF
		expect	1320
		subw	$0x10000,r7
		endexpect
		subw	$-1,r7		; 3EFF
		subw	$-14,r7		; 3EF2
		subw	$-15,r7		; 3EF1 FFF1
		subw	$-16,r7		; 3EF0
		subw	$-17,r7		; 3EF1 FFEF
		subw	$-32768,r7	; 3EF1 8000
		expect	1315
		subw	$-32769,r7
		endexpect

		subcb	r5,r7		; 5AEB
		; SUBCB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		subcb	$0,r7		; 1AE0
		subcb	$1,r7		; 1AE1
		subcb	$15,r7		; 1AEF
		subcb	$16,r7		; 1AF1 0010
		subcb	$127,r7		; 1AF1 007F
		subcb	$0xef,r7	; 1AF1 00EF
		subcb	$0xf0,r7	; 1AF0
		subcb	$0xf1,r7	; 1AF1 00F1
		subcb	$0xf2,r7	; 1AF2
		subcb	$0xff,r7	; 1AFF
		expect	1320
		subcb	$0x100,r7
		endexpect
		subcb	$-1,r7		; 1AFF
		subcb	$-14,r7		; 1AF2
		subcb	$-15,r7		; 1AF1 00F1
		subcb	$-16,r7		; 1AF0
		subcb	$-17,r7		; 1AF1 00EF
		subcb	$-128,r7	; 1AF1 0080
		expect	1315
		subcb	$-129,r7
		endexpect

		subcw	r5,r7		; 7AEB
		; SUBCW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		subcw	$0,r7		; 3AE0
		subcw	$1,r7		; 3AE1
		subcw	$15,r7		; 3AEF
		subcw	$16,r7		; 3AF1 0010
		subcw	$32767,r7	; 3AF1 7FFF
		subcw	$0xffef,r7	; 3AF1 FFEF
		subcw	$0xfff0,r7	; 3AF0
		subcw	$0xfff1,r7	; 3AF1 FFF1
		subcw	$0xfff2,r7	; 3AF2
		subcw	$0xffff,r7	; 3AFF
		expect	1320
		subcw	$0x10000,r7
		endexpect
		subcw	$-1,r7		; 3AFF
		subcw	$-14,r7		; 3AF2
		subcw	$-15,r7		; 3AF1 FFF1
		subcw	$-16,r7		; 3AF0
		subcw	$-17,r7		; 3AF1 FFEF
		subcw	$-32768,r7	; 3AF1 8000
		expect	1315
		subcw	$-32769,r7
		endexpect

		tbit	r5,r7		; 6AEB
		; TBIT: 5 bit immediate value sign-extended to 16 bits, but meaningful rande is 0...15
		tbit	$0,r7		; 2AE0
		tbit	$1,r7		; 2AE1
		tbit	$15,r7		; 2AEF
		expect	1320
		tbit	$16,r7
		endexpect
		expect	1320
		tbit	$0xffef,r7
		endexpect
		expect	1320
		tbit	$0xfff0,r7
		endexpect
		expect	1320
		tbit	$0xffff,r7
		endexpect
		expect	1315
		tbit	$-1,r7
		endexpect
		expect	1315
		tbit	$-16,r7
		endexpect
		expect	1315
		tbit	$-17,r7
		endexpect
		expect	1315
		tbit	$-32768,r7
		endexpect

		wait			; 7FFE

		xorb	r5,r7		; 4CEB
		; XORB: 5 bit immediate value sign-extended to 8 bits, may be used as -16,-14...-1 or 0xf0,0xf2...0xff
		xorb	$0,r7		; 0CE0
		xorb	$1,r7		; 0CE1
		xorb	$15,r7		; 0CEF
		xorb	$16,r7		; 0CF1 0010
		xorb	$127,r7		; 0CF1 007F
		xorb	$0xef,r7	; 0CF1 00EF
		xorb	$0xf0,r7	; 0CF0
		xorb	$0xf1,r7	; 0CF1 00F1
		xorb	$0xf2,r7	; 0CF2
		xorb	$0xff,r7	; 0CFF
		expect	1320
		xorb	$0x100,r7
		endexpect
		xorb	$-1,r7		; 0CFF
		xorb	$-14,r7		; 0CF2
		xorb	$-15,r7		; 0CF1 00F1
		xorb	$-16,r7		; 0CF0
		xorb	$-17,r7		; 0CF1 00EF
		xorb	$-128,r7	; 0CF1 0080
		expect	1315
		xorb	$-129,r7
		endexpect

		xorw	r5,r7		; 6CEB
		; ORW: 5 bit immediate value sign-extended to 16 bits, may be used as -16,-14...-1 or 0xfff0,0xfff2...0xffff
		xorw	$0,r7		; 1CE0
		xorw	$1,r7		; 1CE1
		xorw	$15,r7		; 1CEF
		xorw	$16,r7		; 1CF1 0010
		xorw	$32767,r7	; 1CF1 7FFF
		xorw	$0xffef,r7	; 1CF1 FFEF
		xorw	$0xfff0,r7	; 1CF0
		xorw	$0xfff1,r7	; 1CF1 FFF1
		xorw	$0xfff2,r7	; 1CF2
		xorw	$0xffff,r7	; 1CFF
		expect	1320
		xorw	$0x10000,r7
		endexpect
		xorw	$-1,r7		; 1CFF
		xorw	$-14,r7		; 1CF2
		xorw	$-15,r7		; 1CF1 FFF1
		xorw	$-16,r7		; 1CF0
		xorw	$-17,r7		; 1CF1 FFEF
		xorw	$-32768,r7	; 1CF1 8000
		expect	1315
		xorw	$-32769,r7
		endexpect

