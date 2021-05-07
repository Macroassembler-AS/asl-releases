	cpu	mico8_v3

; from user's manual

	b _start		; 0x33001 110011000000000001
_start:
	nop		; 0x10000 010000000000000000
add:
	movi R00,0x55	; 0x12055 010010000001010101
	movi R01,0x05	; 0x12105 010010000100000101
	movi R02,0x03	; 0x12203 010010001000000011
	add R01,R02	; 0x08110 001000000100010000
	addi R01,0x01	; 0x0A101 001010000100000001
	mov R03,R01	; 0x10308 010000001100001000
	mov R04,R02	; 0x10410 010000010000010000
	movi R05,0x35	; 0x12535 010010010100110101
	movi R06,0x43	; 0x12643 010010011001000011
	add R06,R05	; 0x08628 001000011000101000
	addi R06,0x13	; 0x0A613 001010011000010011
	mov R07,R05	; 0x10728 010000011100101000

; This program will allow user to run a fibonacci number
; generator and updown counter. This program responds to
; the interrupt from the user (through Orcastra).
; When there is an interrupt, the program will halt the current program,
; and execute the int_handler function. When the intr_handler function
; is done, the program will continue from its last position

	b 	int_handler
	nop
	nop
	seti 			; set the program to be able to receive interrupt
	nop
	nop
	b 	start
start:
	import 	r5, 5
	mov 	r6, r5
	andi 	r5, 0xf0	; masking r5 to decide type of program
	mov 	r7, r5
	mov 	r5, r6
	andi 	r5, 0x0f	; masking r5 to get the speed
	mov 	r25, r5
	cmpi 	r7, 0x10
	bz 	phase2
	cmpi 	r7, 0x20
	bz 	phase2
	b 	start
phase2:
	cmpi 	r25, 0x01
	bz 	phase3
	cmpi 	r25, 0x02
	bz 	phase3
	cmpi 	r25, 0x03
	bz 	phase3
	cmpi 	r25, 0x04
	bz 	phase3
	b 	start
phase3:
	cmpi 	r7, 0x10
;	bz 	fibo
	cmpi 	r7, 0x20 	; 1 = fibonacci, 2 = counter
;	bz 	counter
	b 	start
int_handler:
	iret

; examples of instruction classes

	add	r15,r23
	addi	r2,0x12

	rolc	r10,r5

	clri
	setz

	b 	$ + 10

	import	r10,21
	export	r10,21

; register aliases

myreg1e		equ	r15
myreg2e		equ	r14
myreg1r		reg	r15
myreg2r		reg	r14
myreg1re	reg	myreg1e
myreg2re	reg	myreg2e

		add	r14,r15
		add	myreg2e,myreg1e
		add	myreg2re,myreg1r
		add	myreg2re,myreg1re
