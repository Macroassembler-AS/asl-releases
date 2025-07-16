                ifndef  regz280inc      ; avoid multiple inclusion
reg280inc       equ     1

                save
                listing off		; no listing over this file
;****************************************************************************
;*                                                                          *
;*   ASl File REGZ280.INC                                                   *
;*   								            *
;*   Contains Register Definitions for the Zilog Z280                       *
;*                                                                          *
;****************************************************************************

                if      (MOMCPUNAME<>"Z280")
                 fatal  "wrong target selected: only Z280 allowed"
		endif


                if      MOMPASS=1
                 message "Z280 Register Definitions (C) 2025 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; Control Registers

MSR		equ	00h		; Master Status Register
ISR		equ	16h		; Interrupt Status Register
IVTP		equ	06h		; Interrupt/Trap Vector Table
IOPR		equ	08h		; I/O Page Register
BTIR		equ	0ffh		; Bus Timing and Initialization Register
BTCR		equ	02h		; Bus Timing and control Register
SLR		equ	04h		; Stack Limit Register
TCR		equ	10h		; Trap Control Register
CCR		equ	12h		; Cache Control Register
LAR		equ	14h		; Local Address Register

;----------------------------------------------------------------------------

		restore                 ; re-allow listing

                endif                   ; reg380inc


