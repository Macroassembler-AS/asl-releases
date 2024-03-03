		ifndef	__regz88c0xinc
__regz88c0xinc	equ	1
                save
                listing off   ; kein Listing ueber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.42 - File REGZ88C0X.INC                                           *
;*                                                                          *
;*   Contains Bit & Register Definitions for Zilog Z88C0x                   *
;*                                                                          *
;****************************************************************************

;----------------------------------------------------------------------------
; CPU Core

SPL		sfr	0d9h		; Stack Pointer
SPH		sfr	0d8h
SP		sfr	SPH
RP0		sfr	0d6h		; Register Pointers
RP1		sfr	0d7h
FLAGS		sfr	0d5h		; CPU Flags
IPH		sfr	0dah		; Instruction Pointer
IPL		sfr	0dbh
IP		sfr	IPH
SYM		sfr	0deh		; System Mode
GIE		__z8bit	SYM,0		;  Global Interrupt Enable
FIE		__z8bit	SYM,1		;  Fast Interrupt Enable
FIS		__z8bfield SYM,2,3	;  Fast Interrupt Select
EMT		sfr	0feh		; External Memory Timing (Bank 0)
DMASEL		__z8bit	EMT,0		;  DMA Select
STKSEL		__z8bit	EMT,1		;  Stack Select
DATAWS		__z8bFIELD EMT,2,2	;  Data Memory Automatic Waits
PROGWS		__z8bfield EMT,4,2	;  Program Memory Automatic Waits
SLOW		__z8bit	EMT,6		;  Slow Memory Timing
EWI		__z8bit EMT,7		;  External Wait Input
WUMCH		sfr	0feh		; Wakeup Match Register
WUMSK		sfr	0ffh		; Wakeup Mask Register

;----------------------------------------------------------------------------
; Memory Limits

;----------------------------------------------------------------------------
; GPIO

P0              sfr	0d0h		; Port 0 Data
P1		sfr	0d1h		; Port 1 Data
P2		sfr	0d2h		; Port 2 Data
P3		sfr	0d3h		; Port 3 Data
P4		sfr	0d4h		; Port 4 Data
P0M		sfr	0f0h		; Port 0 Mode (Bank 0)
P00M		__z8bit	P0M,0		;  P00 Mode
P01M		__z8bit	P0M,1		;  P01 Mode
P02M		__z8bit	P0M,2		;  P02 Mode
P03M		__z8bit	P0M,3		;  P03 Mode
P04M		__z8bit	P0M,4		;  P04 Mode
P05M		__z8bit	P0M,5		;  P05 Mode
P06M		__z8bit	P0M,6		;  P06 Mode
P07M		__z8bit	P0M,7		;  P07 Mode
PM		sfr	0f1h		; Port Mode (Bank 0)
P0DIR		__z8bit	PM,0		;  Port 0 Direction
P0OD		__z8bit	PM,1		;  Open-Drain Port 0
P1OD		__z8bit	PM,2		;  Open-Drain Port 1
P35DM		__z8bit	PM,3		;  Enable DM P35
P1M		__z8bfield PM,4,2	;  Port 1 Mode
P4D		sfr	0f6h		; Port 4 Direction (Bank 0)
P4OD		sfr	0f7h		; Port 4 Open Drain (Bank 0)
P2AM		sfr	0f8h		; Port 2/3 A Mode (Bank 0)
P20M		__z8bfield P2AM,0,2	;  P20 Mode
P21M		__z8bfield P2AM,2,2	;  P21 Mode
P30M		__z8bfield P2AM,4,2	;  P30 Mode
P31M		__z8bfield P2AM,6,2	;  P31 Mode
P2BM		sfr	0f9h		; Port 2/3 B Mode (Bank 0)
P22M		__z8bfield P2BM,0,2	;  P22 Mode
P23M		__z8bfield P2BM,2,2	;  P23 Mode
P32M		__z8bfield P2BM,4,2	;  P32 Mode
P33M		__z8bfield P2BM,6,2	;  P33 Mode
P2CM		sfr	0fah		; Port 2/3 C Mode (Bank 0)
P24M		__z8bfield P2CM,0,2	;  P24 Mode
P25M		__z8bfield P2CM,2,2	;  P25 Mode
P34M		__z8bfield P2CM,4,2	;  P34 Mode
P35M		__z8bfield P2CM,6,2	;  P35 Mode
P2DM		sfr	0fbh		; Port 2/3 D Mode (Bank 0)
P26M		__z8bfield P2DM,0,2	;  P26 Mode
P27M		__z8bfield P2DM,2,2	;  P27 Mode
P36M		__z8bfield P2DM,4,2	;  P36 Mode
P37M		__z8bfield P2DM,6,2	;  P37 Mode
P2AIP		sfr	0fch		; Port 2/3 A Interrupt Pending (Bank 0)
P2BIP		sfr	0fdh		; Port 2/3 B Interrupt Pending (Bank 0)

;----------------------------------------------------------------------------
; Interrupt Vectors

		enumconf 2,code
		enum	 RESET_vect=0	; Reset Entry
		nextenum IRQ0_vect	; External Interrupt Request 0, used by...
P22_vect	label	IRQ0_vect	;  EXTERNAL INTERRUPT (P22)
P23_vect	label	IRQ0_vect	;  EXTERNAL INTERRUPT (P23)
		nextenum IRQ1_vect	; External Interrupt Request 1, used by...
U_ZCOUNT_vect	label	IRQ1_vect	;  UART ZERO COUNT
P21_vect	label	IRQ1_vect	;  EXTERNAL INTERRUPT (P21)
U_TXDATA_vect	label	IRQ1_vect	;  UART TRANSMIT DATA
P31_vect	label	IRQ1_vect	;  EXTERNAL INTERRUPT (P31)
		nextenum IRQ2_vect	; External Interrupt Request 2, used by...
C0ZERO_vect	label	IRQ2_vect	;  COUNTER 0 ZERO COUNT
P26_vect	label	IRQ2_vect	;  EXTERNAL INTERRUPT (P26)
P27_vect	label	IRQ2_vect	;  EXTERNAL INTERRUPT (P27)
		nextenum IRQ3_vect	; External Interrupt Request 3, used by...
P32_vect	label	IRQ3_vect	;  EXTERNAL INTERRUPT (P32)
P33_vect	label	IRQ3_vect	;  EXTERNAL INTERRUPT (P33)
		nextenum IRQ4_vect	; External Interrupt Request 2, used by...
HSCHAN0_vect	label	IRQ4_vect	;  HANDSHAKE CHANNEL 0
P24_vect	label	IRQ4_vect	;  EXTERNAL INTERRUPT (P24)
P25_vect	label	IRQ4_vect	;  EXTERNAL INTERRUPT (P25)
		nextenum IRQ5_vect	; External Interrupt Request 3, used by...
C1ZERO_vect	label	IRQ2_vect	;  COUNTER 1 ZERO COUNT
P36_vect	label	IRQ2_vect	;  EXTERNAL INTERRUPT (P36)
P37_vect	label	IRQ2_vect	;  EXTERNAL INTERRUPT (P37)
		nextenum IRQ6_vect	; External Interrupt Request 2, used by...
U_RXOVR_vect	label	IRQ6_vect	;  UART RECEIVE OVERRUN
U_FRMERR_vect	label	IRQ6_vect	;  UART FRAMING ERROR
U_PARITY_vect	label	IRQ6_vect	;  UART PARITY ERROR
U_WKUP_vect	label	IRQ6_vect	;  UART WAKEUP DETECT
U_BRK_vect	label	IRQ6_vect	;  UART BREAK DETECT
U_CC_vect	label	IRQ6_vect	;  UART CONTROL CHAR DETECT
U_RXDATA	label	IRQ6_vect	;  UART RECEIVE DATA
P30_vect	label	IRQ6_vect	;  EXTERNAL INTERRUPT (P30)
P20_vect	label	IRQ6_vect	;  EXTERNAL INTERRUPT (P20)
		nextenum IRQ7_vect	; External Interrupt Request 3, used by...
HSCHAN1_vect	label	IRQ7_vect	;  HANDSHAKE CHANNEL 1
P34_vect	label	IRQ7_vect	;  EXTERNAL INTERRUPT (P34)
P35_vect	label	IRQ7_vect	;  EXTERNAL INTERRUPT (P35)

;----------------------------------------------------------------------------
; Interrupt Control

IRQ		sfr	0dch		; Interrupt Request
IRR		sfr	IRQ
IMR		sfr	0ddh		; Interrupt Mask
IPR		sfr	0ffh		; Interrupt Priority Register (Bank 0)
GROUPA		__z8bit	IPR,0		;  Group A
GROUPB		__z8bit	IPR,2		;  Group B
SUBGROUPB	__z8bit	IPR,3		;  Subgroup B
GROUPC		__z8bit	IPR,5		;  Goup C
SUBGROUPC	__z8bit	IPR,6		;  Subgroup C

;----------------------------------------------------------------------------
; Timers

C0CT		sfr	0e0h		; CTR0 Control (Bank 0)
C0EN		__z8bit	C0CT,0		;  Enable CTR0
C0EOC		__z8bit C0CT,1		;  CTR0 End-of-Count
C0ZCIEN		__z8bit	C0CT,2		;  CTR0 Zero Count Interrupt Enable
C0SC		__z8bit	C0CT,3		;  CTR0 Software Capture
C0ST		__z8bit	C0CT,4		;  CTR0 Software Trigger
C0LC		__z8bit	C0CT,5		;  CTR0 Load Counter
C0DIR		__z8bit	C0CT,6		;  CTR0 up/down
C0CONT		__z8bit	C0CT,7		;  CTR0 Continuous/Single Cycle
C0M		sfr	0e0h		; CTR0 Mode (Bank 1)
C0CM		__z8bfield C0M,0,2	;  CTR0 Capture Mode
C0EDGE		__z8bit	 C0M,2		;  CTR0 Edges of P27
C0RETRIG	__z8bit	 C0M,3		;  CTR0 Enable Retrigger
C0ASSIGN	__z8bfield C0M,4,4	;  CTR0 Input Pin Assignments
C0CH		sfr	0e2h		; CTR0 Capture Register, bits 8-15 (Bank 0)
C0CL		sfr	0e3h		; CTR0 Capture Register, bits 0-7 (Bank 0)
C0C		sfr	C0CH
C0TCH		sfr	0e2h		; CTR0 Timer Constant, bits 8-15 (Bank 1)
C0TCL		sfr	0e3h		; CTR0 Timer Constant, bits 0-7 (Bank 1)
C0TC		sfr	C0TCH

C1CT		sfr	0e1h		; CTR1 Control (Bank 0)
C1EN		__z8bit	C0CT,0		;  Enable CTR1
C1EOC		__z8bit C0CT,1		;  CTR1 End-of-Count
C1ZCIEN		__z8bit	C0CT,2		;  CTR1 Zero Count Interrupt Enable
C1SC		__z8bit	C0CT,3		;  CTR1 Software Capture
C1ST		__z8bit	C0CT,4		;  CTR1 Software Trigger
C1LC		__z8bit	C0CT,5		;  CTR1 Load Counter
C1DIR		__z8bit	C0CT,6		;  CTR1 up/down
C1CONT		__z8bit	C0CT,7		;  CTR1 Continuous/Single Cycle
C1M		sfr	0e1h		; CTR1 Mode (Bank 1)
C1CM		__z8bfield C1M,0,2	;  CTR1 Capture Mode
C1EDGE		__z8bit	 C1M,2		;  CTR1 Edges of P37
C1RETRIG	__z8bit	 C1M,3		;  CTR1 Enable Retrigger
C1ASSIGN	__z8bfield C1M,4,4	;  CTR1 Input Pin Assignments
C1CH		sfr	0e4h		; CTR1 Capture Register, bits 8-15 (Bank 0)
C1CL		sfr	0e5h		; CTR1 Capture Register, bits 0-7 (Bank 0)
C1C		sfr	C1CH
C1TCH		sfr	0e4h		; CTR1 Timer Constant, bits 8-15 (Bank 1)
C1TCL		sfr	0e5h		; CTR1 Timer Constant, bits 0-7 (Bank 1)
C1TC		sfr	C1TCH

;----------------------------------------------------------------------------
; UART

UTC		sfr	0ebh		; UART Transmit Control (Bank 0)
UTDMAEN		__z8bit	UTC,0		;  Transmit DMA Enable
UTBE		__z8bit	UTC,1		;  Transmit Buffer Empty
UZC		__z8bit	UTC,2		;  Zero Count
UTE		__z8bit	UTC,3		;  Transmit Enable
UWUPEN		__z8bit	UTC,4		;  Wake-up Enable
USTOP		__z8bit	UTC,5		;  Stop Bits
UBRK		__z8bit	UTC,6		;  Send Break
UTDSEL		__z8bit	UTC,7		;  Transmit Data Select
URC		sfr	0ech		; UART Receive Control (Bank 0)
URCE		__z8bit	URC,0		;  Receive Character Available
URE		__z8bit	URC,1		;  Receive Enable
UPE		__z8bit	URC,2		;  Parity Error
UOE		__z8bit	URC,3		;  Overrun Error
UFE		__z8bit	URC,4		;  Framing Error
UBRKDET		__z8bit	URC,5		;  Break Detect
UCCDET		__z8bit	URC,6		;  Control Character Detect
UWUPDET		__z8bit	URC,7		;  Wake-Up Detect
UIE		sfr	0edh		; UART Interrupt Enable (Bank 0)
URCAIE		__z8bit	UIE,0		;  Receive Character Available Interrupt Enable
URDMAE		__z8bit	UIE,1		;  Receive DMA Enable
UTIE		__z8bit	UIE,2		;  Transmit Interrupt Enable
UZCIE		__z8bit	UIE,3		;  Zero Count Interrupt Enable
UREIE		__z8bit	UIE,4		;  Receive Error Interrupt Enable
UBRKIE		__z8bit	UIE,5		;  Break Interrupt Enable
UCCIE		__z8bit	UIE,6		;  Control Character Interrupt Enable
UWUPIE		__z8bit	UIE,7		;  Wake-Up Interrupt Enable
UIO		sfr	0efh		; UART Data (Bank 0)
UBGH		sfr	0f8h		; UART Baud Rate Generator, bits 8-15 (Bank 1)
UBGL		sfr	0f9h		; UART Baud Rate Generator, bits 0-7 (Bank 1)
UBG		sfr	UBGH
UMA		sfr	0fah		; UART Mode A (Bank 1)
UTWKUPVAL	__z8bit	UMA,0		;  Transmit Wake-Up Value
URWKUPVAL	__z8bit	UMA,1		;  Receive Wake-Up Value
UPEVEN		__z8bit	UMA,2		;  Even Parity
UPEN		__z8bit	UMA,3		;  Parity Enable
UBPC		__z8bfield UMA,4,2	;  Bits Per Character
UCLK		__z8bfield UMA,6,2	;  Clock Rate
UMB		sfr	0fbh		; UART Mode B (Bank 1)
ULPBKEN		__z8bit	UMB,0		;  Loopback Enable
UBGEN		__z8bit	UMB,1		;  Baud-Rate Generator Enable
UBGSRC		__z8bit	UMB,2		;  Baud-Rate Generator Source
UTXCLKSEL	__z8bit	UMB,3		;  Transmit Clock Input Select
URXCLKSEL	__z8bit	UMB,4		;  Receive Clock Input Select
UAUTOECHO	__z8bit	UMB,5		;  Auto-Echo
UCLKOUSEL	__z8bfield UMB,6,2	;  Clock Output Select

;----------------------------------------------------------------------------
; DMA

DCH		sfr	0f0h		; DMA Count, bits 8-15 (Bank 1)
DCL		sfr	0f1h		; DMA Count, bits 0-7 (Bank 1)
DC		sfr	DCH
H0C		sfr	0f4h		; Handshake Channel 0 Control (Bank 0)
HSEN		__z8bit	H0C,0		;  Handshake Enable
PSEL		__z8bit	H0C,1		;  Port Select
DMAEN		__z8bit	H0C,2		;  DMA Enable
DMAMODE		__z8bit	H0C,3		;  Mode
DESKEWCNT	__z8bfield H0C,4,4	;  Deskew Counter
H1C		sfr	0f5h		; Handshake Channel 1 Control (Bank 0)

;----------------------------------------------------------------------------

		restore

		endif			; __regz88c0xinc
