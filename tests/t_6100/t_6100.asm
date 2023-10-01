
	CPU	6100

	ORG	00000

	NOP
	NOP

	JMP	I ZLABEL
	NOP
ZLABEL:	DC	TOP

	ORG	.+8

	ORG	01000

	;;
	;; Intersil IM6100
	;;

TOP:
	;; Memory Reference Instruction
	AND	CLABEL
	TAD	ZLABEL
	ISZ	I CLABEL
	DCA	I ZLABEL
	JMS	CLABEL
	JMP	ZLABEL

	;; Operate Instruction (Group 1)
	NOP
	IAC
	BSW
	RAL
	RTL
	RAR
	RTR
	EXPECT 1500
	R3L			; 6120 only
	ENDEXPECT
	CML
	CMA
	CIA
	CLL
	CLL RAL
	CLL RTL
	CLL RAR
	CLL RTR
	STL
	CLA
	CLA IAC
	GLK
	STA
	CLA CLL

	BSW IAC
	bsw IAC
	bsw iac

	;; Operate Instruction (Group 2)
	;; NOP
	HLT
	OSR
	SKP
	SNL
	SZL
	SZA
	SNA
	SZA SNL
	SNA SZL
	SMA
	SPA
	SMA SNL
	SPA SZL
	SMA SZA
	SPA SNA
	SMA SZA SNL
	SPA SNA SZL
	;; CLA
	LAS
	SZA CLA
	SNA CLA
	SMA CLA
	SPA CLA
	OSR HLT		; 0xF06 / 07406
	SNL OSR HLT	; 0xF16 / 07426
	SZL OSR HLT	; 0xF1E / 07436

	;; Operate Instruction (Group 3)
	;; NOP
	MQL
	MQA
	SWP
	CLA
	CAM
	ACL
	CLA SWP

CLABEL:	DC	07777

	;; I/O Transfer Instruction
	IOT	63 7
	SKON
	ION
	IOF
	SRQ
	GTF
	RTF
	SGT
	CAF

	PANEL	ON

	EXPECT	400
	SKON
	ENDEXPECT

	PANEL	OFF

	;;
	;; Harris HD1-6120
	;;

	CPU	6120

	;; Stack Operation Instruction
	PPC1
	PPC2
	PAC1
	PAC2
	RTN1
	RTN2
	POP1
	POP2
	RSP1
	RSP2
	LSP1
	LSP2

	;; Internal Control Instruction
	WSR
	GCF

	;; Main Memory Control Instruction
	PR0
	PR1
	PR2
	PR3

	;; Panel Mode Control Instruction
	PANEL	ON

	PRS
	PG0
	PEX
	CPD
	SPD

	PANEL	OFF

	;; Memory Extension Instruction
	CDF	1
	CIF	4
	CDF CIF	6
	RDF
	RIF
	RIB
	RMF

	;;
	;; Pseudo Instruction
	;;
	CPU	6100

	DC	07777
	DC	"IM6100" 0xd 012

	;; DECIMAL / OCTAL
	OCTAL
	DC	2525
	DECIMAL
	DC	2525

	;; LTORG
LABB:
	AND	L LABB
	AND	L LABF
	TAD	L LABG
	TAD	L 01000
LABF:
	LTORG
LABG:

