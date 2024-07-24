		ifndef	__ez80rtcinc
__ez80rtcinc	equ	1

__defrtc       macro   NUM,Base
RTC_SEC		port	0e0h		; RTC Seconds Register (r/w)
RTC_MIN		port	0e1h		; RTC Minutes Register (r/w)
RTC_HRS		port	0e2h		; RTC Hours Register (r/w)
RTC_DOW		port	0e3h		; RTC Day-of-the-Week Register (r/w)
RTC_DOM		port	0e4h		; RTC Day-of-the-Month Register (r/w)
RTC_MON		port	0e5h		; RTC Month Register (r/w)
RTC_YR		port	0e6h		; RTC Year Register (r/w)
RTC_CEN		port	0e7h		; RTC Century Register (r/w)
RTC_ASEC	port	0e8h		; RTC Alarm Seconds Register (r/w)
RTC_AMIN	port	0e9h		; RTC Alarm Minutes Register (r/w)
RTC_AHRS	port	0eah		; RTC Alarm Hours Register (r/w)
RTC_ADOW	port	0ebh		; RTC Alarm Day-of-the-Week Register (r/w)
RTC_ACTRL	port	0ech		; RTC Alarm Control Register (r/w)
ADOW_EN		equ	1 << 3		;  Enable day-of-the-week alarm
AHRS_EN		equ	1 << 2		;  Enable hours alarm
AMIN_EN		equ	1 << 1		;  Enable minutes alarm
ASEC_EN		equ	1 << 0		;  Enable seconds alarm
RTC_CTRL	port	0edh		; RTC Control Register (r/w)
ALARM		equ	1 << 7		;  Alarm interrupt is active
INT_EN		equ	1 << 6		;  Interrupt on alarm condition is enabled
BCD_EN		equ	1 << 5		;  RTC count and alarm value registers are BCD
CLK_SEL		equ	1 << 4		;  RTC clock source is power-line frequency input
FREQ_SEL	equ	1 << 3		;  Power-line frequency is 50 Hz
SLP_WAKE	equ	1 << 1		;  RTC Alarm generated a sleep-mode recovery reset
RTC_UNLOCK	equ	1 << 0		;  Unlock RTC count registers
		endm			; __defrtc

		endif			; __ez80rtcinc
