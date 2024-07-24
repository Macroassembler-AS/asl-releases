		ifndef	__ez80flashinc
__ez80flashinc	equ	1

__defflash       macro   NUM,Base
FLASH_KEY	port	0f5h		; Flash Key Register (w)
FLASH_DATA	port	0f6h		; Flash Data Register (r/w)
FLASH_ADDR_U	port	0f7h		; Flash Address Upper Byte Register (r/w)
FLASH_CTL	port	0f8h		; Flash Control Register (r/w)
FLASH_WAIT_S	equ	5		;  Number of wait states
FLASH_WAIT_M	equ	7 << FLASH_WAIT_S
FLASH_EN	equ	1 << 3		;  Enable flash memory access
FLASH_FDIV	port	0f9h		; Flash Frequency Divider Register (r/w)
FLASH_PROT	port	0fah		; Flash Write/Erase Protection Register (r/w)
FLASH_IRQ	port	0fbh		; Flash Interrupt Control Register (r/w)
DONE_IEN	equ	1 << 7		;  Flash Erase/Row Program Done Interrupt is enabled
ERR_IEN		equ	1 << 6		;  Error Condition Interrupt is enabled
DONE		equ	1 << 5		;  Erase/Row Program Done Flag
WR_VIO		equ	1 << 3		;  Write Violation Error Flag
RP_TMO		equ	1 << 2		;  Row Program Time-Out Error Flag
PG_VIO		equ	1 << 1		;  Page Erase Violation Error Flag
MASS_VIO	equ	1 << 0		;  Mass Erase Violation Error Flag
FLASH_PAGE	port	0fch		; Flash Page Select Register (r/w)
INFO_EN		equ	1 << 7		;  Redirect to info page
FLASH_ROW	port	0fdh		; Flash Row Select Register (r/w)
FLASH_COL	port	0feh		; Flash Column Select Register (r/w)
FLASH_PGCTL	port	0ffh		; Flash Program Control Register (r/w)
ROW_PGM		equ	1 << 2		;  Row Program Enable
PG_ERASE	equ	1 << 1		;  Page Erase Enable
MASS_ERASE	equ	1 << 0		;  Mass Erase Enable
		endm			; __defflash

		endif			; __ez80flashinc
