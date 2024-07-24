		ifndef	__reg80190inc
__reg80190inc	equ	1
		save
		listing	off	; no listing over this file

;****************************************************************************
;*                                                                          *
;*   AS(L) - File REG80190.INC                                              *
;*                                                                          *
;*   Contains Bit & Register Definitions for Zilog eZ80190                  *
;*                                                                          *
;****************************************************************************

		include	"spi.inc"
		include "uart.inc"
		include "i2c.inc"
		include "gpio.inc"

;----------------------------------------------------------------------------
; Memory Limits

RAMSTART	equ	0e000h		; assumes RAM_ADDR_U = 00h
RAMEND		equ	RAMSTART+1fffh

;----------------------------------------------------------------------------
; Timer

TMR0_CTL	port	80h		; Timer 0 Control Register (r/w)
PRT_IRQ		equ	1 << 7		;  Timer Interrupt occured
PRT_IRQ_EN	equ	1 << 6		;  Interrupt Enable
PRT_MODE	equ	1 << 4		;  Continuous Mode
CLK_DIV_S	equ	2		;  Clock Divider
CLK_DIV_M	equ	3 << CLK_DIV_S
CLK_DIV_2	equ	0 << CLK_DIV_S	;   Source = Clock/2
CLK_DIV_4	equ	1 << CLK_DIV_S	;   Source = Clock/4
CLK_DIV_8	equ	2 << CLK_DIV_S	;   Source = Clock/8
CLK_DIV_16	equ	3 << CLK_DIV_S	;   Source = Clock/16
RST_EN		equ	1 << 1		;  Automatic Restart Enable
PRT_EN		equ	1 << 0		;  Timer Enable
TMR0_DR_L	port	81h		; Timer 0 Data Register Low (r)
TMR0_RR_L	port	81h		; Timer 0 Reload Register Low (w)
TMR0_DR_H	port	82h		; Timer 0 Data Register High (r)
TMR0_RR_H	port	82h		; Timer 0 Reload Register High (w)

IVEC_PRT0	equ	06h		; Timer 0 Interrupt Vector

TMR1_CTL	port	83h		; Timer 1 Control Register (r/w)
TMR1_DR_L	port	84h		; Timer 1 Data Register Low (r)
TMR1_RR_L	port	84h		; Timer 1 Reload Register Low (w)
TMR1_DR_H	port	85h		; Timer 1 Data Register High (r)
TMR1_RR_H	port	85h		; Timer 1 Reload Register High (w)

IVEC_PRT1	equ	08h		; Timer 1 Interrupt Vector

TMR2_CTL	port	86h		; Timer 2 Control Register (r/w)
TMR2_DR_L	port	87h		; Timer 2 Data Register Low (r)
TMR2_RR_L	port	87h		; Timer 2 Reload Register Low (w)
TMR2_DR_H	port	88h		; Timer 2 Data Register High (r)
TMR2_RR_H	port	88h		; Timer 2 Reload Register High (w)

IVEC_PRT2	equ	0ah		; Timer 2 Interrupt Vector

TMR3_CTL	port	89h		; Timer 3 Control Register (r/w)
TMR3_DR_L	port	8ah		; Timer 3 Data Register Low (r)
TMR3_RR_L	port	8ah		; Timer 3 Reload Register Low (w)
TMR3_DR_H	port	8bh		; Timer 3 Data Register High (r)
TMR3_RR_H	port	8bh		; Timer 3 Reload Register High (w)

IVEC_PRT3	equ	0ch		; Timer 3 Interrupt Vector

TMR4_CTL	port	8ch		; Timer 4 Control Register (r/w)
TMR4_DR_L	port	8dh		; Timer 4 Data Register Low (r)
TMR4_RR_L	port	8dh		; Timer 4 Reload Register Low (w)
TMR4_DR_H	port	8eh		; Timer 4 Data Register High (r)
TMR4_RR_H	port	8eh		; Timer 4 Reload Register High (w)

IVEC_PRT4	equ	0eh		; Timer 4 Interrupt Vector

TMR5_CTL	port	8fh		; Timer 5 Control Register (r/w)
TMR5_DR_L	port	90h		; Timer 5 Data Register Low (r)
TMR5_RR_L	port	90h		; Timer 5 Reload Register Low (w)
TMR5_DR_H	port	91h		; Timer 5 Data Register High (r)
TMR5_RR_H	port	91h		; Timer 5 Reload Register High (w)

IVEC_PRT5	equ	10h		; Timer 5 Interrupt Vector

;----------------------------------------------------------------------------
; Watchdog Timer

WDT_CTL		port	93h		; Watchdog Timer Control Register (r/w)
WDT_EN		equ	1 << 7		; Watchdog Timer Enable
NMI_OUT		equ	1 << 6		;  NMI instead of reset
RST_FLAG	equ	1 << 5		;  Reset caused by WDT
WDT_PERIOD_S	equ	0		;  WDT Period
WDT_PERIOD_M	equ	3 << WDT_PERIOD_S
WDT_PERIOD_27	equ	0 << WDT_PERIOD_S ; WDT period is 2**27 clock cycles
WDT_PERIOD_25	equ	1 << WDT_PERIOD_S ; WDT period is 2**25 clock cycles
WDT_PERIOD_22	equ	2 << WDT_PERIOD_S ; WDT period is 2**22 clock cycles
WDT_PERIOD_18	equ	3 << WDT_PERIOD_S ; WDT period is 2**18 clock cycles

WDT_RR		port	94h		; Watchdog Timer Reset Register (w)

;----------------------------------------------------------------------------
; General-Purpose Input/Output Ports

		__defgpio "A",96h

IVEC_PA0	equ	16h		; Port A 0 Interrupt Vector
IVEC_PA1	equ	18h		; Port A 1 Interrupt Vector
IVEC_PA2	equ	1ah		; Port A 2 Interrupt Vector
IVEC_PA3	equ	1ch		; Port A 3 Interrupt Vector
IVEC_PA4	equ	1eh		; Port A 4 Interrupt Vector
IVEC_PA5	equ	20h		; Port A 5 Interrupt Vector
IVEC_PA6	equ	22h		; Port A 6 Interrupt Vector
IVEC_PA7	equ	24h		; Port A 7 Interrupt Vector

		__defgpio "B",9ah

IVEC_PB0	equ	26h		; Port B 0 Interrupt Vector
IVEC_PB1	equ	28h		; Port B 1 Interrupt Vector
IVEC_PB2	equ	2ah		; Port B 2 Interrupt Vector
IVEC_PB3	equ	2ch		; Port B 3 Interrupt Vector
IVEC_PB4	equ	2eh		; Port B 4 Interrupt Vector
IVEC_PB5	equ	30h		; Port B 5 Interrupt Vector
IVEC_PB6	equ	32h		; Port B 6 Interrupt Vector
IVEC_PB7	equ	34h		; Port B 7 Interrupt Vector

		__defgpio "C",9eh

IVEC_PC0	equ	36h		; Port C 0 Interrupt Vector
IVEC_PC1	equ	38h		; Port C 1 Interrupt Vector
IVEC_PC2	equ	3ah		; Port C 2 Interrupt Vector
IVEC_PC3	equ	3ch		; Port C 3 Interrupt Vector
IVEC_PC4	equ	3eh		; Port C 4 Interrupt Vector
IVEC_PC5	equ	40h		; Port C 5 Interrupt Vector
IVEC_PC6	equ	42h		; Port C 6 Interrupt Vector
IVEC_PC7	equ	44h		; Port C 7 Interrupt Vector

		__defgpio "D",0a2h

IVEC_PD0	equ	46h		; Port D 0 Interrupt Vector
IVEC_PD1	equ	48h		; Port D 1 Interrupt Vector
IVEC_PD2	equ	4ah		; Port D 2 Interrupt Vector
IVEC_PD3	equ	4ch		; Port D 3 Interrupt Vector
IVEC_PD4	equ	4eh		; Port D 4 Interrupt Vector
IVEC_PD5	equ	50h		; Port D 5 Interrupt Vector
IVEC_PD6	equ	52h		; Port D 6 Interrupt Vector
IVEC_PD7	equ	54h		; Port D 7 Interrupt Vector

;----------------------------------------------------------------------------
; Chip Select/Wait State Generator

CS0_LBR		port	0a8h		; Chip Select 0 Lower Bound Register (r/w)
CS0_UBR		port	0a9h		; Chip Select 0 Upper Bound Register (r/w)
CS0_CTL		port	0aah		; Chip Select 0 Control Register (r/w)
CS_WAIT_S	equ	5		;  # of wait states
CS_WAIT_M	equ	7 << CS_WAIT_S
CS_IO		equ	1 << 4		; Match I/O range
CS_EN		equ	1 << 3		; Enable Chip Select

CS1_LBR		port	0abh		; Chip Select 1 Lower Bound Register (r/w)
CS1_UBR		port	0ach		; Chip Select 1 Upper Bound Register (r/w)
CS1_CTL		port	0adh		; Chip Select 1 Control Register (r/w)

CS2_LBR		port	0aeh		; Chip Select 2 Lower Bound Register (r/w)
CS2_UBR		port	0afh		; Chip Select 2 Upper Bound Register (r/w)
CS2_CTL		port	0b0h		; Chip Select 2 Control Register (r/w)

CS3_LBR		port	0b1h		; Chip Select 3 Lower Bound Register (r/w)
CS3_UBR		port	0b2h		; Chip Select 3 Upper Bound Register (r/w)
CS3_CTL		port	0b3h		; Chip Select 3 Control Register (r/w)

;----------------------------------------------------------------------------
; On-Chip RAM Control

RAM_CTL		port	0b4h		; RAM Control Register (r/w)
RAM_ADDR_U	port	0b5h		; RAM Address Upper Byte (r/w)

;----------------------------------------------------------------------------
; Universal Zilog Interface Blocks

		__defspi "0",0b6h

		__defspi "1",0bah

		__defuart "0",0c0h
BRG0_DLR_L	port	UART0_BRG_L	; Alias names
BRG0_DLR_H	port	UART0_BRG_H

		__defi2c "0",0c8h

UZI0_CTL	port	0cfh		; UZI 0 Control Register (r/w)
UZI_MODE_S	equ	0		;  Mode
UZI_MODE_M	equ	3 << UZI_MODE_S
UZI_MODE_DIS	equ	0 << UZI_MODE_S	;   All modes disabled
UZI_MODE_UART	equ	1 << UZI_MODE_S ;   UART mode
UZI_MODE_SPI	equ	2 << UZI_MODE_S ;   SPI mode
UZI_MODE_I2C	equ	3 << UZI_MODE_S	;   I2C mode

IVEC_UZI0	equ	12h		; UZI 0 Interrupt Vector

		__defuart "1",0d0h
BRG1_DLR_L	port	UART1_BRG_L	; Alias names
BRG1_DLR_H	port	UART1_BRG_H

		__defi2c "1",0d8h

UZI1_CTL	port	0dfh		; UZI 1 Control Register (r/w)

IVEC_UZI1	equ	14h		; UZI 1 Interrupt Vector

;----------------------------------------------------------------------------
; Multiply-Accumulator

MACC_xSTART	port	0e0h		; Multiply-Accumulator x Starting Address Register (r/w)
MACC_xEND	port	0e1h		; Multiply-Accumulator x Ending Address Register (r/w)
MACC_xRELOAD	port	0e2h		; Multiply-Accumulator x Reload Register (r/w)
MACC_LENGTH	port	0e3h		; Multiply-Accumulator Length Register (r/w)
MACC_ySTART	port	0e4h		; Multiply-Accumulator y Starting Address Register (r/w)
MACC_yEND	port	0e5h		; Multiply-Accumulator y Ending Address Register (r/w)
MACC_yRELOAD	port	0e6h		; Multiply-Accumulator y Reload Register (r/w)
MACC_CTL	port	0e7h		; Multiply-Accumulator Control Register (r/w)
MACC_AC0	port	0e8h		; Multiply-Accumulator Byte 0 Register (r/w)
MACC_AC1	port	0e9h		; Multiply-Accumulator Byte 1 Register (r/w)
MACC_AC2	port	0eah		; Multiply-Accumulator Byte 2 Register (r/w)
MACC_AC3	port	0ebh		; Multiply-Accumulator Byte 3 Register (r/w)
MACC_AC4	port	0ech		; Multiply-Accumulator Byte 4 Register (r/w)
MACC_STAT	port	0edh		; Multiply-Accumulator Status Register (r/w)
BANK		equ	1 << 4		;  Select CALC/DATA Bank
CALC_STAT_S	equ	2		;  CALC Bank Status
CALC_STAT_M	equ	3 << CALC_STAT_S
CALC_STAT_EMPTY	equ	0 << CALC_STAT_S
CALC_STAT_PROGRESS equ	2 << CALC_STAT_S
CALC_STAT_DONE	equ	3 << CALC_STAT_S
DATA_STAT_S	equ	0		;  DATA Bank Status
DATA_STAT_M	equ	3 << DATA_STAT_S
DATA_STAT_EMPTY	equ	0 << DATA_STAT_S
DATA_STAT_READY equ	1 << DATA_STAT_S
DATA_STAT_DONE	equ	3 << DATA_STAT_S

IVEC_MACC	equ	00h		; MACC Interrupt Vector

;----------------------------------------------------------------------------
; DMA Controllers

DMA0_SAR_L	port	0eeh		; DMA0 Source Address Register - Low Byte Register (r/w)
DMA0_SAR_H	port	0efh		; DMA0 Source Address Register - High Byte Register (r/w)
DMA0_SAR_U	port	0f0h		; DMA0 Source Address Register - Upper Byte Register (r/w)
DMA0_DAR_L	port	0f1h		; DMA0 Destination Address Register - Low Byte Register (r/w)
DMA0_DAR_H	port	0f2h		; DMA0 Destination Address Register - High Byte Register (r/w)
DMA0_DAR_U	port	0f3h		; DMA0 Destination Address Register - Upper Byte Register (r/w)
DMA0_BC_L	port	0f4h		; DMA0 Byte Count Register - Low Byte (r/w)
DMA0_BC_H	port	0f5h		; DMA0 Byte Count Register - High Byte (r/w)
DMA0_CTL	port	0f6h		; DMA0 Control Register (r/w)
DMA_EN		equ	1 << 7		;  Enable DMA channel
IRQ_DMA		equ	1 << 6		;  Enable Interrupt
BURST		equ	1 << 4		;  Use Burst Mode
DAR_CTL_S	equ	2		;  Destination Address:
DAR_CTL_M	equ	3 << DAR_CTL_S
DAR_CTL_UNCH	equ	0 << DAR_CTL_S	;  unchanged after transfer
DAR_CTL_INC	equ	1 << DAR_CTL_S	;  increment after transfer
DAR_CTL_DEC	equ	2 << DAR_CTL_S	;  decrement after transfer
SAR_CTL_S	equ	2		;  Source Address:
SAR_CTL_M	equ	3 << SAR_CTL_S
SAR_CTL_UNCH	equ	0 << SAR_CTL_S	;  unchanged after transfer
SAR_CTL_INC	equ	1 << SAR_CTL_S	;  increment after transfer
SAR_CTL_DEC	equ	2 << SAR_CTL_S	;  decrement after transfer

IVEC_DMA0	equ	02h		; DMA0 Interrupt Vector

DMA1_SAR_L	port	0f7h		; DMA1 Source Address Register - Low Byte Register (r/w)
DMA1_SAR_H	port	0f8h		; DMA1 Source Address Register - High Byte Register (r/w)
DMA1_SAR_U	port	0f9h		; DMA1 Source Address Register - Upper Byte Register (r/w)
DMA1_DAR_L	port	0fah		; DMA1 Destination Address Register - Low Byte Register (r/w)
DMA1_DAR_H	port	0fbh		; DMA1 Destination Address Register - High Byte Register (r/w)
DMA1_DAR_U	port	0fch		; DMA1 Destination Address Register - Upper Byte Register (r/w)
DMA1_BC_L	port	0fdh		; DMA1 Byte Count Register - Low Byte (r/w)
DMA1_BC_H	port	0feh		; DMA1 Byte Count Register - High Byte (r/w)
DMA1_CTL	port	0ffh		; DMA1 Control Register (r/w)

IVEC_DMA1	equ	04h		; DMA0 Interrupt Vector

;----------------------------------------------------------------------------

		restore
		endif			; __reg80190inc
		