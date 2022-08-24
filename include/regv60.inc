		ifndef  regv60inc     	; avoid multiple inclusion
regv60inc     	equ     1
;****************************************************************************
;*                                                                          *
;*   AS File REGV60.INC                                                     *
;*   								            *
;*   Contains Register Definitions for V60...V80 Processors                 *
;* 									    *
;****************************************************************************

		save
                listing off		; no listing over this file

                if      (MOMCPUNAME<>"70616")
                 fatal  "wrong target selected: only 70616 supported"
		endif


                if      MOMPASS=1
		 message "NEC V60 Register Definitions (C) 2022 Alfred Arnold"
		endif

;--------------------------------------------------------------------------
; Privileged Registers

ISP		equ	0	; Interrupt Stack Pointer
L0SP		equ	1	; Level 0 Stack Pointer
L1SP		equ	2	; Level 1 Stack Pointer
L2SP		equ	3	; Level 2 Stack Pointer
L3SP		equ	4	; Level 3 Stack Pointer
SBR		equ	5	; System Base Register
TR		equ	6	; Task Register
SYCW		equ	7	; System Control Word
TKCW		equ	8	; Task Control Word
PIR		equ	9	; Processor ID Register
PSW2		equ	15	; Emulation Mode Status Word
ATBR0		equ	16	; Area Table Base 0 Register
ATLR0		equ	17	; Area Table Length 0 Register
ATBR1		equ	18	; Area Table Base 1 Register
ATLR1		equ	19	; Area Table Length 1 Register
ATBR2		equ	20	; Area Table Base 2 Register
ATLR2		equ	21	; Area Table Length 2 Register
ATBR3		equ	22	; Area Table Base 3 Register
ATLR3		equ	23	; Area Table Length 3 Register
TRMOD		equ	24	; Trap Mode Register
ADTR0		equ	25	; Address Trap Register 0
ADTR1		equ	26	; Address Trap Register 1
ADTRM0		equ	27	; Address Trap Mask Register 0
ADTRM1		equ	28	; Address Trap Mask Register 1

;--------------------------------------------------------------------------

		restore                 ; re-enable listing

                endif			; regv60inc
