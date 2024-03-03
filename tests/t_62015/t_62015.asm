;;; -*- asm -*-
;;;

	CPU	SC62015
	ORG	0000H

	;; a1 MV (Immediate)
	MV	A,0
	MV	BA,1024
	MV	X,131072
	MV	(00H),10
	MVW	(BP+10H),1034
	MVP	(PX-10H),131082

	;; a2 MV (Inter-Register)
	MV	B,A
	MV	A,B
	MV	BA,I
	MV	X,Y

	;; a3 MV (Internal RAM Direct / Indirect)
	MV	IL,(BP+PX)
	MV	I,(08H)
	MV	U,(BP+20H)
	MV	(PX-20H),A
	MV	(BP+PX),BA
	MV	(18H),S
	MV	(BP+30H),(PY-30H)
	MVW	(BP+PX),(20H)
	MVP	(PX+40H),(BP+PY)
	MVL	(28H),(30H)
	MVLD	(BP-40H),(BP+50H)

	;; a4 MV (Direct)
	MV	IL,[12345H]
	MV	I,[23456H]
	MV	X,[34567H]
	MV	[45678H],A
	MV	[56789H],BA
	MV	[6789AH],Y
	MV	(38H),[789ABH]
	MVW	(BP-50H),[89ABCH]
	MVP	(PX+60H),[9ABCDH]
	MVL	(BP+PX),[54321H]
	MV	[65432H],(40H)
	MVW	[76543H],(BP-60H)
	MVP	[87654H],(PX+70H)
	MVL	[98765H],(BP+PX)

	;; a5 Register Indirect
	MV	IL,[U]
	MV	I,[S]
	MV	X,[Y]
	MV	[U],A
	MV	[S],BA
	MV	[X],Y
	MV	(48H),[U]
	MVW	(BP-70H),[S]
	MVP	(PX-80H),[X]
	MV	[Y],(BP+PX)
	MVW	[U],(50H)
	MVP	[S],(BP+0)

	;; a6 MV (with Post Increment)
	MV	IL,[X++]
	MV	I,[Y++]
	MV	U,[S++]
	MV	[X++],A
	MV	[Y++],BA
	MV	[U++],X
	MV	(PX+8),[S++]
	MVW	(BP+PX),[X++]
	MVP	(58H),[Y++]
	MVL	(BP+16),[U++]
	MV	[S++],(PX+24)
	MVW	[X++],(BP+PX)
	MVP	[Y++],(60H)
	MVL	[U++],(BP+32)

	;; a7 MV (with Pre Decrement)
	MV	IL,[--S]
	MV	I,[--X]
	MV	Y,[--U]
	MV	[--S],A
	MV	[--X],BA
	MV	[--Y],U
	MV	(PX+40),[--S]
	MVW	(BP+PX),[--X]
	MVP	(68H),[--Y]
	MVL	(BP+48),[--U]
	MV	[--S],(PX+56)
	MVW	[--X],(BP+PX)
	MVP	[--Y],(70H)
	MVL	[--U],(BP+64)

	;; a8 MV (Register Base)
	MV	IL,[S+1]
	MV	I,[X-2]
	MV	Y,[U+3]
	MV	[S-4],A
	MV	[X+5],BA
	MV	[Y-6],U

	MV	(78H),[S+7]
	MVW	(BP+64),[X-8]
	MVP	(PX+72),[Y+9]
	MVL	(BP+PX),[U-10]
	MV	[S+11],(80H)
	MVW	[X-12],(BP+80)
	MVP	[Y+13],(PX+88)
	MVL	[U-14],(BP+PX)

	;; a9 MV (Internal RAM Indirect)
	MV	IL,[(88H)]
	MV	I,[(BP+96)]
	MV	X,[(PX+104)]
	MV	[(BP+PX)],A
	MV	[(90H)],BA
	MV	[(BP+112)],Y
	MV	(PX+120),[(BP+PY)]
	MVW	(98H),[(00H)]
	MVP	(BP-128),[(PY-120)]
	MVL	(BP+PX),[(04H)]
	MV	[(BP-112)],(PY-104)
	MVW	[(BP+PX)],(BP+PY)
	MVP	[(08H)],(BP-96)
	MVL	[(PX-88)],(BP+PY)

	;; a10 MV (Internal RAM Base)
	MV	A,[(0CH)+16]
	MV	I,[(BP-80)-32]
	MV	U,[(PX-72)+48]
	MV	[(BP+PX)-64],IL
	MV	[(10H)+80],BA
	MV	[(BP-64)-96],X
	MV	(PX-56),[(BP+PY)+112]
	MVW	(14H),[(18H)-128]
	MVP	(BP-48),[(BP-40)+144]
	MVL	(PX-32),[(PY-24)-160]
	MV	[(BP+PX)+176],(BP+PY)
	MVW	[(1CH)-192],(BP-16)
	MVP	[(PX-8)+208],(BP+PY)
	MVL	[(20H)-224],(BP-0)

	;; b1 EX instruction
	EX	A,B
	EX	BA,I
	EX	X,Y
	EX	(00H),(BP+10)
	EXW	(PX-10),(BP+PY)
	EXP	(BP+20),(PY-20)
	EXL	(BP+PX),(10H)

	;; b2 SWAP instruction
	SWAP	A

	;; c1 ADD instruction
	ADD	A,20
	ADD	(BP+30),30
	ADD	(PX-30),A
	ADD	A,(BP+PX)
	ADD	A,IL
	ADD	BA,A
	ADD	I,BA
	ADD	X,IL

	;; c2 ADC
	ADC	A,40
	ADC	(20H),50
	ADC	(BP+40),A
	ADC	A,(PX-40)

	;; c3 SUB
	SUB	A,60
	SUB	(BP+PX),70
	SUB	(30H),A
	SUB	A,(BP+50)
	SUB	IL,A
	SUB	I,IL
	SUB	BA,I
	SUB	Y,BA

	;; c4 SBC
	SBC	A,60
	SBC	(PX-50),70
	SBC	(BP+PX),A
	SBC	A,(40H)

	;;  c5 ADCL
	ADCL	(BP+60),(PY-60)
	ADCL	(BP+PX),A

	;; c6 SBCL
	SBCL	(BP+PX),(50H)
	SBCL	(BP+70),A

	;; c7 DADL
	DADL	(PX-70),(BP+PY)
	DADL	(60H),A

	;; c8 DSBL
	DSBL	(BP+80),(PY-80)
	DSBL	(BP+PX),A

	;; c9 PMDF
	PMDF	(70H),80
	PMDF	(BP+90),A

	;; d1 AND
	AND	A,90
	AND	(PX-90),100
	AND	[12345H],110
	AND	(BP+PX),A
	AND	A,(80H)
	AND	(BP+100),(PY-100)

	;; d2 OR
	OR	A,120
	OR	(BP+PX),130
	OR	[23456H],140
	OR	(90H),A
	OR	A,(BP+110)
	OR	(PX-110),(BP+PY)

	;; d3 XOR
	XOR	A,150
	XOR	(08H),160
	XOR	[34567H],170
	XOR	(BP+120),A
	XOR	A,(PX-120)
	XOR	(BP+PX),(18H)

	;; e1 INC
	INC	A
	INC	(BP+0)

	;; e2 DEC
	DEC	BA
	DEC	(PX+8)

	;; f1 ROR
	ROR	A
	ROR	(BP+PX)

	;; f2 ROL
	ROL	A
	ROL	(18H)

	;; f3 SHR
	SHR	A
	SHR	(BP-8)

	;; f4 SHL
	SHL	A
	SHL	(PX+16)

	;; f5 DSRL
	DSRL	(BP+PX)
	DSLL	(28H)

	;; g1 CMP
	CMP	A,180
	CMP	(BP+24),190
	CMP	[45678H],200
	CMP	(PX+32),A
	CMP	(BP+PX),(30H)
	CMPW	(BP+40),(PY+48)
	CMPW	(BP+PX),I
	CMPP	(38H),(BP+56)
	CMPP	(PX+64),U

	;; g2 TEST
	TEST	A,210
	TEST	(BP+PX),220
	TEST	[56789H],230
	TEST	(40H),A

	;; h1 JP/JPF (Direct)
	JP	1234H
	JPF	12345H

	;; h2 JP (Indirect)
	JP	(BP+72)
	JP	S

	;; h3 JR
L1:	JR	L1

	;; h4 JPcc
	JPZ	2345H
	JPNZ	3456H
	JPC	4567H
	JPNC	5678H

	;; h5 JRcc
L2:	JRZ	L2
	JRNZ	L4
L3:	JRC	L2
L4:	JRNC	L4

	;; i1 CALL/CALLF
	CALL	6789H
	CALLF	23456H

	;; i2 RET
	RET
	RETF

	;;  j1 PUSH
	PUSHS	A
	PUSHS	BA
	PUSHS	X
	PUSHS	F
	PUSHS	IMR
	PUSHU	IL
	PUSHU	I
	PUSHU	Y
	PUSHU	F
	PUSHU	IMR

	;; j2 POP
	POPS	IL
	POPS	I
	POPS	Y
	POPS	F
	POPS	IMR
	POPU	A
	POPU	BA
	POPU	X
	POPU	F
	POPU	IMR

	;; k1 NOP
	NOP

	;; k2 WAIT
	WAIT

	;; k3 SC/RC
	SC
	RC

	;; k4 RETI
	RETI

	;; k5 HALT/OFF
	HALT
	OFF

	;; k6 TCL/IR/RESET
	TCL
	IR
	RESET


	;;
	DB	100,50H
	DB	'AB'
	DW	1234H
