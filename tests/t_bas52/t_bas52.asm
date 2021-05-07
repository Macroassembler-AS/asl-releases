;  December 18, 1986
;  MS-DOS compatible Source code for MCS BASIC-52 (tm)
;  Assembles with ASM51 Macro Assembler Version 2.2
;
;  The following source code does not include the floating point math
;  routines. These are seperately compiled using FP52.SRC.
;
;  Both the BASIC.SRC and FP52.SRC programs assemble into ABSOLUTE
;  object files, and do not need to be relocated or linked. The FP52
;  object code and the BASIC object code, when compiled without modification
;  of the source listings, create the same object code that is found on
;  the MCS BASIC-52 Version 1.1 microcontrollers.
;
;  The original source code had 7 "include" files that have been incorporated
;  into this file for ease of assembly.
;  These 7 files are: LOOK52.SRC, BAS52.RST, BAS52.PGM, BAS52.TL, BAS52.OUT,
;  BAS52.PWM, and BAS52.CLK.
;
;
;                       Intel Corporation, Embedded Controller Operations

        cpu	8052

	page	0
        newpage

	include	stddef51.inc
	include bitfuncs.inc
	bigendian on

	segment	code

	;**************************************************************
	;
	; TRAP VECTORS TO MONITOR
	;
	; RESET TAG (0AAH) ---------2001H
	;
	; TAG LOCATION (5AH) ------ 2002H
	;
	; EXTERNAL INTERRUPT 0 ---- 2040H
	;
	; COMMAND MODE ENTRY ------ 2048H
	;
	; SERIAL PORT ------------- 2050H
	;
	; MONITOR (BUBBLE) OUTPUT - 2058H
	;
	; MONITOR (BUBBLE) INPUT -- 2060H
	;
	; MONITOR (BUBBLE) CSTS --- 2068H
	;
	; GET USER JUMP VECTOR ---- 2070H
	;
	; GET USER LOOKUP VECTOR -- 2078H
	;
	; PRINT AT VECTOR --------- 2080H
	;
	; INTERRUPT PWM ----------- 2088H
	;
	; EXTERNAL RESET ---------- 2090H
	;
	; USER OUTPUT-------------- 4030H
	;
	; USER INPUT -------------- 4033H
	;
	; USER CSTS --------------- 4036H
	;
	; USER RESET -------------- 4039H
	;
	; USER DEFINED PRINT @ ---  403CH
	;
	;***************************************************************
	;
	newpage
	;***************************************************************
	;
	; MCS - 51  -  8K BASIC VERSION 1.1
	;
	;***************************************************************
	;
	AJMP	CRST		;START THE PROGRAM
        db	037h		; ******AA inserted
	;
	ORG	3H
	;
	;***************************************************************
	;
	;EXTERNAL INTERRUPT 0
	;
	;***************************************************************
	;
	JB	DRQ,STQ		;SEE IF DMA IS SET
	PUSH	PSW		;SAVE THE STATUS
	LJMP	4003H		;JUMP TO USER IF NOT SET
	;
	ORG	0BH
	;
	;***************************************************************
	;
	;TIMER 0 OVERFLOW INTERRUPT
	;
	;***************************************************************
	;
	PUSH	PSW		;SAVE THE STATUS
	JB	C_BIT,STJ	;SEE IF USER WANTS INTERRUPT
	LJMP	400BH		;EXIT IF USER WANTS INTERRUPTS
	;
	ORG	13H
	;
	;***************************************************************
	;
	;EXTERNAL INTERRUPT 1
	;
	;***************************************************************
	;
	JB	INTBIT,STK
	PUSH	PSW
	LJMP	4013H
	;
	newpage
	;
	ORG	1BH
	;
	;***************************************************************
	;
	;TIMER 1 OVERFLOW INTERRUPT
	;
	;***************************************************************
	;
	PUSH	PSW
	LJMP	CKS_I
	;
STJ:	LJMP	I_DR		;DO THE INTERRUPT
	;
	;***************************************************************
	;
	;SERIAL PORT INTERRUPT
	;
	;***************************************************************
	;
	ORG	23H
	;
	PUSH	PSW
	JB	SPINT,STU	;SEE IF MONITOR EANTS INTERRUPT
	LJMP	4023H
	;
	ORG	2BH
	;
	;**************************************************************
	;
	;TIMER 2 OVERFLOW INTERRUPT
	;
	;**************************************************************
	;
	PUSH	PSW
	LJMP	402BH
	;
	newpage
	;**************************************************************
	;
	;USER ENTRY
	;
	;**************************************************************
	;
	ORG	30H
	;
	LJMP	IBLK		;LINK TO USER BLOCK
	;
STQ:	JB	I_T0,STS	;SEE IF MONITOR WANTS IT
	CLR	DACK
	JNB	P3.2,$		;WAIT FOR DMA TO END
	SETB	DACK
	RETI
	;
STS:	LJMP	2040H		;GO TO THE MONITOR
	;
STK:	SETB	INTPEN		;TELL BASIC AN INTERRUPT WAS RECEIVED
	RETI
	;
STU:	LJMP	2050H		;SERIAL PORT INTERRUPT
	;
	newpage

	include look52.inc	; ******AA
	
EIG:	DB	"EXTRA IGNORED",'"'
	;
EXA:	DB	"A-STACK",'"'
	;
EXC:	DB	"C-STACK",'"'
	;
	newpage

	include	bas52.rst	; ******AA

	newpage
	;***************************************************************
	;
	; CIPROG AND CPROG - Program a prom
	;
	;***************************************************************
	;
	include	bas52.pgm	; ******AA
	newpage
	;**************************************************************
	;
PGU:	;PROGRAM A PROM FOR THE USER
	;
	;**************************************************************
	;
	CLR	PROMV		;TURN ON THE VOLTAGE
	MOV	PSW,#00011000B	;SELECT RB3
	ACALL	PG1		;DO IT
	SETB	PROMV		;TURN IT OFF
	RET
	;
	;
	;*************************************************************
	;
CCAL:	; Set up for prom moves
	; R3:R1 gets source
	; R7:R6 gets # of bytes
	;
	;*************************************************************
	;
	ACALL	GETEND		;GET THE LAST LOCATION
	INC	DPTR		;BUMP TO LOAD EOF
	MOV	R3,BOFAH
	MOV	R1,BOFAL	;RESTORE START
	CLR	C		;PREPARE FOR SUBB
	MOV	A,DPL		;SUB DPTR - BOFA > R7:R6
	SUBB	A,R1
	MOV	R6,A
	MOV	A,DPH
	SUBB	A,R3
	MOV	R7,A
	RET
	;
	;
	include	bas52.tl	; ******AA
	newpage
	;***************************************************************
	;
CROM:	; The command action routine - ROM - Run out of rom
	;
	;***************************************************************
	;
	CLR	CONB		;CAN'T CONTINUE IF MODE CHANGE
	ACALL	RO1		;DO IT
	;
C_K:	LJMP	CL3		;EXIT
	;
RO1:	LCALL	DELTST		;SEE IF INTGER PRESENT ******AA CALL-->LCALL, INTGER-->DELTST
	MOV	R4,#R1B0	;SAVE THE NUMBER ******AA ABS-->IMM, R0B0-->R0B1 ?!?
	JNC	$+6		; ******AA $+4-->$+6 ???
	;MOV	R4,#01H		;ONE IF NO INTEGER PRESENT ******AA repl. by next two
	LCALL	ONE		; ******AA
        MOV	R4,A            ; ******AA
	ACALL	ROMFD		;FIND THE PROGRAM
	CJNE	R4,#0,RFX	;EXIT IF R4 <> 0
	INC	DPTR		;BUMP PAST TAG
	MOV	BOFAH,DPH	;SAVE THE ADDRESS
	MOV	BOFAL,DPL
	RET
	;
ROMFD:	MOV	DPTR,#ROMADR+16	;START OF USER PROGRAM
	;
RF1:	MOVX	A,@DPTR		;GET THE BYTE
	CJNE	A,#55H,RF3	;SEE IF PROPER TAG
	DJNZ	R4,RF2		;BUMP COUNTER
	;
RFX:	RET			;DPTR HAS THE START ADDRESS
	;
RF2:	INC	DPTR		;BUMP PAST TAG
	ACALL	G5
	INC	DPTR		;BUMP TO NEXT PROGRAM
	SJMP	RF1		;DO IT AGAIN
	;
RF3:	JBC	INBIT,RFX	;EXIT IF SET
	;
NOGO:	MOV	DPTR,#NOROM
	AJMP	ERRLK
	;
	newpage
	;***************************************************************
	;
L20DPI:	; load R2:R0 with the location the DPTR is pointing to
	;
	;***************************************************************
	;
	MOVX	A,@DPTR
	MOV	R2,A
	INC	DPTR
	MOVX	A,@DPTR
	MOV	R0,A
	RET			;DON'T BUMP DPTR
	;
	;***************************************************************
	;
X31DP:	; swap R3:R1 with DPTR
	;
	;***************************************************************
	;
	XCH	A,R3
	XCH	A,DPH
	XCH	A,R3
	XCH	A,R1
	XCH	A,DPL
	XCH	A,R1
	RET
	;
	;***************************************************************
	;
LD_T:	; Load the timer save location with the value the DPTR is
	; pointing to.
	;
	;****************************************************************
	;
	MOVX	A,@DPTR
	MOV	T_HH,A
	INC	DPTR
	MOVX	A,@DPTR
	MOV	T_LL,A
	RET
	;
	newpage
	;
	;***************************************************************
	;
	;GETLIN - FIND THE LOCATION OF THE LINE NUMBER IN R3:R1
	;         IF ACC = 0 THE LINE WAS NOT FOUND I.E. R3:R1
	;         WAS TOO BIG, ELSE ACC <> 0 AND THE DPTR POINTS
	;         AT THE LINE THAT IS GREATER THAN OR EQUAL TO THE
	;         VALUE IN R3:R1.
	;
	;***************************************************************
	;
GETEND:	SETB	ENDBIT		;GET THE END OF THE PROGRAM
	;
GETLIN:	LCALL	DP_B		;GET BEGINNING ADDRESS ******AA CALL-->LCALL
	;
G1:	LCALL	B_C             ; ******AA CALL-->LCALL
	JZ	G3		;EXIT WITH A ZERO IN A IF AT END
	INC	DPTR		;POINT AT THE LINE NUMBER
	JB	ENDBIT,G2	;SEE IF WE WANT TO FIND THE END
	ACALL	DCMPX		;SEE IF (DPTR) = R3:R1
	ACALL	DECDP		;POINT AT LINE COUNT
	MOVX	A,@DPTR		;PUT LINE LENGTH INTO ACC
	JB	UBIT,G3		;EXIT IF EQUAL
	JC	G3		;SEE IF LESS THAN OR ZERO
	;
G2:	ACALL	ADDPTR		;ADD IT TO DPTR
	SJMP	G1		;LOOP
	;
G3:	CLR	ENDBIT		;RESET ENDBIT
	RET			;EXIT
	;
G4:	MOV	DPTR,#PSTART	;DO RAM
	;
G5:	SETB	ENDBIT
	SJMP	G1		;NOW DO TEST
	;
	newpage
	;***************************************************************
	;
	; LDPTRI - Load the DATA POINTER with the value it is pointing
	;          to - DPH = (DPTR) , DPL = (DPTR+1)
	;
	; acc gets wasted
	;
	;***************************************************************
	;
LDPTRI:	MOVX	A,@DPTR		;GET THE HIGH BYTE
	PUSH	ACC		;SAVE IT
	INC	DPTR		;BUMP THE POINTER
	MOVX	A,@DPTR		;GET THE LOW BYTE
	MOV	DPL,A		;PUT IT IN DPL
	POP	DPH		;GET THE HIGH BYTE
	RET			;GO BACK
	;
	;***************************************************************
	;
	;L31DPI - LOAD R3 WITH (DPTR) AND R1 WITH (DPTR+1)
	;
	;ACC GETS CLOBBERED
	;
	;***************************************************************
	;
L31DPI:	MOVX	A,@DPTR		;GET THE HIGH BYTE
	MOV	R3,A		;PUT IT IN THE REG
	INC	DPTR		;BUMP THE POINTER
	MOVX	A,@DPTR		;GET THE NEXT BYTE
	MOV	R1,A		;SAVE IT
	RET
	;
	;***************************************************************
	;
	;DECDP - DECREMENT THE DATA POINTER - USED TO SAVE SPACE
	;
	;***************************************************************
	;
DECDP2:	ACALL	DECDP
	;
DECDP:	XCH	A,DPL		;GET DPL
	JNZ	$+4		;BUMP IF ZERO
	DEC	DPH
	DEC	A		;DECREMENT IT
	XCH	A,DPL		;GET A BACK
	RET			;EXIT
	;
	newpage
	;***************************************************************
	;
	;DCMPX - DOUBLE COMPARE - COMPARE (DPTR) TO R3:R1
	;R3:R1 - (DPTR) = SET CARRY FLAG
	;
	;IF R3:R1 > (DPTR) THEN C = 0
	;IF R3:R1 < (DPTR) THEN C = 1
	;IF R3:R1 = (DPTR) THEN C = 0
	;
	;***************************************************************
	;
DCMPX:	CLR	UBIT		;ASSUME NOT EQUAL
	MOVX	A,@DPTR		;GET THE BYTE
	CJNE	A,R3B0,D1	;IF A IS GREATER THAN R3 THEN NO CARRY
				;WHICH IS R3<@DPTR = NO CARRY AND
				;R3>@DPTR CARRY IS SET
	INC	DPTR		;BUMP THE DATA POINTER
	MOVX	A,@DPTR		;GET THE BYTE
	ACALL	DECDP		;PUT DPTR BACK
	CJNE	A,R1B0,D1	;DO THE COMPARE
	CPL	C		;FLIP CARRY
	;
	CPL	UBIT		;SET IT
D1:	CPL	C		;GET THE CARRY RIGHT
	RET			;EXIT
	;
	;***************************************************************
	;
	; ADDPTR - Add acc to the dptr
	;
	; acc gets wasted
	;
	;***************************************************************
	;
ADDPTR:	ADD	A,DPL		;ADD THE ACC TO DPL
	MOV	DPL,A		;PUT IT IN DPL
	JNC	$+4		;JUMP IF NO CARRY
	INC	DPH		;BUMP DPH
	RET			;EXIT
	;
	newpage
	;*************************************************************
	;
LCLR:	; Set up the storage allocation
	;
	;*************************************************************
	;
	ACALL	ICLR		;CLEAR THE INTERRUPTS
	ACALL	G4		;PUT END ADDRESS INTO DPTR
	MOV	A,#6		;ADJUST MATRIX SPACE
	ACALL	ADDPTR		;ADD FOR PROPER BOUNDS
	ACALL	X31DP		;PUT MATRIX BOUNDS IN R3:R1
	MOV	DPTR,#MT_ALL	;SAVE R3:R1 IN MATRIX FREE SPACE
	ACALL	S31DP		;DPTR POINTS TO MEMTOP
	ACALL	L31DPI		;LOAD MEMTOP INTO R3:R1
	MOV	DPTR,#STR_AL	;GET MEMORY ALLOCATED FOR STRINGS
	ACALL	LDPTRI
	LCALL	DUBSUB		;R3:R1 = MEMTOP - STRING ALLOCATION ******AA CALL-->LCALL
	MOV	DPTR,#VARTOP	;SAVE R3:R1 IN VARTOP
	;
	; FALL THRU TO S31DP2
	;
	;***************************************************************
	;
	;S31DP - STORE R3 INTO (DPTR) AND R1 INTO (DPTR+1)
	;
	;ACC GETS CLOBBERED
	;
	;***************************************************************
	;
S31DP2:	ACALL	S31DP		;DO IT TWICE
	;
S31DP:	MOV	A,R3		;GET R3 INTO ACC
	MOVX	@DPTR,A		;STORE IT
	INC	DPTR		;BUMP DPTR
	MOV	A,R1		;GET R1
	MOVX	@DPTR,A		;STORE IT
	INC	DPTR		;BUMP IT AGAIN TO SAVE PROGRAM SPACE
	RET			;GO BACK
	;
	;
	;***************************************************************
	;
STRING:	; Allocate memory for strings
	;
	;***************************************************************
	;
	LCALL	TWO		;R3:R1 = NUMBER, R2:R0 = LEN
	MOV	DPTR,#STR_AL	;SAVE STRING ALLOCATION
	ACALL	S31DP
	INC	R6		;BUMP
	MOV	S_LEN,R6	;SAVE STRING LENGTH
	AJMP	RCLEAR		;CLEAR AND SET IT UP
	;
	newpage
	;***************************************************************
	;
	; F_VAR - Find  the variable in symbol table
	;         R7:R6 contain the variable name
	;         If not found create a zero entry and set the carry
	;         R2:R0 has the address of variable on return
	;
	;***************************************************************
	;
F_VAR:	MOV	DPTR,#VARTOP	;PUT VARTOP IN DPTR
	ACALL	LDPTRI
	ACALL	DECDP2		;ADJUST DPTR FOR LOOKUP
	;
F_VAR0:	MOVX	A,@DPTR		;LOAD THE VARIABLE
	JZ	F_VAR2		;TEST IF AT THE END OF THE TABLE
	INC	DPTR		;BUMP FOR NEXT BYTE
	CJNE	A,R7B0,F_VAR1	;SEE IF MATCH
	MOVX	A,@DPTR		;LOAD THE NAME
	CJNE	A,R6B0,F_VAR1
	;
	; Found the variable now adjust and put in R2:R0
	;
DLD:	MOV	A,DPL		;R2:R0 = DPTR-2
	SUBB	A,#2
	MOV	R0,A
	MOV	A,DPH
	SUBB	A,#0		;CARRY IS CLEARED
	MOV	R2,A
	RET
	;
F_VAR1:	MOV	A,DPL		;SUBTRACT THE STACK SIZE+ADJUST
	CLR	C
	SUBB	A,#STESIZ
	MOV	DPL,A		;RESTORE DPL
	JNC	F_VAR0
	DEC	DPH
	SJMP	F_VAR0		;CONTINUE COMPARE
	;
	newpage
	;
	; Add the entry to the symbol table
	;
F_VAR2:	LCALL	R76S		;SAVE R7 AND R6
	CLR	C
	ACALL	DLD		;BUMP THE POINTER TO GET ENTRY ADDRESS
	;
	; Adjust pointer and save storage allocation
	; and make sure we aren't wiping anything out
	; First calculate new storage allocation
	;
	MOV	A,R0
	SUBB	A,#STESIZ-3	;NEED THIS MUCH RAM
	MOV	R1,A
	MOV	A,R2
	SUBB	A,#0
	MOV	R3,A
	;
	; Now save the new storage allocation
	;
	MOV	DPTR,#ST_ALL
	CALL	S31DP		;SAVE STORAGE ALLOCATION
	;
	; Now make sure we didn't blow it, by wiping out MT_ALL
	;
	ACALL	DCMPX		;COMPARE STORAGE ALLOCATION
	JC	CCLR3		;ERROR IF CARRY
	SETB	C		;DID NOT FIND ENTRY
	RET			;EXIT IF TEST IS OK
	;
	newpage
	;***************************************************************
	;
	; Command action routine - NEW
	;
	;***************************************************************
	;
CNEW:	MOV	DPTR,#PSTART	;SAVE THE START OF PROGRAM
	MOV	A,#EOF		;END OF FILE
	MOVX	@DPTR,A		;PUT IT IN MEMORY
	;
	; falls thru
	;
	;*****************************************************************
	;
	; The statement action routine - CLEAR
	;
	;*****************************************************************
	;
	CLR	LINEB		;SET UP FOR RUN AND GOTO
	;
RCLEAR:	ACALL	LCLR		;CLEAR THE INTERRUPTS, SET UP MATRICES
	MOV	DPTR,#MEMTOP	;PUT MEMTOP IN R3:R1
	ACALL	L31DPI
	ACALL	G4		;DPTR GETS END ADDRESS
	ACALL	CL_1		;CLEAR THE MEMORY
	;
RC1:	MOV	DPTR,#STACKTP	;POINT AT CONTROL STACK TOP
	CLR	A		;CONTROL UNDERFLOW
	;
RC2:	MOVX	@DPTR,A		;SAVE IN MEMORY
	MOV	CSTKA,#STACKTP
	MOV	ASTKA,#STACKTP
	CLR	CONB		;CAN'T CONTINUE
	RET
	;
	newpage
	;***************************************************************
	;
	; Loop until the memory is cleared
	;
	;***************************************************************
	;
CL_1:	INC	DPTR		;BUMP MEMORY POINTER
	CLR	A		;CLEAR THE MEMORY
	MOVX	@DPTR,A		;CLEAR THE RAM
	MOVX	A,@DPTR		;READ IT
	JNZ	CCLR3		;MAKE SURE IT IS CLEARED
	MOV	A,R3		;GET POINTER FOR COMPARE
	CJNE	A,DPH,CL_1	;SEE TO LOOP
	MOV	A,R1		;NOW TEST LOW BYTE
	CJNE	A,DPL,CL_1
	;
CL_2:	RET
	;
CCLR3:	LJMP	TB		;ALLOCATED MEMORY DOESN'T EXSIST ******AA JMP-->LJMP
	;
	;**************************************************************
	;
SCLR:	;Entry point for clear return
	;
	;**************************************************************
	;
	LCALL	DELTST		;TEST FOR A CR ******AA CALL-->LCALL
	JNC	RCLEAR
	LCALL	GCI1		;BUMP THE TEST POINTER ******AA CALL-->LCALL
	CJNE	A,#'I',RC1	;SEE IF I, ELSE RESET THE STACK
	;
	;**************************************************************
	;
ICLR:	; Clear interrupts and system garbage
	;
	;**************************************************************
	;
	JNB	INTBIT,$+5	;SEE IF BASIC HAS INTERRUPTS
	CLR	EX1		;IF SO, CLEAR INTERRUPTS
	ANL	34,#00100000B	;SET INTERRUPTS + CONTINUE
	RETI
	;
	newpage
	;***************************************************************
	;
	;OUTPUT ROUTINES
	;
	;***************************************************************
	;
CRLF2:	ACALL	CRLF		;DO TWO CRLF'S
	;
CRLF:	MOV	R5,#CR		;LOAD THE CR
	ACALL	TEROT		;CALL TERMINAL OUT
	MOV	R5,#LF		;LOAD THE LF
	AJMP	TEROT		;OUTPUT IT AND RETURN
	;
	;PRINT THE MESSAGE ADDRESSED IN ROM OR RAM BY THE DPTR
	;ENDS WITH THE CHARACTER IN R4
	;DPTR HAS THE ADDRESS OF THE TERMINATOR
	;
CRP:	ACALL	CRLF		;DO A CR THEN PRINT ROM
	;
ROM_P:	CLR	A		;CLEAR A FOR LOOKUP
	MOVC	A,@A+DPTR	;GET THE CHARACTER
	CLR	ACC.7		;CLEAR MS BIT
	CJNE	A,#'"',$+4	;EXIT IF TERMINATOR
	RET
	SETB	C0ORX1
	;
PN1:	MOV	R5,A		;OUTPUT THE CHARACTER
	ACALL	TEROT
	INC	DPTR		;BUMP THE POINTER
	SJMP	PN0
	;
UPRNT:	ACALL	X31DP
	;
PRNTCR:	MOV	R4,#CR		;OUTPUT UNTIL A CR
	;
PN0:	JBC	C0ORX1,ROM_P
	MOVX	A,@DPTR		;GET THE RAM BYTE
	JZ	$+5
	CJNE	A,R4B0,$+4	;SEE IF THE SAME AS TERMINATOR
	RET			;EXIT IF THE SAME
	CJNE	A,#CR,PN1	;NEVER PRINT A CR IN THIS ROUTINE
	LJMP	E1XX		;BAD SYNTAX
	;
	newpage
	;***************************************************************
	;
	; INLINE - Input a line to IBUF, exit when a CR is received
	;
	;***************************************************************
	;
INL2:	CJNE 	A,#CNTRLD,INL2B	;SEE IF A CONTROL D
	;
INL0:	ACALL	CRLF		;DO A CR
	;
INLINE:	MOV	P2,#HI(IBUF)	;IBUF IS IN THE ZERO PAGE
	MOV	R0,#LO(IBUF)	;POINT AT THE INPUT BUFFER
	;
INL1:	ACALL	INCHAR		;GET A CHARACTER
	MOV	R5,A		;SAVE IN R5 FOR OUTPUT
	CJNE	A,#7FH,INL2	;SEE IF A DELETE CHARACTER
	CJNE	R0,#LO(IBUF),INL6
	MOV	R5,#BELL	;OUTPUT A BELL
	;
INLX:	ACALL	TEROT		;OUTPUT CHARACTER
	SJMP	INL1		;DO IT AGAIN
	;
INL2B:	MOVX	@R0,A		;SAVE THE CHARACTER
	CJNE	A,#CR,$+5	;IS IT A CR
	AJMP	CRLF		;OUTPUT A CRLF AND EXIT
	CJNE	A,#20H,$+3
	JC	INLX		;ONLY ECHO CONTROL CHARACTERS
	INC	R0		;BUMP THE POINTER
	CJNE	R0,#IBUF+79,INLX
	DEC	R0		;FORCE 79
	SJMP	INLX-2		;OUTPUT A BELL
	;
INL6:	DEC	R0		;DEC THE RAM POINTER
	MOV	R5,#BS		;OUTPUT A BACK SPACE
	ACALL	TEROT
	ACALL	STEROT		;OUTPUT A SPACE
	MOV	R5,#BS		;ANOTHER BACK SPACE
	SJMP	INLX		;OUTPUT IT
	;
PTIME:	DB	128-2		; PROM PROGRAMMER TIMER
	DB	00H
	DB	00H
	DB	50H
	DB	67H
	DB	41H
	;
	newpage
	include	bas52.out	; ******AA
	;
BCK:	ACALL	CSTS		;CHECK STATUS
	JNC	CI_RET+1	;EXIT IF NO CHARACTER
	;
	newpage
	;***************************************************************
	;
	;INPUTS A CHARACTER FROM THE SYSTEM CONSOLE.
	;
	;***************************************************************
	;
INCHAR:	JNB	BI,$+8		;CHECK FOR MONITOR (BUBBLE)
	LCALL	2060H
	SJMP	INCH1
	JNB	CIUB,$+8	;CHECK FOR USER
	LCALL	4033H
	SJMP	INCH1
	JNB	RI,$		;WAIT FOR RECEIVER READY.
	MOV	A,SBUF
	CLR	RI		;RESET READY
	CLR	ACC.7		;NO BIT 7
	;
INCH1:	CJNE	A,#13H,$+5
	SETB	CNT_S
	CJNE	A,#11H,$+5
	CLR	CNT_S
	CJNE	A,#CNTRLC,$+7
	JNB	NO_C,C_EX	;TRAP NO CONTROL C
	RET
	;
	CLR	JKBIT
	CJNE	A,#17H,CI_RET	;CONTROL W
	SETB	JKBIT
	;
CI_RET:	SETB	C		;CARRY SET IF A CHARACTER
	RET			;EXIT
	;
	;*************************************************************
	;
	;RROM - The Statement Action Routine RROM
	;
	;*************************************************************
	;
RROM:	SETB	INBIT		;SO NO ERRORS
	ACALL	RO1		;FIND THE LINE NUMBER
	JBC	INBIT,CRUN
	RET			;EXIT
	;
	newpage
	;***************************************************************
	;
CSTS:	;	RETURNS CARRY = 1 IF THERE IS A CHARACTER WAITING FROM
	;       THE SYSTEM CONSOLE. IF NO CHARACTER THE READY CHARACTER
	;       WILL BE CLEARED
	;
	;***************************************************************
	;
	JNB	BI,$+6		;BUBBLE STATUS
	LJMP	2068H
	JNB	CIUB,$+6	;SEE IF EXTERNAL CONSOLE
	LJMP	4036H
	MOV	C,RI
	RET
	;
	MOV	DPTR,#WB	;EGO MESSAGE
	ACALL	ROM_P
	;
C_EX:	CLR	CNT_S		;NO OUTPUT STOP
	LCALL	SPRINT+4	;ASSURE CONSOLE
	ACALL	CRLF
	JBC	JKBIT,C_EX-5
	;
	JNB	DIRF,SSTOP0
	AJMP	C_K		;CLEAR COB AND EXIT
	;
T_CMP:	MOV	A,TVH		;COMPARE TIMER TO SP_H AND SP_L
	MOV	R1,TVL
	CJNE	A,TVH,T_CMP
	XCH	A,R1
	SUBB	A,SP_L
	MOV	A,R1
	SUBB	A,SP_H
	RET
	;
	;*************************************************************
	;
BR0:	; Trap the timer interrupt
	;
	;*************************************************************
	;
	CALL	T_CMP		;COMPARE TIMER
	JC	BCHR+6		;EXIT IF TEST FAILS
	SETB	OTI		;DOING THE TIMER INTERRUPT
	CLR	OTS		;CLEAR TIMER BIT
	MOV	C,INPROG	;SAVE IN PROGRESS
	MOV	ISAV,C
	MOV	DPTR,#TIV
	SJMP	BR2
	;
	newpage
	;***************************************************************
	;
	; The command action routine - RUN
	;
	;***************************************************************
	;
CRUN:	LCALL	RCLEAR-2	;CLEAR THE STORAGE ARRAYS
	ACALL	SRESTR+2	;GET THE STARTING ADDRESS
	ACALL	B_C
	JZ	CMNDLK		;IF NULL GO TO COMMAND MODE
	;
	ACALL	T_DP
	ACALL	B_TXA		;BUMP TO STARTING LINE
	;
CILOOP:	ACALL	SP0		;DO A CR AND A LF
	CLR	DIRF		;NOT IN DIRECT MODE
	;
	;INTERPERTER DRIVER
	;
ILOOP:	MOV	SP,SPSAV	;RESTORE THE STACK EACH TIME
	JB	DIRF,$+9	;NO INTERRUPTS IF IN DIRECT MODE
	MOV	INTXAH,TXAH	;SAVE THE TEXT POINTER
	MOV	INTXAL,TXAL
	LCALL	BCK		;GET CONSOLE STATUS
	JB	DIRF,I_L	;DIRECT MODE
	ANL	C,/GTRD		;SEE IF CHARACTER READY
	JNC	BCHR		;NO CHARACTER = NO CARRY
	;
	; DO TRAP OPERATION
	;
	MOV	DPTR,#GTB	;SAVE TRAP CHARACTER
	MOVX	@DPTR,A
	SETB	GTRD		;SAYS READ A BYTE
	;
BCHR:	JB	OTI,I_L		;EXIT IF TIMER INTERRUPT IN PROGRESS
	JB	OTS,BR0		;TEST TIMER VALUE IF SET
	JNB	INTPEN,I_L	;SEE IF INTERRUPT PENDING
	JB	INPROG,I_L	;DON'T DO IT AGAIN IF IN PROGRESS
	MOV	DPTR,#INTLOC	;POINT AT INTERRUPT LOCATION
	;
BR2:	MOV	R4,#GTYPE	;SETUP FOR A FORCED GOSUB
	ACALL	SGS1		;PUT TXA ON STACK
	SETB	INPROG		;INTERRUPT IN PROGRESS
	;
ERL4:	CALL	L20DPI
	AJMP	D_L1		;GET THE LINE NUMBER
	;
I_L:	ACALL	ISTAT		;LOOP
	ACALL	CLN_UP		;FINISH IT OFF
	JNC	ILOOP		;LOOP ON THE DRIVER
	JNB	DIRF,CMNDLK	;CMND1 IF IN RUN MODE
	LJMP	CMNDR		;DON'T PRINT READY
	;
CMNDLK:	LJMP	CMND1		;DONE ******AA JMP-->LJMP
	newpage
	;**************************************************************
	;
	; The Statement Action Routine - STOP
	;
	;**************************************************************
	;
SSTOP:	ACALL	CLN_UP		;FINISH OFF THIS LINE
	MOV	INTXAH,TXAH	;SAVE TEXT POINTER FOR CONT
	MOV	INTXAL,TXAL
	;
SSTOP0:	SETB	CONB		;CONTINUE WILL WORK
	MOV	DPTR,#STP	;PRINT THE STOP MESSAGE
	SETB	STOPBIT		;SET FOR ERROR ROUTINE
	LJMP	ERRS		;JUMP TO ERROR ROUTINE ******AA JMP-->LJMP
	;
	newpage
	;**************************************************************
	;
	; ITRAP - Trap special function register operators
	;
	;**************************************************************
	;
ITRAP:	CJNE	A,#TMR0,$+8	;TIMER 0
	MOV	TH0,R3
	MOV	TL0,R1
	RET
	;
	CJNE	A,#TMR1,$+8	;TIMER 1
	MOV	TH1,R3
	MOV	TL1,R1
	RET
	;
	CJNE	A,#TMR2,$+8	;TIMER 2
	DB	8BH		;MOV R3 DIRECT OP CODE
	DB	0CDH		;T2H LOCATION
	DB	89H		;MOV R1 DIRECT OP CODE
	DB	0CCH		;T2L LOCATION
	RET
	;
	CJNE	A,#TRC2,$+8	;RCAP2 TOKEN
RCL:	DB	8BH		;MOV R3 DIRECT OP CODE
	DB	0CBH		;RCAP2H LOCATION
	DB	89H		;MOV R1 DIRECT OP CODE
	DB	0CAH		;RCAP2L LOCATION
	RET
	;
	ACALL	R3CK		;MAKE SURE THAT R3 IS ZERO
	CJNE	A,#TT2C,$+6
	DB	89H		;MOV R1 DIRECT OP CODE
	DB	0C8H		;T2CON LOCATION
	RET
	;
	CJNE	A,#T_IE,$+6	;IE TOKEN
	MOV	IE,R1
	RET
	;
	CJNE	A,#T_IP,$+6	;IP TOKEN
	MOV	IP,R1
	RET
	;
	CJNE	A,#TTC,$+6	;TCON TOKEN
	MOV	TCON,R1
	RET
	;
	CJNE	A,#TTM,$+6	;TMOD TOKEN
	MOV	TMOD,R1
	RET
	;
	CJNE	A,#T_P1,T_T2	;P1 TOKEN
	MOV	P1,R1
	RET
	;
	;***************************************************************
	;
	; T_TRAP - Trap special operators
	;
	;***************************************************************
	;
T_T:	MOV	TEMP5,A		;SAVE THE TOKEN
	ACALL	GCI1		;BUMP POINTER
	ACALL	SLET2		;EVALUATE AFTER =
	MOV	A,TEMP5		;GET THE TOKEN BACK
	CJNE	A,#T_XTAL,$+6
	LJMP	AXTAL1		;SET UP CRYSTAL
	;
	ACALL	IFIXL		;R3:R1 HAS THE TOS
	MOV	A,TEMP5		;GET THE TOKEN AGAIN
	CJNE	A,#T_MTOP,T_T1	;SEE IF MTOP TOKEN
	MOV	DPTR,#MEMTOP
	CALL	S31DP
	JMP	RCLEAR		;CLEAR THE MEMORY
	;
T_T1:	CJNE	A,#T_TIME,ITRAP	;SEE IF A TIME TOKEN
	MOV	C,EA		;SAVE INTERRUPTS
	CLR	EA		;NO TIMER 0 INTERRUPTS DURING LOAD
	MOV	TVH,R3		;SAVE THE TIME
	MOV	TVL,R1
	MOV	EA,C		;RESTORE INTERRUPTS
	RET			;EXIT
	;
T_T2:	CJNE	A,#T_PC,INTERX	;PCON TOKEN
	DB	89H		;MOV DIRECT, R1 OP CODE
	DB	87H		;ADDRESS OF PCON
	RET			;EXIT
	;
T_TRAP:	CJNE	A,#T_ASC,T_T	;SEE IF ASC TOKEN
	ACALL	IGC		;EAT IT AND GET THE NEXT CHARACTER
	CJNE	A,#'$',INTERX	;ERROR IF NOT A STRING
	ACALL	CSY		;CALCULATE ADDRESS
	ACALL	X3120
	LCALL	TWO_EY          ; ******AA CALL-->LCALL
	ACALL	SPEOP+4		;EVALUATE AFTER EQUALS
	AJMP	ISTAX1		;SAVE THE CHARACTER
	;
	newpage
	;**************************************************************
	;
	;INTERPERT THE STATEMENT POINTED TO BY TXAL AND TXAH
	;
	;**************************************************************
	;
ISTAT:	ACALL	GC		;GET THR FIRST CHARACTER
	JNB	XBIT,IAT	;TRAP TO EXTERNAL RUN PACKAGE
	CJNE	A,#20H,$+3
	JNC	IAT
	LCALL	2070H		;LET THE USER SET UP THE DPTR
	ACALL	GCI1
	ANL	A,#0FH		;STRIP OFF BIAS
	SJMP	ISTA1
	;
IAT:	CJNE	A,#T_XTAL,$+3
	JNC	T_TRAP
	JNB	ACC.7,SLET	;IMPLIED LET IF BIT 7 NOT SET
	CJNE	A,#T_UOP+12,ISTAX	;DBYTE TOKEN
	ACALL	SPEOP		;EVALUATE SPECIAL OPERATOR
	ACALL	R3CK		;CHECK LOCATION
	MOV	@R1,A		;SAVE IT
	RET
	;
ISTAX:	CJNE	A,#T_UOP+13,ISTAY	;XBYTE TOKEN
	ACALL	SPEOP
	;
ISTAX1:	MOV	P2,R3
	MOVX	@R1,A
	RET
	;
ISTAY:	CJNE	A,#T_CR+1,$+3	;TRAP NEW OPERATORS
	JC	I_S
	CJNE	A,#0B0H,$+3	;SEE IF TOO BIG
	JNC	INTERX
	ADD	A,#0F9H		;BIAS FOR LOOKUP TABLE
	SJMP	ISTA0		;DO THE OPERATION
	;
I_S:	CJNE	A,#T_LAST,$+3	;MAKE SURE AN INITIAL RESERVED WORD
	JC	$+5		;ERROR IF NOT
	;
INTERX:	LJMP	E1XX		;SYNTAX ERROR
	;
	JNB	DIRF,ISTA0	;EXECUTE ALL STATEMENTS IF IN RUN MODE
	CJNE	A,#T_DIR,$+3	;SEE IF ON TOKEN
	JC	ISTA0		;OK IF DIRECT
	CJNE	A,#T_GOSB+1,$+5	;SEE IF FOR
	SJMP	ISTA0		;FOR IS OK
	CJNE	A,#T_REM+1,$+5	;NEXT IS OK
	SJMP	ISTA0
	CJNE	A,#T_STOP+6,INTERX	;SO IS REM
	;
	newpage
ISTA0:	ACALL	GCI1		;ADVANCE THE TEXT POINTER
	MOV	DPTR,#STATD	;POINT DPTR TO LOOKUP TABLE
	CJNE	A,#T_GOTO-3,$+5	;SEE IF LET TOKEN
	SJMP	ISTAT		;WASTE LET TOKEN
	ANL	A,#3FH		;STRIP OFF THE GARBAGE
	;
ISTA1:	RL	A		;ROTATE FOR OFFSET
	ADD	A,DPL		;BUMP
	MOV	DPL,A		;SAVE IT
	CLR	A
	MOVC	A,@A+DPTR	;GET HIGH BYTE
	PUSH	ACC		;SAVE IT
	INC	DPTR
	CLR	A
	MOVC	A,@A+DPTR	;GET LOW BYTE
	POP	DPH
	MOV	DPL,A
	;
AC1:	CLR	A
	JMP	@A+DPTR		;GO DO IT
	;
	newpage
	;***************************************************************
	;
	; The statement action routine - LET
	;
	;***************************************************************
	;
SLET:	ACALL	S_C		;CHECK FOR POSSIBLE STRING
	JC	SLET0		;NO STRING
	CLR	LINEB		;USED STRINGS
	;
	CALL	X31DP		;PUT ADDRESS IN DPTR
	MOV	R7,#T_EQU	;WASTE =
	ACALL	EATC
	ACALL	GC		;GET THE NEXT CHARACTER
	CJNE	A,#'"',S_3	;CHECK FOR A "
	MOV	R7,S_LEN	;GET THE STRING LENGTH
	;
S_0:	ACALL	GCI1		;BUMP PAST "
	ACALL	DELTST		;CHECK FOR DELIMITER
	JZ	INTERX		;EXIT IF CARRIAGE RETURN
	MOVX	@DPTR,A		;SAVE THE CHARACTER
	CJNE	A,#'"',S_1	;SEE IF DONE
	;
S_E:	MOV	A,#CR		;PUT A CR IN A
	MOVX	@DPTR,A		;SAVE CR
	AJMP	GCI1
	;
S_3:	PUSH	DPH
	PUSH	DPL		;SAVE DESTINATION
	ACALL	S_C		;CALCULATE SOURCE
	JC	INTERX		;ERROR IF CARRY
	POP	R0B0		;GET DESTINATION BACK
	POP	R2B0
	;
SSOOP:	MOV	R7,S_LEN	;SET UP COUNTER
	;
S_4:	LCALL	TBYTE		;TRANSFER THE BYTE ******AA CALL-->LCALL
	CJNE	A,#CR,$+4	;EXIT IF A CR
	RET
	DJNZ	R7,S_5		;BUMP COUNTER
	MOV	A,#CR		;SAVE A CR
	MOVX	@R0,A
	AJMP	EIGP		;PRINT EXTRA IGNORED
	;
	newpage
	;
S_5:	CALL	INC3210		;BUMP POINTERS
	SJMP	S_4		;LOOP
	;
S_1:	DJNZ	R7,$+8		;SEE IF DONE
	ACALL	S_E
	ACALL	EIGP		;PRINT EXTRA IGNORED
	AJMP	FINDCR		;GO FIND THE END
	INC	DPTR		;BUMP THE STORE POINTER
	SJMP	S_0		;CONTINUE TO LOOP
	;
E3XX:	MOV	DPTR,#E3X	;BAD ARG ERROR
	AJMP	EK
	;
SLET0:	ACALL	SLET1
	AJMP	POPAS		;COPY EXPRESSION TO VARIABLE
	;
SLET1:	ACALL	VAR_ER		;CHECK FOR A"VARIABLE"
	;
SLET2:	PUSH	R2B0		;SAVE THE VARIABLE ADDRESS
	PUSH	R0B0
	MOV	R7,#T_EQU	;GET EQUAL TOKEN
	ACALL	WE
	POP	R1B0		;POP VARIABLE TO R3:R1
	POP	R3B0
	RET			;EXIT
	;
R3CK:	CJNE	R3,#00H,E3XX	;CHECK TO SEE IF R3 IS ZERO
	RET
	;
SPEOP:	ACALL	GCI1		;BUMP TXA
	ACALL	P_E		;EVALUATE PAREN
	ACALL	SLET2		;EVALUATE AFTER =
	CALL	TWOL		;R7:R6 GETS VALUE, R3:R1 GETS LOCATION
	MOV	A,R6		;SAVE THE VALUE
	;
	CJNE	R7,#00H,E3XX	;R2 MUST BE = 0
	RET
	;
	newpage
	;**************************************************************
	;
	; ST_CAL - Calculate string Address
	;
	;**************************************************************
	;
IST_CAL:;
	;
	ACALL	I_PI		;BUMP TEXT, THEN EVALUATE
	ACALL	R3CK		;ERROR IF R3 <> 0
	INC	R1		;BUMP FOR OFFSET
	MOV	A,R1		;ERROR IF R1 = 255
	JZ	E3XX
	MOV	DPTR,#VARTOP	;GET TOP OF VARIABLE STORAGE
	MOV	B,S_LEN		;MULTIPLY FOR LOCATION
	ACALL	VARD		;CALCULATE THE LOCATION
	MOV	DPTR,#MEMTOP	;SEE IF BLEW IT
	CALL	FUL1
	MOV	DPL,S_LEN	;GET STRING LENGTH, DPH = 00H
	DEC	DPH		;DPH = 0
	;
DUBSUB:	CLR	C
	MOV	A,R1
	SUBB	A,DPL
	MOV	R1,A
	MOV	A,R3
	SUBB	A,DPH
	MOV	R3,A
	ORL	A,R1
	RET
	;
	;***************************************************************
	;
	;VARD - Calculate the offset base
	;
	;***************************************************************
	;
VARB:	MOV	B,#FPSIZ	;SET UP FOR OPERATION
	;
VARD:	CALL	LDPTRI		;LOAD DPTR
	MOV	A,R1		;MULTIPLY BASE
	MUL	AB
	ADD	A,DPL
	MOV	R1,A
	MOV	A,B
	ADDC	A,DPH
	MOV	R3,A
	RET
	;
	newpage
	;*************************************************************
	;
CSY:	; Calculate a biased string address and put in R3:R1
	;
	;*************************************************************
	;
	ACALL	IST_CAL		;CALCULATE IT
	PUSH	R3B0		;SAVE IT
	PUSH	R1B0
	MOV	R7,#','		;WASTE THE COMMA
	ACALL	EATC
	ACALL	ONE		;GET THE NEXT EXPRESSION
	MOV	A,R1		;CHECK FOR BOUNDS
	CJNE	A,S_LEN,$+3
	JNC	E3XX		;MUST HAVE A CARRY
	DEC	R1		;BIAS THE POINTER
	POP	ACC		;GET VALUE LOW
	ADD	A,R1		;ADD IT TO BASE
	MOV	R1,A		;SAVE IT
	POP	R3B0		;GET HIGH ADDRESS
	JNC	$+3		;PROPAGATE THE CARRY
	INC	R3
	AJMP	ERPAR		;WASTE THE RIGHT PAREN
	;
	newpage
	;***************************************************************
	;
	; The statement action routine FOR
	;
	;***************************************************************
	;
SFOR:	ACALL	SLET1		;SET UP CONTROL VARIABLE
	PUSH	R3B0		;SAVE THE CONTROL VARIABLE LOCATION
	PUSH	R1B0
	ACALL	POPAS		;POP ARG STACK AND COPY CONTROL VAR
	MOV	R7,#T_TO	;GET TO TOKEN
	ACALL	WE
	ACALL	GC		;GET NEXT CHARACTER
	CJNE	A,#T_STEP,SF2
	ACALL	GCI1		;EAT THE TOKEN
	ACALL	EXPRB		;EVALUATE EXPRESSION
	SJMP	$+5		;JUMP OVER
	;
SF2:	LCALL	PUSH_ONE	;PUT ONE ON THE STACK
	;
	MOV	A,#-FSIZE	;ALLOCATE FSIZE BYTES ON THE CONTROL STACK
	ACALL	PUSHCS		;GET CS IN R0
	ACALL	CSC		;CHECK CONTROL STACK
	MOV	R3,#CSTKAH	;IN CONTROL STACK
	MOV	R1,R0B0		;STACK ADDRESS
	ACALL	POPAS		;PUT STEP ON STACK
	ACALL	POPAS		;PUT LIMIT ON STACK
	ACALL	DP_T		;DPTR GETS TEXT
	MOV	R0,R1B0		;GET THE POINTER
	ACALL	T_X_S		;SAVE THE TEXT
	POP	TXAL		;GET CONTROL VARIABLE
	POP	TXAH
	MOV	R4,#FTYPE	;AND THE TYPE
	ACALL	T_X_S		;SAVE IT
	;
SF3:	ACALL	T_DP		;GET THE TEXT POINTER
	AJMP	ILOOP		;CONTINUE TO PROCESS
	;
	newpage
	;**************************************************************
	;
	; The statement action routines - PUSH and POP
	;
	;**************************************************************
	;
SPUSH:	ACALL	EXPRB		;PUT EXPRESSION ON STACK
	ACALL	C_TST		;SEE IF MORE TO DO
	JNC	SPUSH		;IF A COMMA PUSH ANOTHER
	RET
	;
	;
SPOP:	ACALL	VAR_ER		;GET VARIABLE
	ACALL	XPOP		;FLIP THE REGISTERS FOR POPAS
	ACALL	C_TST		;SEE IF MORE TO DO
	JNC	SPOP
	;
	RET
	;
	;***************************************************************
	;
	; The statement action routine - IF
	;
	;***************************************************************
	;
SIF:	ACALL	RTST		;EVALUATE THE EXPRESSION
	MOV	R1,A		;SAVE THE RESULT
	ACALL	GC		;GET THE CHARACTER AFTER EXPR
	CJNE	A,#T_THEN,$+5	;SEE IF THEN TOKEN
	ACALL	GCI1		;WASTE THEN TOKEN
	CJNE	R1,#0,T_F1	;CHECK R_OP RESULT
	;
E_FIND:	MOV	R7,#T_ELSE	;FIND ELSE TOKEN
	ACALL	FINDC
	JZ	SIF-1		;EXIT IF A CR
	ACALL	GCI1		;BUMP PAST TOKEN
	CJNE	A,#T_ELSE,E_FIND;WASTE IF NO ELSE
	;
T_F1:	ACALL	INTGER		;SEE IF NUMBER
	JNC	D_L1		;EXECUTE LINE NUMBER
	AJMP	ISTAT		;EXECUTE STATEMENT IN NOT
	;
B_C:	MOVX	A,@DPTR
	DEC	A
	JB	ACC.7,FL3-5
	RET
	;
	newpage
	;***************************************************************
	;
	; The statement action routine - GOTO
	;
	;***************************************************************
	;
SGOTO:	ACALL	RLINE		;R2:R0 AND DPTR GET INTGER
	;
SGT1:	ACALL	T_DP		;TEXT POINTER GETS DPTR
	;
	JBC	RETBIT,SGT2	;SEE IF RETI EXECUTED
	;
	JNB	LINEB,$+6	;SEE IF A LINE WAS EDITED
	LCALL	RCLEAR-2	;CLEAR THE MEMORY IF SET
	AJMP	ILOOP-2		;CLEAR DIRF AND LOOP
	;
SGT2:	JBC	OTI,$+8		;SEE IF TIMER INTERRUPT
	ANL	34,#10111101B	;CLEAR INTERRUPTS
	AJMP	ILOOP		;EXECUTE
	MOV	C,ISAV
	MOV	INPROG,C
	AJMP	ILOOP		;RESTORE INTERRUPTS AND RET
	;
	;
	;*************************************************************
	;
RTST:	; Test for ZERO
	;
	;*************************************************************
	;
	ACALL	EXPRB		;EVALUATE EXPRESSION
	CALL	INC_ASTKA	;BUMP ARG STACK
	JZ	$+4		;EXIT WITH ZERO OR 0FFH
	MOV	A,#0FFH
	RET
	;
	newpage
	;
	;**************************************************************
	;
	; GLN - get the line number in R2:R0, return in DPTR
	;
	;**************************************************************
	;
GLN:	ACALL	DP_B		;GET THE BEGINNING ADDRESS
	;
FL1:	MOVX	A,@DPTR		;GET THE LENGTH
	MOV	R7,A		;SAVE THE LENGTH
	DJNZ	R7,FL3		;SEE IF END OF FILE
	;
	MOV	DPTR,#E10X	;NO LINE NUMBER
	AJMP	EK		;HANDLE THE ERROR
	;
FL3:	JB	ACC.7,$-5	;CHECK FOR BIT 7
	INC	DPTR		;POINT AT HIGH BYTE
	MOVX	A,@DPTR		;GET HIGH BYTE
	CJNE	A,R2B0,FL2	;SEE IF MATCH
	INC	DPTR		;BUMP TO LOW BYTE
	DEC	R7		;ADJUST AGAIN
	MOVX	A,@DPTR		;GET THE LOW BYTE
	CJNE	A,R0B0,FL2	;SEE IF LOW BYTE MATCH
	INC	DPTR		;POINT AT FIRST CHARACTER
	RET			;FOUND IT
	;
FL2:	MOV	A,R7		;GET THE LENGTH COUNTER
	CALL	ADDPTR		;ADD A TO DATA POINTER
	SJMP	FL1		;LOOP
	;
	;
	;*************************************************************
	;
	;RLINE - Read in ASCII string, get line, and clean it up
	;
	;*************************************************************
	;
RLINE:	ACALL	INTERR		;GET THE INTEGER
	;
RL1:	ACALL	GLN
	AJMP	CLN_UP
	;
	;
D_L1:	ACALL	GLN		;GET THE LINE
	AJMP	SGT1		;EXECUTE THE LINE
	;
	newpage
	;***************************************************************
	;
	; The statement action routines WHILE and UNTIL
	;
	;***************************************************************
	;
SWHILE:	ACALL	RTST		;EVALUATE RELATIONAL EXPRESSION
	CPL	A
	SJMP	S_WU
	;
SUNTIL:	ACALL	RTST		;EVALUATE RELATIONAL EXPRESSION
	;
S_WU:	MOV	R4,#DTYPE	;DO EXPECTED
	MOV	R5,A		;SAVE R_OP RESULT
	SJMP	SR0		;GO PROCESS
	;
	;
	;***************************************************************
	;
CNULL:	; The Command Action Routine - NULL
	;
	;***************************************************************
	;
	ACALL	INTERR		;GET AN INTEGER FOLLOWING NULL
	MOV	NULLCT,R0	;SAVE THE NULLCOUNT
	AJMP	CMNDLK		;JUMP TO COMMAND MODE
	;
	newpage
	;***************************************************************
	;
	; The statement action routine - RETI
	;
	;***************************************************************
	;
SRETI:	SETB	RETBIT		;SAYS THAT RETI HAS BEEN EXECUTED
	;
	;***************************************************************
	;
	; The statement action routine - RETURN
	;
	;***************************************************************
	;
SRETRN:	MOV	R4,#GTYPE	;MAKE SURE OF GOSUB
	MOV	R5,#55H		;TYPE RETURN TYPE
	;
SR0:	ACALL	CSETUP		;SET UP CONTROL STACK
	MOVX	A,@R0		;GET RETURN TEXT ADDRESS
	MOV	DPH,A
	INC	R0
	MOVX	A,@R0
	MOV	DPL,A
	INC	R0		;POP CONTROL STACK
	MOVX	A,@DPTR		;SEE IF GOSUB WAS THE LAST STATEMENT
	CJNE	A,#EOF,$+5
	AJMP	CMNDLK
	MOV	A,R5		;GET TYPE
	JZ	SGT1		;EXIT IF ZERO
	MOV	CSTKA,R0	;POP THE STACK
	CPL	A		;OPTION TEST, 00H, 55H, 0FFH, NOW 55H
	JNZ	SGT1		;MUST BE GOSUB
	RET			;NORMAL FALL THRU EXIT FOR NO MATCH
	;
	newpage
	;***************************************************************
	;
	; The statement action routine - GOSUB
	;
	;***************************************************************
	;
SGOSUB:	ACALL	RLINE		;NEW TXA IN DPTR
	;
SGS0:	MOV	R4,#GTYPE
	ACALL	SGS1		;SET EVERYTHING UP
	AJMP	SF3		;EXIT
	;
SGS1:	MOV	A,#-3		;ALLOCATE 3 BYTES ON CONTROL STACK
	ACALL	PUSHCS
	;
T_X_S:	MOV	P2,#CSTKAH	;SET UP PORT FOR CONTROL STACK
	MOV	A,TXAL		;GET RETURN ADDRESS AND SAVE IT
	MOVX	@R0,A
	DEC	R0
	MOV	A,TXAH
	MOVX	@R0,A
	DEC	R0
	MOV	A,R4		;GET TYPE
	MOVX	@R0,A		;SAVE TYPE
	RET			;EXIT
	;
	;
CS1:	MOV	A,#3		;POP 3 BYTES
	ACALL	PUSHCS
	;
CSETUP:	MOV	R0,CSTKA	;GET CONTROL STACK
	MOV	P2,#CSTKAH
	MOVX	A,@R0		;GET BYTE
	CJNE	A,R4B0,$+5	;SEE IF TYPE MATCH
	INC	R0
	RET
	JZ	E4XX		;EXIT IF STACK UNDERFLOW
	CJNE	A,#FTYPE,CS1	;SEE IF FOR TYPE
	ACALL	PUSHCS-2	;WASTE THE FOR TYPE
	SJMP	CSETUP		;LOOP
	;
	newpage
	;***************************************************************
	;
	; The statement action routine - NEXT
	;
	;***************************************************************
	;
SNEXT:	MOV	R4,#FTYPE	;FOR TYPE
	ACALL	CSETUP		;SETUP CONTROL STACK
	MOV	TEMP5,R0	;SAVE CONTROL VARIABLE ADDRESS
	MOV	R1,#TEMP1	;SAVE VAR + RETURN IN TEMP1-4
	;
XXI:	MOVX	A,@R0		;LOOP UNTIL DONE
	MOV	@R1,A
	INC	R1
	INC	R0
	CJNE	R1,#TEMP5,XXI
	;
	ACALL	VAR		;SEE IF THE USER HAS A VARIABLE
	JNC	$+6
	MOV	R2,TEMP1
	MOV	R0,TEMP2
	MOV	A,R2		;SEE IF VAR'S AGREE
	CJNE	A,TEMP1,E4XX
	MOV	A,R0
	CJNE	A,TEMP2,E4XX
	ACALL	PUSHAS		;PUT CONTROL VARIABLE ON STACK
	MOV	A,#FPSIZ+FPSIZ+2;COMPUTE ADDRESS TO STEP VALUE SIGN
	ADD	A,TEMP5		;ADD IT TO BASE OF STACK
	MOV	R0,A		;SAVE IN R0
	MOV	R2,#CSTKAH	;SET UP TO PUSH STEP VALUE
	MOV	P2,R2		;SET UP PORT
	MOVX	A,@R0		;GET SIGN
	INC	R0		;BACK TO EXPONENT
	PUSH	ACC		;SAVE SIGN OF STEP
	ACALL	PUSHAS		;PUT STEP VALUE ON STACK
	PUSH	R0B0		;SAVE LIMIT VALUE LOCATION
	CALL	AADD		;ADD STEP VALUE TO VARIABLE
	CALL	CSTAKA		;COPY STACK
	MOV	R3,TEMP1	;GET CONTROL VARIABLE
	MOV	R1,TEMP2
	ACALL	POPAS		;SAVE THE RESULT
	MOV	R2,#CSTKAH	;RESTORE LIMIT LOCATION
	POP	R0B0
	ACALL	PUSHAS		;PUT LIMIT ON STACK
	CALL	FP_BASE+4	;DO THE COMPARE
	POP	ACC		;GET LIMIT SIGN BACK
	JZ	$+3		;IF SIGN NEGATIVE, TEST "BACKWARDS"
	CPL	C
	ORL	C,F0		;SEE IF EQUAL
	JC	N4		;STILL SMALLER THAN LIMIT?
	MOV	A,#FSIZE	;REMOVE CONTROL STACK ENTRY
	;
	; Fall thru to PUSHCS
	;
	newpage
	;***************************************************************
	;
	; PUSHCS - push frame onto control stack
	;          acc has - number of bytes, also test for overflow
	;
	;***************************************************************
	;
PUSHCS:	ADD	A,CSTKA		;BUMP CONTROL STACK
	CJNE	A,#CONVT+17,$+3	;SEE IF OVERFLOWED
	JC	E4XX		;EXIT IF STACK OVERFLOW
	XCH	A,CSTKA		;STORE NEW CONTROL STACK VALUE, GET OLD
	DEC	A		;BUMP OLD VALUE
	MOV	R0,A		;PUT OLD-1 IN R0
	;
	RET			;EXIT
	;
CSC:	ACALL	CLN_UP		;FINISH OFF THE LINE
	JNC	CSC-1		;EXIT IF NO TERMINATOR
	;
E4XX:	MOV	DPTR,#EXC	;CONTROL STACK ERROR
	AJMP	EK		;STACK ERROR
	;
N4:	MOV	TXAH,TEMP3	;GET TEXT POINTER
	MOV	TXAL,TEMP4
	AJMP	ILOOP		;EXIT
	;
	;***************************************************************
	;
	; The statement action routine - RESTORE
	;
	;***************************************************************
	;
SRESTR:	ACALL	X_TR		;SWAP POINTERS
	ACALL	DP_B		;GET THE STARTING ADDRESS
	ACALL	T_DP		;PUT STARTING ADDRESS IN TEXT POINTER
	ACALL	B_TXA		;BUMP TXA
	;
	; Fall thru
	;
X_TR:	;swap txa and rtxa
	;
	XCH	A,TXAH
	XCH	A,RTXAH
	XCH	A,TXAH
	XCH	A,TXAL
	XCH	A,RTXAL
	XCH	A,TXAL
	RET			;EXIT
	;
	newpage
	;***************************************************************
	;
	; The statement action routine - READ
	;
	;***************************************************************
	;
SREAD:	ACALL	X_TR		;SWAP POINTERS
	;
SRD0:	ACALL	C_TST		;CHECK FOR COMMA
	JC	SRD4		;SEE WHAT IT IS
	;
SRD:	ACALL	EXPRB		;EVALUATE THE EXPRESSION
	ACALL	GC		;GET THE CHARACTER AFTER EXPRESSION
	CJNE	A,#',',SRD1	;SEE IF MORE DATA
	SJMP	SRD2		;BYBASS CLEAN UP IF A COMMA
	;
SRD1:	ACALL	CLN_UP		;FINISH OFF THE LINE, IF AT END
	;
SRD2:	ACALL	X_TR		;RESTORE POINTERS
	ACALL	VAR_ER		;GET VARIABLE ADDRESS
	ACALL	XPOP		;FLIP THE REGISTERS FOR POPAS
	ACALL	C_TST		;SEE IF A COMMA
	JNC	SREAD		;READ AGAIN IF A COMMA
	RET			;EXIT IF NOT
	;
SRD4:	CJNE	A,#T_DATA,SRD5	;SEE IF DATA
	ACALL	GCI1		;BUMP POINTER
	SJMP	SRD
	;
SRD5:	CJNE	A,#EOF,SRD6	;SEE IF YOU BLEW IT
	ACALL	X_TR		;GET THE TEXT POINTER BACK
	MOV	DPTR,#E14X	;READ ERROR
	;
EK:	LJMP	ERROR
	;
SRD6:	ACALL	FINDCR		;WASTE THIS LINE
	ACALL	CLN_UP		;CLEAN IT UP
	JC	SRD5+3		;ERROR IF AT END
	SJMP	SRD0
	;
NUMC:	ACALL	GC		;GET A CHARACTER
	CJNE	A,#'#',NUMC1	;SEE IF A #
	SETB	COB		;VALID LINE PRINT
	AJMP	IGC		;BUMP THE TEXT POINTER
	;
NUMC1:	CJNE	A,#'@',SRD4-1	;EXIT IF NO GOOD
	SETB	LPB
	AJMP	IGC
	;
	newpage
	;***************************************************************
	;
	; The statement action routine - PRINT
	;
	;***************************************************************
	;
SPH0:	SETB	ZSURP		;NO ZEROS
	;
SPH1:	SETB	HMODE		;HEX MODE
	;
SPRINT:	ACALL	NUMC		;TEST FOR A LINE PRINT
	ACALL	$+9		;PROCEED
	ANL	35,#11110101B	;CLEAR COB AND LPB
	ANL	38,#00111111B	;NO HEX MODE
	;
	RET
	;
	ACALL	DELTST		;CHECK FOR A DELIMITER
	JC	SP1
	;
SP0:	JMP	CRLF		;EXIT WITH A CR IF SO
	;
SP2:	ACALL	C_TST		;CHECK FOR A COMMA
	JC	SP0		;EXIT IF NO COMMA
	;
SP1:	ACALL	CPS		;SEE IF A STRING TO PRINT
	JNC	SP2		;IF A STRING, CHECK FOR A COMMA
	;
SP4:	CJNE	A,#T_TAB,SP6
	ACALL	I_PI		;ALWAYS CLEARS CARRY
	SUBB	A,PHEAD		;TAKE DELTA BETWEEN TAB AND PHEAD
	JC	SP2		;EXIT IF PHEAD > TAB
	SJMP	SP7		;OUTPUT SPACES
	;
SP6:	CJNE	A,#T_SPC,SM
	ACALL	I_PI		;SET UP PAREN VALUE
	;
SP7:	JZ	SP2
	LCALL	STEROT		;OUTPUT A SPACE
	DEC	A		;DECREMENT COUNTER
	SJMP	SP7		;LOOP
	;
	newpage
SM:	CJNE	A,#T_CHR,SP8
	ACALL	IGC
	CJNE	A,#'$',$+9
	ACALL	CNX		;PUT THE CHARACTER ON THE STACK
	ACALL	IFIXL		;PUT THE CHARACTER IN R1
	SJMP	$+6
	ACALL	ONE		;EVALUATE THE EXPRESSION, PUT IN R3:R1
	ACALL	ERPAR
	MOV	R5,R1B0		;BYTE TO OUTPUT
	SJMP	SQ
	;
SP8:	CJNE	A,#T_CR,SX
	ACALL	GCI1		;EAT THE TOKEN
	MOV	R5,#CR
	;
SQ:	CALL	TEROT
	SJMP	SP2		;OUTPUT A CR AND DO IT AGAIN
	;
SX:	CJNE	A,#T_USE,SP9	;USING TOKEN
	ACALL	IGC		;GE THE CHARACTER AFTER THE USING TOKEN
	CJNE	A,#'F',U4	;SEE IF FLOATING
	MOV	FORMAT,#0F0H	;SET FLOATING
	ACALL	IGC		;BUMP THE POINTER AND GET THE CHARACTER
	ACALL	GCI1		;BUMP IT AGAIN
	ANL	A,#0FH		;STRIP OFF ASCII BIAS
	JZ	U3		;EXIT IF ZERO
	CJNE	A,#3,$+3	;SEE IF AT LEAST A THREE
	JNC	U3		;FORCE A THREE IF NOT A THREE
	MOV	A,#3
	;
U3:	ORL	FORMAT,A	;PUT DIGIT IN FORMAT
	SJMP	U8		;CLEAN UP END
	;
U4:	CJNE	A,#'0',U5
	MOV	FORMAT,#0	;FREE FORMAT
	ACALL	GCI1		;BUMP THE POINTER
	SJMP	U8
	;
U5:	CJNE	A,#'#',U8	;SEE IF INTGER FORMAT
	ACALL	U6
	MOV	FORMAT,R7	;SAVE THE FORMAT
	CJNE	A,#'.',U8A	;SEE IF TERMINATOR WAS RADIX
	ACALL	IGC		;BUMP PAST .
	ACALL	U6		;LOOP AGAIN
	MOV	A,R7		;GET COUNT
	ADD	A,FORMAT	;SEE IF TOO BIG
	ADD	A,#0F7H
	JNC	U5A
	;
	newpage
SE0:	AJMP	INTERX		;ERROR, BAD SYNTAX
	;
U5A:	MOV	A,R7		;GET THE COUNT BACK
	SWAP	A		;ADJUST
	ORL	FORMAT,A	;GET THE COUNT
	;
U8A:	MOV	A,FORMAT
	;
U8B:	SWAP	A		;GET THE FORMAT RIGHT
	MOV	FORMAT,A
	;
U8:	ACALL	ERPAR
	AJMP	SP2		;DONE
	;
U6:	MOV	R7,#0		;SET COUNTER
	;
U7:	CJNE	A,#'#',SP9A	;EXIT IF NOT A #
	INC	R7		;BUMP COUNTER
	ACALL	IGC		;GET THE NEXT CHARACTER
	SJMP	U7		;LOOP
	;
SP9:	ACALL	DELTST+2	;CHECK FOR DELIMITER
	JNC	SP9A		;EXIT IF A DELIMITER
	;
	CJNE	A,#T_ELSE,SS
	;
SP9A:	RET			;EXIT IF ELSE TOKEN
	;
	;**************************************************************
	;
	; P_E - Evaluate an expression in parens ( )
	;
	;**************************************************************
	;
P_E:	MOV	R7,#T_LPAR
	ACALL	WE
	;
ERPAR:	MOV	R7,#')'		;EAT A RIGHT PAREN
	;
EATC:	ACALL	GCI		;GET THE CHARACTER
	CJNE	A,R7B0,SE0	;ERROR IF NOT THE SAME
	RET
	;
	newpage
	;***************************************************************
	;
S_ON:	; ON Statement
	;
	;***************************************************************
	;
	ACALL	ONE		;GET THE EXPRESSION
	ACALL	GCI		;GET THE NEXT CHARACTER
	CJNE	A,#T_GOTO,C0
	ACALL	C1		;EAT THE COMMAS
	AJMP	SF3		;DO GOTO
	;
C0:	CJNE	A,#T_GOSB,SE0
	ACALL	C1
	AJMP	SGS0		;DO GOSUB
	;
C1:	CJNE	R1,#0,C2
	ACALL	INTERR		;GET THE LINE NUMBER
	ACALL	FINDCR
	AJMP	RL1		;FINISH UP THIS LINE
	;
C2:	MOV	R7,#','
	ACALL	FINDC
	CJNE	A,#',',SE0	;ERROR IF NOT A COMMA
	DEC	R1
	ACALL	GCI1		;BUMP PAST COMMA
	SJMP	C1
	;
	newpage
	;
SS:	ACALL	S_C		;SEE IF A STRING
	JC	SA		;NO STRING IF CARRY IS SET
	LCALL	UPRNT		;PUT POINTER IN DPTR
	AJMP	SP2		;SEE IF MORE
	;
SA:	ACALL	EXPRB		;MUST BE AN EXPRESSION
	MOV	A,#72
	CJNE	A,PHEAD,$+3	;CHECK PHEAD POSITION
	JNC	$+4
	ACALL	SP0		;FORCE A CRLF
	JNB	HMODE,S13	;HEX MODE?
	CALL	FCMP		;SEE IF TOS IS < 0FFFH
	JC	S13		;EXIT IF GREATER
	CALL	AABS		;GET THE SIGN
	JNZ	OOPS		;WASTE IF NEGATIVE
	ACALL	IFIXL
	CALL	FP_BASE+22	;PRINT HEXMODE
	AJMP	SP2
OOPS:	CALL	ANEG		;MAKE IT NEGATIVE
	;
S13:	CALL	FP_BASE+14	;DO FP OUTPUT
	MOV	A,#1		;OUTPUT A SPACE
	AJMP	SP7
	;
	newpage
	;***************************************************************
	;
	; ANU -  Get variable name from text - set carry if not found
	;        if succeeds returns variable in R7:R6
	;        R6 = 0 if no digit in name
	;
	;***************************************************************
	;
ANU:	ACALL	IGC		;INCREMENT AND GET CHARACTER
	LCALL	1FEDH		;CHECK FOR DIGIT
	JC	$+14		;EXIT IF VALID DIGIT
	CJNE	A,#'_',$+4	;SEE IF A _
	RET
	;
AL:	CJNE	A,#'A',$+3	;IS IT AN ASCII A?
	JC	$+6		;EXIT IF CARRY IS SET
	CJNE	A,#'Z'+1,$+3	;IS IT LESS THAN AN ASCII Z
	CPL	C		;FLIP CARRY
	RET
	;
	JNB	F0,VAR2
	;
SD0:	MOV	DPTR,#E6X
	AJMP	EK
	;
SDIMX:	SETB	F0		;SAYS DOING A DIMENSION
	SJMP	VAR1
	;
VAR:	CLR	F0		;SAYS DOING A VARIABLE
	;
VAR1:	ACALL	GC		;GET THE CHARACTER
	ACALL	AL		;CHECK FOR ALPHA
	JNC	$+6		;ERROR IF IN DIM
	JB	F0,SD0
	RET
	MOV	R7,A		;SAVE ALPHA CHARACTER
	CLR	A		;ZERO IN CASE OF FAILURE
	MOV	R5,A		;SAVE IT
	;
VY:	MOV	R6,A
	ACALL	ANU		;CHECK FOR ALPHA OR NUMBER
	JC	VX		;EXIT IF NO ALPHA OR NUM
	;
	XCH	A,R7
	ADD	A,R5		;NUMBER OF CHARACTERS IN ALPHABET
	XCH	A,R7		;PUT IT BACK
	MOV	R5,#26		;FOR THE SECOND TIME AROUND
	SJMP	VY
	;
VX:	CLR	LINEB		;TELL EDITOR A VARIABLE IS DECLARED
	CJNE	A,#T_LPAR,V4	;SEE IF A LEFT PAREN
	;
	ORL	R6B0,#80H	;SET BIT 7 TO SIGINIFY MATRIX
	CALL	F_VAR		;FIND THE VARIABLE
	PUSH	R2B0		;SAVE THE LOCATION
	PUSH	R0B0
	JNC	SD0-3		;DEFAULT IF NOT IN TABLE
	JB	F0,SDI		;NO DEFAULT FOR DIMENSION
	MOV	R1,#10
	MOV	R3,#0
	ACALL	D_CHK
	;
VAR2:	ACALL	PAREN_INT	;EVALUATE INTEGER IN PARENS
	CJNE	R3,#0,SD0	;ERROR IF R3<>0
	POP	DPL		;GET VAR FOR LOOKUP
	POP	DPH
	MOVX	A,@DPTR		;GET DIMENSION
	DEC	A		;BUMP OFFSET
	SUBB	A,R1		;A MUST BE > R1
	JC	SD0
	LCALL	DECDP2		;BUMP POINTER TWICE
	ACALL	VARB		;CALCULATE THE BASE
	;
X3120:	XCH	A,R1		;SWAP R2:R0, R3:R1
	XCH	A,R0
	XCH	A,R1
	XCH	A,R3
	XCH	A,R2
	XCH	A,R3
	RET
	;
V4:	JB	F0,SD0		;ERROR IF NO LPAR FOR DIM
	LCALL	F_VAR		;GET SCALAR VARIABLE
	CLR	C
	RET
	;
	newpage
	;
SDI:	ACALL	PAREN_INT	;EVALUATE PAREN EXPRESSION
	CJNE	R3,#0,SD0	;ERROR IF NOT ZERO
	POP	R0B0		;SET UP R2:R0
	POP	R2B0
	ACALL	D_CHK		;DO DIM
	ACALL	C_TST		;CHECK FOR COMMA
	JNC	SDIMX		;LOOP IF COMMA
	RET			;RETURN IF NO COMMA
	;
D_CHK:	INC	R1		;BUMP FOR TABLE LOOKUP
	MOV	A,R1
	JZ	SD0		;ERROR IF 0FFFFH
	MOV	R4,A		;SAVE FOR LATER
	MOV	DPTR,#MT_ALL	;GET MATRIX ALLOCATION
	ACALL	VARB		;DO THE CALCULATION
	MOV	R7,DPH		;SAVE MATRIX ALLOCATION
	MOV	R6,DPL
	MOV	DPTR,#ST_ALL	;SEE IF TOO MUCH MEMORY TAKEN
	CALL	FUL1		;ST_ALL SHOULD BE > R3:R1
	MOV	DPTR,#MT_ALL	;SAVE THE NEW MATRIX POINTER
	CALL	S31DP
	MOV	DPL,R0		;GET VARIABLE ADDRESS
	MOV	DPH,R2
	MOV	A,R4		;DIMENSION SIZE
	MOVX	@DPTR,A		;SAVE IT
	CALL	DECDP2		;SAVE TARGET ADDRESS
	;
R76S:	MOV	A,R7
	MOVX	@DPTR,A
	INC	DPTR
	MOV	A,R6		;ELEMENT SIZE
	MOVX	@DPTR,A
	RET			;R2:R0 STILL HAS SYMBOL TABLE ADDRESS
	;
	newpage
	;***************************************************************
	;
	; The statement action routine - INPUT
	;
	;***************************************************************
	;
SINPUT:	ACALL	CPS		;PRINT STRING IF THERE
	;
	ACALL	C_TST		;CHECK FOR A COMMA
	JNC	IN2A		;NO CRLF
	ACALL	SP0		;DO A CRLF
	;
IN2:	MOV	R5,#'?'		;OUTPUT A ?
	CALL	TEROT
	;
IN2A:	SETB	INP_B		;DOING INPUT
	CALL	INLINE		;INPUT THE LINE
	CLR	INP_B
	MOV	TEMP5,#HI(IBUF)
	MOV	TEMP4,#LO(IBUF)
	;
IN3:	ACALL	S_C		;SEE IF A STRING
	JC	IN3A		;IF CARRY IS SET, NO STRING
	ACALL	X3120		;FLIP THE ADDRESSES
	MOV	R3,TEMP5
	MOV	R1,TEMP4
	ACALL	SSOOP
	ACALL	C_TST		;SEE IF MORE TO DO
	JNC	IN2
	RET
	;
IN3A:	CALL	DTEMP		;GET THE USER LOCATION
	CALL	GET_NUM		;GET THE USER SUPPLIED NUMBER
	JNZ	IN5		;ERROR IF NOT ZERO
	CALL	TEMPD		;SAVE THE DATA POINTER
	ACALL	VAR_ER		;GET THE VARIABLE
	ACALL	XPOP		;SAVE THE VARIABLE
	CALL	DTEMP		;GET DPTR BACK FROM VAR_ER
	ACALL	C_TST		;SEE IF MORE TO DO
	JC	IN6		;EXIT IF NO COMMA
	MOVX	A,@DPTR		;GET INPUT TERMINATOR
	CJNE	A,#',',IN5	;IF NOT A COMMA DO A CR AND TRY AGAIN
	INC	DPTR		;BUMP PAST COMMA AND READ NEXT VALUE
	CALL	TEMPD
	SJMP	IN3
	;
	newpage
	;
IN5:	MOV	DPTR,#IAN	;PRINT INPUT A NUMBER
	CALL	CRP		;DO A CR, THEN, PRINT FROM ROM
	LJMP	CC1		;TRY IT AGAIN
	;
IN6:	MOVX	A,@DPTR
	CJNE	A,#CR,EIGP
	RET
	;
EIGP:	MOV	DPTR,#EIG
	CALL	CRP		;PRINT THE MESSAGE AND EXIT
	AJMP	SP0		;EXIT WITH A CRLF
	;
	;***************************************************************
	;
SOT:	; On timer interrupt
	;
	;***************************************************************
	;
	ACALL	TWO		;GET THE NUMBERS
	MOV	SP_H,R3
	MOV	SP_L,R1
	MOV	DPTR,#TIV	;SAVE THE NUMBER
	SETB	OTS
	AJMP	R76S		;EXIT
	;
	;
	;***************************************************************
	;
SCALL:	; Call a user rountine
	;
	;***************************************************************
	;
	ACALL	INTERR		;CONVERT INTEGER
	CJNE	R2,#0,S_C_1	;SEE IF TRAP
	MOV	A,R0
	JB	ACC.7,S_C_1
	ADD	A,R0
	MOV	DPTR,#4100H
	MOV	DPL,A
	;
S_C_1:	ACALL	AC1		;JUMP TO USER PROGRAM
	ANL	PSW,#11100111B	;BACK TO BANK 0
	RET			;EXIT
	;
	newpage
	;**************************************************************
	;
THREE:	; Save value for timer function
	;
	;**************************************************************
	;
	ACALL	ONE		;GET THE FIRST INTEGER
	CALL	CBIAS		;BIAS FOR TIMER LOAD
	MOV	T_HH,R3
	MOV	T_LL,R1
	MOV	R7,#','		;WASTE A COMMA
	ACALL	EATC		;FALL THRU TO TWO
	;
	;**************************************************************
	;
TWO:	; Get two values seperated by a comma off the stack
	;
	;**************************************************************
	;
	ACALL	EXPRB
	MOV	R7,#','		;WASTE THE COMMA
	ACALL	WE
	JMP	TWOL		;EXIT
	;
	;*************************************************************
	;
ONE:	; Evaluate an expression and get an integer
	;
	;*************************************************************
	;
	ACALL	EXPRB		;EVALUATE EXPERSSION
	;
IFIXL:	CALL	IFIX		;INTEGERS IN R3:R1
	MOV	A,R1
	RET
	;
	;
	;*************************************************************
	;
I_PI:	; Increment text pointer then get an integer
	;
	;*************************************************************
	;
	ACALL	GCI1		;BUMP TEXT, THEN GET INTEGER
	;
PAREN_INT:; Get an integer in parens ( )
	;
	ACALL	P_E
	SJMP	IFIXL
	;
	newpage
	;
DP_B:	MOV	DPH,BOFAH
	MOV	DPL,BOFAL
	RET
	;
DP_T:	MOV	DPH,TXAH
	MOV	DPL,TXAL
	RET
	;
CPS:	ACALL	GC		;GET THE CHARACTER
	CJNE	A,#'"',NOPASS	;EXIT IF NO STRING
	ACALL	DP_T		;GET TEXT POINTER
	INC	DPTR		;BUMP PAST "
	MOV	R4,#'"'
	CALL	PN0		;DO THE PRINT
	INC	DPTR		;GO PAST QUOTE
	CLR	C		;PASSED TEST
	;
T_DP:	MOV	TXAH,DPH	;TEXT POINTER GETS DPTR
	MOV	TXAL,DPL
	RET
	;
	;*************************************************************
	;
S_C:	; Check for a string
	;
	;*************************************************************
	;
	ACALL	GC		;GET THE CHARACTER
	CJNE	A,#'$',NOPASS	;SET CARRY IF NOT A STRING
	AJMP	IST_CAL		;CLEAR CARRY, CALCULATE OFFSET
	;
	;
	;
	;**************************************************************
	;
C_TST:	ACALL	GC		;GET A CHARACTER
	CJNE	A,#',',NOPASS	;SEE IF A COMMA
	;
	newpage
	;***************************************************************
	;
	;GC AND GCI - GET A CHARACTER FROM TEXT (NO BLANKS)
	;             PUT CHARACTER IN THE ACC
	;
	;***************************************************************
	;
IGC:	ACALL	GCI1		;BUMP POINTER, THEN GET CHARACTER
	;
GC:	SETB	RS0		;USE BANK 1
	MOV	P2,R2		;SET UP PORT 2
	MOVX	A,@R0		;GET EXTERNAL BYTE
	CLR	RS0		;BACK TO BANK 0
	RET			;EXIT
	;
GCI:	ACALL	GC
	;
	; This routine bumps txa by one and always clears the carry
	;
GCI1:	SETB	RS0		;BANK 1
	INC	R0		;BUMP TXA
	CJNE	R0,#0,$+4
	INC	R2
	CLR	RS0
	RET			;EXIT
	;
	newpage
	;**************************************************************
	;
	; Check delimiters
	;
	;**************************************************************
	;
DELTST:	ACALL	GC		;GET A CHARACTER
	CJNE	A,#CR,DT1	;SEE IF A CR
	CLR	A
	RET
	;
DT1:	CJNE	A,#':',NOPASS	;SET CARRY IF NO MATCH
	;
L_RET:	RET
	;
	;
	;***************************************************************
	;
	; FINDC - Find the character in R7, update TXA
	;
	;***************************************************************
	;
FINDCR:	MOV	R7,#CR		;KILL A STATEMENT LINE
	;
FINDC:	ACALL	DELTST
	JNC	L_RET
	;
	CJNE	A,R7B0,FNDCL2	;MATCH?
	RET
	;
FNDCL2:	ACALL	GCI1
	SJMP	FINDC		;LOOP
	;
	ACALL	GCI1
	;
WCR:	ACALL	DELTST		;WASTE UNTIL A "REAL" CR
	JNZ	WCR-2
	RET
	;
	newpage
	;***************************************************************
	;
	; VAR_ER - Check for a variable, exit if error
	;
	;***************************************************************
	;
VAR_ER:	ACALL	VAR
	SJMP	INTERR+2
	;
	;
	;***************************************************************
	;
	; S_D0 - The Statement Action Routine DO
	;
	;***************************************************************
	;
S_DO:	ACALL	CSC		;FINISH UP THE LINE
	MOV	R4,#DTYPE	;TYPE FOR STACK
	ACALL	SGS1		;SAVE ON STACK
	AJMP	ILOOP		;EXIT
	;
	newpage
	;***************************************************************
	;
	; CLN_UP - Clean up the end of a statement, see if at end of
	;          file, eat character and line count after CR
	;
	;***************************************************************
	;
C_2:	CJNE	A,#':',C_1	;SEE IF A TERMINATOR
	AJMP	GCI1		;BUMP POINTER AND EXIT, IF SO
	;
C_1:	CJNE	A,#T_ELSE,EP5
	ACALL	WCR		;WASTE UNTIL A CR
	;
CLN_UP:	ACALL	GC		;GET THE CHARACTER
	CJNE	A,#CR,C_2	;SEE IF A CR
	ACALL	IGC		;GET THE NEXT CHARACTER
	CJNE	A,#EOF,B_TXA	;SEE IF TERMINATOR
	;
NOPASS:	SETB	C
	RET
	;
B_TXA:	XCH	A,TXAL		;BUMP TXA BY THREE
	ADD	A,#3
	XCH	A,TXAL
	JBC	CY,$+4
	RET
	INC	TXAH
	RET
	;
	newpage
	;***************************************************************
	;
	;         Get an INTEGER from the text
	;         sets CARRY if not found
	;         returns the INTGER value in DPTR and R2:R0
	;         returns the terminator in ACC
	;
	;***************************************************************
	;
INTERR:	ACALL	INTGER		;GET THE INTEGER
	JC	EP5		;ERROR IF NOT FOUND
	RET			;EXIT IF FOUND
	;
INTGER:	ACALL	DP_T
	CALL	FP_BASE+18	;CONVERT THE INTEGER
	ACALL	T_DP
	MOV	DPH,R2		;PUT THE RETURNED VALUE IN THE DPTR
	MOV	DPL,R0
	;
ITRET:	RET			;EXIT
	;
	;
WE:	ACALL	EATC		;WASTE THE CHARACTER
	;
	; Fall thru to evaluate the expression
	;
	newpage
	;***************************************************************
	;
	; EXPRB - Evaluate an expression
	;
	;***************************************************************
	;
EXPRB:	MOV	R2,#LO(OPBOL)	;BASE PRECEDENCE
	;
EP1:	PUSH	R2B0		;SAVE OPERATOR PRECEDENCE
	CLR	ARGF		;RESET STACK DESIGNATOR
	;
EP2:	MOV	A,SP		;GET THE STACK POINTER
	ADD	A,#12		;NEED AT LEAST 12 BYTES
	JNC	$+5
	LJMP	ERROR-3
	MOV	A,ASTKA		;GET THE ARG STACK
	SUBB	A,#LO(TM_TOP+12);NEED 12 BYTES ALSO
	JNC	$+5
	LJMP	E4YY
	JB	ARGF,EP4	;MUST BE AN OPERATOR, IF SET
	ACALL	VAR		;IS THE VALUE A VARIABLE?
	JNC	EP3		;PUT VARIABLE ON STACK
	;
	ACALL	CONST		;IS THE VALUE A NUMERIC CONSTANT?
	JNC	EP4		;IF SO, CONTINUE, IF NOT, SEE WHAT
	CALL	GC		;GET THE CHARACTER
	CJNE	A,#T_LPAR,EP4	;SEE IF A LEFT PAREN
	MOV	A,#(LO(OPBOL+1))
	SJMP	XLPAR		;PROCESS THE LEFT PAREN
	;
EP3:	ACALL	PUSHAS		;SAVE VAR ON STACK
	;
EP4:	ACALL	GC		;GET THE OPERATOR
	;
	CJNE	A,#T_LPAR,$+3	;IS IT AN OPERATOR
	JNC	XOP		;PROCESS OPERATOR
	CJNE	A,#T_UOP,$+3	;IS IT A UNARY OPERATOR
	JNC	XBILT		;PROCESS UNARY (BUILT IN) OPERATOR
	POP	R2B0		;GET BACK PREVIOUS OPERATOR PRECEDENCE
	JB	ARGF,ITRET	;OK IF ARG FLAG IS SET
	;
EP5:	CLR	C		;NO RECOVERY
	LJMP	E1XX+2
	;
	; Process the operator
	;
XOP:	ANL	A,#1FH		;STRIP OFF THE TOKE BITS
	JB	ARGF,XOP1	;IF ARG FLAG IS SET, PROCESS
	CJNE	A,#T_SUB-T_LPAR,XOP3
	MOV	A,#T_NEG-T_LPAR
	;
	newpage
XOP1:	ADD	A,#LO(OPBOL+1)	;BIAS THE TABLE
	MOV	R2,A
	MOV	DPTR,#00H
	MOVC	A,@A+DPTR	;GET THE CURRENT PRECEDENCE
	MOV	R4,A
	POP	ACC		;GET THE PREVIOUS PRECEDENCE
	MOV	R5,A		;SAVE THE PREVIOUS PRECEDENCE
	MOVC	A,@A+DPTR	;GET IT
	CJNE	A,R4B0,$+7	;SEE WHICH HAS HIGHER PRECEDENCE
	CJNE	A,#12,ITRET	;SEE IF ANEG
	SETB	C
	JNC	ITRET		;PROCESS NON-INCREASING PRECEDENCE
	;
	; Save increasing precedence
	;
	PUSH	R5B0		;SAVE OLD PRECEDENCE ADDRESS
	PUSH	R2B0		;SAVE NEW PRECEDENCE ADDRESS
	ACALL	GCI1		;EAT THE OPERATOR
	ACALL	EP1		;EVALUATE REMAINING EXPRESSION
	POP	ACC
	;
	; R2 has the action address, now setup and perform operation
	;
XOP2:	MOV	DPTR,#OPTAB
	ADD	A,#LO(~OPBOL)
	CALL	ISTA1		;SET UP TO RETURN TO EP2
	AJMP	EP2		;JUMP TO EVALUATE EXPRESSION
	;
	; Built-in operator processing
	;
XBILT:	ACALL	GCI1		;EAT THE TOKEN
	ADD	A,#LO(50H+LO(UOPBOL))
	JB	ARGF,EP5	;XBILT MUST COME AFTER AN OPERATOR
	CJNE	A,#STP,$+3
	JNC	XOP2
	;
XLPAR:	PUSH	ACC		;PUT ADDRESS ON THE STACK
	ACALL	P_E
	SJMP	XOP2-2		;PERFORM OPERATION
	;
XOP3:	CJNE	A,#T_ADD-T_LPAR,EP5
	ACALL	GCI1
	AJMP	EP2		;WASTE + SIGN
	;
	newpage
XPOP:	ACALL	X3120		;FLIP ARGS THEN POP
	;
	;***************************************************************
	;
	; POPAS - Pop arg stack and copy variable to R3:R1
	;
	;***************************************************************
	;
POPAS:	LCALL	INC_ASTKA
	JMP	VARCOP		;COPY THE VARIABLE
	;
AXTAL:	MOV	R2,#HI(CXTAL)
	MOV	R0,#LO(CXTAL)
	;
	; fall thru
	;
	;***************************************************************
	;
PUSHAS:	; Push the Value addressed by R2:R0 onto the arg stack
	;
	;***************************************************************
	;
	CALL	DEC_ASTKA
	SETB	ARGF		;SAYS THAT SOMTHING IS ON THE STACK
	LJMP	VARCOP
	;
	;
	;***************************************************************
	;
ST_A:	; Store at expression
	;
	;***************************************************************
	;
	ACALL	ONE		;GET THE EXPRESSION
	SJMP	POPAS		;SAVE IT
	;
	;
	;***************************************************************
	;
LD_A:	; Load at expression
	;
	;***************************************************************
	;
	ACALL	ONE		;GET THE EXPRESSION
	ACALL	X3120		;FLIP ARGS
	SJMP	PUSHAS
	;
	newpage
	;***************************************************************
	;
CONST:	; Get a constant fron the text
	;
	;***************************************************************
	;
	CALL	GC		;FIRST SEE IF LITERAL
	CJNE	A,#T_ASC,C0C	;SEE IF ASCII TOKEN
	CALL	IGC		;GET THE CHARACTER AFTER TOKEN
	CJNE	A,#'$',CN0	;SEE IF A STRING
	;
CNX:	CALL	CSY		;CALCULATE IT
	LJMP	AXBYTE+2	;SAVE IT ON THE STACK ******AA JMP-->LJMP
	;
CN0:	LCALL	TWO_R2		;PUT IT ON THE STACK ******AA CALL-->LCALL
	CALL	GCI1		;BUMP THE POINTER
	LJMP	ERPAR		;WASTE THE RIGHT PAREN ******AA JMP-->LJMP
	;
	;
C0C:	CALL	DP_T		;GET THE TEXT POINTER
	CALL	GET_NUM		;GET THE NUMBER
	CJNE	A,#0FFH,C1C	;SEE IF NO NUMBER
	SETB	C
C2C:	RET
	;
C1C:	JNZ	FPTST
	CLR	C
	SETB	ARGF
	;
C3C:	JMP	T_DP
	;
FPTST:	ANL	A,#00001011B	;CHECK FOR ERROR
	JZ	C2C		;EXIT IF ZERO
	;
	; Handle the error condition
	;
	MOV	DPTR,#E2X	;DIVIDE BY ZERO
	JNB	ACC.0,$+6	;UNDERFLOW
	MOV	DPTR,#E7X
	JNB	ACC.1,$+6	;OVERFLOW
	MOV	DPTR,#E11X
	;
FPTS:	JMP	ERROR
	;
	newpage
	;***************************************************************
	;
	; The Command action routine - LIST
	;
	;***************************************************************
	;
CLIST:	CALL	NUMC		;SEE IF TO LINE PORT
	ACALL	FSTK		;PUT 0FFFFH ON THE STACK
	CALL	INTGER		;SEE IF USER SUPPLIES LN
	CLR	A		;LN = 0 TO START
	MOV	R3,A
	MOV	R1,A
	JC	CL1		;START FROM ZERO
	;
	CALL	TEMPD		;SAVE THE START ADDTESS
	CALL	GCI		;GET THE CHARACTER AFTER LIST
	CJNE	A,#T_SUB,$+10	;CHECK FOR TERMINATION ADDRESS '-'
	ACALL	INC_ASTKA	;WASTE 0FFFFH
	LCALL	INTERR		;GET TERMINATION ADDRESS
	ACALL	TWO_EY		;PUT TERMINATION ON THE ARG STACK
	MOV	R3,TEMP5	;GET THE START ADDTESS
	MOV	R1,TEMP4
	;
CL1:	CALL	GETLIN		;GET THE LINE NO IN R3:R1
	JZ	CL3		;RET IF AT END
	;
CL2:	ACALL	C3C		;SAVE THE ADDRESS
	INC	DPTR		;POINT TO LINE NUMBER
	ACALL	PMTOP+3		;PUT LINE NUMBER ON THE STACK
	ACALL	CMPLK		;COMPARE LN TO END ADDRESS
	JC	CL3		;EXIT IF GREATER
	CALL	BCK		;CHECK FOR A CONTROL C
	ACALL	DEC_ASTKA	;SAVE THE COMPARE ADDRESS
	CALL	DP_T		;RESTORE ADDRESS
	ACALL	UPPL		;UN-PROCESS THE LINE
	ACALL	C3C		;SAVE THE CR ADDRESS
	ACALL	CL6		;PRINT IT
	INC	DPTR		;BUMP POINTER TO NEXT LINE
	MOVX	A,@DPTR		;GET LIN LENGTH
	DJNZ	ACC,CL2		;LOOP
	ACALL	INC_ASTKA	;WASTE THE COMPARE BYTE
	;
CL3:	AJMP	CMND1		;BACK TO COMMAND PROCESSOR
	;
CL6:	MOV	DPTR,#IBUF	;PRINT IBUF
	CALL	PRNTCR		;PRINT IT
	CALL	DP_T
	;
CL7:	JMP	CRLF
	;
	LCALL	X31DP
	newpage
	;***************************************************************
	;
	;UPPL - UN PREPROCESS A LINE ADDRESSED BY DPTR INTO IBUF
	;       RETURN SOURCE ADDRESS OF CR IN DPTR ON RETURN
	;
	;***************************************************************
	;
UPPL:	MOV	R3,#HI(IBUF)	;POINT R3 AT HIGH IBUF
	MOV	R1,#LO(IBUF)	;POINT R1 AT IBUF
	INC	DPTR		;SKIP OVER LINE LENGTH
	ACALL	C3C		;SAVE THE DPTR (DP_T)
	CALL	L20DPI		;PUT LINE NUMBER IN R2:R0
	CALL	FP_BASE+16	;CONVERT R2:R0 TO INTEGER
	CALL	DP_T
	INC	DPTR		;BUMP DPTR PAST THE LINE NUMBER
	;
UPP0:	CJNE	R1,#LO(IBUF+6),$+3
	JC	UPP1A-4		;PUT SPACES IN TEXT
	INC	DPTR		;BUMP PAST LN HIGH
	MOVX	A,@DPTR		;GET USER TEXT
	MOV	R6,A		;SAVE A IN R6 FOR TOKE COMPARE
	JB	ACC.7,UPP1	;IF TOKEN, PROCESS
	CJNE	A,#20H,$+3	;TRAP THE USER TOKENS
	JNC	$+5
	CJNE	A,#CR,UPP1	;DO IT IF NOT A CR
	CJNE	A,#'"',UPP9	;SEE IF STRING
	ACALL	UPP7		;SAVE IT
	ACALL	UPP8		;GET THE NEXT CHARACTER AND SAVE IT
	CJNE	A,#'"',$-2	;LOOP ON QUOTES
	SJMP	UPP0
	;
UPP9:	CJNE	A,#':',UPP1A	;PUT A SPACE IN DELIMITER
	ACALL	UPP7A
	MOV	A,R6
	ACALL	UPP7
	ACALL	UPP7A
	SJMP	UPP0
	;
UPP1A:	ACALL	UPP8+2		;SAVE THE CHARACTER, UPDATE POINTER
	SJMP	UPP0		;EXIT IF A CR, ELSE LOOP
	;
UPP1:	ACALL	C3C		;SAVE THE TEXT POINTER
	MOV	C,XBIT
	MOV	F0,C		;SAVE XBIT IN F0
	MOV	DPTR,#TOKTAB	;POINT AT TOKEN TABLE
	JNB	F0,UPP2
	LCALL	2078H		;SET UP DPTR FOR LOOKUP
	;
UPP2:	CLR	A		;ZERO A FOR LOOKUP
	MOVC	A,@A+DPTR	;GET TOKEN
	INC	DPTR		;ADVANCE THE TOKEN POINTER
	CJNE	A,#0FFH,UP_2	;SEE IF DONE
	JBC	F0,UPP2-9	;NOW DO NORMAL TABLE
	AJMP	CMND1		;EXIT IF NOT FOUND
	;
UP_2:	CJNE	A,R6B0,UPP2	;LOOP UNTIL THE SAME
	;
UP_3:	CJNE	A,#T_UOP,$+3
	JNC	UPP3
	ACALL	UPP7A		;PRINT THE SPACE IF OK
	;
UPP3:	CLR	A		;DO LOOKUP
	MOVC	A,@A+DPTR
	JB	ACC.7,UPP4	;EXIT IF DONE, ELSE SAVE
	JZ	UPP4		;DONE IF ZERO
	ACALL	UPP7		;SAVE THE CHARACTER
	INC	DPTR
	SJMP	UPP3		;LOOP
	;
UPP4:	CALL	DP_T		;GET IT BACK
	MOV	A,R6		;SEE IF A REM TOKEN
	XRL	A,#T_REM
	JNZ	$+6
	ACALL	UPP8
	SJMP	$-2
	JNC	UPP0		;START OVER AGAIN IF NO TOKEN
	ACALL	UPP7A		;PRINT THE SPACE IF OK
	SJMP	UPP0		;DONE
	;
UPP7A:	MOV	A,#' '		;OUTPUT A SPACE
	;
UPP7:	AJMP	PPL9+1		;SAVE A
	;
UPP8:	INC	DPTR
	MOVX	A,@DPTR
	CJNE	A,#CR,UPP7
	AJMP	PPL7+1
	;
	newpage
	;**************************************************************
	;
	; This table contains all of the floating point constants
	;
	; The constants in ROM are stored "backwards" from the way
	; basic normally treats floating point numbers. Instead of
	; loading from the exponent and decrementing the pointer,
	; ROM constants pointers load from the most significant
	; digits and increment the pointers. This is done to 1) make
	; arg stack loading faster and 2) compensate for the fact that
	; no decrement data pointer instruction exsist.
	;
	; The numbers are stored as follows:
	;
	; BYTE X+5    = MOST SIGNIFICANT DIGITS IN BCD
	; BYTE X+4    = NEXT MOST SIGNIFICANT DIGITS IN BCD
	; BYTE X+3    = NEXT LEAST SIGNIFICANT DIGITS IN BCD
	; BYTE X+2    = LEAST SIGNIFICANT DIGITS IN BCD
	; BYTE X+1    = SIGN OF THE ABOVE MANTISSA 0 = +, 1 = -
	; BYTE X      = EXPONENT IN TWO'S COMPLEMENT BINARY
	;               ZERO EXPONENT = THE NUMBER ZERO
	;
	;**************************************************************
	;
ATTAB:	DB	128-2		; ARCTAN LOOKUP
	DB	00H
	DB	57H
	DB	22H
	DB	66H
	DB	28H
	;
	DB	128-1
	DB	01H
	DB	37H
	DB	57H
	DB	16H
	DB	16H
	;
	DB	128-1
	DB	00H
	DB	14H
	DB	96H
	DB	90H
	DB	42H
	;
	DB	128-1
	DB	01H
	DB	40H
	DB	96H
	DB	28H
	DB	75H
	;
	DB	128
	DB	00H
	DB	64H
	DB	62H
	DB	65H
	DB	10H
	;
	DB	128
	DB	01H
	DB	99H
	DB	88H
	DB	20H
	DB	14H
	;
	DB	128
	DB	00H
	DB	51H
	DB	35H
	DB	99H
	DB	19H
	;
	DB	128
	DB	01H
	DB	45H
	DB	31H
	DB	33H
	DB	33H
	;
	DB	129
	DB	00H
	DB	00H
	DB	00H
	DB	00H
	DB	10H
	;
	DB	0FFH		;END OF TABLE
	;
NTWO:	DB	129
	DB	0
	DB	0
	DB	0
	DB	0
	DB	20H
	;
TTIME:	DB	128-4		; CLOCK CALCULATION
	DB	00H
	DB	00H
	DB	00H
	DB	04H
	DB	13H
	;
	newpage
	;***************************************************************
	;
	; COSINE - Add pi/2 to stack, then fall thru to SIN
	;
	;***************************************************************
	;
ACOS:	ACALL	POTWO		;PUT PI/2 ON THE STACK
	ACALL	AADD		;TOS = TOS+PI/2
	;
	;***************************************************************
	;
	; SINE - use taylor series to calculate sin function
	;
	;***************************************************************
	;
ASIN:	ACALL	PIPI		;PUT PI ON THE STACK
	ACALL	RV		;REDUCE THE VALUE
	MOV	A,MT2		;CALCULATE THE SIGN
	ANL	A,#01H		;SAVE LSB
	XRL	MT1,A		;SAVE SIGN IN MT1
	ACALL	CSTAKA		;NOW CONVERT TO ONE QUADRANT
	ACALL	POTWO
	ACALL	CMPLK		;DO COMPARE
	JC	$+6
	ACALL	PIPI
	ACALL	ASUB
	ACALL	AABS
	MOV	DPTR,#SINTAB	;SET UP LOOKUP TABLE
	ACALL	POLYC		;CALCULATE THE POLY
	ACALL	STRIP
	AJMP	SIN0
	;
	; Put PI/2 on the stack
	;
POTWO:	ACALL	PIPI		;PUT PI ON THE STACK, NOW DIVIDE
	;
DBTWO:	MOV	DPTR,#NTWO
	ACALL	PUSHC
	;MOV	A,#2		;BY TWO
	;ACALL	TWO_R2
	AJMP	ADIV
	;
	newpage
	;*************************************************************
	;
POLYC:	; Expand a power series to calculate a polynomial
	;
	;*************************************************************
	;
	ACALL	CSTAKA2		;COPY THE STACK
	ACALL	AMUL		;SQUARE THE STACK
	ACALL	POP_T1		;SAVE X*X
	ACALL	PUSHC		;PUT CONSTANT ON STACK
	;
POLY1:	ACALL	PUSH_T1		;PUT COMPUTED VALUE ON STACK
	ACALL	AMUL		;MULTIPLY CONSTANT AND COMPUTED VALUE
	ACALL	PUSHC		;PUT NEXT CONSTANT ON STACK
	ACALL	AADD		;ADD IT TO THE OLD VALUE
	CLR	A		;CHECK TO SEE IF DONE
	MOVC	A,@A+DPTR
	CJNE	A,#0FFH,POLY1	;LOOP UNTIL DONE
	;
AMUL:	LCALL	FP_BASE+6
	AJMP	FPTST
	;
	;*************************************************************
	;
RV:	; Reduce a value for Trig and A**X functions
	;
	; value = (value/x - INT(value/x)) * x
	;
	;*************************************************************
	;
	ACALL	C_T2		;COPY TOS TO T2
	ACALL	ADIV		;TOS = TOS/TEMP2
	ACALL	AABS		;MAKE THE TOS A POSITIVE NUMBER
	MOV	MT1,A		;SAVE THE SIGN
	ACALL	CSTAKA2		;COPY THE STACK TWICE
	ACALL	IFIX		;PUT THE NUMBER IN R3:R1
	PUSH	R3B0		;SAVE R3
	MOV	MT2,R1		;SAVE THE LS BYTE IN MT2
	ACALL	AINT		;MAKE THE TOS AN INTEGER
	ACALL	ASUB		;TOS = TOS/T2 - INT(TOS/T2)
	ACALL	P_T2		;TOS = T2
	ACALL	AMUL		;TOS = T2*(TOS/T2 - INT(TOS/T2)
	POP	R3B0		;RESTORE R3
	RET			;EXIT
	;
	newpage
	;**************************************************************
	;
	; TAN
	;
	;**************************************************************
	;
ATAN:	ACALL	CSTAKA		;DUPLACATE STACK
	ACALL	ASIN		;TOS = SIN(X)
	ACALL	SWAP_ASTKA	;TOS = X
	ACALL	ACOS		;TOS = COS(X)
	AJMP	ADIV		;TOS = SIN(X)/COS(X)
	;
STRIP:	ACALL	SETREG		;SETUP R0
	MOV	R3,#1		;LOOP COUNT
	AJMP	AI2-1		;WASTE THE LSB
	;
	;************************************************************
	;
	; ARC TAN
	;
	;************************************************************
	;
AATAN:	ACALL	AABS
	MOV	MT1,A		;SAVE THE SIGN
	ACALL	SETREG		;GET THE EXPONENT
	ADD	A,#7FH		;BIAS THE EXPONENT
	MOV	UBIT,C		;SAVE CARRY STATUS
	JNC	$+4		;SEE IF > 1
	ACALL	RECIP		;IF > 1, TAKE RECIP
	MOV	DPTR,#ATTAB	;SET UP TO CALCULATE THE POLY
	ACALL	POLYC		;CALCULATE THE POLY
	JNB	UBIT,SIN0	;JUMP IF NOT SET
	ACALL	ANEG		;MAKE X POLY NEGATIVE
	ACALL	POTWO		;SUBTRACT PI/2
	ACALL	AADD
	;
SIN0:	MOV	A,MT1		;GET THE SIGN
	JZ	SRT
	AJMP	ANEG
	;
	newpage
	;*************************************************************
	;
	; FCOMP - COMPARE 0FFFFH TO TOS
	;
	;*************************************************************
	;
FCMP:	ACALL	CSTAKA		;COPY THE STACK
	ACALL	FSTK		;MAKE THE TOS = 0FFFFH
	ACALL	SWAP_ASTKA	;NOW COMPARE IS 0FFFFH - X
	;
CMPLK:	JMP	FP_BASE+4	;DO THE COMPARE
	;
	;*************************************************************
	;
DEC_ASTKA:	;Push ARG STACK and check for underflow
	;
	;*************************************************************
	;
	MOV	A,#-FPSIZ
	ADD	A,ASTKA
	CJNE	A,#LO(TM_TOP+6),$+3
	JC	E4YY
	MOV	ASTKA,A
	MOV	R1,A
	MOV	R3,#ASTKAH
	;
SRT:	RET
	;
E4YY:	MOV	DPTR,#EXA
	AJMP	FPTS		;ARG STACK ERROR
	;
	;
AXTAL3:	ACALL	PUSHC		;PUSH CONSTANT, THEN MULTIPLY
	ACALL	AMUL
	;
	; Fall thru to IFIX
	;
	newpage
	;***************************************************************
	;
IFIX:	; Convert a floating point number to an integer, put in R3:R1
	;
	;***************************************************************
	;
	CLR	A		;RESET THE START
	MOV	R3,A
	MOV	R1,A
	MOV	R0,ASTKA	;GET THE ARG STACK
	MOV	P2,#ASTKAH
	MOVX	A,@R0		;READ EXPONENT
	CLR	C
	SUBB	A,#81H		;BASE EXPONENT
	MOV	R4,A		;SAVE IT
	DEC	R0		;POINT AT SIGN
	MOVX	A,@R0		;GET THE SIGN
	JNZ	SQ_ERR		;ERROR IF NEGATIVE
	JC	INC_ASTKA	;EXIT IF EXPONENT IS < 81H
	INC	R4		;ADJUST LOOP COUNTER
	MOV	A,R0		;BUMP THE POINTER REGISTER
	SUBB	A,#FPSIZ-1
	MOV	R0,A
	;
I2:	INC	R0		;POINT AT DIGIT
	MOVX	A,@R0		;GET DIGIT
	SWAP	A		;FLIP
	CALL	FP_BASE+20	;ACCUMULATE
	JC	SQ_ERR
	DJNZ	R4,$+4
	SJMP	INC_ASTKA
	MOVX	A,@R0		;GET DIGIT
	CALL	FP_BASE+20
	JC	SQ_ERR
	DJNZ	R4,I2
	;
	newpage
	;************************************************************
	;
INC_ASTKA:	; Pop the ARG STACK and check for overflow
	;
	;************************************************************
	;
	MOV	A,#FPSIZ	;NUMBER TO POP
	SJMP	SETREG+1
	;
SETREG:	CLR	A		;DON'T POP ANYTHING
	MOV	R0,ASTKA
	MOV	R2,#ASTKAH
	MOV	P2,R2
	ADD	A,R0
	JC	E4YY
	MOV	ASTKA,A
	MOVX	A,@R0
A_D:	RET
	;
	;************************************************************
	;
	; EBIAS - Bias a number for E to the X calculations
	;
	;************************************************************
	;
EBIAS:	ACALL	PUSH_ONE
	ACALL	RV
	CJNE	R3,#00H,SQ_ERR	;ERROR IF R3 <> 0
	ACALL	C_T2		;TEMP 2 GETS FRACTIONS
	ACALL	INC_ASTKA
	ACALL	POP_T1
	ACALL	PUSH_ONE
	;
AELP:	MOV	A,MT2
	JNZ	AEL1
	;
	MOV	A,MT1
	JZ	A_D
	MOV	DPTR,#FPT2-1
	MOVX	@DPTR,A		;MAKE THE FRACTIONS NEGATIVE
	;
RECIP:	ACALL	PUSH_ONE
	ACALL	SWAP_ASTKA
	AJMP	ADIV
	;
AEL1:	DEC	MT2
	ACALL	PUSH_T1
	ACALL	AMUL
	SJMP	AELP
	;
SQ_ERR:	LJMP	E3XX		;LINK TO BAD ARG
	;
	newpage
	;************************************************************
	;
	; SQUARE ROOT
	;
	;************************************************************
	;
ASQR:	ACALL	AABS		;GET THE SIGN
	JNZ	SQ_ERR		;ERROR IF NEGATIVE
	ACALL	C_T2		;COPY VARIABLE TO T2
	ACALL	POP_T1		;SAVE IT IN T1
	MOV	R0,#LO(FPT1)
	MOVX	A,@R0		;GET EXPONENT
	JZ	ALN-2		;EXIT IF ZERO
	ADD	A,#128		;BIAS THE EXPONENT
	JNC	SQR1		;SEE IF < 80H
	RR	A
	ANL	A,#127
	SJMP	SQR2
	;
SQR1:	CPL	A		;FLIP BITS
	INC	A
	RR	A
	ANL	A,#127		;STRIP MSB
	CPL	A
	INC	A
	;
SQR2:	ADD	A,#128		;BIAS EXPONENT
	MOVX	@R0,A		;SAVE IT
	;
	; NEWGUESS = ( X/OLDGUESS + OLDGUESS) / 2
	;
SQR4:	ACALL	P_T2		;TOS = X
	ACALL	PUSH_T1		;PUT NUMBER ON STACK
	ACALL	ADIV		;TOS = X/GUESS
	ACALL	PUSH_T1		;PUT ON AGAIN
	ACALL	AADD		;TOS = X/GUESS + GUESS
	ACALL	DBTWO		;TOS = ( X/GUESS + GUESS ) / 2
	ACALL	TEMP_COMP	;SEE IF DONE
	JNB	F0,SQR4
	;
	AJMP	PUSH_T1		;PUT THE ANSWER ON THE STACK
	;
	newpage
	;*************************************************************
	;
	; NATURAL LOG
	;
	;*************************************************************
	;
ALN:	ACALL	AABS		;MAKE SURE THAT NUM IS POSITIVE
	JNZ	SQ_ERR		;ERROR IF NOT
	MOV	MT2,A		;CLEAR FOR LOOP
	INC	R0		;POINT AT EXPONENT
	MOVX	A,@R0		;READ THE EXPONENT
	JZ	SQ_ERR		;ERROR IF EXPONENT IS ZERO
	CJNE	A,#81H,$+3	;SEE IF NUM >= 1
	MOV	UBIT,C		;SAVE CARRY STATUS
	JC	$+4		;TAKE RECIP IF >= 1
	ACALL	RECIP
	;
	; Loop to reduce
	;
ALNL:	ACALL	CSTAKA		;COPY THE STACK FOR COMPARE
	ACALL	PUSH_ONE	;COMPARE NUM TO ONE
	ACALL	CMPLK
	JNC	ALNO		;EXIT IF DONE
	ACALL	SETREG		;GET THE EXPONENT
	ADD	A,#85H		;SEE HOW BIG IT IS
	JNC	ALN11		;BUMP BY EXP(11) IF TOO SMALL
	ACALL	PLNEXP		;PUT EXP(1) ON STACK
	MOV	A,#1		;BUMP COUNT
	;
ALNE:	ADD	A,MT2
	JC	SQ_ERR
	MOV	MT2,A
	ACALL	AMUL		;BIAS THE NUMBER
	SJMP	ALNL
	;
ALN11:	MOV	DPTR,#EXP11	;PUT EXP(11) ON STACK
	ACALL	PUSHC
	MOV	A,#11
	SJMP	ALNE
	;
	newpage
ALNO:	ACALL	C_T2		;PUT NUM IN TEMP 2
	ACALL	PUSH_ONE	;TOS = 1
	ACALL	ASUB		;TOS = X - 1
	ACALL	P_T2		;TOS = X
	ACALL	PUSH_ONE	;TOS = 1
	ACALL	AADD		;TOS = X + 1
	ACALL	ADIV		;TOS = (X-1)/(X+1)
	MOV	DPTR,#LNTAB	;LOG TABLE
	ACALL	POLYC
	INC	DPTR		;POINT AT LN(10)
	ACALL	PUSHC
	ACALL	AMUL
	MOV	A,MT2		;GET THE COUNT
	ACALL	TWO_R2		;PUT IT ON THE STACK
	ACALL	ASUB		;INT - POLY
	ACALL	STRIP
	JNB	UBIT,AABS
	;
LN_D:	RET
	;
	;*************************************************************
	;
TEMP_COMP:	; Compare FPTEMP1 to TOS, FPTEMP1 gets TOS
	;
	;*************************************************************
	;
	ACALL	PUSH_T1		;SAVE THE TEMP
	ACALL	SWAP_ASTKA	;TRADE WITH THE NEXT NUMBER
	ACALL	CSTAKA		;COPY THE STACK
	ACALL	POP_T1		;SAVE THE NEW NUMBER
	JMP	FP_BASE+4	;DO THE COMPARE
	;
	newpage
AETOX:	ACALL	PLNEXP		;EXP(1) ON TOS
	ACALL	SWAP_ASTKA	;X ON TOS
	;
AEXP:	;EXPONENTIATION
	;
	ACALL	EBIAS		;T1=BASE,T2=FRACTIONS,TOS=INT MULTIPLIED
	MOV	DPTR,#FPT2	;POINT AT FRACTIONS
	MOVX	A,@DPTR		;READ THE EXP OF THE FRACTIONS
	JZ	LN_D		;EXIT IF ZERO
	ACALL	P_T2		;TOS = FRACTIONS
	ACALL	PUSH_T1		;TOS = BASE
	ACALL	SETREG		;SEE IF BASE IS ZERO
	JZ	$+4
	ACALL	ALN		;TOS = LN(BASE)
	ACALL	AMUL		;TOS = FRACTIONS * LN(BASE)
	ACALL	PLNEXP		;TOS = EXP(1)
	ACALL	SWAP_ASTKA	;TOS = FRACTIONS * LN(BASE)
	ACALL	EBIAS		;T2 = FRACTIONS, TOS = INT MULTIPLIED
	MOV	MT2,#00H	;NOW CALCULATE E**X
	ACALL	PUSH_ONE
	ACALL	CSTAKA
	ACALL	POP_T1		;T1 = 1
	;
AEXL:	ACALL	P_T2		;TOS = FRACTIONS
	ACALL	AMUL		;TOS = FRACTIONS * ACCUMLATION
	INC	MT2		;DO THE DEMONIATOR
	MOV	A,MT2
	ACALL	TWO_R2
	ACALL	ADIV
	ACALL	CSTAKA		;SAVE THE ITERATION
	ACALL	PUSH_T1		;NOW ACCUMLATE
	ACALL	AADD		;ADD ACCUMLATION
	ACALL	TEMP_COMP
	JNB	F0,AEXL		;LOOP UNTIL DONE
	;
	ACALL	INC_ASTKA
	ACALL	PUSH_T1
	ACALL	AMUL		;LAST INT MULTIPLIED
	;
MU1:	AJMP	AMUL		;FIRST INT MULTIPLIED
	;
	newpage
	;***************************************************************
	;
	; integer operator - INT
	;
	;***************************************************************
	;
AINT:	ACALL	SETREG		;SET UP THE REGISTERS, CLEAR CARRY
	SUBB	A,#129		;SUBTRACT EXPONENT BIAS
	JNC	AI1		;JUMP IF ACC > 81H
	;
	; Force the number to be a zero
	;
	ACALL	INC_ASTKA	;BUMP THE STACK
	;
P_Z:	MOV	DPTR,#ZRO	;PUT ZERO ON THE STACK
	AJMP	PUSHC
	;
AI1:	SUBB	A,#7
	JNC	AI3
	CPL	A
	INC	A
	MOV	R3,A
	DEC	R0		;POINT AT SIGN
	;
AI2:	DEC	R0		;NOW AT LSB'S
	MOVX	A,@R0		;READ BYTE
	ANL	A,#0F0H		;STRIP NIBBLE
	MOVX	@R0,A		;WRITE BYTE
	DJNZ	R3,$+3
	RET
	CLR	A
	MOVX	@R0,A		;CLEAR THE LOCATION
	DJNZ	R3,AI2
	;
AI3:	RET			;EXIT
	;
	newpage
	;***************************************************************
	;
AABS:	; Absolute value - Make sign of number positive
	;                  return sign in ACC
	;
	;***************************************************************
	;
	ACALL	ANEG		;CHECK TO SEE IF + OR -
	JNZ	ALPAR		;EXIT IF NON ZERO, BECAUSE THE NUM IS
	MOVX	@R0,A		;MAKE A POSITIVE SIGN
	RET
	;
	;***************************************************************
	;
ASGN:	; Returns the sign of the number 1 = +, -1 = -
	;
	;***************************************************************
	;
	ACALL	INC_ASTKA	;POP STACK, GET EXPONENT
	JZ	P_Z		;EXIT IF ZERO
	DEC	R0		;BUMP TO SIGN
	MOVX	A,@R0		;GET THE SIGN
	MOV	R7,A		;SAVE THE SIGN
	ACALL	PUSH_ONE	;PUT A ONE ON THE STACK
	MOV	A,R7		;GET THE SIGN
	JZ	ALPAR		;EXIT IF ZERO
	;
	; Fall thru to ANEG
	;
	;***************************************************************
	;
ANEG:	; Flip the sign of the number on the tos
	;
	;***************************************************************
	;
	ACALL	SETREG
	DEC	R0		;POINT AT THE SIGN OF THE NUMBER
	JZ	ALPAR		;EXIT IF ZERO
	MOVX	A,@R0
	XRL	A,#01H		;FLIP THE SIGN
	MOVX	@R0,A
	XRL	A,#01H		;RESTORE THE SIGN
	;
ALPAR:	RET
	;
	newpage
	;***************************************************************
	;
ACBYTE:	; Read the ROM
	;
	;***************************************************************
	;
	ACALL	IFIX		;GET EXPRESSION
	CALL	X31DP		;PUT R3:R1 INTO THE DP
	CLR	A
	MOVC	A,@A+DPTR
	AJMP	TWO_R2
	;
	;***************************************************************
	;
ADBYTE:	; Read internal memory
	;
	;***************************************************************
	;
	ACALL	IFIX		;GET THE EXPRESSION
	CALL	R3CK		;MAKE SURE R3 = 0
	MOV	A,@R1
	AJMP	TWO_R2
	;
	;***************************************************************
	;
AXBYTE: ; Read external memory
	;
	;***************************************************************
	;
	ACALL	IFIX		;GET THE EXPRESSION
	MOV	P2,R3
	MOVX	A,@R1
	AJMP	TWO_R2
	;
	newpage
	;***************************************************************
	;
	; The relational operators - EQUAL                        (=)
	;                            GREATER THAN                 (>)
	;                            LESS THAN                    (<)
	;                            GREATER THAN OR EQUAL        (>=)
	;                            LESS THAN OR EQUAL           (<=)
	;                            NOT EQUAL                    (<>)
	;
	;***************************************************************
	;
AGT:	ACALL	CMPLK
	ORL	C,F0		;SEE IF EITHER IS A ONE
	JC	P_Z
	;
FSTK:	MOV	DPTR,#FS
	AJMP	PUSHC
	;
FS:	DB	85H
	DB	00H
	DB	00H
	DB	50H
	DB	53H
	DB	65H
	;
ALT:	ACALL	CMPLK
	CPL	C
	SJMP	AGT+4
	;
AEQ:	ACALL	CMPLK
	MOV	C,F0
	SJMP	ALT+2
	;
ANE:	ACALL	CMPLK
	CPL	F0
	SJMP	AEQ+2
	;
AGE:	ACALL	CMPLK
	SJMP	AGT+4
	;
ALE:	ACALL	CMPLK
	ORL	C,F0
	SJMP	ALT+2
	;
	newpage
	;***************************************************************
	;
ARND:	; Generate a random number
	;
	;***************************************************************
	;
	MOV	DPTR,#RCELL	;GET THE BINARY SEED
	CALL	L31DPI
	MOV	A,R1
	CLR	C
	RRC	A
	MOV	R0,A
	MOV	A,#6
	RRC	A
	ADD	A,R1
	XCH	A,R0
	ADDC	A,R3
	MOV	R2,A
	DEC	DPL		;SAVE THE NEW SEED
	ACALL	S20DP
	ACALL	TWO_EY
	ACALL	FSTK
	;
ADIV:	LCALL	FP_BASE+8
	AJMP	FPTST
	;
	newpage
	;***************************************************************
	;
SONERR:	; ON ERROR Statement
	;
	;***************************************************************
	;
	LCALL	INTERR		;GET THE LINE NUMBER
	SETB	ON_ERR
	MOV	DPTR,#ERRNUM	;POINT AT THR ERROR LOCATION
	SJMP	S20DP
	;
	;
	;**************************************************************
	;
SONEXT:	; ON EXT1 Statement
	;
	;**************************************************************
	;
	LCALL	INTERR
	SETB	INTBIT
	ORL	IE,#10000100B	;ENABLE INTERRUPTS
	MOV	DPTR,#INTLOC
	;
S20DP:	MOV	A,R2		;SAVE R2:R0 @DPTR
	MOVX	@DPTR,A
	INC	DPTR
	MOV	A,R0
	MOVX	@DPTR,A
	RET
	;
	newpage
	;***************************************************************
	;
	; CASTAK - Copy and push another top of arg stack
	;
	;***************************************************************
	;
CSTAKA2:ACALL	CSTAKA		;COPY STACK TWICE
	;
CSTAKA:	ACALL	SETREG		;SET UP R2:R0
	SJMP	PUSH_T1+4
	;
PLNEXP:	MOV	DPTR,#EXP1
	;
	;***************************************************************
	;
	; PUSHC - Push constant on to the arg stack
	;
	;***************************************************************
	;
PUSHC:	ACALL	DEC_ASTKA
	MOV	P2,R3
	MOV	R3,#FPSIZ	;LOOP COUNTER
	;
PCL:	CLR	A		;SET UP A
	MOVC	A,@A+DPTR	;LOAD IT
	MOVX	@R1,A		;SAVE IT
	INC	DPTR		;BUMP POINTERS
	DEC	R1
	DJNZ	R3,PCL		;LOOP
	;
	SETB	ARGF
	RET			;EXIT
	;
PUSH_ONE:;
	;
	MOV	DPTR,#FPONE
	AJMP	PUSHC
	;
	newpage
	;
POP_T1:
	;
	MOV	R3,#HI(FPT1)
	MOV	R1,#LO(FPT1)
	JMP	POPAS
	;
PUSH_T1:
	;
	MOV	R0,#LO(FPT1)
	MOV	R2,#HI(FPT1)
	LJMP	PUSHAS
	;
P_T2:	MOV	R0,#LO(FPT2)
	SJMP	$-7			;JUMP TO PUSHAS
	;
	;****************************************************************
	;
SWAP_ASTKA:	; SWAP TOS<>TOS-1
	;
	;****************************************************************
	;
	ACALL	SETREG		;SET UP R2:R0 AND P2
	MOV	A,#FPSIZ	;PUT TOS+1 IN R1
	MOV	R2,A
	ADD	A,R0
	MOV	R1,A
	;
S_L:	MOVX	A,@R0
	MOV	R3,A
	MOVX	A,@R1
	MOVX	@R0,A
	MOV	A,R3
	MOVX	@R1,A
	DEC	R1
	DEC	R0
	DJNZ	R2,S_L
	RET
	;
	newpage
	;
C_T2:	ACALL	SETREG		;SET UP R2:R0
	MOV	R3,#HI(FPT2)
	MOV	R1,#LO(FPT2)	;TEMP VALUE
	;
	; Fall thru
	;
	;***************************************************************
	;
	; VARCOP - Copy a variable from R2:R0 to R3:R1
	;
	;***************************************************************
	;
VARCOP:	MOV	R4,#FPSIZ	;LOAD THE LOOP COUNTER
	;
V_C:	MOV	P2,R2		;SET UP THE PORTS
	MOVX	A,@R0		;READ THE VALUE
	MOV	P2,R3		;PORT TIME AGAIN
	MOVX	@R1,A		;SAVE IT
	ACALL	DEC3210		;BUMP POINTERS
	DJNZ	R4,V_C		;LOOP
	RET			;EXIT
	;
PIPI:	MOV	DPTR,#PIE
	AJMP	PUSHC
	;
	newpage
	;***************************************************************
	;
	; The logical operators ANL, ORL, XRL, NOT
	;
	;***************************************************************
	;
AANL:	ACALL	TWOL		;GET THE EXPRESSIONS
	MOV	A,R3		;DO THE AND
	ANL	A,R7
	MOV	R2,A
	MOV	A,R1
	ANL	A,R6
	SJMP	TWO_EX
	;
AORL:	ACALL	TWOL		;SAME THING FOR OR
	MOV	A,R3
	ORL	A,R7
	MOV	R2,A
	MOV	A,R1
	ORL	A,R6
	SJMP	TWO_EX
	;
ANOT:	ACALL	FSTK		;PUT 0FFFFH ON THE STACK
	;
AXRL:	ACALL	TWOL
	MOV	A,R3
	XRL	A,R7
	MOV	R2,A
	MOV	A,R1
	XRL	A,R6
	SJMP	TWO_EX
	;
TWOL:	ACALL	IFIX
	MOV	R7,R3B0
	MOV	R6,R1B0
	AJMP	IFIX
	;
	newpage
	;*************************************************************
	;
AGET:	; READ THE BREAK BYTE AND PUT IT ON THE ARG STACK
	;
	;*************************************************************
	;
	MOV	DPTR,#GTB	;GET THE BREAK BYTE
	MOVX	A,@DPTR
	JBC	GTRD,TWO_R2
	CLR	A
	;
TWO_R2:	MOV	R2,#00H		;ACC GOES TO STACK
	;
	;
TWO_EX:	MOV	R0,A		;R2:ACC GOES TO STACK
	;
	;
TWO_EY:	SETB	ARGF		;R2:R0 GETS PUT ON THE STACK
	JMP	FP_BASE+24	;DO IT
	;
	newpage
	;*************************************************************
	;
	; Put directs onto the stack
	;
	;**************************************************************
	;
A_IE:	MOV	A,IE		;IE
	SJMP	TWO_R2
	;
A_IP:	MOV	A,IP		;IP
	SJMP	TWO_R2
	;
ATIM0:	MOV	R2,TH0		;TIMER 0
	MOV	R0,TL0
	SJMP	TWO_EY
	;
ATIM1:	MOV	R2,TH1		;TIMER 1
	MOV	R0,TL1
	SJMP	TWO_EY
	;
ATIM2:	DB	0AAH		;MOV R2 DIRECT OP CODE
	DB	0CDH		;T2 HIGH
	DB	0A8H		;MOV R0 DIRECT OP CODE
	DB	0CCH		;T2 LOW
	SJMP	TWO_EY		;TIMER 2
	;
AT2CON:	DB	0E5H		;MOV A,DIRECT OPCODE
	DB	0C8H		;T2CON LOCATION
	SJMP	TWO_R2
	;
ATCON:	MOV	A,TCON		;TCON
	SJMP	TWO_R2
	;
ATMOD:	MOV	A,TMOD		;TMOD
	SJMP	TWO_R2
	;
ARCAP2:	DB	0AAH		;MOV R2, DIRECT OP CODE
	DB	0CBH		;RCAP2H LOCATION
	DB	0A8H		;MOV R0, DIRECT OP CODE
	DB	0CAH		;R2CAPL LOCATION
	SJMP	TWO_EY
	;
AP1:	MOV	A,P1		;GET P1
	SJMP	TWO_R2		;PUT IT ON THE STACK
	;
APCON:	DB	0E5H		;MOV A, DIRECT OP CODE
	DB	87H		;ADDRESS OF PCON
	SJMP	TWO_R2		;PUT PCON ON THE STACK
	;
	newpage
	;***************************************************************
	;
	;THIS IS THE LINE EDITOR
	;
	;TAKE THE PROCESSED LINE IN IBUF AND INSERT IT INTO THE
	;BASIC TEXT FILE.
	;
	;***************************************************************
	;
	LJMP	NOGO		;CAN'T EDIT A ROM
	;
LINE:	MOV	A,BOFAH
	CJNE	A,#HI(PSTART),LINE-3
	CALL	G4		;GET END ADDRESS FOR EDITING
	MOV	R4,DPL
	MOV	R5,DPH
	MOV	R3,TEMP5	;GET HIGH ORDER IBLN
	MOV	R1,TEMP4	;LOW ORDER IBLN
	;
	CALL	GETLIN		;FIND THE LINE
	JNZ	INSR		;INSERT IF NOT ZERO, ELSE APPEND
	;
	;APPEND THE LINE AT THE END
	;
	MOV	A,TEMP3		;PUT IBCNT IN THE ACC
	CJNE	A,#4H,$+4	;SEE IF NO ENTRY
	RET			;RET IF NO ENTRY
	;
	ACALL	FULL		;SEE IF ENOUGH SPACE LEFT
	MOV	R2,R5B0		;PUT END ADDRESS A INTO TRANSFER
	MOV	R0,R4B0		;REGISTERS
	ACALL	IMOV		;DO THE BLOCK MOVE
	;
UE:	MOV	A,#EOF		;SAVE EOF CHARACTER
	AJMP	TBR
	;
	;INSERT A LINE INTO THE FILE
	;
INSR:	MOV	R7,A		;SAVE IT IN R7
	CALL	TEMPD		;SAVE INSERATION ADDRESS
	MOV	A,TEMP3		;PUT THE COUNT LENGTH IN THE ACC
	JC	LTX		;JUMP IF NEW LINE # NOT = OLD LINE #
	CJNE	A,#04H,$+4	;SEE IF NULL
	CLR	A
	;
	SUBB	A,R7		;SUBTRACT LINE COUNT FROM ACC
	JZ	LIN1		;LINE LENGTHS EQUAL
	JC	GTX		;SMALLER LINE
	;
	newpage
	;
	;EXPAND FOR A NEW LINE OR A LARGER LINE
	;
LTX:	MOV	R7,A		;SAVE A IN R7
	MOV	A,TEMP3		;GET THE COUNT IN THE ACC
	CJNE	A,#04H,$+4	;DO NO INSERTATION IF NULL LINE
	RET			;EXIT IF IT IS
	;
	MOV	A,R7		;GET THE COUNT BACK - DELTA IN A
	ACALL	FULL		;SEE IF ENOUGH MEMORY NEW EOFA IN R3:R1
	CALL	DTEMP		;GET INSERATION ADDRESS
	ACALL	NMOV		;R7:R6 GETS (EOFA)-DPTR
	CALL	X3120
	MOV	R1,R4B0		;EOFA LOW
	MOV	R3,R5B0		;EOFA HIGH
	INC	R6		;INCREMENT BYTE COUNT
	CJNE	R6,#00,$+4	;NEED TO BUMP HIGH BYTE?
	INC	R7
	;
	ACALL	RMOV		;GO DO THE INSERTION
	SJMP	LIN1		;INSERT THE CURRENT LINE
	;
GTX:	CPL	A		;FLIP ACC
	INC	A		;TWOS COMPLEMENT
	CALL	ADDPTR		;DO THE ADDITION
	ACALL	NMOV		;R7:R6 GETS (EOFA)-DPTR
	MOV	R1,DPL		;SET UP THE REGISTERS
	MOV	R3,DPH
	MOV	R2,TEMP5	;PUT INSERTATION ADDRESS IN THE RIGHT REG
	MOV	R0,TEMP4
	JZ	$+4		;IF ACC WAS ZERO FROM NMOV, JUMP
	ACALL	LMOV		;IF NO ZERO DO A LMOV
	;
	ACALL	UE		;SAVE NEW END ADDRESS
	;
LIN1:	MOV	R2,TEMP5	;GET THE INSERTATION ADDRESS
	MOV	R0,TEMP4
	MOV	A,TEMP3		;PUT THE COUNT LENGTH IN ACC
	CJNE	A,#04H,IMOV	;SEE IF NULL
	RET			;EXIT IF NULL
	newpage
	;***************************************************************
	;
	;INSERT A LINE AT ADDRESS R2:R0
	;
	;***************************************************************
	;
IMOV:	CLR	A		;TO SET UP
	MOV	R1,#LO(IBCNT)	;INITIALIZE THE REGISTERS
	MOV	R3,A
	MOV	R6,TEMP3	;PUT THE BYTE COUNT IN R6 FOR LMOV
	MOV	R7,A		;PUT A 0 IN R7 FOR LMOV
	;
	;***************************************************************
	;
	;COPY A BLOCK FROM THE BEGINNING
	;
	;R2:R0 IS THE DESTINATION ADDRESS
	;R3:R1 IS THE SOURCE ADDRESS
	;R7:R6 IS THE COUNT REGISTER
	;
	;***************************************************************
	;
LMOV:	ACALL	TBYTE		;TRANSFER THE BYTE
	ACALL	INC3210		;BUMP THE POINTER
	ACALL	DEC76		;BUMP R7:R6
	JNZ	LMOV		;LOOP
	RET			;GO BACK TO CALLING ROUTINE
	;
INC3210:INC	R0
	CJNE	R0,#00H,$+4
	INC	R2
	;
	INC	R1
	CJNE	R1,#00H,$+4
	INC	R3
	RET
	;
	newpage
	;***************************************************************
	;
	;COPY A BLOCK STARTING AT THE END
	;
	;R2:R0 IS THE DESTINATION ADDRESS
	;R3:R1 IS THE SOURCE ADDRESS
	;R6:R7 IS THE COUNT REGISTER
	;
	;***************************************************************
	;
RMOV:	ACALL	TBYTE		;TRANSFER THE BYTE
	ACALL	DEC3210		;DEC THE LOCATIONS
	ACALL	DEC76		;BUMP THE COUNTER
	JNZ	RMOV		;LOOP
	;
DEC_R:	NOP			;CREATE EQUAL TIMING
	RET			;EXIT
	;
DEC3210:DEC	R0		;BUMP THE POINTER
	CJNE	R0,#0FFH,$+4	;SEE IF OVERFLOWED
	DEC	R2		;BUMP THE HIGH BYTE
	DEC	R1		;BUMP THE POINTER
	CJNE	R1,#0FFH,DEC_R	;SEE IF OVERFLOWED
	DEC	R3		;CHANGE THE HIGH BYTE
	RET			;EXIT
	;
	;***************************************************************
	;
	;TBYTE - TRANSFER A BYTE
	;
	;***************************************************************
	;
TBYTE:	MOV	P2,R3		;OUTPUT SOURCE REGISTER TO PORT
	MOVX	A,@R1		;PUT BYTE IN ACC
	;
TBR:	MOV	P2,R2		;OUTPUT DESTINATION TO PORT
	MOVX	@R0,A		;SAVE THE BYTE
	RET			;EXIT
	;
	newpage
	;***************************************************************
	;
	;NMOV - R7:R6 = END ADDRESS - DPTR
	;
	;ACC GETS CLOBBERED
	;
	;***************************************************************
	;
NMOV:	MOV	A,R4		;THE LOW BYTE OF EOFA
	CLR	C		;CLEAR THE CARRY FOR SUBB
	SUBB	A,DPL		;SUBTRACT DATA POINTER LOW
	MOV	R6,A		;PUT RESULT IN R6
	MOV	A,R5		;HIGH BYTE OF EOFA
	SUBB	A,DPH		;SUBTRACT DATA POINTER HIGH
	MOV	R7,A		;PUT RESULT IN R7
	ORL	A,R6		;SEE IF ZERO
	RET			;EXIT
	;
	;***************************************************************
	;
	;CHECK FOR A FILE OVERFLOW
	;LEAVES THE NEW END ADDRESS IN R3:R1
	;A HAS THE INCREASE IN SIZE
	;
	;***************************************************************
	;
FULL:	ADD	A,R4		;ADD A TO END ADDRESS
	MOV	R1,A		;SAVE IT
	CLR	A
	ADDC	A,R5		;ADD THE CARRY
	MOV	R3,A
	MOV	DPTR,#VARTOP	;POINT AT VARTOP
	;
FUL1:	CALL	DCMPX		;COMPARE THE TWO
	JC	FULL-1		;OUT OF ROOM
	;
TB:	MOV	DPTR,#E5X	;OUT OF MEMORY
	AJMP	FPTS
	;
	newpage
	;***************************************************************
	;
	; PP - Preprocesses the line in IBUF back into IBUF
	;      sets F0 if no line number
	;      leaves the correct length of processed line in IBCNT
	;      puts the line number in IBLN
	;      wastes the text address TXAL and TXAH
	;
	;***************************************************************
	;
PP:	ACALL	T_BUF		;TXA GETS IBUF
	CALL	INTGER		;SEE IF A NUMBER PRESENT
	CALL	TEMPD		;SAVE THE INTEGER IN TEMP5:TEMP4
	MOV	F0,C		;SAVE INTEGER IF PRESENT
	MOV	DPTR,#IBLN	;SAVE THE LINE NUMBER, EVEN IF NONE
	ACALL	S20DP
	MOV	R0,TXAL		;TEXT POINTER
	MOV	R1,#LO(IBUF)	;STORE POINTER
	;
	; Now process the line back into IBUF
	;
PPL:	CLR	ARGF		;FIRST PASS DESIGNATOR
	MOV	DPTR,#TOKTAB	;POINT DPTR AT LOOK UP TABLE
	;
PPL1:	MOV	R5B0,R0		;SAVE THE READ POINTER
	CLR	A		;ZERO A FOR LOOKUP
	MOVC	A,@A+DPTR	;GET THE TOKEN
	MOV	R7,A		;SAVE TOKEN IN CASE OF MATCH
	;
PPL2:	MOVX	A,@R0		;GET THE USER CHARACTER
	MOV	R3,A		;SAVE FOR REM
	CJNE	A,#'a',$+3
	JC	PPX		;CONVERT LOWER TO UPPER CASE
	CJNE	A,#('z'+1),$+3
	JNC	PPX
	CLR	ACC.5
	;
PPX:	MOV	R2,A
	MOVX	@R0,A		;SAVE UPPER CASE
	INC	DPTR		;BUMP THE LOOKUP POINTER
	CLR	A
	MOVC	A,@A+DPTR
	CJNE	A,R2B0,PPL3	;LEAVE IF NOT THE SAME
	INC	R0		;BUMP THE USER POINTER
	SJMP	PPL2		;CONTINUE TO LOOP
	;
PPL3:	JB	ACC.7,PPL6	;JUMP IF FOUND MATCH
	JZ	PPL6		;USER MATCH
	;
	;
	; Scan to the next TOKTAB entry
	;
PPL4:	INC	DPTR		;ADVANCE THE POINTER
	CLR	A		;ZERO A FOR LOOKUP
	MOVC	A,@A+DPTR	;LOAD A WITH TABLE
	JB	ACC.7,$+6	;KEEP SCANNING IF NOT A RESERVED WORD
	JNZ	PPL4
	INC	DPTR
	;
	; See if at the end of TOKTAB
	;
	MOV	R0,R5B0		;RESTORE THE POINTER
	CJNE	A,#0FFH,PPL1	;SEE IF END OF TABLE
	;
	; Character not in TOKTAB, so see what it is
	;
	CJNE	R2,#' ',PPLX	;SEE IF A SPACE
	INC	R0		;BUMP USER POINTER
	SJMP	PPL		;TRY AGAIN
	;
PPLX:	JNB	XBIT,PPLY	;EXTERNAL TRAP
	JB	ARGF,PPLY
	SETB	ARGF		;SAYS THAT THE USER HAS TABLE
	LCALL	2078H		;SET UP POINTER
	AJMP	PPL1
	;
PPLY:	ACALL	PPL7		;SAVE CHARACTER, EXIT IF A CR
	CJNE	A,#'"',PPL	;SEE IF QUOTED STRING, START AGAIN IF NOT
	;
	; Just copy a quoted string
	;
	ACALL	PPL7		;SAVE THE CHARACTER, TEST FOR CR
	CJNE	A,#'"',$-2	;IS THERE AN ENDQUOTE, IF NOT LOOP
	SJMP	PPL		;DO IT AGAIN IF ENDQUOTE
	;
PPL6:	MOV	A,R7		;GET THE TOKEN
	ACALL	PPL9+1		;SAVE THE TOKEN
	CJNE	A,#T_REM,PPL	;SEE IF A REM TOKEN
	MOV	A,R3
	ACALL	PPL7+1		;WASTE THE REM STATEMENT
	ACALL	PPL7		;LOOP UNTIL A CR
	SJMP	$-2
	;
PPL7:	MOVX	A,@R0		;GET THE CHARACTER
	CJNE	A,#CR,PPL9	;FINISH IF A CR
	POP	R0B0		;WASTE THE CALLING STACK
	POP	R0B0
	MOVX	@R1,A		;SAVE CR IN MEMORY
	INC	R1		;SAVE A TERMINATOR
	MOV	A,#EOF
	MOVX	@R1,A
	MOV	A,R1		;SUBTRACT FOR LENGTH
	SUBB	A,#4
	MOV	TEMP3,A		;SAVE LENGTH
	MOV	R1,#LO(IBCNT)	;POINT AT BUFFER COUNT
	;
PPL9:	INC	R0
	MOVX	@R1,A		;SAVE THE CHARACTER
	INC	R1		;BUMP THE POINTERS
	RET			;EXIT TO CALLING ROUTINE
	;
	;
	;***************************************************************
	;
	;DEC76 - DECREMENT THE REGISTER PAIR R7:R6
	;
	;ACC = ZERO IF R7:R6 = ZERO ; ELSE ACC DOES NOT
	;
	;***************************************************************
	;
DEC76:	DEC	R6		;BUMP R6
	CJNE	R6,#0FFH,$+4	;SEE IF RAPPED AROUND
	DEC	R7
	MOV	A,R7		;SEE IF ZERO
	ORL	A,R6
	RET			;EXIT
	;
	;***************************************************************
	;
	; MTOP - Get or Put the top of assigned memory
	;
	;***************************************************************
	;
PMTOP:	MOV	DPTR,#MEMTOP
	CALL	L20DPI
	AJMP	TWO_EY		;PUT R2:R0 ON THE STACK
	;
	newpage
	;*************************************************************
	;
	; AXTAL - Crystal value calculations
	;
	;*************************************************************
	;
AXTAL0:	MOV	DPTR,#XTALV	;CRYSTAL VALUE
	ACALL	PUSHC
	;
AXTAL1:	ACALL	CSTAKA2		;COPY CRYSTAL VALUE TWICE
	ACALL	CSTAKA
	MOV	DPTR,#PTIME	;PROM TIMER
	ACALL	AXTAL2
	MOV	DPTR,#PROGS
	ACALL	S31L
	MOV	DPTR,#IPTIME	;IPROM TIMER
	ACALL	AXTAL2
	MOV	DPTR,#IPROGS
	ACALL	S31L
	MOV	DPTR,#TTIME	;CLOCK CALCULATION
	ACALL	AXTAL3
	MOV	A,R1
	CPL	A
	INC	A
	MOV	SAVE_T,A
	MOV	R3,#HI(CXTAL)
	MOV	R1,#LO(CXTAL)
	JMP	POPAS
	;
AXTAL2:	ACALL	AXTAL3
	;
CBIAS:	;Bias the crystal calculations
	;
	MOV	A,R1		;GET THE LOW COUNT
	CPL	A		;FLIP IT FOR TIMER LOAD
	ADD	A,#15		;BIAS FOR CALL AND LOAD TIMES
	MOV	R1,A		;RESTORE IT
	MOV	A,R3		;GET THE HIGH COUNT
	CPL	A		;FLIP IT
	ADDC	A,#00H		;ADD THE CARRY
	MOV	R3,A		;RESTORE IT
	RET
	;
	newpage
	include	bas52.pwm	; ******AA
	newpage
	;LNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLN
	;
LNTAB:	; Natural log lookup table
	;
	;LNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLNLN
	;
	DB	80H
	DB	00H
	DB	71H
	DB	37H
	DB	13H
	DB	19H
	;
	DB	7FH
	DB	00H
	DB	76H
	DB	64H
	DB	37H
	DB	94H
	;
	DB	80H
	DB	00H
	DB	07H
	DB	22H
	DB	75H
	DB	17H
	;
	DB	80H
	DB	00H
	DB	52H
	DB	35H
	DB	93H
	DB	28H
	;
	DB	80H
	DB	00H
	DB	71H
	DB	91H
	DB	85H
	DB	86H
	;
	DB	0FFH
	;
	DB	81H
	DB	00H
	DB	51H
	DB	58H
	DB	02H
	DB	23H
	;
	newpage
	;SINSINSINSINSINSINSINSINSINSINSINSINSINSINSINSINSIN
	;
SINTAB:	; Sin lookup table
	;
	;SINSINSINSINSINSINSINSINSINSINSINSINSINSINSINSINSIN
	;
	DB	128-9
	DB	00H
	DB	44H
	DB	90H
	DB	05H
	DB	16H
	;
	DB	128-7
	DB	01H
	DB	08H
	DB	21H
	DB	05H
	DB	25H
	;
	DB	128-5
	DB	00H
	DB	19H
	DB	73H
	DB	55H
	DB	27H
	;
	newpage
	;
	DB	128-3
	DB	01H
	DB	70H
	DB	12H
	DB	84H
	DB	19H
	;
	DB	128-2
	DB	00H
	DB	33H
	DB	33H
	DB	33H
	DB	83H
	;
	DB	128
	DB	01H
	DB	67H
	DB	66H
	DB	66H
	DB	16H
	;
FPONE:	DB	128+1
	DB	00H
	DB	00H
	DB	00H
	DB	00H
	DB	10H
	;
	DB	0FFH		;END OF TABLE
	;
	newpage
	;
SBAUD:	CALL	AXTAL		;PUT CRYSTAL ON THE STACK
	CALL	EXPRB		;PUT THE NUMBER AFTER BAUD ON STACK
	MOV	A,#12
	ACALL	TWO_R2		;TOS = 12
	ACALL	AMUL		;TOS = 12*BAUD
	ACALL	ADIV		;TOS = XTAL/(12*BAUD)
	ACALL	IFIX
	ACALL	CBIAS
	MOV	DPTR,#SPV
	;
S31L:	JMP	S31DP
	;
AFREE:	CALL	PMTOP		;PUT MTOP ON STACK
	CALL	G4		;GET END ADDRESS
	MOV	R0,DPL
	MOV	R2,DPH
	ACALL	TWO_EY
	;
ASUB:	LCALL	FP_BASE+2	;DO FP SUB
	AJMP	FPTST
	;
ALEN:	CALL	CCAL		;CALCULATE THE LEN OF THE SELECTED PROGRAM
	MOV	R2,R7B0		;SAVE THE HIGH BYTE
	MOV	A,R6		;SAVE THE LOW BYTE
	AJMP	TWO_EX		;PUT IT ON THE STACK
	;
ATIME:	MOV	C,EA		;SAVE INTERRUTS
	CLR	EA
	PUSH	MILLIV		;SAVE MILLI VALUE
	MOV	R2,TVH		;GET THE TIMER
	MOV	A,TVL
	MOV	EA,C		;SAVE INTERRUPTS
	ACALL	TWO_EX		;PUT TIMER ON THE STACK
	POP	ACC		;GET MILLI
	ACALL	TWO_R2		;PUT MILLI ON STACK
	MOV	A,#200
	ACALL	TWO_R2		;DIVIDE MILLI BY 200
	ACALL	ADIV
	;
AADD:	LCALL	FP_BASE		;DO FP ADDITION
	AJMP	FPTST		;CHECK FOR ERRORS
	;
	newpage
	;**************************************************************
	;
	; Here are some error messages that were moved
	;
	;**************************************************************
	;
	;
E1X:	DB	"BAD SYNTAX",'"'
E2X:	DB	128+10
	DB	"DIVIDE BY ZERO",'"'
	;
E6X:	DB	"ARRAY SIZE",'"'
	;
	newpage
	;**************************************************************
	;
T_BUF:	; TXA gets IBUF
	;
	;**************************************************************
	;
	MOV	TXAH,#HI(IBUF)
	MOV	TXAL,#LO(IBUF)
	RET
	;
	;
	;***************************************************************
	;
CXFER:	; Transfer a program from rom to ram
	;
	;***************************************************************
	;
	CALL	CCAL		;GET EVERYTHING SET UP
	MOV	R2,#HI(PSTART)
	MOV	R0,#LO(PSTART)
	ACALL	LMOV		;DO THE TRANSFER
	CALL	RCLEAR		;CLEAR THE MEMORY
	;
	; Fall thru to CRAM
	;
	;***************************************************************
	;
CRAM:	; The command action routine - RAM - Run out of ram
	;
	;***************************************************************
	;
	CLR	CONB		;CAN'T CONTINUE IF MODE CHANGE
	MOV	BOFAH,#HI(PSTART)
	MOV	BOFAL,#LO(PSTART)
	;
	; Fall thru to Command Processor
	;
	newpage
	;***************************************************************
	;
CMND1:	; The entry point for the command processor
	;
	;***************************************************************
	;
	LCALL	SPRINT+4	;WASTE AT AND HEX
	CLR	XBIT		;TO RESET IF NEEDED
	CLR	A
	MOV	DPTR,#2002H	;CHECK FOR EXTERNAL TRAP PACKAGE
	MOVC	A,@A+DPTR
	CJNE	A,#5AH,$+6
	LCALL	2048H		;IF PRESENT JUMP TO LOCATION 200BH
	MOV	DPTR,#RDYS	;PRINT THE READY MESSAGE
	CALL	CRP		;DO A CR, THEN, PRINT FROM THE ROM
	;
CMNDR:	SETB	DIRF		;SET THE DIRECT INPUT BIT
	MOV	SP,SPSAV	;LOAD THE STACK
	ACALL	CL7		;DO A CRLF
	;
CMNX:	CLR	GTRD		;CLEAR BREAK
	MOV	DPTR,#5EH	;DO RUN TRAP
	MOVX	A,@DPTR
	XRL	A,#52
	JNZ	$+5
	LJMP	CRUN
	MOV	R5,#'>'		;OUTPUT A PROMPT
	LCALL	TEROT
	CALL	INLINE		;INPUT A LINE INTO IBUF
	CALL	PP		;PRE-PROCESS THE LINE
	JB	F0,CMND3	;NO LINE NUMBER
	CALL	LINE		;PROCESS THE LINE
	LCALL	LCLR
	JB	LINEB,CMNX	;DON'T CLEAR MEMORY IF NO NEED
	SETB	LINEB
	LCALL	RCLEAR		;CLEAR THE MEMORY
	SJMP	CMNX		;LOOP BACK
	;
CMND3:	CALL	T_BUF		;SET UP THE TEXT POINTER
	CALL	DELTST		;GET THE CHARACTER
	JZ	CMNDR		;IF CR, EXIT
	MOV	DPTR,#CMNDD	;POINT AT THE COMMAND LOOKUP
	CJNE	A,#T_CMND,$+3	;PROCESS STATEMENT IF NOT A COMMAND
	JC	CMND5
	CALL	GCI1		;BUMP TXA
	ANL	A,#0FH		;STRIP MSB'S FOR LOOKUP
	LCALL	ISTA1		;PROCESS COMMAND
	SJMP	CMNDR
	;
CMND5:	LJMP	ILOOP		;CHECK FOR A POSSIBLE BREAK
	;
	;
	;
	;CONSTANTS
	;
XTALV:	DB	128+8		; DEFAULT CRYSTAL VALUE
	DB	00H
	DB	00H
	DB	92H
	DB	05H
	DB	11H
	;
EXP11:	DB	85H
	DB	00H
	DB	42H
	DB	41H
	DB	87H
	DB	59H
	;
EXP1:	DB	128+1		; EXP(1)
	DB	00H
	DB	18H
	DB	28H
	DB	18H
	DB	27H
	;
IPTIME:	DB	128-4		;FPROG TIMING
	DB	00H
	DB	00H
	DB	00H
	DB	75H
	DB	83H
	;
PIE:	DB	128+1		;PI
	DB	00H
	DB	26H
	DB	59H
	DB	41H
	DB	31H		; 3.1415926
	;
	newpage
	;***************************************************************
	;
	; The error messages, some have been moved
	;
	;***************************************************************
	;
E7X:	DB	128+30
	DB	"ARITH. UNDERFLOW",'"'
	;
E5X:	DB	"MEMORY ALLOCATION",'"'
	;
E3X:	DB	128+40
	DB	"BAD ARGUMENT",'"'
	;
EXI:	DB	"I-STACK",'"'
	;
	newpage
	;***************************************************************
	;
	; The command action routine - CONTINUE
	;
	;***************************************************************
	;
CCONT:	MOV	DPTR,#E15X
	JNB	CONB,ERROR	;ERROR IF CONTINUE IS NOT SET
	;
CC1:	;used for input statement entry
	;
	MOV	TXAH,INTXAH	;RESTORE TXA
	MOV	TXAL,INTXAL
	JMP	CILOOP		;EXECUTE
	;
DTEMP:	MOV	DPH,TEMP5	;RESTORE DPTR
	MOV	DPL,TEMP4
	RET
	;
TEMPD:	MOV	TEMP5,DPH
	MOV	TEMP4,DPL
	RET
	;
	newpage
	;**************************************************************
	;
I_DL:	; IDLE
	;
	;**************************************************************
	;
	JB	DIRF,E1XX	;SYNTAX ERROR IN DIRECT INPUT
	CLR	DACK		;ACK IDLE
	;
U_ID1:	DB	01000011B	;ORL DIRECT OP CODE
	DB	87H		;PCON ADDRESS
	DB	01H		;SET IDLE BIT
	JB	INTPEN,I_RET	;EXIT IF EXTERNAL INTERRUPT
	JBC	U_IDL,I_RET	;EXIT IF USER WANTS TO
	JNB	OTS,U_ID1	;LOOP IF TIMER NOT ENABLED
	LCALL	T_CMP		;CHECK THE TIMER
	JC	U_ID1		;LOOP IF TIME NOT BIG ENOUGH
	;
I_RET:	SETB	DACK		;RESTORE EXECUTION
	RET			;EXIT IF IT IS
	;
	;
	;
ER0:	INC	DPTR		;BUMP TO TEXT
	JB	DIRF,ERROR0	;CAN'T GET OUT OF DIRECT MODE
	JNB	ON_ERR,ERROR0	;IF ON ERROR ISN'T SET, GO BACK
	MOV	DPTR,#ERRLOC	;SAVE THE ERROR CODE
	CALL	RC2		;SAVE ERROR AND SET UP THE STACKS
	INC	DPTR		;POINT AT ERRNUM
	JMP	ERL4		;LOAD ERR NUM AND EXIT
	;
	newpage
	;
	; Syntax error
	;
E1XX:	MOV	C,DIRF		;SEE IF IN DIRECT MODE
	MOV	DPTR,#E1X	;ERROR MESSAGE
	SJMP	ERROR+1		;TRAP ON SET DIRF
	;
	MOV	DPTR,#EXI	;STACK ERROR
	;
	; Falls through
	;
	;***************************************************************
	;
	;ERROR PROCESSOR - PRINT OUT THE ERROR TYPE, CHECK TO SEE IF IN
	;                  RUN OR COMMAND MODE, FIND AND PRINT OUT THE
	;                  LINE NUMBER IF IN RUN MODE
	;
	;***************************************************************
	;
ERROR:	CLR	C		;RESET STACK
	MOV	SP,SPSAV	;RESET THE STACK
	LCALL	SPRINT+4	;CLEAR LINE AND AT MODE
	CLR	A		;SET UP TO GET ERROR CODE
	MOVC	A,@A+DPTR
	JBC	ACC.7,ER0	;PROCESS ERROR
	;
ERROR0:	ACALL	TEMPD		;SAVE THE DATA POINTER
	JC	$+5		;NO RESET IF CARRY IS SET
	LCALL	RC1		;RESET THE STACKS
	CALL	CRLF2		;DO TWO CARRIAGE RET - LINE FEED
	MOV	DPTR,#ERS	;OUTPUT ERROR MESSAGE
	CALL	ROM_P
	CALL	DTEMP		;GET THE ERROR MESSAGE BACK
	;
ERRS:	CALL	ROM_P		;PRINT ERROR TYPE
	JNB	DIRF,ER1	;DO NOT PRINT IN LINE IF DIRF=1
	;
SERR1:	CLR	STOPBIT		;PRINT STOP THEN EXIT, FOR LIST
	JMP	CMND1
	;
ER1:	MOV	DPTR,#INS	;OUTPUT IN LINE
	CALL	ROM_P
	;
	;NOW, FIND THE LINE NUMBER
	;
	;
	newpage
	;
	;
	CALL	DP_B		;GET THE FIRST ADDRESS OF THE PROGRAM
	CLR	A		;FOR INITIALIZATION
	;
ER2:	ACALL	TEMPD		;SAVE THE DPTR
	CALL	ADDPTR		;ADD ACC TO DPTR
	ACALL	ER4		;R3:R1 = TXA-DPTR
 JC	ER3		;EXIT IF DPTR>TXA
	JZ	ER3		;EXIT IF DPTR=TXA
	MOVX	A,@DPTR		;GET LENGTH
	CJNE	A,#EOF,ER2	;SEE IF AT THE END
	;
ER3:	ACALL	DTEMP		;PUT THE LINE IN THE DPTR
	ACALL	ER4		;R3:R1 = TXA - BEGINNING OF LINE
	MOV	A,R1		;GET LENGTH
  	ADD	A,#10		;ADD 10 TO LENGTH, DPTR STILL HAS ADR
	MOV	MT1,A		;SAVE THE COUNT
	INC	DPTR		;POINT AT LINE NUMBER HIGH BYTE
	CALL	PMTOP+3		;LOAD R2:R0, PUT IT ON THE STACK
	ACALL	FP_BASE+14	;OUTPUT IT
	JB	STOPBIT,SERR1	;EXIT IF STOP BIT SET
	CALL	CRLF2		;DO SOME CRLF'S
	CALL	DTEMP
	CALL	UPPL		;UNPROCESS THE LINE
	CALL	CL6		;PRINT IT
	MOV	R5,#'-'		;OUTPUT DASHES, THEN AN X
	ACALL	T_L		;PRINT AN X IF ERROR CHARACTER FOUND
	DJNZ	MT1,$-4		;LOOP UNTIL DONE
	MOV	R5,#'X'
	ACALL	T_L
	AJMP	SERR1
	;
ER4:	MOV	R3,TXAH		;GET TEXT POINTER AND PERFORM SUBTRACTION
	MOV	R1,TXAL
	JMP	DUBSUB
	;
	newpage
	;**************************************************************
	;
	; Interrupt driven timer
	;
	;**************************************************************
	;
I_DR:	MOV	TH0,SAVE_T	;LOAD THE TIMER
	XCH	A,MILLIV	;SAVE A, GET MILLI COUNTER
	INC	A		;BUMP COUNTER
	CJNE	A,#200,TR	;CHECK OUT TIMER VALUE
	CLR	A		;FORCE ACC TO BE ZERO
	INC	TVL		;INCREMENT LOW TIMER
	CJNE	A,TVL,TR	;CHECK LOW VALUE
	INC	TVH		;BUMP TIMER HIGH
	;
TR:	XCH	A,MILLIV
	POP	PSW
	RETI
	;
	newpage
	include	bas52.clk
	;***************************************************************
	;
SUI:	; Statement USER IN action routine
	;
	;***************************************************************
	;
	ACALL	OTST
	MOV	CIUB,C		;SET OR CLEAR CIUB
	RET
	;
	;***************************************************************
	;
SUO:	; Statement USER OUT action routine
	;
	;***************************************************************
	;
	ACALL	OTST
	MOV	COUB,C
	RET
	;
OTST:	; Check for a one
	;
	LCALL	GCI		;GET THE CHARACTER, CLEARS CARRY
	SUBB	A,#'1'		;SEE IF A ONE
	CPL	C		;SETS CARRY IF ONE, CLEARS IT IF ZERO
	RET
	;
	newpage
	;**************************************************************
	;
	; IBLK - EXECUTE USER SUPPLIED TOKEN
	;
	;**************************************************************
	;
IBLK:	JB	PSW.4,IBLK-1	;EXIT IF REGISTER BANK <> 0
	JB	PSW.3,IBLK-1
	JBC	ACC.7,$+9	;SEE IF BIT SEVEN IS SET
	MOV	DPTR,#USENT	;USER ENTRY LOCATION
	LJMP	ISTA1
	;
	JB	ACC.0,199FH	;FLOATING POINT INPUT
	JZ	T_L		;DO OUTPUT ON 80H
	MOV	DPTR,#FP_BASE-2
	JMP	@A+DPTR
	;
	;
	;**************************************************************
	;
	; GET_NUM - GET A NUMBER, EITHER HEX OR FLOAT
	;
	;**************************************************************
	;
GET_NUM:ACALL	FP_BASE+10	;SCAN FOR HEX
	JNC	FP_BASE+12	;DO FP INPUT
	;
	ACALL	FP_BASE+18	;ASCII STRING TO R2:R0
	JNZ	H_RET
	PUSH	DPH		;SAVE THE DATA_POINTER
	PUSH	DPL
	ACALL	FP_BASE+24	;PUT R2:R0 ON THE STACK
	POP	DPL		;RESTORE THE DATA_POINTER
	POP	DPH
	CLR	A		;NO ERRORS
	RET			;EXIT
	;
	newpage
	;**************************************************************
	;
	; WB - THE EGO MESSAGE
	;
	;**************************************************************
	;
WB:	DB	'W'+80H,'R'+80H
	DB	'I'+80H,'T'+80H,'T','E'+80H,'N'+80H
	DB	' ','B'+80H,'Y'+80H,' '
	DB	'J'+80H,'O'+80H,'H'+80H,'N'+80H,' '+80H
	DB	'K','A'+80H,'T'+80H,'A'+80H,'U'+80H
	DB	'S','K'+80H,'Y'+80H
	DB	", I",'N'+80H,'T'+80H,'E'+80H,'L'+80H
	DB	' '+80H,'C'+80H,'O'+80H,'R'+80H,'P'+80H
	DB	". 1",'9'+80H,"85"
H_RET:	RET
	;
	newpage
	ORG	1990H
	;
T_L:	LJMP	TEROT
	;
	ORG	1F78H
	;
CKS_I:	JB	CKS_B,CS_I
	LJMP	401BH
	;
CS_I:	LJMP	2088H
	;
E14X:	DB	"NO DATA",'"'
	;
E11X:	DB	128+20
	DB	"ARITH. OVERFLOW",'"'
	;
E16X:	DB	"PROGRAMMING",'"'
	;
E15X:	DB	"CAN"
	DB	27H
	DB	"T CONTINUE",'"'
	;
E10X:	DB	"INVALID LINE NUMBER",'"'
	;
NOROM:	DB	"PROM MODE",'"'
	;
S_N:	DB	"*MCS-51(tm) BASIC V1.1*",'"'
	;
	ORG	1FF8H
	;
ERS:	DB	"ERROR: ",'"'
	;
	newpage
	;***************************************************************
	;
	segment	xdata	;External Ram
	;
	;***************************************************************
	;
	DS	4
IBCNT:	DS	1		;LENGTH OF A LINE
IBLN:	DS	2		;THE LINE NUMBER
IBUF:	DS	LINLEN		;THE INPUT BUFFER
CONVT:	DS	15		;CONVERSION LOCATION FOR FPIN
	;
	ORG	100H
	;
GTB:	DS	1		;GET LOCATION
ERRLOC:	DS	1		;ERROR TYPE
ERRNUM:	DS	2		;WHERE TO GO ON AN ERROR
VARTOP:	DS	2		;TOP OF VARIABLE STORAGE
ST_ALL:	DS	2		;STORAGE ALLOCATION
MT_ALL:	DS	2		;MATRIX ALLOCATION
MEMTOP:	DS	2		;TOP OF MEMORY
RCELL:	DS	2		;RANDOM NUMBER CELL
	DS	FPSIZ-1
CXTAL:	DS	1		;CRYSTAL
	DS	FPSIZ-1
FPT1:	DS	1		;FLOATINP POINT TEMP 1
	DS	FPSIZ-1
FPT2:	DS	1		;FLOATING POINT TEMP 2
INTLOC:	DS	2		;LOCATION TO GO TO ON INTERRUPT
STR_AL:	DS	2		;STRING ALLOCATION
SPV:	DS	2		;SERIAL PORT BAUD RATE
TIV:	DS	2		;TIMER INTERRUPT NUM AND LOC
PROGS:	DS	2		;PROGRAM A PROM TIME OUT
IPROGS:	DS	2		;INTELLIGENT PROM PROGRAMMER TIMEOUT
TM_TOP:	DS	1

	include	bas52.fp

	END
