		ifndef	__reg80f92inc
__reg80f92inc	equ	1
		save
		listing	off	; no listing over this file

;****************************************************************************
;*                                                                          *
;*   AS(L) - File REG80F92.INC                                              *
;*                                                                          *
;*   Contains Bit & Register Definitions for Zilog eZ80[F,L]92/eZ80F93      *
;*                                                                          *
;****************************************************************************

		include	"spi.inc"
		include "uart.inc"
		include "i2c.inc"
		include "gpio.inc"
		include "rtc.inc"

;----------------------------------------------------------------------------
; Memory Limits

		switch	MOMCPUNAME
		case	"EZ80F92"
RAMSTART 	 equ	0e000h		; assumes RAM_ADDR_U = 00h
RAMEND		 equ	RAMSTART+1fffh
FLASHSIZE	 equ	20000h
		case	"EZ80F93"
RAMSTART 	 equ	0f000h		; assumes RAM_ADDR_U = 00h
RAMEND		 equ	RAMSTART+0fffh
FLASHSIZE	 equ	10000h
		case	"EZ80L92"
		endcase

;----------------------------------------------------------------------------
; Timer

TMR0_CTL	port	80h		; Timer 0 Control Register (r/w)
PRT_IRQ		equ	1 << 7		;  Timer Interrupt occured
PRT_IRQ_EN	equ	1 << 6		;  Interrupt Enable
PRT_MODE	equ	1 << 4		;  Continuous Mode
CLK_DIV_S	equ	2		;  Clock Divider
CLK_DIV_M	equ	3 << CLK_DIV_S
CLK_DIV_4	equ	0 << CLK_DIV_S	;   Source = Clock/4
CLK_DIV_16	equ	1 << CLK_DIV_S	;   Source = Clock/16
CLK_DIV_64	equ	2 << CLK_DIV_S	;   Source = Clock/64
CLK_DIV_256	equ	3 << CLK_DIV_S	;   Source = Clock/256
RST_EN		equ	1 << 1		;  Automatic Restart Enable
PRT_EN		equ	1 << 0		;  Timer Enable
TMR0_DR_L	port	81h		; Timer 0 Data Register Low (r)
TMR0_RR_L	port	81h		; Timer 0 Reload Register Low (w)
TMR0_DR_H	port	82h		; Timer 0 Data Register High (r)
TMR0_RR_H	port	82h		; Timer 0 Reload Register High (w)

IVEC_PRT0	equ	0ah		; Timer 0 Interrupt Vector

TMR1_CTL	port	83h		; Timer 1 Control Register (r/w)
TMR1_DR_L	port	84h		; Timer 1 Data Register Low (r)
TMR1_RR_L	port	84h		; Timer 1 Reload Register Low (w)
TMR1_DR_H	port	85h		; Timer 1 Data Register High (r)
TMR1_RR_H	port	85h		; Timer 1 Reload Register High (w)

IVEC_PRT1	equ	0ch		; Timer 1 Interrupt Vector

TMR2_CTL	port	86h		; Timer 2 Control Register (r/w)
TMR2_DR_L	port	87h		; Timer 2 Data Register Low (r)
TMR2_RR_L	port	87h		; Timer 2 Reload Register Low (w)
TMR2_DR_H	port	88h		; Timer 2 Data Register High (r)
TMR2_RR_H	port	88h		; Timer 2 Reload Register High (w)

IVEC_PRT2	equ	0eh		; Timer 2 Interrupt Vector

TMR3_CTL	port	89h		; Timer 3 Control Register (r/w)
TMR3_DR_L	port	8ah		; Timer 3 Data Register Low (r)
TMR3_RR_L	port	8ah		; Timer 3 Reload Register Low (w)
TMR3_DR_H	port	8bh		; Timer 3 Data Register High (r)
TMR3_RR_H	port	8bh		; Timer 3 Reload Register High (w)

IVEC_PRT3	equ	10h		; Timer 3 Interrupt Vector

TMR4_CTL	port	8ch		; Timer 4 Control Register (r/w)
TMR4_DR_L	port	8dh		; Timer 4 Data Register Low (r)
TMR4_RR_L	port	8dh		; Timer 4 Reload Register Low (w)
TMR4_DR_H	port	8eh		; Timer 4 Data Register High (r)
TMR4_RR_H	port	8eh		; Timer 4 Reload Register High (w)

IVEC_PRT4	equ	12h		; Timer 4 Interrupt Vector

TMR5_CTL	port	8fh		; Timer 5 Control Register (r/w)
TMR5_DR_L	port	90h		; Timer 5 Data Register Low (r)
TMR5_RR_L	port	90h		; Timer 5 Reload Register Low (w)
TMR5_DR_H	port	91h		; Timer 5 Data Register High (r)
TMR5_RR_H	port	91h		; Timer 5 Reload Register High (w)

IVEC_PRT5	equ	14h		; Timer 5 Interrupt Vector

TMR_ISS		port	92h		; Timer Input Source Select Register
TMR3_IN_S	equ	6		;  Timer 3 Input
TMR3_IN_M	equ	3 << TMR3_IN_S
TMR3_IN_SYS	equ	0 << TMR3_IN_S 	;   system clock
TMR3_IN_RTC	equ	1 << TMR3_IN_S	;   RTC
TMR3_IN_B1	equ	2 << TMR3_IN_S	;   GPIO B1
TMR2_IN_S	equ	6		;  Timer 2 Input
TMR2_IN_M	equ	3 << TMR2_IN_S
TMR2_IN_SYS	equ	0 << TMR2_IN_S 	;   system clock
TMR2_IN_RTC	equ	1 << TMR2_IN_S	;   RTC
TMR2_IN_B1	equ	2 << TMR2_IN_S	;   GPIO B0
TMR1_IN_S	equ	6		;  Timer 1 Input
TMR1_IN_M	equ	3 << TMR1_IN_S
TMR1_IN_SYS	equ	0 << TMR1_IN_S 	;   system clock
TMR1_IN_RTC	equ	1 << TMR1_IN_S	;   RTC
TMR1_IN_B1	equ	2 << TMR1_IN_S	;   GPIO B1
TMR0_IN_S	equ	6		;  Timer 0 Input
TMR0_IN_M	equ	3 << TMR0_IN_S
TMR0_IN_SYS	equ	0 << TMR0_IN_S 	;   system clock
TMR0_IN_RTC	equ	1 << TMR0_IN_S	;   RTC
TMR0_IN_B1	equ	2 << TMR0_IN_S	;   GPIO B0

;----------------------------------------------------------------------------
; Watchdog Timer

WDT_CTL		port	93h		; Watchdog Timer Control Register (r/w)
WDT_EN		equ	1 << 7		; Watchdog Timer Enable
NMI_OUT		equ	1 << 6		;  NMI instead of reset
RST_FLAG	equ	1 << 5		;  Reset caused by WDT
WDT_CLK_S	equ	3		;  Clock Source
WDT_CLK_M	equ	3 << WDT_CLK_S
WDT_CLK_SYS	equ	0 << WDT_CLK_S	;   System Clock
WDT_CLK_RTC	equ	1 << WDT_CLK_S	;   RTC Clock
WDT_PERIOD_S	equ	0		;  WDT Period
WDT_PERIOD_M	equ	3 << WDT_PERIOD_S
WDT_PERIOD_27	equ	0 << WDT_PERIOD_S ; WDT period is 2**27 clock cycles
WDT_PERIOD_25	equ	1 << WDT_PERIOD_S ; WDT period is 2**25 clock cycles
WDT_PERIOD_22	equ	2 << WDT_PERIOD_S ; WDT period is 2**22 clock cycles
WDT_PERIOD_18	equ	3 << WDT_PERIOD_S ; WDT period is 2**18 clock cycles

WDT_RR		port	94h		; Watchdog Timer Reset Register (w)

;----------------------------------------------------------------------------
; General-Purpose Input/Output Ports

		__defgpio "B",9ah

IVEC_PB0	equ	30h		; Port B 0 Interrupt Vector
IVEC_PB1	equ	32h		; Port B 1 Interrupt Vector
IVEC_PB2	equ	34h		; Port B 2 Interrupt Vector
IVEC_PB3	equ	36h		; Port B 3 Interrupt Vector
IVEC_PB4	equ	38h		; Port B 4 Interrupt Vector
IVEC_PB5	equ	3ah		; Port B 5 Interrupt Vector
IVEC_PB6	equ	3ch		; Port B 6 Interrupt Vector
IVEC_PB7	equ	3eh		; Port B 7 Interrupt Vector

		__defgpio "C",9eh

IVEC_PC0	equ	40h		; Port C 0 Interrupt Vector
IVEC_PC1	equ	42h		; Port C 1 Interrupt Vector
IVEC_PC2	equ	44h		; Port C 2 Interrupt Vector
IVEC_PC3	equ	46h		; Port C 3 Interrupt Vector
IVEC_PC4	equ	48h		; Port C 4 Interrupt Vector
IVEC_PC5	equ	4ah		; Port C 5 Interrupt Vector
IVEC_PC6	equ	4ch		; Port C 6 Interrupt Vector
IVEC_PC7	equ	4eh		; Port C 7 Interrupt Vector

		__defgpio "D",0a2h

IVEC_PD0	equ	50h		; Port D 0 Interrupt Vector
IVEC_PD1	equ	52h		; Port D 1 Interrupt Vector
IVEC_PD2	equ	54h		; Port D 2 Interrupt Vector
IVEC_PD3	equ	56h		; Port D 3 Interrupt Vector
IVEC_PD4	equ	58h		; Port D 4 Interrupt Vector
IVEC_PD5	equ	5ah		; Port D 5 Interrupt Vector
IVEC_PD6	equ	5ch		; Port D 6 Interrupt Vector
IVEC_PD7	equ	5eh		; Port D 7 Interrupt Vector

;----------------------------------------------------------------------------
; Chip Select/Wait State Generator

CS0_LBR		port	0a8h		; Chip Select 0 Lower Bound Register (r/w)
CS0_UBR		port	0a9h		; Chip Select 0 Upper Bound Register (r/w)
CS0_CTL		port	0aah		; Chip Select 0 Control Register (r/w)
CS_WAIT_S	equ	5		;  # of wait states
CS_WAIT_M	equ	7 << CS_WAIT_S
CS_IO		equ	1 << 4		; Match I/O range
CS_EN		equ	1 << 3		; Enable Chip Select
CS0_BMC		equ	0f0h		; Chip Select 0 Bus Mode Control Register (r/w)
BUS_MODE_S	equ	6		;  Bus Mode
BUS_MODE_M	equ	3 << BUS_MODE_S
BUS_MODE_EZ80	equ	0 << BUS_MODE_S ;   eZ80 Mode
BUS_MODE_Z80	equ	1 << BUS_MODE_S	;   Z80 Mode
BUS_MODE_INTEL	equ	2 << BUS_MODE_S	;   Intel Mode
BUS_MODE_MOTO	equ	3 << BUS_MODE_S	;   Motorola Mode
AD_MUX		equ	1 << 5		;  Multiplexed Address/Data
BUS_CYCLE_S	equ	0		;  Length of bus cycle
BUS_CYCLE_M	equ	15 << BUS_CYCLE_S

CS1_LBR		port	0abh		; Chip Select 1 Lower Bound Register (r/w)
CS1_UBR		port	0ach		; Chip Select 1 Upper Bound Register (r/w)
CS1_CTL		port	0adh		; Chip Select 1 Control Register (r/w)
CS1_BMC		equ	0f1h		; Chip Select 1 Bus Mode Control Register (r/w)

CS2_LBR		port	0aeh		; Chip Select 2 Lower Bound Register (r/w)
CS2_UBR		port	0afh		; Chip Select 2 Upper Bound Register (r/w)
CS2_CTL		port	0b0h		; Chip Select 2 Control Register (r/w)
CS2_BMC		equ	0f2h		; Chip Select 2 Bus Mode Control Register (r/w)

CS3_LBR		port	0b1h		; Chip Select 3 Lower Bound Register (r/w)
CS3_UBR		port	0b2h		; Chip Select 3 Upper Bound Register (r/w)
CS3_CTL		port	0b3h		; Chip Select 3 Control Register (r/w)
CS3_BMC		equ	0f3h		; Chip Select 3 Bus Mode Control Register (r/w)

;----------------------------------------------------------------------------
; On-Chip RAM Control

		ifdef	RAMSTART
RAM_CTL		port	0b4h		; RAM Control Register (r/w)
RAM_ADDR_U	port	0b5h		; RAM Address Upper Byte (r/w)
		endif

;----------------------------------------------------------------------------
; SPI

SPI_BRG_L	port	0b8h		; SPI Baud Rate Generator Register - Low Byte
SPI_BRG_H	port	0b9h		; SPI Baud Rate Generator Register - High Byte
		__defspi "",0bah

IVEC_SPI	label	1eh		; SPI Interrupt Vector

;----------------------------------------------------------------------------
; Infrared

IR_CTL		port	0bfh		; Infrared Encoder/Decoder Control  (r/w)
MIN_PULSE_S	equ	4		;  Minimum receive pulse width control
MIN_PULSE_M	equ	15 << MIN_PULSE_S
LOOP_BACK	equ	1 << 2		;  Internal LOOP BACK mode
IR_RxEN		equ	1 << 1		;  IR_RxD data is passed to UART0 RxD
IR_EN		equ	1 << 0		;  Endec is disabled

;----------------------------------------------------------------------------
; UART 0

		__defuart "0",0c0h
TCIE		equ	1 << 4		;  UARTx_IER Transmission complete interrupt enable
FSTS_S		equ	6		;  UARTx_IIR FIFO Status
FSTS_M		equ	3 << FSTS_S
FSTS_DIS	equ	0 << FSTS_S	;  FIFO is disabled
FSTS_RXDIS	equ	2 << FSTS_S	;  Receive FIFO is disabled (MULTIDROP mode)
FSTS_EN		equ	3 << FSTS_S	;  FIFO is enabled
INTSTS_TC	equ	5 << INTSTS_S	;  UARTx_IIR Transmission Complete
MDM		equ	1 << 5		;  UARTx_MCR Multidrop Mode enable

IVEC_UART0	label	18h		; UART0 Interrupt Vector

;----------------------------------------------------------------------------
; I2C

		__defi2c "",0c8h

IVEC_I2C	label	1ch		; I2C Interrupt Vector

;----------------------------------------------------------------------------
; UART 1

		__defuart "1",0d0h

IVEC_UART1	label	1ah		; UART1 Interrupt Vector

;----------------------------------------------------------------------------
; Low-Power Control

CLK_PPD1	port	0dbh		; Clock Peripheral Power-Down Register 1 (r/w)
GPIO_D_OFF	equ	1 << 7		;  Power down system clock to GPIO Port D
GPIO_C_OFF	equ	1 << 6		;  Power down system clock to GPIO Port C
GPIO_B_OFF	equ	1 << 5		;  Power down system clock to GPIO Port B
SPI_OFF		equ	1 << 3		;  Power down system clock to SPI
I2C_OFF		equ	1 << 2		;  Power down system clock to I2C
UART1_OFF	equ	1 << 1		;  Power down system clock to UART1
UART0_OFF	equ	1 << 0		;  Power down system clock to UART0 and IrDA endec

CLK_PPD2	port	0dch		; Clock Peripheral Power-Down Register 2 (r/w)
PHI_OFF		equ	1 << 7		;  Disable PHI Clock output
PRT5_OFF	equ	1 << 5		;  Power down system clock to PRT5
PRT4_OFF	equ	1 << 4		;  Power down system clock to PRT4
PRT3_OFF	equ	1 << 3		;  Power down system clock to PRT3
PRT2_OFF	equ	1 << 2		;  Power down system clock to PRT2
PRT1_OFF	equ	1 << 1		;  Power down system clock to PRT1
PRT0_OFF	equ	1 << 0		;  Power down system clock to PRT0

;----------------------------------------------------------------------------
; Real-Time Clock

		__defrtc

IVEC_RTC	label	6ch		; RTC Interrupt Vector

;----------------------------------------------------------------------------
; Flash Controller

		ifdef	FLASHSIZE
		include "flash.inc"
		__defflash

IVEC_FLASH	label	08h
		endif

;----------------------------------------------------------------------------

		restore
		endif			; __reg80f92inc
		