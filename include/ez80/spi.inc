		ifndef	__ez80spiinc
__ez80spiinc	equ	1

__defspi	macro	NUM,Base
SPI{NUM}_CTL	port	Base+0		; SPI n Control Register (r/w)
		if	NUM<>"1"
SPI_IRQ_EN	 equ	1 << 7		;  Interrupt enable
SPI_EN		 equ	1 << 5		;  SPI enable
MASTER_EN	 equ	1 << 4		;  Operate as master
CPOL		 equ	1 << 3		;  Clock Polarity
CPHA		 equ	1 << 2		;  Clock Phase
		endif
SPI{NUM}_SR	port	Base+1		; SPI n Status Register (r)
		if	NUM<>"1"
SPIF		 equ	1 << 7		;  SPI data transfer completed
WCOL		 equ	1 << 6		;  Write Collision
MODF		 equ	1 << 4		;  Mode Fault
		endif
SPI{NUM}_RBR	port	Base+2		; SPI n Receive Buffer Register (r)
SPI{NUM}_TSR	port	Base+2		; SPI n Transmit Shift Register (w)
		endm			; __defspi

		endif			; __ez80spiinc
