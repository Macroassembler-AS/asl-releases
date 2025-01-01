
	cpu     at90s8515
	page    0
	include regavr.inc

	adc     r3,r1

	add     r28,r28

	and     r2,r16

	andi    r19,0xaa

	adiw    r26,14

	asr     r17

	bclr    7

	bld     r0,4

	brbc    1,*

	brbs    6,*

	brcc    next    ; 1   = 01

	brcs    next    ; 0   = 00
next:
	breq    next    ; -1  = 7F

	brge    next    ; -2  = 7E

	brsh    next    ; -3  = 7D

	brid    next    ; -4  = 7C

	brie    next    ; -5  = 7B

	brlo    next    ; -6  = 7A

	brlt    next    ; -7  = 79

	brmi    next    ; -8  = 78

	brne    next    ; -9  = 77

	brhc    next    ; -10 = 76

	brhs    next    ; -11 = 75

	brpl    next    ; -12 = 74

	brtc    next    ; -13 = 73

	brts    next    ; -14 = 72

	brvc    next    ; -15 = 71

	brvs    next    ; -16 = 70

	bset    6

	bst     r1,2

;	call    0x123456

	cbr     r16,0xf0

	cbi     0x12,7

	clc

	cli

	cln

	clh

	clr     r18

	cls

	clt

	clv

	clz

	com     r4

	cp      r4,r19

	cpc     r3,r1

	cpi     r19,3

	cpse    r4,r0

	dec     r17

	eor     r0,r22

	icall

	ijmp

	in      r23,0x34

	inc     r22

;	jmp     0x123456

	ld      r2,x
	ld      r0,x+
	ld      r3,-x

	ld      r1,y
	ld      r0,y+
	ld      r3,-y
	ldd     r4,y+0x33

	ld      r1,z
	ld      r0,z+
	ld      r3,-z
	ldd     r4,z+0x33

	ldi     r30,0xf0

	lds     r2,0xff00

	lpm

	lsl     r0

	lsr     r0

	mov     r16,r0

;        mul     r6,r5

	neg     r11

	nop

	or      r15,r16

	ori     r16,0xf0

	out     0x18,r16

	pop     r13

	push    r14

	rcall   *

	ret

	reti

	rjmp    *

	rol     r15

	ror     r15

	sbc     r3,r1

	sbci    r17,0x4f

	sbi     0x1c,3

	sbic    0x1c,1

	sbis    0x10,3

	sbr     r16,3

	sbrc    r0,7

	sbrs    r0,7

	sec

	sei

	sen

	seh

	ser     r17

	ses

	set

	sev

	sez

	sleep

	st      x,r1
	st      x+,r0
	st      -x,r3

	st      y,r1
	st      y+,r0
	st      -y,r3
	std     y+2,r4

	st      z,r1
	st      z+,r0
	st      -z,r3
	std     z+2,r4

	sts     0xff00,r2

	sub     r13,r12

	subi    r22,0x11

	swap    r1

	tst     r3

	wdr

	packing	off
	data	1,2,3
	data	0
	data	255
	data	-128
	data	65535
	data	-32768
	data	'a'
	data	'ab'
	expect	360
	data	"a"
	endexpect
	data	"ab"
	expect	360
	data	"abc"
	endexpect
	data	"abcd"

	packing	on
	expect	360
	data	1,2,3
	endexpect
	expect	360
	data	0
	endexpect
	expect	360
	data	255
	endexpect
	expect	360
	data	-128
	endexpect
	expect	1320
	data	65535
	endexpect
	expect	1315
	data	-32768
	endexpect
	expect	360
	data	'a'
	endexpect
	expect	1320
	data	'ab'
	endexpect
	expect	360
	data	"a"
	endexpect
	data	"ab"
	expect	360
	data	"abc"
	endexpect
	data	"abcd"

; register aliases

myreg1e		equ	r16
myreg2e		equ	r17
myreg1r		reg	r16
myreg2r		reg	r17
myreg1re	reg	myreg1e
myreg2re	reg	myreg2e


		add	r16,r17
		add	myreg1e,myreg2e
		add	myreg1r,myreg2r
		add	myreg1re,myreg2re
		add	r17,r16
		add	myreg2e,myreg1e
		add	myreg2r,myreg1r
		add	myreg2re,myreg1re

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

;	include "../t_dx/t_dn.inc"
;	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
