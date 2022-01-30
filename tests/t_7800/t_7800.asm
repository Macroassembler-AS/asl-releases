		cpu	7800
		page	0

xx		equ	0cch
bbaa		equ	0bbaah
		assume	v:0ffh
wa		equ	0ffdah

		NOP			; 00
		HLT			; 01 (not on 78C06)
		INX	SP		; 02
		DCX	SP		; 03
		LXI	SP,bbaa		; 04 aa bb
		ANIW	wa,xx		; 05 wa xx
		;----			; 06
		ANI	A,xx		; 07 xx
		RET			; 08
		SIO			; 09
		MOV	A,B		; 0A
		MOV	A,C		; 0B
		MOV	A,D		; 0C
		MOV	A,E		; 0D
		MOV	A,H		; 0E
		MOV	A,L		; 0F

		EX			; 10 (not on 78C06)
		EXX			; 11 (not on 78C06)
		INX	B		; (BC) 12
		DCX	B		; (BC) 13
		LXI	B,bbaa		; 14 aa bb
		ORIW	wa,xx		; 15 wa xx
		XRI	A,xx		; 16 xx
		ORI	A,xx		; 17 xx
		RETS			; 18
		STM			; 19
		MOV	B,A		; 1A
		MOV	C,A		; 1B
		MOV	D,A		; 1C
		MOV	E,A		; 1D
		MOV	H,A		; 1E
		MOV	L,A		; 1F

		INRW	wa		; 20 wa
		TABLE			; 21 (not on 78C06)
		INX	D		; (DE) 22
		DCX	D		; (DE) 23
		LXI	D,bbaa		; 24 aa bb
		GTIW	wa,xx		; 25 wa xx
		ADINC	A,xx		; 26 xx
		GTI	A,xx		; 27 xx
		LDAW	wa		; 28 wa
		LDAX	B		; (BC) 29
		LDAX	D		; (DE) 2A
		LDAX	H		; (HL) 2B
		LDAX	D+		; (DE) 2C
		LDAX	H+		; (HL) 2D
		LDAX	D-		; (DE) 2E
		LDAX	H-		; (HL) 2F
		expect	1350,1350,1350,1350,1350
		LDAX	(DE+2)
		LDAX	(HL+A)
		LDAX	(HL+B)
		LDAX	(HL+EA)
		LDAX	(HL-3)
		endexpect

		DCRW	wa		; 30 wa
		BLOCK			; 31 (not on 78C06)
		INX	H		; (HL) 32
		DCX	H		; (HL) 33
		LXI	H,bbaa		; 34 aa bb
		LTIW	wa,xx		; 35 wa xx
		SUINB	A,xx		; 36 xx
		LTI	A,xx		; 37 xx
		STAW	wa		; 38 wa
		STAX	B		; (BC) 39
		STAX	D		; (DE) 3A
		STAX	H		; (HL) 3B
		STAX	D+		; (DE) 3C
		STAX	H+		; (HL) 3D
		STAX	D-		; (DE) 3E
		STAX	H-		; (HL) 3F
		expect	1350,1350,1350,1350,1350
		STAX	(DE+2)
		STAX	(HL+A)
		STAX	(HL+B)
		STAX	(HL+EA)
		STAX	(HL-3)
		endexpect

		;---			; 40
		INR	A		; 41
		INR	B		; 42
		INR	C		; 43
		CALL	bbaa		; 44 aa bb
		ONIW	wa,xx		; 45 wa xx
		ADI	A,xx		; 46 xx
		ONI	A,xx		; 47 xx

		SKIT	F0		; 48 00 (not on 78C06)
		SKIT	FT		; 48 01 (not on 78C06)
		SKIT	F1		; 48 02 (not on 78C06)
		SKIT	F2		; 48 03 (not on 78C06)
		SKIT	FS		; 48 04 (not on 78C06)
		SK	CY		; 48 0A (not on 78C06)
		SK	Z		; 48 0C (not on 78C06)
		PUSH	V		; 48 0E (not on 78C06?)
		POP	V		; 48 0F (not on 78C06?)
		SKNIT	F0		; 48 10
		SKNIT	FT		; 48 11
		SKNIT	F1		; 48 12
		SKNIT	F2		; 48 13 (not on 78C06)
		SKNIT	FS		; 48 14
		SKN	CY		; 48 1A
		SKN	Z		; 48 1C
		PUSH	B		; (BC) 48 1E
		POP	B		; (BC) 48 1F
		EI			; 48 20
		DI			; 48 24
		CLC			; 48 2A
		STC			; 48 2B
		PEN			; 48 2C (not on 78C06)
		PEX			; 48 2D
		PUSH	D		; (DE) 48 2E
		POP	D		; (DE) 48 2F
		RAL			; 48 30
		RAR			; 48 31
		RCL			; 48 32 (not on 78C06)
		RCR			; 48 33 (not on 78C06)
		SHAL			; 48 34 (not on 78C06)
		SHAR			; 48 35 (not on 78C06)
		SHCL			; 48 36 (not on 78C06)
		SHCR			; 48 37 (not on 78C06)
		RLD			; 48 38
		RRD			; 48 39
		PER			; 48 3C
		PUSH	H		; (HL) 48 3E
		POP	H		; (HL) 48 3F

		MVIX	B,xx		; 49 xx (not on 78C06)
		MVIX	D,xx		; 4A xx (not on 78C06)
		MVIX	H,xx		; 4B xx (not on 78C06)
		IN	A,xx		; 4C 00....4C BF (not on 78C06)
		MOV	A,PA		; 4C C0
		MOV	A,PB		; 4C C1
		MOV	A,PC		; 4C C2
		MOV	A,MK		; 4C C3
		expect	1449
		MOV	A,MB		; (4C C4)
		endexpect
		MOV	A,S		; 4C C8
		MOV	A,TMM		; 4C C9 (only on 78C06?)
		expect	1445
		MOV	A,SC		; 4C CB (only on 78C06)
		endexpect
		OUT	xx		; 4D 00....4D BF (not on 78C06)
		MOV	PA,A		; 4D C0
		MOV	PB,A		; 4D C1
		MOV	PC,A		; 4D C2 (not on 78C06)
		MOV	MK,A		; 4D C3
		MOV	MB,A		; 4D C4
		MOV	MC,A		; 4D C5 (not on 78C06)
		MOV	TM0,A		; 4D C6
		MOV	TM1,A		; 4D C7 (not on 78C06)
		MOV	S,A		; 4D C8
		MOV	TMM,A		; 4D C9
		expect	1445,1445
		MOV	SM,A		; 4D CA (only on 78C06)
		MOV	SC,A		; 4D CB (only on 78C06)
		endexpect
		JRE	$+xx		; 4E xx
		JRE	$-xx		; 4F xx

		;---			; 50
		DCR	A		; 51
		DCR	B		; 52
		DCR	C		; 53
		JMP	bbaa		; 54 aa bb
		OFFIW	wa,xx		; 55 wa xx
		ACI	A,xx		; 56 xx
		OFFI	A,xx		; 57 xx
		BIT	0,wa		; 58 wa (not on 78C06)
		BIT	1,wa		; 59 wa (not on 78C06)
		BIT	2,wa		; 5A wa (not on 78C06)
		BIT	3,wa		; 5B wa (not on 78C06)
		BIT	4,wa		; 5C wa (not on 78C06)
		BIT	5,wa		; 5D wa (not on 78C06)
		BIT	6,wa		; 5E wa (not on 78C06)
		BIT	7,wa		; 5F wa (not on 78C06)

		ANA	V,A		; 60 08-0F (not on 78C06)
		ANA	A,A
		ANA	B,A
		ANA	C,A
		ANA	D,A
		ANA	E,A
		ANA	H,A
		ANA	L,A
		XRA	V,A		; 60 10-17 (not on 78C06)
		XRA	A,A
		XRA	B,A
		XRA	C,A
		XRA	D,A
		XRA	E,A
		XRA	H,A
		XRA	L,A
		ORA	V,A		; 60 18-1F (not on 78C06)
		ORA	A,A
		ORA	B,A
		ORA	C,A
		ORA	D,A
		ORA	E,A
		ORA	H,A
		ORA	L,A
		ADDNC	V,A		; 60 20-27 (not on 78C06)
		ADDNC	A,A
		ADDNC	B,A
		ADDNC	C,A
		ADDNC	D,A
		ADDNC	E,A
		ADDNC	H,A
		ADDNC	L,A
		GTA	V,A		; 60 28-2F (not on 78C06)
		GTA	A,A
		GTA	B,A
		GTA	C,A
		GTA	D,A
		GTA	E,A
		GTA	H,A
		GTA	L,A
		SUBNB	V,A		; 60 30-37 (not on 78C06)
		SUBNB	A,A
		SUBNB	B,A
		SUBNB	C,A
		SUBNB	D,A
		SUBNB	E,A
		SUBNB	H,A
		SUBNB	L,A
		LTA	V,A		; 60 38-3F (not on 78C06)
		LTA	A,A
		LTA	B,A
		LTA	C,A
		LTA	D,A
		LTA	E,A
		LTA	H,A
		LTA	L,A
		ADD	V,A		; 60 40-47 (not on 78C06)
		ADD	A,A
		ADD	B,A
		ADD	C,A
		ADD	D,A
		ADD	E,A
		ADD	H,A
		ADD	L,A
		ADC	V,A		; 60 50-57 (not on 78C06)
		ADC	A,A
		ADC	B,A
		ADC	C,A
		ADC	D,A
		ADC	E,A
		ADC	H,A
		ADC	L,A
		SUB	V,A		; 60 60-67 (not on 78C06)
		SUB	A,A
		SUB	B,A
		SUB	C,A
		SUB	D,A
		SUB	E,A
		SUB	H,A
		SUB	L,A
		NEA	V,A		; 60 68-6F (not on 78C06)
		NEA	A,A
		NEA	B,A
		NEA	C,A
		NEA	D,A
		NEA	E,A
		NEA	H,A
		NEA	L,A
		SUB	V,A		; 60 70-7F (not on 78C06)
		SUB	A,A
		SUB	B,A
		SUB	C,A
		SUB	D,A
		SUB	E,A
		SUB	H,A
		SUB	L,A
		ANA	A,V		; 60 88-8F (no V on 78C06)
		ANA	A,A
		ANA	A,B
		ANA	A,C
		ANA	A,D
		ANA	A,E
		ANA	A,H
		ANA	A,L
		XRA	A,V		; 60 90-97 (no V on 78C06)
		XRA	A,A
		XRA	A,B
		XRA	A,C
		XRA	A,D
		XRA	A,E
		XRA	A,H
		XRA	A,L
		ORA	A,V		; 60 98-9F (no V on 78C06)
		ORA	A,A
		ORA	A,B
		ORA	A,C
		ORA	A,D
		ORA	A,E
		ORA	A,H
		ORA	A,L
		ADDNC	A,V		; 60 A0-A7 (no V on 78C06)
		ADDNC	A,A
		ADDNC	A,B
		ADDNC	A,C
		ADDNC	A,D
		ADDNC	A,E
		ADDNC	A,H
		ADDNC	A,L
		GTA	A,V		; 60 A8-AF (no V on 78C06)
		GTA	A,A
		GTA	A,B
		GTA	A,C
		GTA	A,D
		GTA	A,E
		GTA	A,H
		GTA	A,L
		SUBNB	A,V		; 60 B0-B7 (no V on 78C06)
		SUBNB	A,A
		SUBNB	A,B
		SUBNB	A,C
		SUBNB	A,D
		SUBNB	A,E
		SUBNB	A,H
		SUBNB	A,L
		LTA	A,V		; 60 B8-BF (no V on 78C06)
		LTA	A,A
		LTA	A,B
		LTA	A,C
		LTA	A,D
		LTA	A,E
		LTA	A,H
		LTA	A,L
		ADD	A,V		; 60 C0-C7 (no V on 78C06)
		ADD	A,A
		ADD	A,B
		ADD	A,C
		ADD	A,D
		ADD	A,E
		ADD	A,H
		ADD	A,L
		ONA	A,V		; 60 C8-CF (not o 78C06)
		ONA	A,A
		ONA	A,B
		ONA	A,C
		ONA	A,D
		ONA	A,E
		ONA	A,H
		ONA	A,L
		ADC	A,V		; 60 D0-D7 (no V on 78C06)
		ADC	A,A
		ADC	A,B
		ADC	A,C
		ADC	A,D
		ADC	A,E
		ADC	A,H
		ADC	A,L
		OFFA	A,V		; 60 D8-DF (not on 78C06)
		OFFA	A,A
		OFFA	A,B
		OFFA	A,C
		OFFA	A,D
		OFFA	A,E
		OFFA	A,H
		OFFA	A,L
		SUB	A,V		; 60 E0-E7 (no V on 78C06)
		SUB	A,A
		SUB	A,B
		SUB	A,C
		SUB	A,D
		SUB	A,E
		SUB	A,H
		SUB	A,L
		NEA	A,V		; 60 E8-EF (no V on 78C06)
		NEA	A,A
		NEA	A,B
		NEA	A,C
		NEA	A,D
		NEA	A,E
		NEA	A,H
		NEA	A,L
		SBB	A,V		; 60 F0-F7 (no V on 78C06)
		SBB	A,A
		SBB	A,B
		SBB	A,C
		SBB	A,D
		SBB	A,E
		SBB	A,H
		SBB	A,L
		EQA	A,V		; 60 F8-FF (no V on 78C06)
		EQA	A,A
		EQA	A,B
		EQA	A,C
		EQA	A,D
		EQA	A,E
		EQA	A,H
		EQA	A,L

		DAA			; 61
		RETI			; 62
		CALB			; 63 (not on 78C06)

		ANI	V,xx		; 64 08-0F xx (not on 78C06)
		ANI	>A,xx
		ANI	B,xx
		ANI	C,xx
		ANI	D,xx
		ANI	E,xx
		ANI	H,xx
		ANI	L,xx
		XRI	V,xx		; 64 10-17 xx (not on 78C06)
		XRI	>A,xx
		XRI	B,xx
		XRI	C,xx
		XRI	D,xx
		XRI	E,xx
		XRI	H,xx
		XRI	L,xx
		ORI	V,xx		; 64 18-1F xx (not on 78C06)
		ORI	>A,xx
		ORI	B,xx
		ORI	C,xx
		ORI	D,xx
		ORI	E,xx
		ORI	H,xx
		ORI	L,xx
		ADINC	V,xx		; 64 20-27 xx (not on 78C06)
		ADINC	>A,xx
		ADINC	B,xx
		ADINC	C,xx
		ADINC	D,xx
		ADINC	E,xx
		ADINC	H,xx
		ADINC	L,xx
		GTI	V,xx		; 64 28-2F xx (not on 78C06)
		GTI	>A,xx
		GTI	B,xx
		GTI	C,xx
		GTI	D,xx
		GTI	E,xx
		GTI	H,xx
		GTI	L,xx
		SUINB	V,xx		; 64 30-37 xx (not on 78C06)
		SUINB	>A,xx
		SUINB	B,xx
		SUINB	C,xx
		SUINB	D,xx
		SUINB	E,xx
		SUINB	H,xx
		SUINB	L,xx
		LTI	V,xx		; 64 38-3F xx (not on 78C06)
		LTI	>A,xx
		LTI	B,xx
		LTI	C,xx
		LTI	D,xx
		LTI	E,xx
		LTI	H,xx
		LTI	L,xx
		ADI	V,xx		; 64 40-47 xx (not on 78C06)
		ADI	>A,xx
		ADI	B,xx
		ADI	C,xx
		ADI	D,xx
		ADI	E,xx
		ADI	H,xx
		ADI	L,xx
		ONI	V,xx		; 64 48-4F xx (not on 78C06)
		ONI	>A,xx
		ONI	B,xx
		ONI	C,xx
		ONI	D,xx
		ONI	E,xx
		ONI	H,xx
		ONI	L,xx
		ACI	V,xx		; 64 50-57 xx (not on 78C06)
		ACI	>A,xx
		ACI	B,xx
		ACI	C,xx
		ACI	D,xx
		ACI	E,xx
		ACI	H,xx
		ACI	L,xx
		OFFI	V,xx		; 64 58-5F xx (not on 78C06)
		OFFI	>A,xx
		OFFI	B,xx
		OFFI	C,xx
		OFFI	D,xx
		OFFI	E,xx
		OFFI	H,xx
		OFFI	L,xx
		SUI	V,xx		; 64 60-67 xx (not on 78C06)
		SUI	>A,xx
		SUI	B,xx
		SUI	C,xx
		SUI	D,xx
		SUI	E,xx
		SUI	H,xx
		SUI	L,xx
		NEI	V,xx		; 64 68-6F xx (not on 78C06)
		NEI	>A,xx
		NEI	B,xx
		NEI	C,xx
		NEI	D,xx
		NEI	E,xx
		NEI	H,xx
		NEI	L,xx
		SBI	V,xx		; 64 70-77 xx (not on 78C06)
		SBI	>A,xx
		SBI	B,xx
		SBI	C,xx
		SBI	D,xx
		SBI	E,xx
		SBI	H,xx
		SBI	L,xx
		EQI	V,xx		; 64 78-7F xx (not on 78C06)
		EQI	>A,xx
		EQI	B,xx
		EQI	C,xx
		EQI	D,xx
		EQI	E,xx
		EQI	H,xx
		EQI	L,xx

		ANI	PA,xx		; (r = PA,PB,PC,MK)         64 88-8B xx (no PC on 78C06)
		ANI	PB,xx
		ANI	PC,xx
		ANI	MK,xx
		XRI	PA,xx		; 64 90-93 xx (not on 78C06)
		XRI	PB,xx
		XRI	PC,xx
		XRI	MK,xx
		ORI	PA,xx		; 64 98-9B xx (no PC on 78C06)
		ORI	PB,xx
		ORI	PC,xx
		ORI	MK,xx
		ADINC	PA,xx		; 64 A0-A3 xx (not on 78C06)
		ADINC	PB,xx
		ADINC	PC,xx
		ADINC	MK,xx
		GTI	PA,xx		; 64 A8-AB xx (not on 78C06)
		GTI	PB,xx
		GTI	PC,xx
		GTI	MK,xx
		SUINB	PA,xx		; 64 B0-B3 xx (not on 78C06)
		SUINB	PB,xx
		SUINB	PC,xx
		SUINB	MK,xx
		LTI	PA,xx		; 64 B8-BB xx (not on 78C06)
		LTI	PB,xx
		LTI	PC,xx
		LTI	MK,xx
		ADI	PA,xx		; 64 C0-C3 xx (not on 78C06)
		ADI	PB,xx
		ADI	PC,xx
		ADI	MK,xx
		ONI	PA,xx		; 64 C8-CB xx (no PC on 78C06)
		ONI	PB,xx
		ONI	PC,xx
		ONI	MK,xx
		ACI	PA,xx		; 64 D0-D3 xx (not on 78C06)
		ACI	PB,xx
		ACI	PC,xx
		ACI	MK,xx
		OFFI	PA,xx		; 64 D8-DB xx (no PC on 78C06)
		OFFI	PB,xx
		OFFI	PC,xx
		OFFI	MK,xx
		SUI	PA,xx		; 64 E0-E3 xx (not on 78C06)
		SUI	PB,xx
		SUI	PC,xx
		SUI	MK,xx
		NEI	PA,xx		; 64 E8-EB xx (not on 78C06)
		NEI	PB,xx
		NEI	PC,xx
		NEI	MK,xx
		SBI	PA,xx		; 64 F0-F3 xx (not on 78C06)
		SBI	PB,xx
		SBI	PC,xx
		SBI	MK,xx
		EQI	PA,xx		; 64 F8-FB xx (not on 78C06)
		EQI	PB,xx
		EQI	PC,xx
		EQI	MK,xx

		NEIW	wa,xx		; 65 wa xx
		SUI	A,xx		; 66 xx
		NEI	A,xx		; 67 xx
		MVI	V,xx		; 68 xx (no V on 78C06)
		MVI	A,xx		; 69 xx
		MVI	B,xx		; 6A xx
		MVI	C,xx		; 6B xx
		MVI	D,xx		; 6C xx
		MVI	E,xx		; 6D xx
		MVI	H,xx		; 6E xx
		MVI	L,xx		; 6F xx

		SSPD	bbaa		; 70 0E aa bb
		LSPD	bbaa		; 70 0F aa bb
		SBCD	bbaa		; 70 1E aa bb
		LBCD	bbaa		; 70 1F aa bb
		SDED	bbaa		; 70 2E aa bb
		LDED	bbaa		; 70 2F aa bb
		SHLD	bbaa		; 70 3E aa bb
		LHLD	bbaa		; 70 3F aa bb

		MOV	V,bbaa		; 70 68 aa bb (no V on 78C06)
		MOV	A,bbaa		; 70 69 aa bb
		MOV	B,bbaa		; 70 6A aa bb
		MOV	C,bbaa		; 70 6B aa bb
		MOV	D,bbaa		; 70 6C aa bb
		MOV	E,bbaa		; 70 6D aa bb
		MOV	H,bbaa		; 70 6E aa bb
		MOV	L,bbaa		; 70 6F aa bb

		MOV	bbaa,V		; 70 78 aa bb (no V on 78C06)
		MOV	bbaa,A		; 70 79 aa bb
		MOV	bbaa,B		; 70 7A aa bb
		MOV	bbaa,C		; 70 7B aa bb
		MOV	bbaa,D		; 70 7C aa bb
		MOV	bbaa,E		; 70 7D aa bb
		MOV	bbaa,H		; 70 7E aa bb
		MOV	bbaa,L		; 70 7F aa bb

		ANAX	B		; (rpa = B,D,H,D+,H+,D-,H-) 70 89-8F
		ANAX	D
		ANAX	H
		ANAX	D+
		ANAX	H+
		ANAX	D-
		ANAX	H-
		XRAX	B		; 70 91-97
		XRAX	D
		XRAX	H
		XRAX	D+
		XRAX	H+
		XRAX	D-
		XRAX	H-
		ORAX	B		; 70 99-9F
		ORAX	D
		ORAX	H
		ORAX	D+
		ORAX	H+
		ORAX	D-
		ORAX	H-
		ADDNCX	B		; 70 A1-A7
		ADDNCX	D
		ADDNCX	H
		ADDNCX	D+
		ADDNCX	H+
		ADDNCX	D-
		ADDNCX	H-
		GTAX	B		; 70 A9-AF
		GTAX	D
		GTAX	H
		GTAX	D+
		GTAX	H+
		GTAX	D-
		GTAX	H-
		SUBNBX	B		; 70 B1-B7
		SUBNBX	D
		SUBNBX	H
		SUBNBX	D+
		SUBNBX	H+
		SUBNBX	D-
		SUBNBX	H-
		LTAX	B		; 70 B9-BF
		LTAX	D
		LTAX	H
		LTAX	D+
		LTAX	H+
		LTAX	D-
		LTAX	H-
		ADDX	B		; 70 C1-C7
		ADDX	D
		ADDX	H
		ADDX	D+
		ADDX	H+
		ADDX	D-
		ADDX	H-
		ONAX	B		; 70 C9-CF
		ONAX	D
		ONAX	H
		ONAX	D+
		ONAX	H+
		ONAX	D-
		ONAX	H-
		ADCX	B		; 70 D1-D7
		ADCX	D
		ADCX	H
		ADCX	D+
		ADCX	H+
		ADCX	D-
		ADCX	H-
		OFFAX	B		; 70 D9-DF
		OFFAX	D
		OFFAX	H
		OFFAX	D+
		OFFAX	H+
		OFFAX	D-
		OFFAX	H-
		SUBX	B		; 70 E1-E7
		SUBX	D
		SUBX	H
		SUBX	D+
		SUBX	H+
		SUBX	D-
		SUBX	H-
		NEAX	B		; 70 EA-EF
		NEAX	D
		NEAX	H
		NEAX	D+
		NEAX	H+
		NEAX	D-
		NEAX	H-
		SBBX	B		; 70 F1-F7
		SBBX	D
		SBBX	H
		SBBX	D+
		SBBX	H+
		SBBX	D-
		SBBX	H-
		EQAX	B		; 70 F9-FF
		EQAX	D
		EQAX	H
		EQAX	D+
		EQAX	H+
		EQAX	D-
		EQAX	H-

		MVIW	wa,xx		; 71 wa xx (not on 78C06)
		SOFTI			; 72 (not on 78C06)
		JB			; 73

		ANAW	wa		; 74 88 wa (not on 78C06)
		XRAW	wa		; 74 90 wa (not on 78C06)
		ORAW	wa		; 74 98 wa (not on 78C06)
		ADDNCW	wa		; 74 A0 wa (not on 78C06)
		GTAW	wa		; 74 A8 wa (not on 78C06)
		SUBNBW	wa		; 74 B0 wa (not on 78C06)
		LTAW	wa		; 74 B8 wa (not on 78C06)
		ADDW	wa		; 74 C0 wa (not on 78C06)
		ONAW	wa		; 74 C8 wa (not on 78C06)
		ADCW	wa		; 74 D0 wa (not on 78C06)
		OFFAW	wa		; 74 D8 wa (not on 78C06)
		SUBW	wa		; 74 E0 wa (not on 78C06)
		NEAW	wa		; 74 E8 wa (not on 78C06)
		SBBW	wa		; 74 F0 wa (not on 78C06)
		EQAW	wa		; 74 F8 wa (not on 78C06)

		EQIW	wa,xx		; 75 wa xx
		SBI	A,xx		; 76 xx
		EQI	A,xx		; 77 xx

		CALF    08aah		; 78 aa...7F aa
		CALF	09aah
		CALF	0aaah
		CALF	0baah
		CALF	0caah
		CALF	0daah
		CALF	0eaah
		CALF	0faah

		CALT	0080H		; 80
		CALT	0082H		; 81
		CALT	0084H		; 82
                CALT	0086H		; 83
		CALT	0088H		; 84
		CALT	008AH		; 85
		CALT	008CH		; 86
                CALT	008EH		; 87
		CALT	0090H		; 88
		CALT	0092H		; 89
		CALT	0094H		; 8A
                CALT	0096H		; 8B
		CALT	0098H		; 8C
		CALT	009AH		; 8D
		CALT	009CH		; 8E
                CALT	009EH		; 8F
		CALT	00A0H		; 90
		CALT	00A2H		; 91
		CALT	00A4H		; 92
                CALT	00A6H		; 93
		CALT	00A8H		; 94
		CALT	00AAH		; 95
		CALT	00ACH		; 96
                CALT	00AEH		; 97
		CALT	00B0H		; 98
		CALT	00B2H		; 99
		CALT	00B4H		; 9A
                CALT	00B6H		; 9B
		CALT	00B8H		; 9C
		CALT	00BAH		; 9D
		CALT	00BCH		; 9E
                CALT	00BEH		; 9F
		CALT	00C0H		; A0
		CALT	00C2H		; A1
		CALT	00C4H		; A2
                CALT	00C6H		; A3
		CALT	00C8H		; A4
		CALT	00CAH		; A5
		CALT	00CCH		; A6
                CALT	00CEH		; A7
		CALT	00D0H		; A8
		CALT	00D2H		; A9
		CALT	00D4H		; AA
                CALT	00D6H		; AB
		CALT	00D8H		; AC
		CALT	00DAH		; AD
		CALT	00DCH		; AE
                CALT	00DEH		; AF
		CALT	00E0H		; B0
		CALT	00E2H		; B1
		CALT	00E4H		; B2
                CALT	00E6H		; B3
		CALT	00E8H		; B4
		CALT	00EAH		; B5
		CALT	00ECH		; B6
                CALT	00EEH		; B7
		CALT	00F0H		; B8
		CALT	00F2H		; B9
		CALT	00F4H		; BA
                CALT	00F6H		; BB
		CALT	00F8H		; BC
		CALT	00FAH		; BD
		CALT	00FCH		; BE
                CALT	00FEH		; BF

		JR	$+01H		; C0...DF
		JR	$+02H
		JR	$+03H
		JR	$+04H
		JR	$+05H
		JR	$+06H
		JR	$+07H
		JR	$+08H
		JR	$+09H
		JR	$+0AH
		JR	$+0BH
		JR	$+0CH
		JR	$+0DH
		JR	$+0EH
		JR	$+0FH
		JR	$+10H
		JR	$+11H
		JR	$+12H
		JR	$+13H
		JR	$+14H
		JR	$+15H
		JR	$+16H
		JR	$+17H
		JR	$+18H
		JR	$+19H
		JR	$+1AH
		JR	$+1BH
		JR	$+1CH
		JR	$+1DH
		JR	$+1EH
		JR	$+1FH
		JR	$+20H
		JR	$-1FH		; E0...FF
		JR	$-1EH
		JR	$-1DH
		JR	$-1CH
		JR	$-1BH
		JR	$-1AH
		JR	$-19H
		JR	$-18H
		JR	$-17H
		JR	$-16H
		JR	$-15H
		JR	$-14H
		JR	$-13H
		JR	$-12H
		JR	$-11H
		JR	$-10H
		JR	$-0FH
		JR	$-0EH
		JR	$-0DH
		JR	$-0CH
		JR	$-0BH
		JR	$-0AH
		JR	$-09H
		JR	$-08H
		JR	$-07H
		JR	$-06H
		JR	$-05H
		JR	$-04H
		JR	$-03H
		JR	$-02H
		JR	$-01H
		JR	$-00H
