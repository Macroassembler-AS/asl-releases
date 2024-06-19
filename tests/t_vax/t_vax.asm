		cpu	vax-11/780
		page	0
		accmode	kernel

pcr		equ	pc
spr		equ	sp
fpr		equ	fp
apr		equ	ap

r_0		equ	r0
r_1		equ	r1
r_2		equ	r2
r_3		equ	r3
r_4		equ	r4
r_5		equ	r5
r_6		equ	r6
r_7		equ	r7
r_8		equ	r8
r_9		equ	r9
r_10		equ	r10
r_11		equ	r11
r_12		equ	r12
r_13		equ	r13
r_14		equ	r14
r_15		equ	r15

only_reg_x	macro	op,code
		op	r4		; code 54
		endm

only_mem_x	macro	op,code
		op	@r4		; code 64
		op	(r4)		; code 64
		op	@(r4)		; code B4 00
		op	-(r4)		; code 74
		op	(r4)+		; code 84
		op	@(r4)+		; code 94
		op	4(r4)		; code A4 04
		op	@4(r4)		; code B4 04
		op	b^4(r4)		; code A4 04
		op	@b^4(r4)	; code B4 04
		op	w^4(r4)		; code C4 04 00
		op	@w^4(r4)	; code D4 04 00
		op	l^4(r4)		; code E4 04 00 00 00
		op	@l^4(r4)	; code F4 04 00 00 00
		expect	1107,1107
		op	s^4(r4)		; invalid attribute
		op	@s^4(r4)	; invalid attribute
		endexpect
		op	444(r4)		; code C4 BC 01
		op	@444(r4)	; code D4 BC 01
		expect	1320,1320
		op	b^444(r4)	; out of range
		op	@b^444(r4)	; out of range
		endexpect
		op	w^444(r4)	; code C4 BC 01
		op	@w^444(r4)	; code D4 BC 01
		op	l^444(r4)	; code E4 BC 01 00 00
		op	@l^444(r4)	; code F4 BC 01 00 00
		op	444444(r4)	; code E4 1C C8 06 00
		op	@444444(r4)	; code F4 1C C8 06 00
		expect	1320,1320,1320,1320
		op	b^444444(r4)
		op	@b^444444(r4)
		op	w^444444(r4)
		op	@w^444444(r4)
		endexpect
		op	l^444444(r4)	; code E4 1C C8 06 00
		op	@l^444444(r4)	; code F4 1C C8 06 00
		op	@#44		; code 9F 2C 00 00 00
		op	@#444		; code 9F BC 01 00 00
		op	@#444444	; code 9F 1C C8 06 00
		op	*+44		; code AF 29
		op	b^*+44		; code AF 29
		op	w^*+44		; code CF 28 00
		op	l^*+44		; code EF 26 00 00 00
		op	*+444		; code CF B8 01
		expect 1320
		op	b^*+444		; out of range
		endexpect
		op	w^*+444		; code CF B8 01
		op	l^*+444		; code EF B6 01 00 00
		op	*+444444	; code EF 16 C8 06 00
		expect	1320,1320
		op	b^*+444444	; out of range
		op	w^*+444444	; out of range
		endexpect
		op	l^*+444444	; code EF 16 C8 06 00
		expect	1107
		op	s^*+444444	; invalid attribute
		endexpect
		expect	1350
		op	r4[r6]	; not allowed
		endexpect
		op	(r4)[r6]	; code 46 64
		op	(r4)+[r6]	; code 46 84
		op	@(r4)+[r6]	; code 46 94
		op	-(r4)[r6]	; code 46 74
		op	@#444[r6]	; code 46 9F BC 01 00 00
		op	@4(r4)[r6]	; code 46 B4 04
		op	*+44[r6]	; code 46 AF 28
		op	*+444[r6]	; code 46 CF B7 01
		op	*+444444[r6]	; code 46 EF 15 C8 06 00
		endm

no_imm_x	macro	op, code
		only_reg_x op, code
		only_mem_x op, code
		endm

all_b		macro	op, code
		no_imm_x op, code
		op	#4		; code 04
		op	s^#4		; code 04
		op	i^#4		; code 8F 04
		op	#88		; code 8F 58
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#88		; out of range 0..63
		endexpect
		op	i^#88		; code 8F 58
		endm

all_w		macro	op, code
		no_imm_x op, code
		op	#4		; code 04
		op	s^#4		; code 04
		op	i^#4		; code 8F 04
		op	#444		; code 8F BC 01
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#444		; out of range 0..63
		endexpect
		op	i^#444		; code 8F BC 01
		endm

all_l		macro	op, code
		no_imm_x op, code
		op	#4		; code 04
		op	s^#4		; code 04
		op	i^#4		; code 8F 04
		op	#444444		; code 8F 16 C8 06 00
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#444444	; out of range 0..63
		endexpect
		op	i^#444444	; code 8F 16 C8 06 00
		endm

all_f		macro	op, code
		no_imm_x op, code
		op	#1.5		; code 0C
		op	s^#1.5		; code 0C
		op	i^#1.5		; code 8F C0 40 00 00
		op	#1000.5		; code 8F 7A 45 00 20
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		op	i^#1000.5	; code 8F 7A 45 00 20
		endm

all_d		macro	op, code
		no_imm_x op, code
		op	#1.5		; code 0C
		op	s^#1.5		; code 0C
		op	i^#1.5		; code 8F C0 40 00 00 00 00 00 00
		op	#1000.5		; code 8F 7A 45 00 20 00 00 00 00
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		op	i^#1000.5	; code 8F 7A 45 00 20 00 00 00 00
		endm

all_g		macro	op, code
		no_imm_x op, code
		op	#1.5		; code 0C
		op	s^#1.5		; code 0C
		op	i^#1.5		; code 8F 18 40 00 00 00 00 00 00
		op	#1000.5		; code 8F AF 40 00 44 00 00 00 00
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		op	i^#1000.5	; code 8F AF 40 00 44 00 00 00 00
		endm

all_h		macro	op, code
		no_imm_x op, code
		op	#1.5		; code 0C
		op	s^#1.5		; code 0C
		op	i^#1.5		; code 8F 01 40 00 80 00 00 00 00 00 00 00 00 00 00 00 00
		op	#1000.5		; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		op	i^#1000.5	; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00
		endm

all_no_imm_b	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#4		; invalid addressing mode
		op	s^#4		; invalid addressing mode
		op	i^#4		; invalid addressing mode
		op	#88		; invalid addressing mode
		endexpect
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#88		; out of range 0..63
		endexpect
		expect	1350
		op	i^#88		; invalid addressing mode
		endexpect
		endm

all_no_imm_w	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#4		; invalid addressing mode
		op	s^#4		; invalid addressing mode
		op	i^#4		; invalid addressing mode
		op	#444		; invalid addressing mode
		endexpect
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#444		; out of range 0..63
		endexpect
		expect	1350
		op	i^#444		; invalid addressing mode
		endexpect
		endm

all_no_imm_l	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#4		; invalid addressing mode
		op	s^#4		; invalid addressing mode
		op	i^#4		; invalid addressing mode
		op	#444444		; invalid addressing mode
		endexpect
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#444444	; out of range 0..63
		endexpect
		expect	1350
		op	i^#444444	; invalid addressing mode
		endexpect
		endm

all_no_imm_q	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#4		; invalid addressing mode
		op	s^#4		; invalid addressing mode
		op	i^#4		; invalid addressing mode
		op	#44444444	; invalid addressing mode
		endexpect
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#44444444	; out of range 0..63
		endexpect
		expect	1350
		op	i^#44444444	; invalid addressing mode
		endexpect
		endm

all_no_imm_o	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#4		; invalid addressing mode
		op	s^#4		; invalid addressing mode
		op	i^#4		; invalid addressing mode
		op	#44444444	; invalid addressing mode
		endexpect
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#44444444	; out of range 0..63
		endexpect
		expect	1350
		op	i^#44444444	; invalid addressing mode
		endexpect
		endm

all_no_imm_f	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#1.5		; invalid addressing mode
		op	s^#1.5		; invalid addressing mode
		op	i^#1.5		; invalid addressing mode
		op	#1000.5		; invalid addressing mode
		endexpect
		expect	1107
		op	b^#1.5		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		expect	1350
		op	i^#1000.5	; invalid addressing mode
		endexpect
		endm

all_no_imm_d	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#1.5		; invalid addressing mode
		op	s^#1.5		; invalid addressing mode
		op	i^#1.5		; invalid addressing mode
		op	#1000.5		; invalid addressing mode
		endexpect
		expect	1107
		op	b^#1.5		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		expect	1350
		op	i^#1000.5	; invalid addressing mode
		endexpect
		endm

all_no_imm_g	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#1.5		; invalid addressing mode
		op	s^#1.5		; invalid addressing mode
		op	i^#1.5		; invalid addressing mode
		op	#1000.5		; invalid addressing mode
		endexpect
		expect	1107
		op	b^#1.5		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		expect	1350
		op	i^#1000.5	; invalid addressing mode
		endexpect
		endm

all_no_imm_h	macro	op, code
		no_imm_x op, code
		expect	1350,1350,1350,1350
		op	#1.5		; invalid addressing mode
		op	s^#1.5		; invalid addressing mode
		op	i^#1.5		; invalid addressing mode
		op	#1000.5		; invalid addressing mode
		endexpect
		expect	1107
		op	b^#1.5		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		expect	1350
		op	i^#1000.5	; invalid addressing mode
		endexpect
		endm

all_addr_b	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#4		; code 8F 04 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4		; code 8F 04
		op	#88		; code 8F 58
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#88		; out of range 0..63
		endexpect
		op	i^#88		; code 8F 58
		endm

all_addr_w	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#4		; code 8F 04 00 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4		; code 8F 04 00
		op	#444		; code 8F BC 01
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#444		; out of range 0..63
		endexpect
		op	i^#444		; code 8F BC 01
		endm

all_addr_l	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#4		; code 8F 04 00 00 00 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4		; code 8F 04 00 00 00
		op	#444444		; code 8F 1C C8 06 00
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#444444	; out of range 0..63
		endexpect
		op	i^#444444	; code 8F 1C C8 06 00
		endm

all_addr_q	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#4		; code 8F 04 00 00 00 00 00 00 00 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4		; code 8F 04 00 00 00 00 00 00 00
		op	#44444444	; code 8F 1C 2B A6 02 00 00 00 00
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#44444444	; out of range 0..63
		endexpect
		op	i^#44444444	; code 8F 1C 2B A6 02 00 00 00 00
		endm

all_addr_o	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#4		; code 8F 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4		; code 8F 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
		op	#44444444	; code 8F 1C 2B A6 02 00 00 00 00 00 00 00 00 00 00 00 00
		expect	1107
		op	b^#4		; invalid attribute
		endexpect
		expect	1320
		op	s^#44444444	; out of range 0..63
		endexpect
		op	i^#44444444	; code 8F 1C 2B A6 02 00 00 00 00 00 00 00 00 00 00 00 00
		endm

all_addr_f	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#1.5		; code 8F C0 40 00 00 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#1.5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#1.5		; code 8F C0 40 00 00
		op	#1000.5		; code 8F 7A 45 00 20
		expect	1107
		op	b^#1.5		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		op	i^#1000.5	; code 8F 7A 45 00 20
		endm

all_addr_d	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#1.5		; code 8F C0 40 00 00 00 00 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#1.5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#1.5		; code 8F C0 40 00 00 00 00 00 00
		op	#1000.5		; code 8F 7A 45 00 20 00 00 00 00
		expect	1107
		op	b^#1.5		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		op	i^#1000.5	; code 8F 7A 45 00 20 00 00 00 00
		endm

all_addr_g	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#1.5		; code 8F 18 40 00 00 00 00 00 00 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#1.5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#1.5		; code 8F 18 40 00 00 00 00 00 00
		op	#1000.5		; code 8F AF 40 00 44 00 00 00 00
		expect	1107
		op	b^#1.5		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		op	i^#1000.5	; code 8F AF 40 00 44 00 00 00 00
		endm

all_addr_h	macro	op, code
		expect	1350
		op	r4
		endexpect
		only_mem_x op, code
		op	#1.5		; code 8F  (immediate because no literal mode for 'a')
		expect	1350
		op	s^#1.5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#1.5		; code 8F 01 40 00 80 00 00 00 00 00 00 00 00 00 00 00 00
		op	#1000.5		; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00
		expect	1107
		op	b^#1.5		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5	; out of range 0..63
		endexpect
		op	i^#1000.5	; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00
		endm

only_reg_2_r_x	macro	op, code
		op	r4,r5		; code 54 55
		endm

only_mem_2_r_x	macro	op,code
		op	@r4,r5		; code 64 55
		op	(r4),r5		; code 64 55
		op	@(r4),r5	; code B4 00 55
		op	-(r4),r5	; code 74 55
		op	(r4)+,r5	; code 84 55
		op	@(r4)+,r5	; code 94 55
		op	4(r4),r5	; code A4 04 55
		op	@4(r4),r5	; code B4 04 55
		op	b^4(r4),r5	; code A4 04 55
		op	@b^4(r4),r5	; code B4 04 55
		op	w^4(r4),r5	; code C4 04 00 55
		op	@w^4(r4),r5	; code D4 04 00 55
		op	l^4(r4),r5	; code E4 04 00 00 00 55
		op	@l^4(r4),r5	; code F4 04 00 00 00 55
		expect	1107,1107
		op	s^4(r4),r5	; invalid attribute
		op	@s^4(r4),r5	; invalid attribute
		endexpect
		op	444(r4),r5	; code C4 BC 01 55
		op	@444(r4),r5	; code D4 BC 01 55
		expect	1320,1320
		op	b^444(r4),r5	; out of range
		op	@b^444(r4),r5	; out of range
		endexpect
		op	w^444(r4),r5	; code C4 BC 01 55
		op	@w^444(r4),r5	; code D4 BC 01 55
		op	l^444(r4),r5	; code E4 BC 01 00 00 55
		op	@l^444(r4),r5	; code F4 BC 01 00 00 55
		op	444444(r4),r5	; code E4 1C C8 06 00 55
		op	@444444(r4),r5	; code F4 1C C8 06 00 55
		expect	1320,1320,1320,1320
		op	b^444444(r4),r5
		op	@b^444444(r4),r5
		op	w^444444(r4),r5
		op	@w^444444(r4),r5
		endexpect
		op	l^444444(r4),r5 ; code E4 1C C8 06 00 55
		op	@l^444444(r4),r5 ; code F4 1C C8 06 00 55
		op	@#44,r5		; code 9F 2C 00 00 00 55
		op	@#444,r5	; code 9F BC 01 00 00 55
		op	@#444444,r5	; code 9F 1C C8 06 00 55
		op	*+44,r5		; code AF 29 55
		op	b^*+44,r5	; code AF 29 55
		op	w^*+44,r5	; code CF 28 00 55
		op	l^*+44,r5	; code EF 26 00 00 00 55
		op	*+444,r5	; code CF B8 01 55
		expect 1320
		op	b^*+444,r5	; out of range
		endexpect
		op	w^*+444,r5	; code CF B8 01 55
		op	l^*+444,r5	; code EF B6 01 00 00 55
		op	*+444444,r5	; code EF 16 C8 06 00 55
		expect	1320,1320
		op	b^*+444444,r5	; out of range
		op	w^*+444444,r5	; out of range
		endexpect
		op	l^*+444444,r5	; code EF 16 C8 06 00 55
		expect	1107
		op	s^*+444444,r5	; invalid attribute
		endexpect
		expect	1350
		op	r4[r6],r5	; not allowed
		endexpect
		op	(r4)[r6],r5	; code 46 64 55
		op	(r4)+[r6],r5	; code 46 84 55
		op	@(r4)+[r6],r5	; code 46 94 55
		op	-(r4)[r6],r5	; code 46 74 55
		op	@#444[r6],r5	; code 46 9F BC 01 00 00 55
		op	@4(r4)[r6],r5	; code 46 B4 04 55
		op	*+44[r6],r5	; code 46 AF 28 55
		op	*+444[r6],r5	; code 46 CF B7 01 55
		op	*+444444[r6],r5	; code 46 EF 15 C8 06 00 55
		endm

all_2_r_b	macro	op, code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#4,r5		; code 04 55
		op	s^#4,r5		; code 04 55
		op	i^#4,r5		; code 8F 04 55
		op	#88,r5		; code 8F 58 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#88,r5	; out of range 0..63
		endexpect
		op	i^#88,r5	; code 8F 58 55
		endm

all_2_r_w	macro	op,code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#4,r5		; code 04 55
		op	s^#4,r5		; code 04 55
		op	i^#4,r5		; code 8F 04 00 55
		op	#444,r5		; code 8F BC 01 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#444,r5	; out of range 0..63
		endexpect
		op	i^#444,r5	; code 8F BC 01 55
		endm

all_2_r_l	macro	op, code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#4,r5		; code 04 55
		op	s^#4,r5		; code 04 55
		op	i^#4,r5		; code 8F 04 00 00 00 55
		op	#444444,r5	; code 8F 1C C8 06 00 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#444444,r5	; out of range 0..63
		endexpect
		op	i^#444444,r5	; code 8F 1C C8 06 00 55
		endm

all_2_r_q	macro	op, code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#4,r5		; code 04 55
		op	s^#4,r5		; code 04 55
		op	i^#4,r5		; code 8F 04 00 00 00 00 00 00 00 55
		op	#44444444,r5	; code 8F 1C 2B A6 02 00 00 00 00 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#44444444,r5	; out of range 0..63
		endexpect
		op	i^#44444444,r5	; code 8F 1C 2B A6 02 00 00 00 00 55
		endm

all_2_r_o	macro	op, code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#4,r5		; code 04 55
		op	s^#4,r5		; code 04 55
		op	i^#4,r5		; code 8F 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 55
		op	#44444444,r5	; code 8F 1C 2B A6 02 00 00 00 00 00 00 00 00 00 00 00 00 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#44444444,r5	; out of range 0..63
		endexpect
		op	i^#44444444,r5	; code 8F 1C 2B A6 02 00 00 00 00 00 00 00 00 00 00 00 00 55
		endm

all_2_r_f	macro	op, code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#1.5,r5		; code 0C 55
		op	s^#1.5,r5	; code 0C 55
		op	i^#1.5,r5	; code 8F C0 40 00 00 55
		op	#1000.5,r5	; code 8F 7A 45 00 20 55
		expect	1107
		op	b^#1.5,r5	; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5	; out of range 0.5 .. 120
		endexpect
		op	i^#1000.5,r5	; code 8F 7A 45 00 20 55
		endm

all_2_r_d	macro	op, code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#1.5,r5		; code 0C 55
		op	s^#1.5,r5	; code 0C 55
		op	i^#1.5,r5	; code 8F C0 40 00 00 00 00 00 00 55
		op	#1000.5,r5	; code 8F 7A 45 00 20 00 00 00 00 55
		expect	1107
		op	b^#1.5,r5	; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5	; out of range 0.5 .. 120
		endexpect
		op	i^#1000.5,r5	; code 8F 7A 45 00 20 00 00 00 00 55
		endm

all_2_r_g	macro	op, code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#1.5,r5		; code 0C 55
		op	s^#1.5,r5	; code 0C 55
		op	i^#1.5,r5	; code 8F 18 40 00 00 00 00 00 00 55
		op	#1000.5,r5	; code 8F AF 40 00 44 00 00 00 00 55
		expect	1107
		op	b^#1.5,r5	; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5	; out of range 0.5 .. 120
		endexpect
		op	i^#1000.5,r5	; code 8F AF 40 00 44 00 00 00 00 55
		endm

all_2_r_h	macro	op, code
		only_reg_2_r_x op, code
		only_mem_2_r_x op, code
		op	#1.5,r5		; code 0C 55
		op	s^#1.5,r5	; code 0C 55
		op	i^#1.5,r5	; code 8F 01 40 00 80 00 00 00 00 00 00 00 00 00 00 00 00 55
		op	#1000.5,r5	; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00 55
		expect	1107
		op	b^#1.5,r5	; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5	; out of range 0.5 .. 120
		endexpect
		op	i^#1000.5,r5	; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00 55
		endm

all_addr_2_r_b	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#4,r5		; code 8F 04 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4,r5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4,r5		; code 8F 04 55
		op	#88,r5		; code 8F 58 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#88,r5	; out of range 0..63
		endexpect
		op	i^#88,r5	; code 8F 58 55
		endm

all_addr_2_r_w	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#4,r5		; code 8F 04 00 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4,r5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4,r5		; code 8F 04 00 55
		op	#444,r5		; code 8F BC 01 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#444,r5	; out of range 0..63
		endexpect
		op	i^#444,r5	; code 8F BC 01 55
		endm

all_addr_2_r_l	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#4,r5		; code 8F 04 00 00 00 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4,r5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4,r5		; code 8F 04 00 00 00 55
		op	#444444,r5	; code 8F 1C C8 06 00 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#444444,r5	; out of range 0..63
		endexpect
		op	i^#444444,r5	; code 8F 1C C8 06 00 55
		endm

all_addr_2_r_q	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#4,r5		; code 8F 04 00 00 00 00 00 00 00 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4,r5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4,r5		; code 8F 04 00 00 00 00 00 00 00 55
		op	#44444444,r5	; code 8F 1B 2B A6 02 00 00 00 00 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#44444444,r5	; out of range 0..63
		endexpect
		op	i^#44444444,r5	; code 8F 1B 2B A6 02 00 00 00 00 55
		endm

all_addr_2_r_o	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#4,r5		; code 8F 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#4,r5		; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#4,r5		; code 8F 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 55
		op	#44444444,r5	; code 8F 1B 2B A6 02 00 00 00 00 00 00 00 00 00 00 00 00 55
		expect	1107
		op	b^#4,r5		; invalid attribute
		endexpect
		expect	1320
		op	s^#44444444,r5	; out of range 0..63
		endexpect
		op	i^#44444444,r5	; code 8F 1B 2B A6 02 00 00 00 00 00 00 00 00 00 00 00 00 55
		endm

all_addr_2_r_f	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#1.5,r5		; code 8F C0 40 00 00 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#1.5,r5	; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#1.5,r5	; code 8F C0 40 00 00 55
		op	#1000.5,r5	; code 8F 7A 45 00 20 55
		expect	1107
		op	b^#1.5,r5	; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5	; out of range 0..63
		endexpect
		op	i^#1000.5,r5	; code 8F 7A 45 00 20 55
		endm

all_addr_2_r_d	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#1.5,r5		; code 8F C0 40 00 00 00 00 00 00 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#1.5,r5	; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#1.5,r5	; code 8F C0 40 00 00 00 00 00 00 55
		op	#1000.5,r5	; code 8F 7A 45 00 20 00 00 00 00 55
		expect	1107
		op	b^#1.5,r5	; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5	; out of range 0..63
		endexpect
		op	i^#1000.5,r5	; code 8F 7A 45 00 20 00 00 00 00 55
		endm

all_addr_2_r_g	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#1.5,r5		; code 8F 18 40 00 00 00 00 00 00 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#1.5,r5	; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#1.5,r5	; code 8F 18 40 00 00 00 00 00 00 55
		op	#1000.5,r5	; code 8F AF 40 00 44 00 00 00 00 55
		expect	1107
		op	b^#1.5,r5	; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5	; out of range 0..63
		endexpect
		op	i^#1000.5,r5	; code 8F AF 40 00 44 00 00 00 00 55
		endm

all_addr_2_r_h	macro	op, code
		expect	1350
		op	r4,r5
		endexpect
		only_mem_2_r_x op, code
		op	#1.5,r5		; code 8F 01 40 00 80 00 00 00 00 00 00 00 00 00 00 00 00 55 (immediate because no literal mode for 'a')
		expect	1350
		op	s^#1.5,r5	; invalid addressing mode (no literal mode for 'a')
		endexpect
		op	i^#1.5,r5	; code 8F 01 40 00 80 00 00 00 00 00 00 00 00 00 00 00 00 55
		op	#1000.5,r5	; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00 55
		expect	1107
		op	b^#1.5,r5	; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5	; out of range 0..63
		endexpect
		op	i^#1000.5,r5	; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00 55
		endm

only_reg_r_2_r_x macro op, code
		op	r4,r5,r8		; code 54 55 58
		endm

only_mem_r_2_r_x macro op, code
		op	@r4,r5,r8		; code 64 55 58
		op	(r4),r5,r8		; code 64 55 58
		op	@(r4),r5,r8		; code B4 00 55 58
		op	-(r4),r5,r8		; code 74 55 58
		op	(r4)+,r5,r8		; code 84 55 58
		op	@(r4)+,r5,r8		; code 94 55 58
		op	4(r4),r5,r8		; code A4 04 55 58
		op	@4(r4),r5,r8		; code B4 04 55 58
		op	b^4(r4),r5,r8		; code A4 04 55 58
		op	@b^4(r4),r5,r8		; code B4 04 55 58
		op	w^4(r4),r5,r8		; code C4 04 00 55 58
		op	@w^4(r4),r5,r8		; code D4 04 00 55 58
		op	l^4(r4),r5,r8		; code E4 04 00 00 00 55 58
		op	@l^4(r4),r5,r8		; code F4 04 00 00 00 55 58
		expect	1107,1107
		op	s^4(r4),r5,r8		; invalid attribute
		op	@s^4(r4),r5,r8		; invalid attribute
		endexpect
		op	444(r4),r5,r8		; code C4 BC 01 55 58
		op	@444(r4),r5,r8		; code D4 BC 01 55 58
		expect	1320,1320
		op	b^444(r4),r5,r8		; out of range
		op	@b^444(r4),r5,r8	; out of range
		endexpect
		op	w^444(r4),r5,r8		; code C4 BC 01 55 58
		op	@w^444(r4),r5,r8	; code D4 BC 01 55 58
		op	l^444(r4),r5,r8		; code E4 BC 01 00 00 55 58
		op	@l^444(r4),r5,r8	; code F4 BC 01 00 00 55 58
		op	444444(r4),r5,r8	; code E4 1C C8 06 00 55 58
		op	@444444(r4),r5,r8	; code F4 1C C8 06 00 55 58
		expect	1320,1320,1320,1320
		op	b^444444(r4),r5,r8
		op	@b^444444(r4),r5,r8
		op	w^444444(r4),r5,r8
		op	@w^444444(r4),r5,r8
		endexpect
		op	l^444444(r4),r5,r8	; code E4 1C C8 06 00 55 58
		op	@l^444444(r4),r5,r8	; code F4 1C C8 06 00 55 58
		op	@#44,r5,r8		; code 9F 2C 00 00 00 55 58
		op	@#444,r5,r8		; code 9F BC 01 00 00 55 58
		op	@#444444,r5,r8		; code 9F 1C C8 06 00 55 58
		op	*+44,r5,r8		; code AF 29 55 58
		op	b^*+44,r5,r8		; code AF 29 55 58
		op	w^*+44,r5,r8		; code CF 28 00 55 58
		op	l^*+44,r5,r8		; code EF 26 00 00 00 55 58
		op	*+444,r5,r8		; code CF B8 01 55 58
		expect 1320
		op	b^*+444,r5,r8		; out of range
		endexpect
		op	w^*+444,r5,r8		; code CF B8 01 55 58
		op	l^*+444,r5,r8		; code EF B6 01 00 00 55 58
		op	*+444444,r5,r8		; code EF 16 C8 06 00 55 58
		expect	1320,1320
		op	b^*+444444,r5,r8	; out of range
		op	w^*+444444,r5,r8	; out of range
		endexpect
		op	l^*+444444,r5,r8	; code EF 16 C8 06 00 55 58
		expect	1107
		op	s^*+444444,r5,r8	; invalid attribute
		endexpect
		expect	1350
		op	r4[r6],r5,r8	; not allowed
		endexpect
		op	(r4)[r6],r5,r8		; code 46 64 55 58
		op	(r4)+[r6],r5,r8		; code 46 84 55 58
		op	@(r4)+[r6],r5,r8	; code 46 94 55 58
		op	-(r4)[r6],r5,r8		; code 46 74 55 58
		op	@#444[r6],r5,r8		; code 46 9F BC 01 00 00 55 58
		op	@4(r4)[r6],r5,r8	; code 46 B4 04 55 58
		op	*+44[r6],r5,r8		; code 46 AF 28 55 58
		op	*+444[r6],r5,r8		; code 46 CF B7 01 55 58
		op	*+444444[r6],r5,r8	; code 46 EF 15 C8 06 00 55 58 
		endm

all_r_2_r_b	macro	op,code
		only_reg_r_2_r_x op, code
		only_mem_r_2_r_x op, code
		op	#4,r5,r8		; code 04 55 58
		op	s^#4,r5,r8		; code 04 55 58
		op	i^#4,r5,r8		; code 8F 04 55 58
		op	#88,r5,r8		; code 8F 58 55 58
		expect	1107
		op	b^#4,r5,r8		; invalid attribute
		endexpect
		expect	1320
		op	s^#88,r5,r8		; out of range 0..63
		endexpect
		op	i^#88,r5,r8		; code 8F 58 55 58
		endm

all_r_2_r_w	macro	op,code
		only_reg_r_2_r_x op, code
		only_mem_r_2_r_x op, code
		op	#4,r5,r8		; code 04 55 58
		op	s^#4,r5,r8		; code 04 55 58
		op	i^#4,r5,r8		; code 8F 04 00 55 58
		op	#444,r5,r8		; code 8F BC 01 55 58
		expect	1107
		op	b^#4,r5,r8		; invalid attribute
		endexpect
		expect	1320
		op	s^#444,r5,r8		; out of range 0..63
		endexpect
		op	i^#444,r5,r8		; code 8F BC 01 55 58
		endm

all_r_2_r_l	macro	op,code
		only_reg_r_2_r_x op, code
		only_mem_r_2_r_x op, code
		op	#4,r5,r8		; code 04 55 58
		op	s^#4,r5,r8		; code 04 55 58
		op	i^#4,r5,r8		; code 8F 04 00 00 00 55 58
		op	#444444,r5,r8		; code 8F 1C C8 06 00 55 58
		expect	1107
		op	b^#4,r5,r8		; invalid attribute
		endexpect
		expect	1320
		op	s^#444444,r5,r8		; out of range 0..63
		endexpect
		op	i^#444444,r5,r8		; code 8F 1C C8 06 00 55 58
		endm

all_r_2_r_f	macro	op,code
		only_reg_r_2_r_x op, code
		only_mem_r_2_r_x op, code
		op	#1.5,r5,r8		; code 0C 55 58
		op	s^#1.5,r5,r8		; code 0C 55 58
		op	i^#1.5,r5,r8		; code 8F C0 40 00 00 55 58
		op	#1000.5,r5,r8		; code 8F 7A 45 00 20 55 58
		expect	1107
		op	b^#1.5,r5,r8		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5,r8		; out of range 0.5 .. 120
		endexpect
		op	i^#1000.5,r5,r8		; code 8F 7A 45 00 20 55 58
		endm

all_r_2_r_d	macro	op,code
		only_reg_r_2_r_x op, code
		only_mem_r_2_r_x op, code
		op	#1.5,r5,r8		; code 0C 55 58
		op	s^#1.5,r5,r8		; code 0C 55 58
		op	i^#1.5,r5,r8		; code 8F C0 40 00 00 00 00 00 00 55 58
		op	#1000.5,r5,r8		; code 8F 7A 45 00 20 00 00 00 00 55 58
		expect	1107
		op	b^#1.5,r5,r8		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5,r8		; out of range 0.5 .. 120
		endexpect
		op	i^#1000.5,r5,r8		; code 8F 7A 45 00 20 00 00 00 00 55 58
		endm

all_r_2_r_g	macro	op,code
		only_reg_r_2_r_x op, code
		only_mem_r_2_r_x op, code
		op	#1.5,r5,r8		; code 0C 55 58
		op	s^#1.5,r5,r8		; code 0C 55 58
		op	i^#1.5,r5,r8		; code 8F 18 40 00 00 00 00 00 00 55 58
		op	#1000.5,r5,r8		; code 8F AF 40 00 44 00 00 00 00 55 58
		expect	1107
		op	b^#1.5,r5,r8		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5,r8		; out of range 0.5 .. 120
		endexpect
		op	i^#1000.5,r5,r8		; code 8F AF 40 00 44 00 00 00 00 55 58
		endm

all_r_2_r_h	macro	op,code
		only_reg_r_2_r_x op, code
		only_mem_r_2_r_x op, code
		op	#1.5,r5,r8		; code 0C 55 58
		op	s^#1.5,r5,r8		; code 0C 55 58
		op	i^#1.5,r5,r8		; code 8F 01 40 00 80 00 00 00 00 00 00 00 00 00 00 00 00 55 58
		op	#1000.5,r5,r8		; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00 55 58
		expect	1107
		op	b^#1.5,r5,r8		; invalid attribute
		endexpect
		expect	1320
		op	s^#1000.5,r5,r8		; out of range 0.5 .. 120
		endexpect
		op	i^#1000.5,r5,r8		; code 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00 55 58
		endm

bitfield_first	macro	op, code
		; both pos and size are literals
		op	#10,#20,@r4,r5		; code 0A 14 64 55
		; pos is 32 bits, so four bytes as non-literal
		op	i^#10,#20,@r4,r5	; code 8F 0A 00 00 00 14 64 55
		; size is 8 bits, so one byte as non-literal
		op	#10,i^#20,@r4,r5	; code 0A 8F 14 64 55
		; pos is too large for literal
		op	#1000,#20,@r4,r5	; code 8F F8 03 00 00 14 64 55
		expect	2180,2180
		; size may never be > 32, neither for memory...
		op	#10,#33,@r4,r5
		; ...nor for register
		op	#10,#33,r4,r5
		endexpect
		; zero-length bit field in register may have arbitrary position
		op	#33,#0,r4,r5		; code 21 00 54 55
		expect	2180
		; if non-zero size, position in rregister must be < 32
		op	#32,#1,r4,r5
		endexpect
		endm

bitfield_last	macro	op, code
		; both pos and size are literals
		op	r5,#10,#20,@r4		; code 55 0A 14 64
		; pos is 32 bits, so four bytes as non-literal
		op	r5,i^#10,#20,@r4	; code 55 8F 0A 00 00 00 14 64
		; size is 8 bits, so one byte as non-literal
		op	r5,#10,i^#20,@r4	; code 55 0A 8F 14 64
		; pos is too large for literal
		op	r5,#1000,#20,@r4	; code 55 8F F8 03 00 00 14 64
		expect	2180,2180
		; size may never be > 32, neither for memory...
		op	r5,#10,#33,@r4
		; ...nor for register
		op	r5,#10,#33,r4
		endexpect
		; zero-length bit field in register may have arbitrary position
		op	r5,#33,#0,r4		; code 55 21 00 54
		expect	2180
		; if non-zero size, position in register must be < 32
		op	r5,#32,#1,r4
		endexpect
		endm

;=================================================

		all_2_r_w adawi, 58

;-------------------------------------------------

		all_2_r_b addb, 80
		all_2_r_w addw, A0
		all_2_r_l addl, C0

		all_2_r_b addb2, 80
		all_2_r_w addw2, A0
		all_2_r_l addl2, C0
		expect 1110
		addl2	r4,r5,r6
		endexpect

		all_r_2_r_b addb, 81
		all_r_2_r_w addw, A1
		all_r_2_r_l addl, C1

		all_r_2_r_b addb3, 81
		all_r_2_r_w addw3, A1
		all_r_2_r_l addl3, C1
		expect 1110
		addl3	r4,r5
		endexpect

		addl	(r3)[r4],r1	; C0 44 63 51
		addl2	(r3)[r4],r1	; C0 44 63 51

		addl	(r3)[r4],737(r2),r1 ; C1 44 63 C2 E1 02 51
		addl3	(r3)[r4],737(r2),r1 ; C1 44 63 C2 E1 02 51

;-------------------------------------------------

		all_2_r_f addf, 40
		all_2_r_d addd, 60
		all_2_r_g addg, fd 40
		all_2_r_h addh, fd 60

		all_2_r_f addf2, 40
		all_2_r_d addd2, 60
		all_2_r_g addg2, fd 40
		all_2_r_h addh2, fd 60
		expect 1110
		addf2	r4,r5,r6
		endexpect

		all_r_2_r_f addf, 41
		all_r_2_r_d addd, 61
		all_r_2_r_g addg, fd 41
		all_r_2_r_h addh, fd 61

		all_r_2_r_f addf3, 41
		all_r_2_r_d addd3, 61
		all_r_2_r_g addg3, fd 41
		all_r_2_r_h addh3, fd 61
		expect 1110
		addl3	r4,r5
		endexpect

;-------------------------------------------------

		all_2_r_l adwc, D8

;-------------------------------------------------

		all_r_2_r_b ashl, 78
		all_r_2_r_b ashq, 79

;-------------------------------------------------

		all_2_r_b bicb, 8A
		all_2_r_w bicw, AA
		all_2_r_l bicl, CA

		all_2_r_b bicb2, 8A
		all_2_r_w bicw2, AA
		all_2_r_l bicl2, CA
		expect 1110
		bicl2	r4,r5,r6
		endexpect

		all_r_2_r_b bicb, 8B
		all_r_2_r_w bicw, AB
		all_r_2_r_l bicl, CB

		all_r_2_r_b bicb3, 8B
		all_r_2_r_w bicw3, AB
		all_r_2_r_l bicl3, CB
		expect 1110
		bicl3	r4,r5
		endexpect

		bicl	(r3)[r4],r1	; CA 44 63 51
		bicl2	(r3)[r4],r1	; CA 44 63 51

		bicl	(r3)[r4],737(r2),r1 ; CB 44 63 C2 E1 02 51
		bicl3	(r3)[r4],737(r2),r1 ; CB 44 63 C2 E1 02 51

;-------------------------------------------------

		all_w	bicpsw, b9

;-------------------------------------------------

		all_2_r_b bisb, 88
		all_2_r_w bisw, A8
		all_2_r_l bisl, C8

		all_2_r_b bisb2, 88
		all_2_r_w bisw2, A8
		all_2_r_l bisl2, C8
		expect 1110
		bisl2	r4,r5,r6
		endexpect

		all_r_2_r_b bisb, 89
		all_r_2_r_w bisw, A9
		all_r_2_r_l bisl, C9

		all_r_2_r_b bisb3, 89
		all_r_2_r_w bisw3, A9
		all_r_2_r_l bisl3, C9
		expect 1110
		bisl3	r4,r5
		endexpect

		bisl	(r3)[r4],r1	; C8 44 63 51
		bisl2	(r3)[r4],r1	; C8 44 63 51

		bisl	(r3)[r4],737(r2),r1 ; C9 44 63 C2 E1 02 51
		bisl3	(r3)[r4],737(r2),r1 ; C9 44 63 C2 E1 02 51

;-------------------------------------------------

		all_w	bispsw, b8

;-------------------------------------------------

		all_2_r_b bitb, 93
		all_2_r_w bitw, b3
		all_2_r_l bitl, d3
		bitb	#44,#88		; 93 2C 8F 58
		bitw	#444,#888	; B3 8F BC 01 8F 78 03
		bitl	#444444,#888888	; D3 8F 1C C8 06 00 8F 38 90 0D 00

;-------------------------------------------------

		bpt			; 03

;-------------------------------------------------

		bugw			; FF FE
		bugl			; FF FD

;-------------------------------------------------

		callg	@r5,@#0x12345678	; FA 65 9F 78 56 34 12

;-------------------------------------------------

		calls	#3,@#0x87654321		; FB 03 9F 21 43 65 87

;-------------------------------------------------

		chmk	#1234			; BC 8F D2 04
		chme	#0			; BD 00
		chms	r5			; BE 55
		chmu	@r8			; BF 68

;-------------------------------------------------

		all_no_imm_b clrb, 94
		all_no_imm_w clrw, b4
		all_no_imm_l clrl, d4
		all_no_imm_q clrq, 7c
		all_no_imm_o clro, fd 7c

;-------------------------------------------------

		all_no_imm_f clrf, d4
		all_no_imm_d clrd, 7c
		all_no_imm_g clrg, 7c
		all_no_imm_h clrh, fd 7c

;-------------------------------------------------

		all_2_r_b cmpb, 91
		all_2_r_w cmpw, b1
		all_2_r_l cmpl, d1
		cmpb	#44,#88		; 91 2C 8F 58
		cmpw	#444,#888	; B1 8F BC 01 8F 78 03
		cmpl	#444444,#888888	; D1 8F 1C C8 06 00 8F 38 90 0D 00

;-------------------------------------------------

		all_2_r_f cmpf, 51
		all_2_r_d cmpd, 71
		all_2_r_g cmpg, fd 51
		all_2_r_h cmph, fd 71
		cmpf	#1.5,#1000.5	; 51 0C 8F 7A 45 00 20

;-------------------------------------------------

		bitfield_first cmpv, ec
		bitfield_first cmpzv, ed

;-------------------------------------------------

		all_2_r_b cvtbw, 99
		all_2_r_b cvtbl, 98
		all_2_r_w cvtwb, 33
		all_2_r_w cvtwl, 32
		all_2_r_l cvtlb, f6
		all_2_r_l cvtlw, f7

;-------------------------------------------------

		all_2_r_f cvtfb, 48
		all_2_r_f cvtfw, 49
		all_2_r_f cvtfl, 4a
		all_2_r_f cvtrfl, 4b
		all_2_r_b cvtbf, 4c
		all_2_r_w cvtwf, 4d
		all_2_r_l cvtlf, 4e
		all_2_r_f cvtfd, 56
		all_2_r_d cvtdb, 68
		all_2_r_d cvtdw, 69
		all_2_r_d cvtdl, 6a
		all_2_r_d cvtrdl, 6b
		all_2_r_b cvtbd, 6c
		all_2_r_w cvtwd, 6d
		all_2_r_l cvtld, 6e
		all_2_r_d cvtdf, 76
		all_2_r_d cvtdh, fd 32
		all_2_r_g cvtgf, fd 33
		all_2_r_g cvtgb, fd 48
		all_2_r_g cvtgw, fd 49
		all_2_r_g cvtgl, fd 4a
		all_2_r_g cvtrgl, fd 4b
		all_2_r_b cvtbg, fd 4c
		all_2_r_w cvtwg, fd 4d
		all_2_r_l cvtlg, fd 4e
		all_2_r_g cvtgh, fd 56
		all_2_r_h cvthb, fd 68
		all_2_r_h cvthw, fd 69
		all_2_r_h cvthl, fd 6a
		all_2_r_d cvtrhl, fd 6b
		all_2_r_b cvtbh, fd 6c
		all_2_r_w cvtwh, fd 6d
		all_2_r_l cvtlh, fd 6e
		all_2_r_h cvthg, fd 76
		all_2_r_f cvtfh, fd 98
		all_2_r_f cvtfg, fd 99
		all_2_r_h cvthf, fd f6
		all_2_r_h cvthd, fd f7

;-------------------------------------------------

		all_no_imm_b decb, 97
		all_no_imm_w decw, b7
		all_no_imm_l decl, d7

;-------------------------------------------------

		all_2_r_b divb, 86
		all_2_r_w divw, A6
		all_2_r_l divl, C6

		all_2_r_b divb2, 86
		all_2_r_w divw2, A6
		all_2_r_l divl2, C6

		all_r_2_r_b divb, 87
		all_r_2_r_w divw, A7
		all_r_2_r_l divl, C7

		all_r_2_r_b divb3, 87
		all_r_2_r_w divw3, A7
		all_r_2_r_l divl3, C7
		expect 1110
		divl3	r4,r5
		endexpect

		divl	(r3)[r4],r1	; C6 44 63 51
		divl2	(r3)[r4],r1	; C6 44 63 51

		divl	(r3)[r4],737(r2),r1 ; C7 44 63 C2 E1 02 51
		divl3	(r3)[r4],737(r2),r1 ; C7 44 63 C2 E1 02 51

;-------------------------------------------------

		all_2_r_f divf, 46
		all_2_r_d divd, 66
		all_2_r_g divg, fd 46
		all_2_r_h divh, fd 66

		all_2_r_f divf2, 46
		all_2_r_d divd2, 66
		all_2_r_g divg2, fd 46
		all_2_r_h divh2, fd 66
		expect 1110
		divf2	r4,r5,r6
		endexpect

		all_r_2_r_f divf, 47
		all_r_2_r_d divd, 67
		all_r_2_r_g divg, fd 47
		all_r_2_r_h divh, fd 67

		all_r_2_r_f divf3, 47
		all_r_2_r_d divd3, 67
		all_r_2_r_g divg3, fd 47
		all_r_2_r_h divh3, fd 67
		expect 1110
		divf3	r4,r5
		endexpect

;-------------------------------------------------

		emodf	r5,#0x22,#1.5,r4,r6	; 54 55 22 0C 54 56
		emodd	r5,#0xaa,#1000.5,r4,r6	; 74 55 8F AA 8F 7A 45 00 20 00 00 00 00 54 56
		emodg	r5,#0xaa,#1000.5,r4,r6  ; FD 54 55 8F AA 00 8F AF 40 00 44 00 00 00 00 54 56
		emodh	r5,#0xaa,#1000.5,r4,r6	; FD 74 55 8F AA 00 8F 0A 40 40 F4 00 00 00 00 00 00 00 00 00 00 00 00 54 56

;-------------------------------------------------

		bitfield_first extv, ee
		bitfield_first extzv, ef

;-------------------------------------------------

		bitfield_first ffc, eb
		bitfield_first ffs, ea

;-------------------------------------------------

		halt			; 00

;-------------------------------------------------

		all_no_imm_b incb, 96
		all_no_imm_w incw, b6
		all_no_imm_l incl, d6

;-------------------------------------------------

		index	r11, #0x01, #0x0f, #0x19, #0x00, r0 ; 0A 5B 01 0F 19 00 50

;-------------------------------------------------

		insqhi	444(r5),888(r4)	; 5C C5 BC 01 C4 78 03
		insqti	444(r5),888(r4) ; 5D C5 BC 01 C4 78 03
		insque	444(r5),888(r4) ; 0E C5 BC 01 C4 78 03
		expect	1350
		insque	r5,888(r4)
		endexpect

;-------------------------------------------------

                bitfield_last insv, f0

;-------------------------------------------------

		all_addr_b jmp, 17

;-------------------------------------------------

		all_addr_b jsb, 16

;-------------------------------------------------

		ldpctx			; 06

;-------------------------------------------------

		all_2_r_b mcomb, 92
		all_2_r_w mcomw, b2
		all_2_r_l mcoml, d2

;-------------------------------------------------

		all_2_r_b mnegb, 8e
		all_2_r_w mnegw, ae
		all_2_r_l mnegl, ce

;-------------------------------------------------

		all_2_r_f mnegf, 52
		all_2_r_d mnegd, 72
		all_2_r_g mnegg, fd 52
		all_2_r_h mnegh, fd 72

;-------------------------------------------------

		all_2_r_b movb, 90
		all_2_r_w movw, b0
		all_2_r_l movl, d0
		all_2_r_q movq, 7d
		all_2_r_o movo, fd 7d

;-------------------------------------------------

		all_2_r_f movf, 50
		all_2_r_d movd, 70
		all_2_r_g movg, fd 50
		all_2_r_h movh, fd 70

;-------------------------------------------------

		all_addr_2_r_b movab, 9e
		all_addr_2_r_w movaw, 3e
		all_addr_2_r_l moval, de
		all_addr_2_r_q movaq, 7e
		all_addr_2_r_o movao, fd 7e

;-------------------------------------------------

		all_addr_2_r_f movaf, de
		all_addr_2_r_d movad, 7e
		all_addr_2_r_g movag, 7e
		all_addr_2_r_h movah, fd 7e

;-------------------------------------------------

		all_no_imm_l movpsl, dc

;-------------------------------------------------

		all_2_r_b movzbw, 9b
		all_2_r_b movzbl, 9a
		all_2_r_w movzwl, 3c

;-------------------------------------------------

		mtpr	r4,#6		; DA 54 06
		mfpr	#6,(r7)		; DB 06 67

;-------------------------------------------------

		all_2_r_b mulb, 84
		all_2_r_w mulw, A4
		all_2_r_l mull, C4

		all_2_r_b mulb2, 84
		all_2_r_w mulw2, A4
		all_2_r_l mull2, C4
		expect 1110
		mull2	r4,r5,r6
		endexpect

		all_r_2_r_b mulb, 85
		all_r_2_r_w mulw, A5
		all_r_2_r_l mull, C5

		all_r_2_r_b mulb3, 85
		all_r_2_r_w mulw3, A5
		all_r_2_r_l mull3, C5
		expect 1110
		mull3	r4,r5
		endexpect

		mull	(r3)[r4],r1	; C4 44 63 51
		mull2	(r3)[r4],r1	; C4 44 63 51

		mull	(r3)[r4],737(r2),r1 ; C5 44 63 C2 E1 02 51
		mull3	(r3)[r4],737(r2),r1 ; C5 44 63 C2 E1 02 51

;-------------------------------------------------

		all_2_r_f mulf, 44
		all_2_r_d muld, 64
		all_2_r_g mulg, fd 44
		all_2_r_h mulh, fd 64

		all_2_r_f mulf2, 44
		all_2_r_d muld2, 64
		all_2_r_g mulg2, fd 44
		all_2_r_h mulh2, fd 64
		
		expect 1110
		mulf2	r4,r5,r6
		endexpect

		all_r_2_r_f mulf, 45
		all_r_2_r_d muld, 65
		all_r_2_r_g mulg, fd 45
		all_r_2_r_h mulh, fd 65

		all_r_2_r_f mulf3, 45
		all_r_2_r_d muld3, 65
		all_r_2_r_g mulg3, fd 45
		all_r_2_r_h mulh3, fd 65
		expect 1110
		mulf3	r4,r5
		endexpect

;-------------------------------------------------

		nop			; 01

;-------------------------------------------------

		polyf	r5,#3,@r6		; 55 55 03 66
		polyd	r5,#17,@r6		; 75 55 11 66
		polyg	r5,#100,@r6		; FD 55 55 8F 64 00 66
		polyh	r5,#101,@#0x12345678	; FD 75 55 8F 65 00 9F 78 56 34 12

;-------------------------------------------------

		all_w popr, ba

;-------------------------------------------------

		prober	#1,#444,@r5		; 0C 01 8F BC 01 65
		probew	#1,#444,@r5		; 0D 01 8F BC 01 65

;-------------------------------------------------

		all_addr_b pushab, 9f
		all_addr_w pushaw, 3f
		all_addr_l pushal, df
		all_addr_q pushaq, 7f
		all_addr_o pushao, fd 7f

;-------------------------------------------------

		all_addr_f pushaf, df
		all_addr_d pushad, 7f
		all_addr_g pushag, 7f
		all_addr_h pushah, fd 7f

;-------------------------------------------------

		all_l pushl, dd

;-------------------------------------------------

		all_w pushr, bb

;-------------------------------------------------

		rei		; 02

;-------------------------------------------------

		remqhi 444(r4),r5	; 5E C4 BC 01 55
		remqti 444(r4),r5	; 5F C4 BC 01 55
		remque 444(r4),r5	; 0F C4 BC 01 55 

;-------------------------------------------------

		ret		; 04

;-------------------------------------------------

		all_r_2_r_b rotl, 9c

;-------------------------------------------------

		rsb		; 05

;-------------------------------------------------

		all_2_r_l sbwc, D9

;-------------------------------------------------

		all_2_r_b subb, 82
		all_2_r_w subw, A2
		all_2_r_l subl, C2

		all_2_r_b subb2, 82
		all_2_r_w subw2, A2
		all_2_r_l subl2, C2
		expect 1110
		subl2	r4,r5,r6
		endexpect

		all_r_2_r_b subb, 83
		all_r_2_r_w subw, A3
		all_r_2_r_l subl, C3

		all_r_2_r_b subb3, 83
		all_r_2_r_w subw3, A3
		all_r_2_r_l subl3, C3
		expect 1110
		subl3	r4,r5
		endexpect

		subl	(r3)[r4],r1	; C2 44 63 51
		subl2	(r3)[r4],r1	; C2 44 63 51

		subl	(r3)[r4],737(r2),r1 ; C3 44 63 C2 E1 02 51
		subl3	(r3)[r4],737(r2),r1 ; C3 44 63 C2 E1 02 51

;-------------------------------------------------

		all_2_r_f subf, 42
		all_2_r_d subd, 62
		all_2_r_g subg, fd 42
		all_2_r_h subh, fd 62

		all_2_r_f subf2, 42
		all_2_r_d subd2, 62
		all_2_r_g subg2, fd 42
		all_2_r_h subh2, fd 62
		expect 1110
		subf2	r4,r5,r6
		endexpect

		all_r_2_r_f subf, 43
		all_r_2_r_d subd, 63
		all_r_2_r_g subg, fd 43
		all_r_2_r_h subh, fd 63

		all_r_2_r_f subf3, 43
		all_r_2_r_d subd3, 63
		all_r_2_r_g subg3, fd 43
		all_r_2_r_h subh3, fd 63
		expect 1110
		subf3	r4,r5
		endexpect

;-------------------------------------------------

		svpctx			; 07

;-------------------------------------------------

		all_b tstb, 95
		all_w tstw, b5
		all_l tstl, d5

;-------------------------------------------------

		all_f tstf, 53
		all_d tstd, 73
		all_g tstg, fd 53
		all_h tsth, fd 73

;-------------------------------------------------

		xfc		; FC

;-------------------------------------------------

		all_2_r_b xorb, 8C
		all_2_r_w xorw, AC
		all_2_r_l xorl, CC

		all_2_r_b xorb2, 8C
		all_2_r_w xorw2, AC
		all_2_r_l xorl2, CC
		expect 1110
		xorl2	r4,r5,r6
		endexpect

		all_r_2_r_b xorb, 8D
		all_r_2_r_w xorw, AD
		all_r_2_r_l xorl, CD

		all_r_2_r_b xorb3, 8D
		all_r_2_r_w xorw3, AD
		all_r_2_r_l xorl3, CD
		expect 1110
		xorl3	r4,r5
		endexpect

		xorl	(r3)[r4],r1	; CC 44 63 51
		xorl2	(r3)[r4],r1	; CC 44 63 51

		xorl	(r3)[r4],737(r2),r1 ; CD 44 63 C2 E1 02 51
		xorl3	(r3)[r4],737(r2),r1 ; CD 44 63 C2 E1 02 51

;-------------------------------------------------

		cmpc3	r4,@r5,@r7		; 29 54 65 67
		cmpc	r4,@r5,@r7		; 29 54 65 67
		cmpc5	r4,@r5,#' ',r6,@r7	; 2D 54 65 20 56 67
		cmpc	r4,@r5,#' ',r6,@r7	; 2D 54 65 20 56 67
		expect	1110
		cmpc	r4,@r5,r6,@r7
		endexpect
		locc	#' ',r4,@r5		; 3A 20 54 65
		matchc	r4,@r5,r6,@r7		; 39 54 65 56 67
		movc3	r4,@r5,@r7		; 28 54 65 67
		movc	r4,@r5,@r7		; 28 54 65 67
		movc5	r4,@r5,#' ',r6,@r7	; 2C 54 65 20 56 67
		movc	r4,@r5,#' ',r6,@r7	; 2C 54 65 20 56 67
		movtc	r4,@r5,#' ',@r8,r6,@r7	; 2E 54 65 20 68 56 67
		movtuc	r4,@r5,#' ',@r8,r6,@r7	; 2F 54 65 20 68 56 67
		scanc	r4,@r5,@r8,#0xaa	; 2A 54 65 68 8F AA
		skpc	#' ',r4,@r5		; 3B 20 54 65
		spanc	r4,@r5,@r8,#0xaa	; 2B 54 65 68 8F AA

;-------------------------------------------------

		crc	200(r4),#0xffffffff,#512,@r5 ; 0B C4 C8 00 8F FF FF FF FF 8F 00 02 65

;-------------------------------------------------

		addp4	r4,@r5,r6,@r7		; 20 54 65 56 67
		addp	r4,@r5,r6,@r7		; 20 54 65 56 67
		addp6	r4,@r5,r6,@r7,r8,@r9	; 21 54 65 56 67 58 69
		addp	r4,@r5,r6,@r7,r8,@r9	; 21 54 65 56 67 58 69
		ashp	#3,r4,@r5,#0,r6,@r7	; F8 03 54 65 00 56 67
		cmpp3	r4,@r5,@r7		; 35 54 65 67
		cmpp	r4,@r5,@r7		; 35 54 65 67
		cmpp4	r4,@r5,r6,@r7		; 37 54 65 56 67
		cmpp	r4,@r5,r6,@r7		; 37 54 65 56 67
		cvtlp	r2,r6,@r7		; F9 52 56 67
		cvtpl	r4,@r5,r2		; 36 54 65 52
		cvtps	r4,@r5,r6,@r7		; 08 54 65 56 67
		cvtpt	r4,@r5,@r10,r6,@r7	; 24 54 65 6A 56 67
		cvtsp	r4,@r5,r6,@r7		; 09 54 65 56 67
		cvttp	r4,@r5,@r10,r6,@r7	; 26 54 65 6A 56 67
		divp	r4,@r5,r6,@r7,r8,@r9	; 27 54 65 56 67 58 69
		movp	r4,@r5,@r7		; 34 54 65 67
		mulp	r4,@r5,r6,@r7,r8,@r9	; 25 54 65 56 67 58 69
		subp4	r4,@r5,r6,@r7		; 22 54 65 56 67
		subp	r4,@r5,r6,@r7		; 22 54 65 56 67
		subp6	r4,@r5,r6,@r7,r8,@r9	; 23 54 65 56 67 58 69
		subp	r4,@r5,r6,@r7,r8,@r9	; 23 54 65 56 67 58 69
		editpc	r4,@r5,@r7,@r9		; 38 54 65 67 69

;-------------------------------------------------

		; The instruction's length and the address of the branch
		; displacement depends on the previous arguments, which
		; depend on the operand size if immediate. Additionally,
		; ACBG and ACBH are two-byte opcodes.  So the range
		; of the reachable addresses varies:

		acbb	#100, #1, r4, *		; 9D 8F 64 01 54 F9 FF
		acbb	#100, #1, r4, *+0x8006	; 9D 8F 64 01 54 FF 7F
		expect	1370
		acbb	#100, #1, r4, *+0x8007
		endexpect
		acbb	#100, #1, r4, *-0x7ff9	; 9D 8F 64 01 54 00 80
		expect	1370
		acbb	#100, #1, r4, *-0x7ffa
		endexpect

		acbw	#100, #1, r4, *		; 3D 8F 64 00 01 54 F8 FF
		acbw	#100, #1, r4, *+0x8007	; 3D 8F 64 00 01 54 FF 7F
		expect	1370
		acbw	#100, #1, r4, *+0x8008
		endexpect
		acbw	#100, #1, r4, *-0x7ff8	; 3D 8F 64 00 01 54 00 80
		expect	1370
		acbw	#100, #1, r4, *-0x7ff9
		endexpect

		acbl	#100, #1, r4, *		; F1 8F 64 00 00 00 01 54 F6 FF
		acbl	#100, #1, r4, *+0x8009	; F1 8F 64 00 00 00 01 54 FF 7F
		expect	1370
		acbl	#100, #1, r4, *+0x800a
		endexpect
		acbl	#100, #1, r4, *-0x7ff6	; F1 8F 64 00 00 00 01 54 00 80
		expect	1370
		acbl 	#100, #1, r4, *-0x7ff7
		endexpect

		acbf	#100, #1, r4, *		; 4F 8F C8 43 00 00 08 54 F6 FF
		acbf	#100, #1, r4, *+0x8009	; 4F 8F C8 43 00 00 08 54 FF 7F
		expect	1370
		acbf	#100, #1, r4, *+0x800a
		endexpect
		acbf	#100, #1, r4, *-0x7ff6	; 4F 8F C8 43 00 00 08 54 00 80
		expect	1370
		acbf 	#100, #1, r4, *-0x7ff7
		endexpect

		acbd	#100, #1, r4, *		; 6F 8F C8 43 00 00 00 00 00 00 08 54 F2 FF
		acbd	#100, #1, r4, *+0x800d	; 6F 8F C8 43 00 00 00 00 00 00 08 54 FF 7F
		expect	1370
		acbd	#100, #1, r4, *+0x800e
		endexpect
		acbd	#100, #1, r4, *-0x7ff2	; 6F 8F C8 43 00 00 00 00 00 00 08 54 00 80
		expect	1370
		acbd 	#100, #1, r4, *-0x7ff3
		endexpect

		acbg	#100, #1, r4, *		; FD 4F 8F 79 40 00 00 00 00 00 00 08 54 F1 FF
		acbg	#100, #1, r4, *+0x800e	; FD 4F 8F 79 40 00 00 00 00 00 00 08 54 FF 7F
		expect	1370
		acbg	#100, #1, r4, *+0x800f
		endexpect
		acbg	#100, #1, r4, *-0x7ff1	; FD 4F 8F 79 40 00 00 00 00 00 00 08 54 00 80
		expect	1370
		acbg 	#100, #1, r4, *-0x7ff2
		endexpect

		acbh	#100, #1, r4, *		; FD 6F 8F 07 40 00 90 00 00 00 00 00 00 00 00 00 00 00 00 08 54 E9 FF
		acbh	#100, #1, r4, *+0x8016	; FD 6F 8F 07 40 00 90 00 00 00 00 00 00 00 00 00 00 00 00 08 54 FF 7F
		expect	1370
		acbh	#100, #1, r4, *+0x8017
		endexpect
		acbh	#100, #1, r4, *-0x7fe9	; FD 6F 8F 07 40 00 90 00 00 00 00 00 00 00 00 00 00 00 00 08 54 00 80
		expect	1370
		acbh 	#100, #1, r4, *-0x7fea
		endexpect

;-------------------------------------------------

		aobleq	#100, r4, *		; F3 8F 64 00 00 00 54 F8
		aobleq	#100, r4, *+0x87	; F3 8F 64 00 00 00 54 7F
		expect	1370
		aobleq	#100, r4, *+0x88
		endexpect
		aobleq	#100, r4, *-0x78	; F3 8F 64 00 00 00 54 80
		expect	1370
		aobleq	#100, r4, *-0x79
		endexpect

;-------------------------------------------------

		aoblss	#100, r4, *		; F2 8F 64 00 00 00 54 F8
		aoblss	#100, r4, *+0x87	; F2 8F 64 00 00 00 54 7F
		expect	1370
		aoblss	#100, r4, *+0x88
		endexpect
		aoblss	#100, r4, *-0x78	; F2 8F 64 00 00 00 54 80
		expect	1370
		aoblss	#100, r4, *-0x79
		endexpect

;-------------------------------------------------

all_b_b		macro	op, code
		op	*	; code FE
		op	*+0x81	; code 7F
		expect	1370
		op	*+0x82
		endexpect
		op	*-0x7e	; code 80
		expect	1370
		op	*-0x7f
		endexpect
		endm

		all_b_b	bgtr, 14
		all_b_b bleq, 15
		all_b_b bneq, 12
		all_b_b bnequ, 12
		all_b_b beql, 13
		all_b_b beqlu, 13
		all_b_b bgeq, 18
		all_b_b blss, 19
		all_b_b bgtru, 1a
		all_b_b blequ, 1b
		all_b_b bvc, 1c
		all_b_b bvs, 1d
		all_b_b bgequ, 1e
		all_b_b bcc, 1e
		all_b_b blssu,1f
		all_b_b bcs, 1f

;-------------------------------------------------

all_bf_b	macro	op, code
		op	#10,@r5,*	; code 0A 65 FC
		op	#10,@r5,*+0x83	; code 0A 65 7F
		expect	1370
		op	#10,@r5,*+0x84
		endexpect
		op	#10,@r5,*-0x7c	; code 0A 65 80
		expect	1370
		op	#10,@r5,*-0x7d
		endexpect
		op	#10,r5,*	; code 0A 55 FC
		op	#10,r5,*+0x83	; code 0A 55 7F
		expect	1370
		op	#10,r5,*+0x84
		endexpect
		op	#10,r5,*-0x7c	; code 0A 55 80
		expect	1370
		op	#10,r5,*-0x7d
		endexpect
		expect	2180
		op	#32,r5,*
		endexpect
		endm

		all_bf_b bbc, e1
		all_bf_b bbs, e0
		all_bf_b bbss, e2
		all_bf_b bbcs, e3
		all_bf_b bbsc, e4
		all_bf_b bbcc, e5
		all_bf_b bbssi, e6
		all_bf_b bbcci, e7

;-------------------------------------------------

all_bl_b	macro	op, code
		op	@r5,*		; code 65 FD
		op	@r5,*+0x82	; code 65 7F
		expect	1370
		op	@r5,*+0x83
		endexpect
		op	@r5,*-0x7d	; code 65 80
		expect	1370
		op	@r5,*-0x7e
		endexpect
		op	r5,*		; code 55 FD
		op	r5,*+0x82	; code 55 7F
		expect	1370
		op	r5,*+0x83
		endexpect
		op	r5,*-0x7d	; code 55 80
		expect	1370
		op	r5,*-0x7e
		endexpect
		endm

		all_bl_b blbs, e8
		all_bl_b blbc, e9

;-------------------------------------------------

__S		set	""
all_br		macro	op, codeb, codew
__S		set	"b"
		op{__S}	*		; codeb FE
		op{__S}	*+0x81		; codeb 7F
		expect	1370
		op{__S}	*+0x82
		endexpect
		op{__S}	*-0x7e		; codeb 80
		expect	1370
		op{__S}	*-0x7f

__S		set	"w"
		endexpect
		op{__S}	*		; codew FD FF
		op{__S}	*+0x8002	; codew FF 7F
		expect	1370
		op{__S}	*+0x8003
		endexpect
		op{__S}	*-0x7ffd	; codew 00 80
		expect	1370
		op{__S}	*-0x7ffe
		endexpect

		op	*		; codeb FE
		op	*+0x81		; codeb 7F
		op	*+0x82		; codew 7F 00
		op	*-0x7e		; codeb 80
		op	*-0x7f		; codew 7E FF
		op	*+0x8002	; codew FF 7F
		expect	1370
		op	*+0x8003
		endexpect
		op	*-0x7ffd	; codew 00 80
		expect	1370
		op	*-0x7ffe
		endexpect
		
		endm

		all_br	br,11,31
		all_br	bsb,10,30

;-------------------------------------------------

		; 8F 54 20 05 60 00 C4 00 28 01 8C 01 F0 01 54 02
		caseb	r4, #32, #(37-32), *+100, *+200, *+300, *+400, *+500, *+600
		expect	460
		; 8F 54 20 05 60 00 C4 00 28 01 8C 01 F0 01
		caseb	r4, #32, #(37-32), *+100, *+200, *+300, *+400, *+500
		endexpect
		; AF 54 8F E8 03 03 5E 00 C2 00 26 01 8A 01
		casew	r4, #1000, #(1003-1000), *+100, *+200, *+300, *+400
		; CF 54 8F E8 03 00 00 03 5C 00 C0 00 24 01 88 01
		casel	r4, #1000, #(1003-1000), *+100, *+200, *+300, *+400


;-------------------------------------------------

		sobgeq	r4,*		; F4 54 FD
		sobgeq	r4,*+0x82	; F4 54 7F
		expect	1370
		sobgeq	r4,*+0x83
		endexpect
		sobgeq	r4,*-0x7d	; F4 54 80
		expect	1370
		sobgeq	r4,*-0x7e
		endexpect

		sobgtr	r4,*		; F5 54 FD
		sobgtr	r4,*+0x82	; F5 54 7F
		expect	1370
		sobgtr	r4,*+0x83
		endexpect
		sobgtr	r4,*-0x7d	; F5 54 80
		expect	1370
		sobgtr	r4,*-0x7e
		endexpect

;-------------------------------------------------

		irp	instr,blkb,blkw,blkl,blkq,blko,blkf,blkd,blkg,blkh
		word	_next-_start
_start		instr	3
_next
		endm

		byte	1,2,3		; 01 02 03
		;expect
		;byte	"dontdothat"
		;endexpect
		word	1,2,3		; 01 00 02 00 03 00
		lword	1,2,3		; 01 00 00 00 02 00 00 00 03 00 00 00
		quad	1,2,3		; 01 00 00 00 00 00 00 00 02 00 00 00 00 00 00 00 03 00 00 00 00 00 00 00
		octa	1,2,3		; 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
					; 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
					; 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

		float	1.0,2.0,3.0	; 80 40 00 00 00 41 00 00 40 41 00 00
		f_floating 1.0,2.0,3.0	; 80 40 00 00 00 41 00 00 40 41 00 00
		double	1.0,2.0,3.0	; 80 40 00 00 00 00 00 00 00 41 00 00 00 00 00 00 40 41 00 00 00 00 00 00
		d_floating 1.0,2.0,3.0	; 80 40 00 00 00 00 00 00 00 41 00 00 00 00 00 00 40 41 00 00 00 00 00 00
		g_floating 1.0,2.0,3.0	; 10 40 00 00 00 00 00 00 20 40 00 00 00 00 00 00 28 40 00 00 00 00 00 00
		h_floating 1.0,2.0,3.0	; 01 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00
					; 02 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00
					; 02 40 00 80 00 00 00 00 00 00 00 00 00 00 00 00

		ascii	"The quick brown fox jumps over the lazy dog."
		asciz	"The quick brown fox jumps over the lazy dog."
		ascic	"The quick brown fox jumps over the lazy dog."

		packed	-12,pack	; 01 2D
		byte	pack		; 02
		packed	500		; 50 0C
		packed	0		; 0C
		packed	-0,sum		; 0C
		byte	sum		; 01
		packed	"1234",e6	; 01 23 4C
		packed	"1234567890123456789012345678901",maxpack ; 12 34 56 78 90 12 34 56 78 90 12 34 56 78 90 1C
		byte	maxpack		; 1F
		expect	1324
		packed  "12345678901234567890123456789012" ; too long
		endexpect
		expect	1323
		packed  "1234BCD"
		endexpect
