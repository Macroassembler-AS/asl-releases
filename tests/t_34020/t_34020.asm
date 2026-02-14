	cpu	34020
	page	0
	org	100000h

a15	equ	100

	addxyi	1234h,b5	; 0C15 1234 0000
	expect	1130
	addxyi	1234h,b5,w
	endexpect
	addxyi	1234h,b5,l	; 0C15 1234 0000
	addxyi	12345678h,b5	; 0C15 5678 1234
	expect	1130
	addxyi	12345678h,b5,w
	endexpect
	addxyi	12345678h,b5,l	; 0C15 5678 1234

	blmove	0,0		; 00F0
	blmove	0,1		; 00F1
	blmove	1,0		; 00F2
	blmove	1,1		; 00F3
	expect	1320
	blmove	2,1
	endexpect

CMPF_RA3_RB4 .set 069005h
	cexec	0, CMPF_RA3_RB4, 3	; D80A 6690
	cexec	0, CMPF_RA3_RB4, 3, l	; 0600 0500 6690

	clip			; 08F2

MOV_RB2_20 .set 00124fh
	cmovcg	A7,MOV_RB2_20		; 0667 4F00 0012
	cmovcg	A7,MOV_RB2_20,		; 0667 4F00 0012
	cmovcg	A7,MOV_RB2_20,3		; 0667 4F00 6012
	cmovcg	A7,A2,MOV_RB2_20	; 0667 4F82 0012
	cmovcg	A7,A2,MOV_RB2_20,	; 0667 4F82 0012
	cmovcg	A7,A2,MOV_RB2_20,3	; 0667 4F82 6012
	cmovcg	A7,A2,0,MOV_RB2_20	; 0667 4F02 0012
	cmovcg	A7,A2,0,MOV_RB2_20,	; 0667 4F02 0012
	cmovcg	A7,A2,0,MOV_RB2_20,3	; 0667 4F02 6012
	cmovcg	A7,A2,1,MOV_RB2_20	; 0667 4F82 0012
	cmovcg	A7,A2,1,MOV_RB2_20,	; 0667 4F82 0012
	cmovcg	A7,A2,1,MOV_RB2_20,3	; 0667 4F82 6012

MOVE5_RB3 .set 00138Eh
	cmovcm	*a0+,32,0,MOVE5_RB3	; 06A0 8E00 0013
	cmovcm	*a0+,32,0,MOVE5_RB3,	; 06A0 8E00 0013
	cmovcm	*a0+,32,0,MOVE5_RB3,2	; 06A0 8E00 4013
	cmovcm	*b7+,8,1,MOVE5_RB3	; 06B7 8E90 0013
	cmovcm	*b7+,8,1,MOVE5_RB3,	; 06B7 8E90 0013
	cmovcm	*b7+,8,1,MOVE5_RB3,3	; 06B7 8E90 6013
	cmovcm	*a3+,5,0,MOVE5_RB3	; 06A3 8E05 0013
	cmovcm	*a3+,5,0,MOVE5_RB3,	; 06A3 8E05 0013
	cmovcm	*a3+,5,0,MOVE5_RB3,1	; 06A3 8E05 2013
MOVE_RA1_20 .set 00018Eh
	cmovcm	*-a3,19,0,MOVE_RA1_20	; 06C3 8E13 0001
	cmovcm	*-a3,19,0,MOVE_RA1_20,	; 06C3 8E13 0001
	cmovcm	*-a3,19,0,MOVE_RA1_20,3	; 06C3 8E13 6001
	cmovcm	*-a0,2,1,MOVE_RA1_20	; 06C0 8E84 0001
	cmovcm	*-a0,2,1,MOVE_RA1_20,	; 06C0 8E84 0001
	cmovcm	*-a0,2,1,MOVE_RA1_20,3	; 06C0 8E84 6001
	cmovcm	*-b7,8,1,MOVE_RA1_20	; 06D7 8E90 0001
	cmovcm	*-b7,8,1,MOVE_RA1_20,	; 06D7 8E90 0001
	cmovcm	*-b7,8,1,MOVE_RA1_20,3	; 06D7 8E90 6001
	cmovcm	*-a1,5,0,MOVE_RA1_20	; 06C1 8E05 0001
	cmovcm	*-a1,5,0,MOVE_RA1_20,	; 06C1 8E05 0001
	cmovcm	*-a1,5,0,MOVE_RA1_20,3	; 06C1 8E05 6001

wxyz	.set	012345h	
	cmovcs	wxyz			; 0660 4501 0123
	cmovcs	wxyz,			; 0660 4501 0123
	cmovcs	wxyz,2			; 0660 4501 4123

MOVE_R3 .set	00034ch
	cmovgc	b5,MOVE_R3		; 0635 4C00 0003
	cmovgc	b5,MOVE_R3,		; 0635 4C00 0003
	cmovgc	b5,MOVE_R3,2		; 0635 4C00 4003
MOVEF_RA1 .set	00014dh
	cmovgc	b3,b4,0,MOVEF_RA1,	; 0653 4D14 0001
	cmovgc	b3,b4,0,MOVEF_RA1,	; 0653 4D14 0001
	cmovgc	b3,b4,0,MOVEF_RA1,2	; 0653 4D14 4001

	cmovmc	*a0+,32,0,wxyz		; 0680 4500 0123
	cmovmc	*a0+,32,0,wxyz,		; 0680 4500 0123
	cmovmc	*a0+,32,0,wxyz,2	; 0680 4500 4123
	cmovmc	*b7+,8,1,wxyz		; 0690 4597 0123
	cmovmc	*b7+,8,1,wxyz,		; 0690 4597 0123
	cmovmc	*b7+,8,1,wxyz,2		; 0690 4597 4123
MOVD3_RB5 equ	00158dh
	cmovmc	*a3+,3,1,MOVD3_RB5	; 0686 8D83 0015
	cmovmc	*a3+,3,1,MOVD3_RB5,	; 0686 8D83 0015
	cmovmc	*a3+,3,1,MOVD3_RB5,1	; 0686 8D83 2015
	cmovmc	*-a0,32,0,wxyz		; 0820 4500 0123
	cmovmc	*-a0,32,0,wxyz,		; 0820 4500 0123
	cmovmc	*-a0,32,0,wxyz,2	; 0820 4500 4123
	cmovmc	*-b7,8,1,wxyz		; 0830 4597 0123
	cmovmc	*-b7,8,1,wxyz,		; 0830 4597 0123
	cmovmc	*-b7,8,1,wxyz,2		; 0830 4597 4123
MC_3_DOUB: .set	00038dh
stk	reg	sp
	cmovmc	*-stk,3,1,MC_3_DOUB	; 0826 8D8F 0003
	cmovmc	*-stk,3,1,MC_3_DOUB,	; 0826 8D8F 0003
	cmovmc	*-stk,3,1,MC_3_DOUB,1	; 0826 8D8F 2003
	cmovmc	*a0+,a2,0,wxyz		; 06E2 4500 0123
	cmovmc	*a0+,a2,0,wxyz,		; 06E2 4500 0123
	cmovmc	*a0+,a2,0,wxyz,2	; 06E2 4500 4123
	cmovmc	*a3+,b7,1,wxyz		; 06F7 4583 0123
	cmovmc	*a3+,b7,1,wxyz,		; 06F7 4583 0123
	cmovmc	*a3+,b7,1,wxyz,2	; 06F7 4583 4123
MOV0_RB5 .set 001580h
	cmovmc	*a3+,b4,1,MOV0_RB5	; 06F4 8083 0015
	cmovmc	*a3+,b4,1,MOV0_RB5,	; 06F4 8083 0015
	cmovmc	*a3+,b4,1,MOV0_RB5,1	; 06F4 8083 2015

	cmpk	1,b3		; 3433
	cmpk	32,b3		; 3413
	expect	1315
	cmpk	0,b3
	endexpect
	expect	1320
	cmpk	33,b3
	endexpect

	cvdxyl	a0		; 0A80
	cvdxyl	a1		; 0A81
	cvdxyl	a2		; 0A82
	cvdxyl	a3		; 0A83
	cvdxyl	a4		; 0A84
	cvdxyl	a5		; 0A85
	cvdxyl	a6		; 0A86
	cvdxyl	a7		; 0A87
	cvdxyl	a8		; 0A88
	cvdxyl	a9		; 0A89
	cvdxyl	a10		; 0A8A
	cvdxyl	a11		; 0A8B
	cvdxyl	a12		; 0A8C
	cvdxyl	a13		; 0A8D
	cvdxyl	a14		; 0A8E
	expect	1147
	cvdxyl	a15
	endexpect
	cvdxyl	sp		; 0A8F
	cvdxyl	b0		; 0A90
	cvdxyl	saddr
	cvdxyl	b1		; 0A91
	cvdxyl	sptch
	cvdxyl	b2		; 0A92
	cvdxyl	daddr
	cvdxyl	b3		; 0A93
	cvdxyl	dptch
	cvdxyl	b4		; 0A94
	cvdxyl	offset
	cvdxyl	b5		; 0A95
	cvdxyl	wstart
	cvdxyl	b6		; 0A96
	cvdxyl	wend
	cvdxyl	b7		; 0A97
	cvdxyl	dydx
	cvdxyl	b8		; 0A98
	cvdxyl	color0
	cvdxyl	b9		; 0A99
	cvdxyl	color1
	cvdxyl	b10		; 0A9A
	cvdxyl	count
	cvdxyl	b11		; 0A9B
	cvdxyl	inc1
	cvdxyl	b12		; 0A9C
	cvdxyl	inc2
	cvdxyl	b13		; 0A9D
	cvdxyl	pattrn
	cvdxyl	b14		; 0A9E

	cvmxyl	a0		; 0A60
	cvmxyl	a1		; 0A61
	cvmxyl	a2		; 0A62
	cvmxyl	a3		; 0A63
	cvmxyl	a4		; 0A64
	cvmxyl	a5		; 0A65
	cvmxyl	a6		; 0A66
	cvmxyl	a7		; 0A67
	cvmxyl	a8		; 0A68
	cvmxyl	a9		; 0A69
	cvmxyl	a10		; 0A6A
	cvmxyl	a11		; 0A6B
	cvmxyl	a12		; 0A6C
	cvmxyl	a13		; 0A6D
	cvmxyl	a14		; 0A6E
	expect	1147
	cvmxyl	a15
	endexpect
	cvmxyl	sp		; 0A6F
	cvmxyl	b0		; 0A70
	cvmxyl	saddr
	cvmxyl	b1		; 0A71
	cvmxyl	sptch
	cvmxyl	b2		; 0A72
	cvmxyl	daddr
	cvmxyl	b3		; 0A73
	cvmxyl	dptch
	cvmxyl	b4		; 0A74
	cvmxyl	offset
	cvmxyl	b5		; 0A75
	cvmxyl	wstart
	cvmxyl	b6		; 0A76
	cvmxyl	wend
	cvmxyl	b7		; 0A77
	cvmxyl	dydx
	cvmxyl	b8		; 0A78
	cvmxyl	color0
	cvmxyl	b9		; 0A79
	cvmxyl	color1
	cvmxyl	b10		; 0A7A
	cvmxyl	count
	cvmxyl	b11		; 0A7B
	cvmxyl	inc1
	cvmxyl	b12		; 0A7C
	cvmxyl	inc2
	cvmxyl	b13		; 0A7D
	cvmxyl	pattrn
	cvmxyl	b14		; 0A7E

	cvsxyl	a2,a5		; EA45
	cvsxyl	b5,b2		; EAB2
	cvsxyl	a7,sp		; EAEF
	cvsxyl	sp,a8		; EBE8
	cvsxyl	b9,sp		; EB3F
	cvsxyl	sp,b10		; EBFA
	expect	1350
	cvsxyl	a1,b10
	endexpect

	exgps	a0		; 02A0
	exgps	a1		; 02A1
	exgps	a2		; 02A2
	exgps	a3		; 02A3
	exgps	a4		; 02A4
	exgps	a5		; 02A5
	exgps	a6		; 02A6
	exgps	a7		; 02A7
	exgps	a8		; 02A8
	exgps	a9		; 02A9
	exgps	a10		; 02AA
	exgps	a11		; 02AB
	exgps	a12		; 02AC
	exgps	a13		; 02AD
	exgps	a14		; 02AE
	expect	1147
	exgps	a15
	endexpect
	exgps	sp		; 02AF
	exgps	b0		; 02B0
	exgps	saddr
	exgps	b1		; 02B1
	exgps	sptch
	exgps	b2		; 02B2
	exgps	daddr
	exgps	b3		; 02B3
	exgps	dptch
	exgps	b4		; 02B4
	exgps	offset
	exgps	b5		; 02B5
	exgps	wstart
	exgps	b6		; 02B6
	exgps	wend
	exgps	b7		; 02B7
	exgps	dydx
	exgps	b8		; 02B8
	exgps	color0
	exgps	b9		; 02B9
	exgps	color1
	exgps	b10		; 02BA
	exgps	count
	exgps	b11		; 02BB
	exgps	inc1
	exgps	b12		; 02BC
	exgps	inc2
	exgps	b13		; 02BD
	exgps	pattrn
	exgps	b14		; 02BE

	fline	0		; DE1A
	fline	1		; DE9A
	expect	1320
	fline	2
	endexpect

	fpixeq			; 0ABB

	fpixne			; 0ADB

	getps	a0		; 02C0
	getps	a1		; 02C1
	getps	a2		; 02C2
	getps	a3		; 02C3
	getps	a4		; 02C4
	getps	a5		; 02C5
	getps	a6		; 02C6
	getps	a7		; 02C7
	getps	a8		; 02C8
	getps	a9		; 02C9
	getps	a10		; 02CA
	getps	a11		; 02CB
	getps	a12		; 02CC
	getps	a13		; 02CD
	getps	a14		; 02CE
	expect	1147
	getps	a15
	endexpect
	getps	sp		; 02CF
	getps	b0		; 02D0
	getps	saddr
	getps	b1		; 02D1
	getps	sptch
	getps	b2		; 02D2
	getps	daddr
	getps	b3		; 02D3
	getps	dptch
	getps	b4		; 02D4
	getps	offset
	getps	b5		; 02D5
	getps	wstart
	getps	b6		; 02D6
	getps	wend
	getps	b7		; 02D7
	getps	dydx
	getps	b8		; 02D8
	getps	color0
	getps	b9		; 02D9
	getps	color1
	getps	b10		; 02DA
	getps	count
	getps	b11		; 02DB
	getps	inc1
	getps	b12		; 02DC
	getps	inc2
	getps	b13		; 02DD
	getps	pattrn
	getps	b14		; 02DE

	idle			; 0040

	linit			; 0C57

	mwait			; 0080

	pfill			; 0A37
	pfill	xy

	pixblt	l,m,l		; 0E17

	retm			; 0860

	rmo	a2,a5		; 7A45
	rmo	b5,b2		; 7AB2
	rmo	a7,sp		; 7AEF
	rmo	sp,a8		; 7BE8
	rmo	b9,sp		; 7B3F
	rmo	sp,b10		; 7BFA
	expect	1350
	rmo	a1,b10
	endexpect

	rpix	a0		; 0280
	rpix	a1		; 0281
	rpix	a2		; 0282
	rpix	a3		; 0283
	rpix	a4		; 0284
	rpix	a5		; 0285
	rpix	a6		; 0286
	rpix	a7		; 0287
	rpix	a8		; 0288
	rpix	a9		; 0289
	rpix	a10		; 028A
	rpix	a11		; 028B
	rpix	a12		; 028C
	rpix	a13		; 028D
	rpix	a14		; 028E
	expect	1147
	rpix	a15
	endexpect
	rpix	sp		; 028F
	rpix	b0		; 0290
	rpix	saddr
	rpix	b1		; 0291
	rpix	sptch
	rpix	b2		; 0292
	rpix	daddr
	rpix	b3		; 0293
	rpix	dptch
	rpix	b4		; 0294
	rpix	offset
	rpix	b5		; 0295
	rpix	wstart
	rpix	b6		; 0296
	rpix	wend
	rpix	b7		; 0297
	rpix	dydx
	rpix	b8		; 0298
	rpix	color0
	rpix	b9		; 0299
	rpix	color1
	rpix	b10		; 029A
	rpix	count
	rpix	b11		; 029B
	rpix	inc1
	rpix	b12		; 029C
	rpix	inc2
	rpix	b13		; 029D
	rpix	pattrn
	rpix	b14		; 029E

	setcdp			; 0273

	setcmp			; 02FB

	setcsp			; 0251

	swapf	*a2,a5		; 7E45
	swapf	*a2,a5,0
	swapf	*b5,b2		; 7EB2
	swapf	*b5,b2,0
	swapf	*a7,sp		; 7EEF
	swapf	*a7,sp,0
	swapf	*sp,a8		; 7FE8
	swapf	*sp,a8,0
	swapf	*b9,sp		; 7F3F
	swapf	*b9,sp,0
	swapf	*sp,b10		; 7FFA
	swapf	*sp,b10,0
	expect	1350
	swapf	*a1,b10
	endexpect
	expect	1350
	swapf	*sp,b10,1
	endexpect
	expect	1350
	swapf	sp,b10
	endexpect


	expect	1350
	swapf	*sp,b10,1
	endexpect

	expect	1350
	swapf	*sp,b10,1
	endexpect


	tfill			; 0EFA
	tfill	xy

	trapl	0		; 080F 0000
	trapl	-32768		; 080F 8000
	trapl	32767		; 080F 7FFF
	expect	1315
	trapl	-32769
	endexpect
	expect	1320
	trapl	32768
	endexpect

	vblt	b,l		; 0857

	vfill	l		; 0A57

	vlcol			; 0A00
