;;;
;;;	Test HP Nanoprocessor
;;;

	CPU	NANO

	;; A. Accumulator Group
	SBS	0
	SBZ	1
	SBN	2
	CBN	3
	INB
	IND
	DEB
	DED
	CLA
	CMA
	LSA
	RSA
	SES
	SEZ
	LDR	0x80

	;; B. Register Transfer Group
	LDA	4
	STA	5
	LDI	6
	STI	7
	STR	8,192

	;; C. Extend Register Group
	STE
	CLE

	;; D. Interrupt Group
	DSI
	ENI

	;; E. Comparator Group
	SLT
	SEQ
	SAZ
	SLE
	SGE
	SNE
	SAN
	SGT

	;; F. Input/Output Group
	INA	9
	OTA	10
	EXPECT	1320		; Range overflow
	OTA	15
	ENDEXPECT
	OTR	11,64
	EXPECT	1320		; Range overflow
	OTR	15,100
	ENDEXPECT
	STC	0
	EXPECT	1320		; Range overflow
	STC	7
	ENDEXPECT
	CLC	1
	SFS	2
	SFZ	3
	RTI
	RTE
	NOP
	JAI	4
	JAS	5

	;; G. Program Control Group
LOOP:
	JMP	LOOP
	JSB	SUB
SUB:
	RTS
	RSE
	JMP	1920

	EXPECT	1925
	ORG	2048
	NOP
	ENDEXPECT
