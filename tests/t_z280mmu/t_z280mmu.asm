	cpu	z280
	page	0
	supmode	off

	;--------------------
	; default layout: CPU addresses map 1:1 to physical ones, MMU is disabled

	assume	mmumcr:0000h
	dw	phys2cpu(0000h)
	dw	phys2cpu(1000h)
	dw	phys2cpu(2000h)
	dw	phys2cpu(3000h)
	dw	phys2cpu(4000h)
	dw	phys2cpu(5000h)
	dw	phys2cpu(6000h)
	dw	phys2cpu(7000h)
	dw	phys2cpu(8000h)
	dw	phys2cpu(9000h)
	dw	phys2cpu(0a000h)
	dw	phys2cpu(0b000h)
	dw	phys2cpu(0c000h)
	dw	phys2cpu(0d000h)
	dw	phys2cpu(0e000h)
	dw	phys2cpu(0f000h)

	;---------------------
	; enable MMU for user mode: all PDRs have V bit(3) set to 0 -> nothing is addressable

	assume	mmumcr:8000h
	expect	1860
	dw	phys2cpu(0000h)
	endexpect

	;---------------------
	; map lowest 4K 1:1, 4..8K -> 64K

	assume	upd0:0008h,upd1:1008h
	;prwins
	dw	phys2cpu(0123h)		; 0123
	expect	1320
	dw	phys2cpu(1800h)
	endexpect
	dw	phys2cpu(100123h)	; 1123

	;---------------------
	; the same for absolute addressing

	ld	a,(0123h)		; 3A 23 01
	expect	2270
	ld	a,(1800h)
	endexpect
	ld	a,(100123h)		; 3A 23 11

	;---------------------
	; the same for PC-relative addressing

	ld	a,<0123h>		; FD 78 F5 00
	expect	2270
	ld	a,<1800h>
	endexpect
	ld	a,<100123h>		; FD 78 F1 10


	;---------------------
	; JP/CALL with immediate argument is like absolute addressing in code space:

	jp	0123h			; C3 23 01
	expect	2270
	jp	1800h
	endexpect
	jp	100123h			; C3 23 11

	;=====================
	; now separate CODE & DATA space:
	; map lowest 16K in CODE and last 16K in DATA

	assume mmumcr:0c000h,upd0:nothing,upd1:nothing,upd6:0ffc8h,upd7:0ffe8h,upd8:0008h,upd9:0028h
	;prwins

	;---------------------
	; Absolute addressing uses data mapping:

	ld	a,(0ffe234h)		; 3A 34 E2
	expect	2270
	ld	a,(0ff4567h)		; not mapped at all
	endexpect
	expect	2270
	ld	a,(1234h)		; only mapped for CODE accesses
	endexpect

	;---------------------
	; PC-relative addressing uses code mapping:

	expect	2270
	ld	a,<0ffe234h>		; only mapped for DATA accesses
	endexpect
	expect	2270
	ld	a,<0ff4567h>		; not mapped at all
	endexpect
	ld	a,<1234h>		; FD 78 F5 11

	;=====================
	; supervisor mode on: MMU is assumed to be off (bit 11 not set):

	supmode	on

	dw	phys2cpu(9000h)		; 00 90
