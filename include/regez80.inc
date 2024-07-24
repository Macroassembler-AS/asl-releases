                ifndef  __regez80inc     ; avoid multiple inclusion
__regez80inc	equ     1

                save
                listing off   ; kein Listing ueber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.42 - File REGAVR.INC                                              *
;*                                                                          *
;*   Sinn : contains SFR and Bit Definitions for eZ80 Processors            *
;*                                                                          *
;****************************************************************************

;----------------------------------------------------------------------------
; include proper CPU-specific register definitions

                switch  MOMCPUNAME
		case	"EZ80190"
		  include "ez80/reg80190.inc"

		case	"EZ80F91"
		  include "ez80/reg80f91.inc"

		case	"EZ80F92","EZ80F93","EZ80L92"
		  include "ez80/reg80f92.inc"

		elsecase
		  error "wrong processor type set: only eZ80190, eZ80L92, eZ80F91, eZ80F92, or eZ80F93 allowed!"
		endcase

		if	MOMPASS=1
		  message "Zilog eZ80 Register Definitions (C) 2024 Alfred Arnold"
		endif

;----------------------------------------------------------------------------

		restore			; allow again

		endif			; __regez80inc
