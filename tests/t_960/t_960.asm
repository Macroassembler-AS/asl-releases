	cpu	80960
	org	0

	fpu	on
	supmode on

start:
	flushreg
	fmark
	mark
	ret
	syncf
	faultno
	faultg
	faulte
	faultge
	faultl
	faultne
	faultle
	faulto

	addc	r3,r4,r5	; lokale Register
	addc	g3,g4,g5	; globale Register
	addc	g3,g5		; impl. src2=dest
	addc	10,r4,g10	; src1 immediate
	addc	g2,20,g4	; src2 immediate

	addr	r3,r4,r5	; Float: normale Register
	addr	fp2,r4,r5	; Float-Register
	addr	r3,fp1,r5
	addr	r3,r4,fp3
	addr	0,r4,r5		; Float-Konstanten
	addr	r3,1,r5

	addc	sp,fp,rip	; Sonderregister

	addi	r3,r4,r5	; nur noch mal zum Testen der Opcodes...
	addo	r3,r4,r5	

	calls	g2		; nur ein Operand:
	calls	4

	chkbit	7,r5		; kein Ziel

	classr	fp1		; ein Float-Operand
	classrl	g4

	cosr	fp1		; dito mit Ziel
	cosr	g4,fp2

	modpc	r3,r4,r5	; nur im Supervisor-Mode erlaubt

	bbc	r4,g5,$+32	; COBR-Anweisungen
	bbs	10,r10,$+32
	cmpobge 10,r4,$+32
	testne	r5

	b	$-32		; CTRL-Anweisungen
	call	$+32
	bal	$+64
	ble	$-64

	bx	(r5)		; MEMA ohne disp
	bx	2345(r5)	; MEMA mit disp
        bx	2345		; MEMA ohne base
	bx	-30(r5)		; MEMB base+disp
	bx	5000(r5)	; dito positiv
	bx	$(ip)		; PC-relativ
	bx	[r4]		; nur Index
	bx	[r4*1]		; Scaling
	bx      [r4*2]
        bx      [r4*4]
        bx      [r4*8]
	bx	[r4*16]
	bx	0(r5)[r4]	; base+index
	bx	12345678h	; nur disp
	bx	450[r6*4]	; disp+index
	bx	123(r5)[r6*4]	; volles Programm

	st	r7,123(r5)[r6*4]; mit 2 Ops
	ld	123(r5)[r6*4],r7

	db	1
	align	4
	db	1,2
	align	4
	db	1,2,3
	align	4
	db	1,2,3,4
	dw	1
	align	4
	dw	1,2
	dd	1
	dd	1.0
	dq	1.0
	dt	1.0
	dw	0

; register aliases

reg_r0	reg	r0
reg_r1	reg	r1
reg_r2	reg	r2
reg_r3	reg	r3
reg_r4	reg	r4
reg_r5	reg	r5
reg_r6	reg	r6
reg_r7	reg	r7
reg_r8	reg	r8
reg_r9	reg	r9
reg_r10	reg	r10
reg_r11	reg	r11
reg_r12	reg	r12
reg_r13	reg	r13
reg_r14	reg	r14
reg_r15	reg	r15
reg_g0	equ	g0
reg_g1	equ	g1
reg_g2	equ	g2
reg_g3	equ	g3
reg_g4	equ	g4
reg_g5	equ	g5
reg_g6	equ	g6
reg_g7	equ	g7
reg_g8	equ	g8
reg_g9	equ	g9
reg_g10	equ	g10
reg_g11	equ	g11
reg_g12	equ	g12
reg_g13	equ	g13
reg_g14	equ	g14
reg_g15	equ	g15
reg_fp	reg	fp
reg_sp	reg	sp
reg_pfp	reg	pfp
reg_rip	reg	rip
reg_fp0	equ	fp0
reg_fp1	equ	fp1
reg_fp2	equ	fp2
reg_fp3	equ	fp3

	addc	r3,r4,r5
	addc	reg_r3,reg_r4,reg_r5
	addc	g3,g4,g5
	addc	reg_g3,reg_g4,reg_g5
	addc    sp,fp,rip
	addc    reg_sp,reg_fp,reg_rip
	st	r7,123(r5)[r6*4]
	st	reg_r7,123(reg_r5)[reg_r6*4]
	addr	fp2,r4,r5
	addr	reg_fp2,reg_r4,reg_r5

	space	32

	word	1,2,3,4,5
