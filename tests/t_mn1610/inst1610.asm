;;;
;;; MN1610 Instructions
;;;
	
	;; Addressing
	L	R0,ADR+1	; C001
	L	R0,2(IC)	; C802
	L	R0,(ADR+3)	; D003
	L	R0,(4(IC))	; D804
	L	R0,5(X0)	; E005
	L	R0,6(X1)	; E806
	L	R0,(7)(X0)	; F007
	L	R0,(8)(X1)	; F808

	;; Register
	MVI	R0,1		; 0801
	MVI	R1,2		; 0902
	MVI	R2,3		; 0A03
	MVI	R3,4		; 0B04
	MVI	X0,5		; 0B05
	MVI	R4,6		; 0C06
	MVI	X1,7		; 0C07
	MVI	SP,8		; 0D08
	MVI	STR,9		; 0E09

	;; Skip Condition
	A	R0,R0		; 5808
	A	R0,R0,SKP	; 5818
	A	R0,R0,M		; 5828
	A	R0,R0,PZ	; 5838
	A	R0,R0,Z		; 5848
	A	R0,R0,E		; 5848
	A	R0,R0,NZ	; 5858
	A	R0,R0,NE	; 5858
	A	R0,R0,MZ	; 5868
	A	R0,R0,P		; 5878
	A	R0,R0,EZ	; 5888
	A	R0,R0,ENZ	; 5898
	A	R0,R0,OZ	; 58A8
	A	R0,R0,ONZ	; 58B8
	A	R0,R0,LMZ	; 58C8
	A	R0,R0,LP	; 58D8
	A	R0,R0,LPZ	; 58E8
	A	R0,R0,LM	; 58F8

	;; E Register Manipulation
	SL	R0		; 200C
	SL	R0,RE		; 200D
	SL	R0,SE		; 200E
	SL	R0,CE		; 200F

	;; Memory
	L	R0,(ADR)	; D000
	ST	R1,(ADR+1)	; 9101
	B	ADR		; C700
	BAL	-1(IC)		; 8FFF
	IMS	4(X0)		; E604
	DMS	8(X1)		; AE08

	;; 2 Operand
	A	R0,R1		; 5809
	S	R1,R4,LMZ	; 59C4
	AND	R2,R2,E		; 6A4A
	OR	R0,R3		; 600B
	EOR	R2,R3,NZ	; 6253
	C	R3,R1,M		; 5329
	CB	R4,R0,LP	; 54D0
	MV	R0,R3		; 780B
	MVB	R3,R0,P		; 7B70
	BSWP	R3,R2,SKP	; 731A
	DSWP	R3,R0		; 7300
	LAD	R2,R4,Z		; 6A44

	;; Shift
	SL	R0,CE		; 200F
	SR	R2,SKP		; 2218

	;; Shift / Add/Sub Immediate
	SBIT	R0,8,EZ		; 3888
	RBIT	R1,15		; 310F
	TBIT	R2,0,Z		; 2A40
	AI	R3,4,M		; 4B24
	SI	R4,7,PZ		; 4437

	;; I/O / MVI
	RD	R0,64		; 1840
	WR	R1,100		; 1164
	MVI	R2,255		; 0AFF

	;; Misc
	LPSW	3		; 2007
	H			; 2000
	PUSH	R3		; 2301
	POP	R4		; 2402
	RET			; 2003

	;; Alias
	NOP			; 7808
	CLR	R1		; 6101

	;; Pseudo Instruction
	DC	123,X'AA55'
	DC	'AB'
	DS	16
	DC	"Hello, ", "world"
	DC	"A",0,"B"
	
