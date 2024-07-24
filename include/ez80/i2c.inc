		ifndef	__ez80i2cinc
__ez80i2cinc	equ	1

__defi2c	macro	NUM,Base
I2C{NUM}_SAR	port	Base+0		; I2C n Slave Address Register (r/w)
		if	NUM<>"1"
SLA_S		 equ	1		;  Slave address
SLA_M		 equ	127 << SLA_S
GCE		 equ	1 << 0		;  Recognize General Call Address
		endif
I2C{NUM}_xSAR	port	Base+1		; I2C n Extended Slave Address Register (r/w)
I2C{NUM}_DR	port	Base+2		; I2C n Data Register (r/w)
I2C{NUM}_CTL	port	Base+3		; I2C n Control Register (r/w)
		if	NUM<>"1"
IEN		 equ	1 << 7		;  Interrupt Enable
ENAB		 equ	1 << 6		;  Enable Bus
STA		 equ	1 << 5		;  Send START
STP		 equ	1 << 4		;  Send STOP
IFLG		 equ	1 << 3		;  Interrupt Status
ACK		 equ	1 << 2		;  Acknowledge
		endif
I2C{NUM}_SR	port	Base+4		; I2C n Status Register (r)
		if	NUM<>"1"
STAT_S		 equ	3		;  I2C status
STAT_M		 equ	31 << STAT_S
		endif
I2C{NUM}_CCR	port	Base+4		; I2C n Clock Control Register (w)
I2C{NUM}_SRR	port	Base+5		; I2C n Software Reset Register (w)
		endm			; __defi2c

		endif			; __ez80i2cinc
