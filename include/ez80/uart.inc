		ifndef	__ez80uartinc
__ez80uartinc	equ	1

__defuart       macro   NUM,Base
UART{NUM}_RBR	port	Base+0		; UART n Receive Buffer Register (r)
UART{NUM}_THR	port	Base+0		; UART n Transmit Holding Register (w)
UART{NUM}_BRG_L	port	Base+0		; UART n Baud Rate Generator - Low Byte (r/w)
UART{NUM}_BRG_H	port	Base+1		; UART n Baud Rate Generator - High Byte (r/w)
UART{NUM}_IER	port	Base+1		; UART n Interrupt Enable Register (r/w)
		if	NUM == "0"
MIIE		 equ	1 << 3		;  Enable edge detect modem status interrupt
LSIE		 equ	1 << 2		;  Enable line status interrupt
TIE		 equ	1 << 1		;  Enable transmit interrupt
RIE		 equ	1 << 0		;  Enable receive interrupt 
		endif
UART{NUM}_IIR	port	Base+2		; UART n Interrupt Identification Register (r)
		if	NUM == "0"
INTSTS_S	 equ	1		;  Interrupt Status Code
INTSTS_M	 equ	7 << INTSTS_S
INTSTS_RLS	 equ	3 << INTSTS_S	;   Receiver Line Status
INTSTS_RDR	 equ	2 << INTSTS_S	;   Receiver Data Ready
INTSTS_CTO	 equ	6 << INTSTS_S	;   Character Time out
INTSTS_TBE	 equ	1 << INTSTS_S	;   Transmit buffer empty
INTSTS_MS	 equ	0 << INTSTS_S	;   Modem Status
INTBIT		 equ	1 << 0		;  Active interrupt source
		endif
UART{NUM}_FCTL	port	Base+2		; UART n FIFO Control Register (w)
		if	NUM == "0"
TRIG_S		 equ	6		;  Receive FIFO Trigger Level
TRIG_M		 equ	3 << TRIG_S
CLRTXF		 equ	1 << 2		;  Clear Tx FIFO
CLRRXF		 equ	1 << 1		;  Clear Rx FIFO
FIFOEN		 equ	1 << 0		;  Enable FIFOs
		endif
UART{NUM}_LCTL	port	Base+3		; UART n Line Control Register (r/w)
		if	NUM == "0"
DLAB		 equ	1 << 7		;  Access baud rate registers
SB		 equ	1 << 6		;  Send break
FPE		 equ	1 << 5		;  Force parity error
EPS		 equ	1 << 4		;  Use even parity
PEN		 equ	1 << 3		;  Parity enable
CHAR_S		 equ	0		;  Data bit count
CHAR_M		 equ	7 << CHAR_S
CHAR_5_1	 equ	0 << CHAR_S	;  5x1
CHAR_6_1	 equ	1 << CHAR_S	;  6x1
CHAR_7_1	 equ	2 << CHAR_S	;  7x1
CHAR_8_1	 equ	3 << CHAR_S	;  8x1
CHAR_5_2	 equ	4 << CHAR_S	;  5x2
CHAR_6_2	 equ	5 << CHAR_S	;  6x2
CHAR_7_2	 equ	6 << CHAR_S	;  7x2
CHAR_8_2	 equ	7 << CHAR_S	;  8x2
		endif
UART{NUM}_MCTL	port	Base+4		; UART n Modem Control Register (r/w)
		if	NUM == "0"
LOOP		 equ	1 << 4		;  Enable Loopback
OUT2		 equ	1 << 3		;  DCD in loopback mode
OUT1		 equ	1 << 2		;  RI in loopback mode
RTS		 equ	1 << 1		;  set RTS
DTR		 equ	1 << 0		;  set DTR
		endif
UART{NUM}_LSR	port	Base+5		; UART n Line Status Register (r)
		if	NUM == "0"
ERR		 equ	1 << 7		;  Error detected in FIFO
TEMPT		 equ	1 << 6		;  Transmitter empty
THRE		 equ	1 << 5		;  Transmitter Holding Register Empty
BI		 equ	1 << 4		;  Break Indication
FE		 equ	1 << 3		;  Framing Error
PE		 equ	1 << 2		;  Parity Error
OE		 equ	1 << 1		;  Overrun Error
DR		 equ	1 << 0		;  Data Ready
		endif
UART{NUM}_MSR	port	Base+6		; UART n Modem Status Register (r)
		if	NUM == "0"
DCD		 equ	1 << 7		;  Carrier Detect
RI		 equ	1 << 6		;  Ring Indicator
DSR		 equ	1 << 5		;  Data Set Ready
CTS		 equ	1 << 4		;  Data Carrier Detect
DDCD		 equ	1 << 3		;  DCD status change
TERI		 equ	1 << 2		;  RI trailing edge
DDSR		 equ	1 << 1		;  DSR status change
DCTS		 equ	1 << 0		;  CTS status change
		endif
UART{NUM}_SPR	port	Base+7		; UART n Scratch Pad Register (r/w)
		endm			; __defuart

		endif			; __ez80uartinc
