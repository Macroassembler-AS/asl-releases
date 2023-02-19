	ifndef	ebcdicinc
ebcdicinc equ	1

	; this is a file to include all defined EBCDIC
	; codepage variations:

	include	"cp037.inc"	; Code Page 037
	include	"cp5110.inc"	; EBCDIC variant on IBM5110

	endif		; ebcdicinc
