	cpu	34010
	page	0
	org	100000h

a15	equ	100

	abs	a0		; 0380
	abs	a1		; 0381
	abs	a2		; 0382
	abs	a3		; 0383
	abs	a4		; 0384
	abs	a5		; 0385
	abs	a6		; 0386
	abs	a7		; 0387
	abs	a8		; 0388
	abs	a9		; 0389
	abs	a10		; 038A
	abs	a11		; 038B
	abs	a12		; 038C
	abs	a13		; 038D
	abs	a14		; 038E
	expect	1147
	abs	a15
	endexpect
	abs	sp		; 038F
	abs	b0		; 0390
	abs	saddr
	abs	b1		; 0391
	abs	sptch
	abs	b2		; 0392
	abs	daddr
	abs	b3		; 0393
	abs	dptch
	abs	b4		; 0394
	abs	offset
	abs	b5		; 0395
	abs	wstart
	abs	b6		; 0396
	abs	wend
	abs	b7		; 0397
	abs	dydx
	abs	b8		; 0398
	abs	color0
	abs	b9		; 0399
	abs	color1
	abs	b10		; 039A
	abs	count
	abs	b11		; 039B
	abs	inc1
	abs	b12		; 039C
	abs	inc2
	abs	b13		; 039D
	abs	pattrn
	abs	b14		; 039E

	add	a2,a5		; 4045
	add	b5,b2		; 40B2
	add	a7,sp		; 40EF
	add	sp,a8		; 41E8
	add	b9,sp		; 413F
	add	sp,b10		; 41FA
	expect	1350
	add	a1,b10
	endexpect

	addc	a2,a5		; 4245
	addc	b5,b2		; 42B2
	addc	a7,sp		; 42EF
	addc	sp,a8		; 43E8
	addc	b9,sp		; 433F
	addc	sp,b10		; 43FA
	expect	1350
	addc	a1,b10
	endexpect

	addi	1234h,b5	; 0B15 1234
	addi	1234h,b5,w	; 0B15 1234
	addi	1234h,b5,l	; 0B35 1234 0000
	addi	12345678h,b5	; 0B35 5678 1234
	expect	260
	addi	12345678h,b5,w	; 0B15 5678
	endexpect
	addi	12345678h,b5,l	; 0B35 5678 1234

	addk	1,b3		; 1033
	addk	32,b3		; 1013
	expect	1315
	addk	0,b3
	endexpect
	expect	1320
	addk	33,b3
	endexpect

	addxy	a2,a5		; E045
	addxy	b5,b2		; E0B2
	addxy	a7,sp		; E0EF
	addxy	sp,a8		; E1E8
	addxy	b9,sp		; E13F
	addxy	sp,b10		; E1FA
	expect	1350
	addxy	a1,b10
	endexpect

	and	a2,a5		; 5045
	and	b5,b2		; 50B2
	and	a7,sp		; 50EF
	and	sp,a8		; 51E8
	and	b9,sp		; 513F
	and	sp,b10		; 51FA
	expect	1350
	and	a1,b10
	endexpect

	andi	1234h,b5	; 0B95 EDCB FFFF
	expect	1130
	andi	1234h,b5,w
	endexpect
	andi	1234h,b5,l	; 0B95 EDCB FFFF
	andi	12345678h,b5	; 0B95 A987 EDCB
	expect	1130
	andi	12345678h,b5,w
	endexpect
	andi	12345678h,b5,l	; 0B95 A987 EDCB

	andn	a2,a5		; 5245
	andn	b5,b2		; 52B2
	andn	a7,sp		; 52EF
	andn	sp,a8		; 53E8
	andn	b9,sp		; 533F
	andn	sp,b10		; 53FA
	expect	1350
	andn	a1,b10
	endexpect

	andni	1234h,b5	; 0B95 1234 0000
	expect	1130
	andni	1234h,b5,w
	endexpect
	andni	1234h,b5,l	; 0B95 1234 0000
	andni	12345678h,b5	; 0B95 5678 1234
	expect	1130
	andni	12345678h,b5,w
	endexpect
	andni	12345678h,b5,l	; 0B95 5678 1234

	btst	a2,a5		; 4A45
	btst	b5,b2		; 4AB2
	btst	a7,sp		; 4AEF
	btst	sp,a8		; 4BE8
	btst	b9,sp		; 4B3F
	btst	sp,b10		; 4BFA
	expect	1350
	btst	a1,b10
	endexpect

	call	a0		; 0920
	call	a1		; 0921
	call	a2		; 0922
	call	a3		; 0923
	call	a4		; 0924
	call	a5		; 0925
	call	a6		; 0926
	call	a7		; 0927
	call	a8		; 0928
	call	a9		; 0929
	call	a10		; 092A
	call	a11		; 092B
	call	a12		; 092C
	call	a13		; 092D
	call	a14		; 092E
	expect	1147
	call	a15
	endexpect
	call	sp		; 092F
	call	b0		; 0930
	call	saddr
	call	b1		; 0931
	call	sptch
	call	b2		; 0932
	call	daddr
	call	b3		; 0933
	call	dptch
	call	b4		; 0934
	call	offset
	call	b5		; 0935
	call	wstart
	call	b6		; 0936
	call	wend
	call	b7		; 0937
	call	dydx
	call	b8		; 0938
	call	color0
	call	b9		; 0939
	call	color1
	call	b10		; 093A
	call	count
	call	b11		; 093B
	call	inc1
	call	b12		; 093C
	call	inc2
	call	b13		; 093D
	call	pattrn
	call	b14		; 093E

	calla	12345670h	; 0D5F 5670 1234
	expect	1352
	calla	12345678h
	endexpect

	callr	$		; 0D3F FFFE
	callr	$-7e0h		; 0D3F FF80
	callr	$+810h		; 0D3F 007F
	callr	$-7f0h		; 0D3F FF7F
	callr	$+820h		; 0D3F 0080
	callr	$-7ffe0h	; 0D3F 8000
	callr	$+80010h	; 0D3F 7FFF
	expect	1370
	callr	$-7fff0h
	endexpect
	expect	1370
	callr	$+80020h
	endexpect

	clr	a0		; 5600
	clr	a1		; 5621
	clr	a2		; 5642
	clr	a3		; 5663
	clr	a4		; 5684
	clr	a5		; 56A5
	clr	a6		; 56C6
	clr	a7		; 56E7
	clr	a8		; 5708
	clr	a9		; 5729
	clr	a10		; 574A
	clr	a11		; 576B
	clr	a12		; 578C
	clr	a13		; 57AD
	clr	a14		; 57CE
	expect	1147
	clr	a15
	endexpect
	clr	sp		; 57EF
	clr	b0		; 5610
	clr	saddr
	clr	b1		; 5631
	clr	sptch
	clr	b2		; 5652
	clr	daddr
	clr	b3		; 5673
	clr	dptch
	clr	b4		; 5694
	clr	offset
	clr	b5		; 53B5
	clr	wstart
	clr	b6		; 56D6
	clr	wend
	clr	b7		; 56F7
	clr	dydx
	clr	b8		; 5718
	clr	color0
	clr	b9		; 5739
	clr	color1
	clr	b10		; 575A
	clr	count
	clr	b11		; 577B
	clr	inc1
	clr	b12		; 579C
	clr	inc2
	clr	b13		; 57BD
	clr	pattrn
	clr	b14		; 57DE

	clrc			; 0320

	cmp	a2,a5		; 4845
	cmp	b5,b2		; 48B2
	cmp	a7,sp		; 48EF
	cmp	sp,a8		; 49E8
	cmp	b9,sp		; 493F
	cmp	sp,b10		; 49FA
	expect	1350
	cmp	a1,b10
	endexpect

	cmpi	1234h,b5	; 0B55 1234
	cmpi	1234h,b5,w	; 0B55 1234
	cmpi	1234h,b5,l	; 0B75 1234 0000
	cmpi	12345678h,b5	; 0B75 5678 1234
	expect	260
	cmpi	12345678h,b5,w	; 0B55 5678
	endexpect
	cmpi	12345678h,b5,l	; 0B75 5678 1234

	cmpxy	a2,a5		; E445
	cmpxy	b5,b2		; E4B2
	cmpxy	a7,sp		; E4EF
	cmpxy	sp,a8		; E5E8
	cmpxy	b9,sp		; E53F
	cmpxy	sp,b10		; E5FA
	expect	1350
	cmpxy	a1,b10
	endexpect

	cpw	a2,a5		; E645
	cpw	b5,b2		; E6B2
	cpw	a7,sp		; E6EF
	cpw	sp,a8		; E7E8
	cpw	b9,sp		; E73F
	cpw	sp,b10		; E7FA
	expect	1350
	cpw	a1,b10
	endexpect

	cvxyl	a2,a5		; E845
	cvxyl	b5,b2		; E8B2
	cvxyl	a7,sp		; E8EF
	cvxyl	sp,a8		; E9E8
	cvxyl	b9,sp		; E93F
	cvxyl	sp,b10		; E9FA
	expect	1350
	cvxyl	a1,b10
	endexpect

	dec	a0		; 1420
	dec	a1		; 1421
	dec	a2		; 1422
	dec	a3		; 1423
	dec	a4		; 1424
	dec	a5		; 1425
	dec	a6		; 1426
	dec	a7		; 1427
	dec	a8		; 1428
	dec	a9		; 1429
	dec	a10		; 142A
	dec	a11		; 142B
	dec	a12		; 142C
	dec	a13		; 142D
	dec	a14		; 142E
	expect	1147
	dec	a15
	endexpect
	dec	sp		; 142F
	dec	b0		; 1430
	dec	saddr
	dec	b1		; 1431
	dec	sptch
	dec	b2		; 1432
	dec	daddr
	dec	b3		; 1433
	dec	dptch
	dec	b4		; 1434
	dec	offset
	dec	b5		; 1435
	dec	wstart
	dec	b6		; 1436
	dec	wend
	dec	b7		; 1437
	dec	dydx
	dec	b8		; 1438
	dec	color0
	dec	b9		; 1439
	dec	color1
	dec	b10		; 143A
	dec	count
	dec	b11		; 143B
	dec	inc1
	dec	b12		; 143C
	dec	inc2
	dec	b13		; 143D
	dec	pattrn
	dec	b14		; 143E

	dint			; 0360

	divs	a2,a5		; 5845
	divs	b5,b2		; 58B2
	divs	a7,sp		; 58EF
	divs	sp,a8		; 59E8
	divs	b9,sp		; 593F
	divs	sp,b10		; 59FA
	expect	1350
	divs	a1,b10
	endexpect
	expect	510
	divs	a3,a14		; 586E
	endexpect
	expect	510
	divs	b6,b14		; 58DE
	endexpect

	divu	a2,a5		; 5A45
	divu	b5,b2		; 5AB2
	divu	a7,sp		; 5AEF
	divu	sp,a8		; 5BE8
	divu	b9,sp		; 5B3F
	divu	sp,b10		; 5BFA
	expect	1350
	divu	a1,b10
	endexpect
	expect	510
	divu	a3,a14		; 5A6E
	endexpect
	expect	510
	divu	b6,b14		; 5ADE
	endexpect

	drav	a2,a5		; F645
	drav	b5,b2		; F6B2
	drav	a7,sp		; F6EF
	drav	sp,a8		; F7E8
	drav	b9,sp		; F73F
	drav	sp,b10		; F7FA
	expect	1350
	drav	a1,b10
	endexpect

	dsj	a5,$		; 3C25 (DSJS)
	dsj	a5,$+200h	; 3BE5 (DSJS)
	dsj	a5,$-1e0h	; 3FE5 (DSJS)
	dsj	a5,$+210h	; 0D85 001F
	dsj	a5,$-1f0h	; 0D85 FFDF
	dsj	a5,$-7ffe0h	; 0D85 8000
	dsj	a5,$+80010h	; 0D85 7FFF
	expect	1370
	dsj	a5,$-7fff0h
	endexpect
	expect	1370
	dsj	a5,$+80020h
	endexpect

	dsjeq	a5,$		; 0DA5 FFFE
	dsjeq	a5,$+200h	; 0DA5 001E
	dsjeq	a5,$-1e0h	; 0DA5 FFE0
	dsjeq	a5,$+210h	; 0DA5 001F
	dsjeq	a5,$-1f0h	; 0DA5 FFDF
	dsjeq	a5,$-7ffe0h	; 0DA5 8000
	dsjeq	a5,$+80010h	; 0DA5 7FFF
	expect	1370
	dsjeq	a5,$-7fff0h
	endexpect
	expect	1370
	dsjeq	a5,$+80020h
	endexpect

	dsjne	a5,$		; 0DC5 FFFE
	dsjne	a5,$+200h	; 0DC5 001E
	dsjne	a5,$-1e0h	; 0DC5 FFE0
	dsjne	a5,$+210h	; 0DC5 001F
	dsjne	a5,$-1f0h	; 0DC5 FFDF
	dsjne	a5,$-7ffe0h	; 0DC5 8000
	dsjne	a5,$+80010h	; 0DC5 7FFF
	expect	1370
	dsjne	a5,$-7fff0h
	endexpect
	expect	1370
	dsjne	a5,$+80020h
	endexpect

	dsjs	a5,$		; 3C25
	dsjs	a5,$+200h	; 3BE5
	dsjs	a5,$-1e0h	; 3FE5
	expect	1370
	dsjs	a5,$+210h
	endexpect
	expect	1370
	dsjs	a5,$-1f0h
	endexpect

	eint			; 0D60

	exgf	a5,0		; D505
	exgf	b5,1		; D715
	exgf	a5		; D505

	emu			; 0100

	exgpc	a0		; 0120
	exgpc	a1		; 0121
	exgpc	a2		; 0122
	exgpc	a3		; 0123
	exgpc	a4		; 0124
	exgpc	a5		; 0125
	exgpc	a6		; 0126
	exgpc	a7		; 0127
	exgpc	a8		; 0128
	exgpc	a9		; 0129
	exgpc	a10		; 012A
	exgpc	a11		; 012B
	exgpc	a12		; 012C
	exgpc	a13		; 012D
	exgpc	a14		; 012E
	expect	1147
	exgpc	a15
	endexpect
	exgpc	sp		; 012F
	exgpc	b0		; 0130
	exgpc	saddr
	exgpc	b1		; 0131
	exgpc	sptch
	exgpc	b2		; 0132
	exgpc	daddr
	exgpc	b3		; 0133
	exgpc	dptch
	exgpc	b4		; 0134
	exgpc	offset
	exgpc	b5		; 0135
	exgpc	wstart
	exgpc	b6		; 0136
	exgpc	wend
	exgpc	b7		; 0137
	exgpc	dydx
	exgpc	b8		; 0138
	exgpc	color0
	exgpc	b9		; 0139
	exgpc	color1
	exgpc	b10		; 013A
	exgpc	count
	exgpc	b11		; 013B
	exgpc	inc1
	exgpc	b12		; 013C
	exgpc	inc2
	exgpc	b13		; 013D
	exgpc	pattrn
	exgpc	b14		; 013E

	fill	l		; 0FC0
	fill	xy		; 0FE0
	expect	1985
	fill	nix
	endexpect

	getpc	a0		; 0140
	getpc	a1		; 0141
	getpc	a2		; 0142
	getpc	a3		; 0143
	getpc	a4		; 0144
	getpc	a5		; 0145
	getpc	a6		; 0146
	getpc	a7		; 0147
	getpc	a8		; 0148
	getpc	a9		; 0149
	getpc	a10		; 014A
	getpc	a11		; 014B
	getpc	a12		; 014C
	getpc	a13		; 014D
	getpc	a14		; 014E
	expect	1147
	getpc	a15
	endexpect
	getpc	sp		; 014F
	getpc	b0		; 0150
	getpc	saddr
	getpc	b1		; 0151
	getpc	sptch
	getpc	b2		; 0152
	getpc	daddr
	getpc	b3		; 0153
	getpc	dptch
	getpc	b4		; 0154
	getpc	offset
	getpc	b5		; 0155
	getpc	wstart
	getpc	b6		; 0156
	getpc	wend
	getpc	b7		; 0157
	getpc	dydx
	getpc	b8		; 0158
	getpc	color0
	getpc	b9		; 0159
	getpc	color1
	getpc	b10		; 015A
	getpc	count
	getpc	b11		; 015B
	getpc	inc1
	getpc	b12		; 015C
	getpc	inc2
	getpc	b13		; 015D
	getpc	pattrn
	getpc	b14		; 015E

	getst	a0		; 0180
	getst	a1		; 0181
	getst	a2		; 0182
	getst	a3		; 0183
	getst	a4		; 0184
	getst	a5		; 0185
	getst	a6		; 0186
	getst	a7		; 0187
	getst	a8		; 0188
	getst	a9		; 0189
	getst	a10		; 018A
	getst	a11		; 018B
	getst	a12		; 018C
	getst	a13		; 018D
	getst	a14		; 018E
	expect	1147
	getst	a15
	endexpect
	getst	sp		; 018F
	getst	b0		; 0190
	getst	saddr
	getst	b1		; 0191
	getst	sptch
	getst	b2		; 0192
	getst	daddr
	getst	b3		; 0193
	getst	dptch
	getst	b4		; 0194
	getst	offset
	getst	b5		; 0195
	getst	wstart
	getst	b6		; 0196
	getst	wend
	getst	b7		; 0197
	getst	dydx
	getst	b8		; 0198
	getst	color0
	getst	b9		; 0199
	getst	color1
	getst	b10		; 019A
	getst	count
	getst	b11		; 019B
	getst	inc1
	getst	b12		; 019C
	getst	inc2
	getst	b13		; 019D
	getst	pattrn
	getst	b14		; 019E

	inc	a0		; 1020
	inc	a1		; 1021
	inc	a2		; 1022
	inc	a3		; 1023
	inc	a4		; 1024
	inc	a5		; 1025
	inc	a6		; 1026
	inc	a7		; 1027
	inc	a8		; 1028
	inc	a9		; 1029
	inc	a10		; 102A
	inc	a11		; 102B
	inc	a12		; 102C
	inc	a13		; 102D
	inc	a14		; 102E
	expect	1147
	inc	a15
	endexpect
	inc	sp		; 102F
	inc	b0		; 1030
	inc	saddr
	inc	b1		; 1031
	inc	sptch
	inc	b2		; 1032
	inc	daddr
	inc	b3		; 1033
	inc	dptch
	inc	b4		; 1034
	inc	offset
	inc	b5		; 1035
	inc	wstart
	inc	b6		; 1036
	inc	wend
	inc	b7		; 1037
	inc	dydx
	inc	b8		; 1038
	inc	color0
	inc	b9		; 1039
	inc	color1
	inc	b10		; 103A
	inc	count
	inc	b11		; 103B
	inc	inc1
	inc	b12		; 103C
	inc	inc2
	inc	b13		; 103D
	inc	pattrn
	inc	b14		; 103E

	jauc	12345670h	; C080 5670 1234
	jap	12345670h	; C180 5670 1234
	jals	12345670h	; C280 5670 1234
	jayle	12345670h	; C280 5670 1234
	jahi	12345670h	; C380 5670 1234
	jaygt	12345670h	; C380 5670 1234
	jalt	12345670h	; C480 5670 1234
	jaxle	12345670h	; C480 5670 1234
	jage	12345670h	; C580 5670 1234
	jaxgt	12345670h	; C580 5670 1234
	jale	12345670h	; C680 5670 1234
	jagt	12345670h	; C780 5670 1234
	jalo	12345670h	; C880 5670 1234
	jac	12345670h	; C880 5670 1234
	jab	12345670h	; C880 5670 1234
	jayn	12345670h	; C880 5670 1234
	jahs	12345670h	; C980 5670 1234
	janc	12345670h	; C980 5670 1234
	janb	12345670h	; C980 5670 1234
	jaynn	12345670h	; C980 5670 1234
	jaeq	12345670h	; CA80 5670 1234
	jaz	12345670h	; CA80 5670 1234
	jayz	12345670h	; CA80 5670 1234
	jane	12345670h	; CB80 5670 1234
	janz	12345670h	; CB80 5670 1234
	jaynz	12345670h	; CB80 5670 1234
	jav	12345670h	; CC80 5670 1234
	jaxn	12345670h	; CC80 5670 1234
	janv	12345670h	; CD80 5670 1234
	jaxnn	12345670h	; CD80 5670 1234
	jan	12345670h	; CE80 5670 1234
	jaxz	12345670h	; CE80 5670 1234
	jann	12345670h	; CF80 5670 1234
	jaxnz	12345670h	; CF80 5670 1234
	expect	1352
	jaeq	12345678h
	endexpect

	jruc	$		; C0FF
	expect	470
	jruc	$+10h		; 0300
	endexpect
	jruc	$-7e0h		; C081
	jruc	$+800h		; C07F
	jruc	$-800h		; C000 FF7E
	jruc	$+810h		; C000 007F
	jruc	$-7ffe0h	; C000 8000
	jruc	$+80010h	; C000 7FFF
	expect	1370
	jruc	$-7fff0h
	endexpect
	expect	1370
	jruc	$+80020h
	endexpect
	jrp	$		; C1FF          
	expect	470
	jrp	$+10h		; 0300
	endexpect
	jrp	$-7e0h		; C181
	jrp	$+800h		; C17F
	jrp	$-800h		; C100 FF7E
	jrp	$+810h		; C100 007F
	jrp	$-7ffe0h	; C100 8000
	jrp	$+80010h	; C100 7FFF
	expect	1370
	jrp	$-7fff0h
	endexpect
	expect	1370
	jrp	$+80020h
	endexpect
	jrls	$		; C2FF          
	expect	470
	jrls	$+10h		; 0300
	endexpect
	jrls	$-7e0h		; C281
	jrls	$+800h		; C27F
	jrls	$-800h		; C200 FF7E
	jrls	$+810h		; C200 007F
	jrls	$-7ffe0h	; C200 8000
	jrls	$+80010h	; C200 7FFF
	expect	1370
	jrls	$-7fff0h
	endexpect
	expect	1370
	jrls	$+80020h
	endexpect
	jryle	$		; C2FF          
	expect	470
	jryle	$+10h		; 0300
	endexpect
	jryle	$-7e0h		; C281
	jryle	$+800h		; C27F
	jryle	$-800h		; C200 FF7E
	jryle	$+810h		; C200 007F
	jryle	$-7ffe0h	; C200 8000
	jryle	$+80010h	; C200 7FFF
	expect	1370
	jryle	$-7fff0h
	endexpect
	expect	1370
	jryle	$+80020h
	endexpect
	jrhi	$		; C3FF          
	expect	470
	jrhi	$+10h		; 0300
	endexpect
	jrhi	$-7e0h		; C381
	jrhi	$+800h		; C37F
	jrhi	$-800h		; C300 FF7E
	jrhi	$+810h		; C300 007F
	jrhi	$-7ffe0h	; C300 8000
	jrhi	$+80010h	; C300 7FFF
	expect	1370
	jrhi	$-7fff0h
	endexpect
	expect	1370
	jrhi	$+80020h
	endexpect
	jrygt	$		; C3FF          
	expect	470
	jrygt	$+10h		; 0300
	endexpect
	jrygt	$-7e0h		; C381
	jrygt	$+800h		; C37F
	jrygt	$-800h		; C300 FF7E
	jrygt	$+810h		; C300 007F
	jrygt	$-7ffe0h	; C300 8000
	jrygt	$+80010h	; C300 7FFF
	expect	1370
	jrygt	$-7fff0h
	endexpect
	expect	1370
	jrygt	$+80020h
	endexpect
	jrlt	$		; C4FF          
	expect	470
	jrlt	$+10h		; 0300
	endexpect
	jrlt	$-7e0h		; C481
	jrlt	$+800h		; C47F
	jrlt	$-800h		; C400 FF7E
	jrlt	$+810h		; C400 007F
	jrlt	$-7ffe0h	; C400 8000
	jrlt	$+80010h	; C400 7FFF
	expect	1370
	jrlt	$-7fff0h
	endexpect
	expect	1370
	jrlt	$+80020h
	endexpect
	jrxle	$		; C4FF          
	expect	470
	jrxle	$+10h		; 0300
	endexpect
	jrxle	$-7e0h		; C481
	jrxle	$+800h		; C47F
	jrxle	$-800h		; C400 FF7E
	jrxle	$+810h		; C400 007F
	jrxle	$-7ffe0h	; C400 8000
	jrxle	$+80010h	; C400 7FFF
	expect	1370
	jrxle	$-7fff0h
	endexpect
	expect	1370
	jrxle	$+80020h
	endexpect
	jrge	$		; C5FF          
	expect	470
	jrge	$+10h		; 0300
	endexpect
	jrge	$-7e0h		; C581
	jrge	$+800h		; C57F
	jrge	$-800h		; C500 FF7E
	jrge	$+810h		; C500 007F
	jrge	$-7ffe0h	; C500 8000
	jrge	$+80010h	; C500 7FFF
	expect	1370
	jrge	$-7fff0h
	endexpect
	expect	1370
	jrge	$+80020h
	endexpect
	jrxgt	$		; C5FF          
	expect	470
	jrxgt	$+10h		; 0300
	endexpect
	jrxgt	$-7e0h		; C581
	jrxgt	$+800h		; C57F
	jrxgt	$-800h		; C500 FF7E
	jrxgt	$+810h		; C500 007F
	jrxgt	$-7ffe0h	; C500 8000
	jrxgt	$+80010h	; C500 7FFF
	expect	1370
	jrxgt	$-7fff0h
	endexpect
	expect	1370
	jrxgt	$+80020h
	endexpect
	jrle	$		; C6FF          
	expect	470
	jrle	$+10h		; 0300
	endexpect
	jrle	$-7e0h		; C681
	jrle	$+800h		; C67F
	jrle	$-800h		; C600 FF7E
	jrle	$+810h		; C600 007F
	jrle	$-7ffe0h	; C600 8000
	jrle	$+80010h	; C600 7FFF
	expect	1370
	jrle	$-7fff0h
	endexpect
	expect	1370
	jrle	$+80020h
	endexpect
	jrgt	$		; C7FF          
	expect	470
	jrgt	$+10h		; 0300
	endexpect
	jrgt	$-7e0h		; C781
	jrgt	$+800h		; C77F
	jrgt	$-800h		; C700 FF7E
	jrgt	$+810h		; C700 007F
	jrgt	$-7ffe0h	; C700 8000
	jrgt	$+80010h	; C700 7FFF
	expect	1370
	jrgt	$-7fff0h
	endexpect
	expect	1370
	jrgt	$+80020h
	endexpect
	jrlo	$		; C8FF          
	expect	470
	jrlo	$+10h		; 0300
	endexpect
	jrlo	$-7e0h		; C881
	jrlo	$+800h		; C87F
	jrlo	$-800h		; C800 FF7E
	jrlo	$+810h		; C800 007F
	jrlo	$-7ffe0h	; C800 8000
	jrlo	$+80010h	; C800 7FFF
	expect	1370
	jrlo	$-7fff0h
	endexpect
	expect	1370
	jrlo	$+80020h
	endexpect
	jrc	$		; C8FF          
	expect	470
	jrc	$+10h		; 0300
	endexpect
	jrc	$-7e0h		; C881
	jrc	$+800h		; C87F
	jrc	$-800h		; C800 FF7E
	jrc	$+810h		; C800 007F
	jrc	$-7ffe0h	; C800 8000
	jrc	$+80010h	; C800 7FFF
	expect	1370
	jrc	$-7fff0h
	endexpect
	expect	1370
	jrc	$+80020h
	endexpect
	jrb	$		; C8FF          
	expect	470
	jrb	$+10h		; 0300
	endexpect
	jrb	$-7e0h		; C881
	jrb	$+800h		; C87F
	jrb	$-800h		; C800 FF7E
	jrb	$+810h		; C800 007F
	jrb	$-7ffe0h	; C800 8000
	jrb	$+80010h	; C800 7FFF
	expect	1370
	jrb	$-7fff0h
	endexpect
	expect	1370
	jrb	$+80020h
	endexpect
	jryn	$		; C8FF          
	expect	470
	jryn	$+10h		; 0300
	endexpect
	jryn	$-7e0h		; C881
	jryn	$+800h		; C87F
	jryn	$-800h		; C800 FF7E
	jryn	$+810h		; C800 007F
	jryn	$-7ffe0h	; C800 8000
	jryn	$+80010h	; C800 7FFF
	expect	1370
	jryn	$-7fff0h
	endexpect
	expect	1370
	jryn	$+80020h
	endexpect
	jrhs	$		; C9FF          
	expect	470
	jrhs	$+10h		; 0300
	endexpect
	jrhs	$-7e0h		; C981
	jrhs	$+800h		; C97F
	jrhs	$-800h		; C900 FF7E
	jrhs	$+810h		; C900 007F
	jrhs	$-7ffe0h	; C900 8000
	jrhs	$+80010h	; C900 7FFF
	expect	1370
	jrhs	$-7fff0h
	endexpect
	expect	1370
	jrhs	$+80020h
	endexpect
	jrnc	$		; C9FF          
	expect	470
	jrnc	$+10h		; 0300
	endexpect
	jrnc	$-7e0h		; C981
	jrnc	$+800h		; C97F
	jrnc	$-800h		; C900 FF7E
	jrnc	$+810h		; C900 007F
	jrnc	$-7ffe0h	; C900 8000
	jrnc	$+80010h	; C900 7FFF
	expect	1370
	jrnc	$-7fff0h
	endexpect
	expect	1370
	jrnc	$+80020h
	endexpect
	jrnb	$		; C9FF          
	expect	470
	jrnb	$+10h		; 0300
	endexpect
	jrnb	$-7e0h		; C981
	jrnb	$+800h		; C97F
	jrnb	$-800h		; C900 FF7E
	jrnb	$+810h		; C900 007F
	jrnb	$-7ffe0h	; C900 8000
	jrnb	$+80010h	; C900 7FFF
	expect	1370
	jrnb	$-7fff0h
	endexpect
	expect	1370
	jrnb	$+80020h
	endexpect
	jrynn	$		; C9FF          
	expect	470
	jrynn	$+10h		; 0300
	endexpect
	jrynn	$-7e0h		; C981
	jrynn	$+800h		; C97F
	jrynn	$-800h		; C900 FF7E
	jrynn	$+810h		; C900 007F
	jrynn	$-7ffe0h	; C900 8000
	jrynn	$+80010h	; C900 7FFF
	expect	1370
	jrynn	$-7fff0h
	endexpect
	expect	1370
	jrynn	$+80020h
	endexpect
	jreq	$		; CAFF          
	expect	470
	jreq	$+10h		; 0300
	endexpect
	jreq	$-7e0h		; CA81
	jreq	$+800h		; CA7F
	jreq	$-800h		; CA00 FF7E
	jreq	$+810h		; CA00 007F
	jreq	$-7ffe0h	; CA00 8000
	jreq	$+80010h	; CA00 7FFF
	expect	1370
	jreq	$-7fff0h
	endexpect
	expect	1370
	jreq	$+80020h
	endexpect
	jrz	$		; CAFF          
	expect	470
	jrz	$+10h		; 0300
	endexpect
	jrz	$-7e0h		; CA81
	jrz	$+800h		; CA7F
	jrz	$-800h		; CA00 FF7E
	jrz	$+810h		; CA00 007F
	jrz	$-7ffe0h	; CA00 8000
	jrz	$+80010h	; CA00 7FFF
	expect	1370
	jrz	$-7fff0h
	endexpect
	expect	1370
	jrz	$+80020h
	endexpect
	jryz	$		; CAFF          
	expect	470
	jryz	$+10h		; 0300
	endexpect
	jryz	$-7e0h		; CA81
	jryz	$+800h		; CA7F
	jryz	$-800h		; CA00 FF7E
	jryz	$+810h		; CA00 007F
	jryz	$-7ffe0h	; CA00 8000
	jryz	$+80010h	; CA00 7FFF
	expect	1370
	jryz	$-7fff0h
	endexpect
	expect	1370
	jryz	$+80020h
	endexpect
	jrne	$		; CBFF          
	expect	470
	jrne	$+10h		; 0300
	endexpect
	jrne	$-7e0h		; CB81
	jrne	$+800h		; CB7F
	jrne	$-800h		; CB00 FF7E
	jrne	$+810h		; CB00 007F
	jrne	$-7ffe0h	; CB00 8000
	jrne	$+80010h	; CB00 7FFF
	expect	1370
	jrne	$-7fff0h
	endexpect
	expect	1370
	jrne	$+80020h
	endexpect
	jrnz	$		; CBFF          
	expect	470
	jrnz	$+10h		; 0300
	endexpect
	jrnz	$-7e0h		; CB81
	jrnz	$+800h		; CB7F
	jrnz	$-800h		; CB00 FF7E
	jrnz	$+810h		; CB00 007F
	jrnz	$-7ffe0h	; CB00 8000
	jrnz	$+80010h	; CB00 7FFF
	expect	1370
	jrnz	$-7fff0h
	endexpect
	expect	1370
	jrnz	$+80020h
	endexpect
	jrynz	$		; CBFF          
	expect	470
	jrynz	$+10h		; 0300
	endexpect
	jrynz	$-7e0h		; CB81
	jrynz	$+800h		; CB7F
	jrynz	$-800h		; CB00 FF7E
	jrynz	$+810h		; CB00 007F
	jrynz	$-7ffe0h	; CB00 8000
	jrynz	$+80010h	; CB00 7FFF
	expect	1370
	jrynz	$-7fff0h
	endexpect
	expect	1370
	jrynz	$+80020h
	endexpect
	jrv	$		; CCFF          
	expect	470
	jrv	$+10h		; 0300
	endexpect
	jrv	$-7e0h		; CC81
	jrv	$+800h		; CC7F
	jrv	$-800h		; CC00 FF7E
	jrv	$+810h		; CC00 007F
	jrv	$-7ffe0h	; CC00 8000
	jrv	$+80010h	; CC00 7FFF
	expect	1370
	jrv	$-7fff0h
	endexpect
	expect	1370
	jrv	$+80020h
	endexpect
	jrxn	$		; CCFF          
	expect	470
	jrxn	$+10h		; 0300
	endexpect
	jrxn	$-7e0h		; CC81
	jrxn	$+800h		; CC7F
	jrxn	$-800h		; CC00 FF7E
	jrxn	$+810h		; CC00 007F
	jrxn	$-7ffe0h	; CC00 8000
	jrxn	$+80010h	; CC00 7FFF
	expect	1370
	jrxn	$-7fff0h
	endexpect
	expect	1370
	jrxn	$+80020h
	endexpect
	jrnv	$		; CDFF          
	expect	470
	jrnv	$+10h		; 0300
	endexpect
	jrnv	$-7e0h		; CD81
	jrnv	$+800h		; CD7F
	jrnv	$-800h		; CD00 FF7E
	jrnv	$+810h		; CD00 007F
	jrnv	$-7ffe0h	; CD00 8000
	jrnv	$+80010h	; CD00 7FFF
	expect	1370
	jrnv	$-7fff0h
	endexpect
	expect	1370
	jrnv	$+80020h
	endexpect
	jrxnn	$		; CDFF          
	expect	470
	jrxnn	$+10h		; 0300
	endexpect
	jrxnn	$-7e0h		; CD81
	jrxnn	$+800h		; CD7F
	jrxnn	$-800h		; CD00 FF7E
	jrxnn	$+810h		; CD00 007F
	jrxnn	$-7ffe0h	; CD00 8000
	jrxnn	$+80010h	; CD00 7FFF
	expect	1370
	jrxnn	$-7fff0h
	endexpect
	expect	1370
	jrxnn	$+80020h
	endexpect
	jrn	$		; CEFF          
	expect	470
	jrn	$+10h		; 0300
	endexpect
	jrn	$-7e0h		; CE81
	jrn	$+800h		; CE7F
	jrn	$-800h		; CE00 FF7E
	jrn	$+810h		; CE00 007F
	jrn	$-7ffe0h	; CE00 8000
	jrn	$+80010h	; CE00 7FFF
	expect	1370
	jrn	$-7fff0h
	endexpect
	expect	1370
	jrn	$+80020h
	endexpect
	jrxz	$		; CEFF          
	expect	470
	jrxz	$+10h		; 0300
	endexpect
	jrxz	$-7e0h		; CE81
	jrxz	$+800h		; CE7F
	jrxz	$-800h		; CE00 FF7E
	jrxz	$+810h		; CE00 007F
	jrxz	$-7ffe0h	; CE00 8000
	jrxz	$+80010h	; CE00 7FFF
	expect	1370
	jrxz	$-7fff0h
	endexpect
	expect	1370
	jrxz	$+80020h
	endexpect
	jrnn	$		; CFFF          
	expect	470
	jrnn	$+10h		; 0300
	endexpect
	jrnn	$-7e0h		; CF81
	jrnn	$+800h		; CF7F
	jrnn	$-800h		; CF00 FF7E
	jrnn	$+810h		; CF00 007F
	jrnn	$-7ffe0h	; CF00 8000
	jrnn	$+80010h	; CF00 7FFF
	expect	1370
	jrnn	$-7fff0h
	endexpect
	expect	1370
	jrnn	$+80020h
	endexpect
	jrxnz	$		; CFFF          
	expect	470
	jrxnz	$+10h		; 0300
	endexpect
	jrxnz	$-7e0h		; CF81
	jrxnz	$+800h		; CF7F
	jrxnz	$-800h		; CF00 FF7E
	jrxnz	$+810h		; CF00 007F
	jrxnz	$-7ffe0h	; CF00 8000
	jrxnz	$+80010h	; CF00 7FFF
	expect	1370
	jrxnz	$-7fff0h
	endexpect
	expect	1370
	jrxnz	$+80020h
	endexpect

	jump	a0		; 0160
	jump	a1		; 0161
	jump	a2		; 0162
	jump	a3		; 0163
	jump	a4		; 0164
	jump	a5		; 0165
	jump	a6		; 0166
	jump	a7		; 0167
	jump	a8		; 0168
	jump	a9		; 0169
	jump	a10		; 016A
	jump	a11		; 016B
	jump	a12		; 016C
	jump	a13		; 016D
	jump	a14		; 016E
	expect	1147
	jump	a15
	endexpect
	jump	sp		; 016F
	jump	b0		; 0170
	jump	saddr
	jump	b1		; 0171
	jump	sptch
	jump	b2		; 0172
	jump	daddr
	jump	b3		; 0173
	jump	dptch
	jump	b4		; 0174
	jump	offset
	jump	b5		; 0175
	jump	wstart
	jump	b6		; 0176
	jump	wend
	jump	b7		; 0177
	jump	dydx
	jump	b8		; 0178
	jump	color0
	jump	b9		; 0179
	jump	color1
	jump	b10		; 017A
	jump	count
	jump	b11		; 017B
	jump	inc1
	jump	b12		; 017C
	jump	inc2
	jump	b13		; 017D
	jump	pattrn
	jump	b14		; 017E

	line	0		; DF1A
	line	1		; DF9A
	expect	1320
	line	2
	endexpect

	lmo	a2,a5		; 6A45
	lmo	b5,b2		; 6AB2
	lmo	a7,sp		; 6AEF
	lmo	sp,a8		; 6BE8
	lmo	b9,sp		; 6B3F
	lmo	sp,b10		; 6BFA
	expect	1350
	lmo	a1,b10
	endexpect

	mmfm	b0,b1,b2,b3,b7,b12,b13,b14,sp	; 09B0 F08E
	mmfm	a0,a1,a2,a3,a7,a12,a13,a14,sp	; 09A0 F08E
	expect	1350
	mmfm	b0,a1,a2,a3,a7,a12,a13,a14,sp
	endexpect
	mmfm	sp,b0,b1,b2,b3,b7,b12,b13,b14	; 09BF 708F
	mmfm	sp,a0,a1,a2,a3,a7,a12,a13,a14	; 09AF 708F
	expect	140
	mmfm	sp,sp				; 09AF 8000
	endexpect

	mmtm	a1,a0,a2,a4,a8,a12,a13,a14,sp	; 0981 A88F
	mmtm	b1,b0,b2,b4,b8,b12,b13,b14,sp	; 0991 A88F
	expect	1350
	mmtm	a1,b0,b2,b4,b8,b12,b13,b14,sp
	endexpect
	mmtm	sp,a1,a0,a2,a4,a8,a12,a13,a14	; 098F E88E
	mmtm	sp,b1,b0,b2,b4,b8,b12,b13,b14	; 099F E88E
	expect	140
	mmtm	sp,sp				; 098F 0001
	endexpect

	mods	a2,a5		; 6C45
	mods	b5,b2		; 6CB2
	mods	a7,sp		; 6CEF
	mods	sp,a8		; 6DE8
	mods	b9,sp		; 6D3F
	mods	sp,b10		; 6DFA
	expect	1350
	mods	a1,b10
	endexpect

	modu	a2,a5		; 6E45
	modu	b5,b2		; 6EB2
	modu	a7,sp		; 6EEF
	modu	sp,a8		; 6FE8
	modu	b9,sp		; 6F3F
	modu	sp,b10		; 6FFA
	expect	1350
	modu	a1,b10
	endexpect

        movb	a2,*a5		; 8C45
        movb	b5,*b2		; 8CB2
        movb	a7,*sp		; 8CEF
        movb	sp,*a8		; 8DE8
        movb	b9,*sp		; 8D3F
        movb	sp,*b10		; 8DFA
        expect  1350
        movb	a1,*b10
        endexpect

	movb	a2,*a5(0)	; AC45 0000
	movb	b5,*b2(1)	; ACB2 0001
	movb	a7,*sp(9)	; ACEF 0009
	movb	sp,*a8(12)	; ADE8 000C
	movb	b9,*sp(32767)	; AD3F 7FFF
	movb	sp,*b10(-32768)	; ADFA 8000
	expect  1350
	movb	a1,*b10(12)
	endexpect

	movb	a2,@12345678h	; 05E2 5678 1234
	movb	b9,@12345678h	; 05F9 5678 1234

	movb	*a2,a5		; 8E45
	movb	*b5,b2		; 8EB2
	movb	*a7,sp		; 8EEF
	movb	*sp,a8		; 8FE8
	movb	*b9,sp		; 8F3F
	movb	*sp,b10		; 8FFA
	expect	1350
	movb	*a1,b10
	endexpect

	movb	*a2,*a5		; 9C45
	movb	*b5,*b2		; 9CB2
	movb	*a7,*sp		; 9CEF
	movb	*sp,*a8		; 9DE8
	movb	*b9,*sp		; 9D3F
	movb	*sp,*b10	; 9DFA
	expect	1350
	movb	*a1,*b10
	endexpect

	movb	*a2(0),a5	; AE45 0000
	movb	*b5(1),b2	; AEB2 0001
	movb	*a7(9),sp	; AEEF 0009
	movb	*sp(12),a8	; AFE8 000C
	movb	*b9(32767),sp	; AF3F 7FFF
	movb	*sp(-32768),b10	; AFFA 8000
	expect  1350
	movb	*a1(12),b10
	endexpect

	movb	*a2(0),*a5(-32768)	; BC45 0000 8000
	movb	*b5(1),*b2(32767)	; BCB2 0001 7FFF
	movb	*a7(9),*sp(12)		; BCEF 0009 000C
	movb	*sp(12),*a8(9)		; BDE8 000C 0009
	movb	*b9(32767),*sp(1)	; BD3F 7FFF 0001
	movb	*sp(-32768),*b10(0)	; BDFA 8000 0000
	expect  1350
	movb	*a1(12),*b10(12)
	endexpect

	movb	*a2,*a5(-32768)		; BC45 0000 8000 (src encoded as '*a2(0)')
	movb	*b5,*b2(32767)		; BCB2 0000 7FFF (src encoded as '*b5(0)')
	movb	*a7,*sp(12)		; BCEF 0000 000C (src encoded as '*a7(0)')
	movb	*sp,*a8(9)		; BDE8 0000 0009 (src encoded as '*sp(0)')
	movb	*b9,*sp(1)		; BD3F 0000 0001 (src encoded as '*b9(0)')
	movb	*sp,*b10(0)		; BDFA 0000 0000 (src encoded as '*sp(0)')
	expect  1350
	movb	*a1,*b10(12)
	endexpect

	movb	*a2(0),*a5		; BC45 0000 0000 (dest encoded as '*a5(0)')
	movb	*b5(1),*b2		; BCB2 0001 0000 (dest encoded as '*b2(0)')
	movb	*a7(9),*sp		; BCEF 0009 0000 (dest encoded as '*sp(0)')
	movb	*sp(12),*a8		; BDE8 000C 0000 (dest encoded as '*a8(0)')
	movb	*b9(32767),*sp		; BD3F 7FFF 0000 (dest encoded as '*sp(0)')
	movb	*sp(-32768),*b10	; BDFA 8000 0000 (dest encoded as '*b10(0)')
	expect  1350
	movb	*a1(12),*b10
	endexpect

	movb	@12345678h,a2	; 07E2 5678 1234
	movb	@12345678h,b9	; 07F9 5678 1234

	movb	@12345678h,@23456789h	; 0340 5678 1234 6789 2345

	move	a2,a9		; 4C49
	move	a2,b9		; 4E49
	move	b2,a9		; 4E59
	move	b2,b9		; 4C59
	expect	1110
	move	b2,b9,1
	endexpect

        move	a2,*a5		; 8045
        move	b5,*b2		; 80B2
        move	a7,*sp		; 80EF
        move	sp,*a8		; 81E8
        move	b9,*sp		; 813F
        move	sp,*b10		; 81FA
        expect  1350
        move	a1,*b10
        endexpect

        move	a2,*a5,1	; 8245
        move	b5,*b2,0	; 80B2
        move	a7,*sp,1	; 82EF
        move	sp,*a8,0	; 81E8
        move	b9,*sp,1	; 833F
        move	sp,*b10,0	; 81FA
        expect  1350
        move	a1,*b10,1
        endexpect

        move	a2,*a5+		; 9045
        move	b5,*b2+		; 90B2
        move	a7,*sp+		; 90EF
        move	sp,*a8+		; 91E8
        move	b9,*sp+		; 913F
        move	sp,*b10+	; 91FA
        expect  1350
        move	a1,*b10+
        endexpect

        move	a2,*a5+,1	; 9245
        move	b5,*b2+,0	; 90B2
        move	a7,*sp+,1	; 92EF
        move	sp,*a8+,0	; 91E8
        move	b9,*sp+,1	; 933F
        move	sp,*b10+,0	; 91FA
        expect  1350
        move	a1,*b10+,1
        endexpect

        move	a2,*-a5		; A045
        move	b5,*-b2		; A0B2
        move	a7,*-sp		; A0EF
        move	sp,*-a8		; A1E8
        move	b9,*-sp		; A13F
        move	sp,*-b10	; A1FA
        expect  1350
        move	a1,*-b10
        endexpect

        move	a2,*-a5,1	; A245
        move	b5,*-b2,0	; A0B2
        move	a7,*-sp,1	; A2EF
        move	sp,*-a8,0	; A1E8
        move	b9,*-sp,1	; A33F
        move	sp,*-b10,0	; A1FA
        expect  1350
        move	a1,*-b10,1
        endexpect

        move	a2,*a5(0)	; B045 0000
        move	b5,*b2(1)	; B0B2 0001
        move	a7,*sp(9)	; B0EF 0009
        move	sp,*a8(12)	; B1E8 000C
        move	b9,*sp(32767)	; B13F 7FFF
        move	sp,*b10(-32768)	; B1FA 8000
        expect  1350
        move	a1,*b10(12)
        endexpect

        move	a2,*a5(0),1	; B245 0000
        move	b5,*b2(1),0	; B0B2 0001
        move	a7,*sp(9),1	; B2EF 0009
        move	sp,*a8(12),0	; B1E8 000C
        move	b9,*sp(32767),1	; B33F 7FFF
        move	sp,*b10(-32768),0	; B1FA 8000
        expect  1350
        move	a1,*b10(12),1
        endexpect

	move	a2,@12345678h	; 0582 5678 1234
	move	b9,@12345678h	; 0599 5678 1234

	move	a2,@12345678h,1	; 0782 5678 1234
	move	b9,@12345678h,0	; 0599 5678 1234

        move	*a2,a5		; 8445
        move	*b5,b2		; 84B2
        move	*a7,sp		; 84EF
        move	*sp,a8		; 85E8
        move	*b9,sp		; 853F
        move	*sp,b10		; 85FA
        expect  1350
        move	*a1,b10
        endexpect

        move	*a2,a5,1	; 8645
        move	*b5,b2,0	; 84B2
        move	*a7,sp,1	; 86EF
        move	*sp,a8,0	; 85E8
        move	*b9,sp,1	; 873F
        move	*sp,b10,0	; 85FA
        expect  1350
        move	*a1,b10,1
        endexpect

        move	*a2,*a5		; 8845
        move	*b5,*b2		; 88B2
        move	*a7,*sp		; 88EF
        move	*sp,*a8		; 89E8
        move	*b9,*sp		; 893F
        move	*sp,*b10	; 89FA
        expect  1350
        move	*a1,*b10
        endexpect

        move	*a2,*a5,1	; 8A45
        move	*b5,*b2,0	; 88B2
        move	*a7,*sp,1	; 8AEF
        move	*sp,*a8,0	; 89E8
        move	*b9,*sp,1	; 8B3F
        move	*sp,*b10,0	; 89FA
        expect  1350
        move	*a1,*b10,1
        endexpect

        move	*a2+,a5		; 9445
        move	*b5+,b2		; 94B2
        move	*a7+,sp		; 94EF
        move	*sp+,a8		; 95E8
        move	*b9+,sp		; 953F
        move	*sp+,b10	; 95FA
        expect  1350
        move	*a1+,b10
        endexpect

        move	*a2+,a5,1	; 9645
        move	*b5+,b2,0	; 94B2
        move	*a7+,sp,1	; 96EF
        move	*sp+,a8,0	; 95E8
        move	*b9+,sp,1	; 973F
        move	*sp+,b10,0	; 95FA
        expect  1350
        move	*a1+,b10,1
        endexpect

        move	*a2+,*a5+	; 9845
        move	*b5+,*b2+	; 98B2
        move	*a7+,*sp+	; 98EF
        move	*sp+,*a8+	; 99E8
        move	*b9+,*sp+	; 993F
        move	*sp+,*b10+	; 99FA
        expect  1350
        move	*a1+,*b10+
        endexpect

        move	*a2+,*a5+,1	; 9A45
        move	*b5+,*b2+,0	; 98B2
        move	*a7+,*sp+,1	; 9AEF
        move	*sp+,*a8+,0	; 99E8
        move	*b9+,*sp+,1	; 9B3F
        move	*sp+,*b10+,0	; 99FA
        expect  1350
        move	*a1+,*b10+,1
        endexpect

        move	*-a2,a5		; A445
        move	*-b5,b2		; A4B2
        move	*-a7,sp		; A4EF
        move	*-sp,a8		; A5E8
        move	*-b9,sp		; A53F
        move	*-sp,b10	; A5FA
        expect  1350
        move	*-a1,b10
        endexpect

        move	*-a2,a5,1	; A645
        move	*-b5,b2,0	; A4B2
        move	*-a7,sp,1	; A6EF
        move	*-sp,a8,0	; A5E8
        move	*-b9,sp,1	; A73F
        move	*-sp,b10,0	; A5FA
        expect  1350
        move	*-a1,b10,1
        endexpect

        move	*-a2,*-a5	; A845
        move	*-b5,*-b2	; A8B2
        move	*-a7,*-sp	; A8EF
        move	*-sp,*-a8	; A9E8
        move	*-b9,*-sp	; A93F
        move	*-sp,*-b10	; A9FA
        expect  1350
        move	*-a1,*-b10
        endexpect

	; Unfortunately, the TI documentation is unconclusive whether
	; the pre-decrement syntax is -*Rn or *-Rn.  30 years later,
	; there is noone left to ask which is right, so we allow either:

        move	*-a2,-*a5,1	; AA45
        move	*-b5,-*b2,0	; A8B2
        move	*-a7,-*sp,1	; AAEF
        move	*-sp,-*a8,0	; A9E8
        move	*-b9,-*sp,1	; AB3F
        move	*-sp,-*b10,0	; A9FA
        expect  1350
        move	*-a1,-*b10,1
        endexpect

        move	*a2(0),a5	; B445 0000
        move	*b5(1),b2	; B4B2 0001
        move	*a7(9),sp	; B4EF 0009
        move	*sp(12),a8	; B5E8 000C
        move	*b9(32767),sp	; B53F 7FFF
        move	*sp(-32768),b10	; B5FA 8000
        expect  1350
        move	*a1(12),b10
        endexpect

        move	*a2(0),a5,1		; B645 0000
        move	*b5(1),b2,0		; B4B2 0001
        move	*a7(9),sp,1		; B6EF 0009
        move	*sp(12),a8,0		; B5E8 000C
        move	*b9(32767),sp,1		; B73F 7FFF
        move	*sp(-32768),b10,0	; B5FA 8000
        expect  1350
        move	*a1(12),b10,1
        endexpect

        move	*a2(0),*a5+		; D045 0000
        move	*b5(1),*b2+		; D0B2 0001
        move	*a7(9),*sp+		; D0EF 0009
        move	*sp(12),*a8+		; D1E8 000C
        move	*b9(32767),*sp+		; D13F 7FFF
        move	*sp(-32768),*b10+	; D1FA 8000
        expect  1350
        move	*a1(12),*b10+
        endexpect

        move	*a2(0),*a5+,1		; D245 0000
        move	*b5(1),*b2+,0		; D0B2 0001
        move	*a7(9),*sp+,1		; D2EF 0009
        move	*sp(12),*a8+,0		; D1E8 000C
        move	*b9(32767),*sp+,1	; D33F 7FFF
        move	*sp(-32768),*b10+,0	; D1FA 8000
        expect  1350
        move	*a1(12),*b10+,1
        endexpect

        move	*a2(0),*a5(-32768)	; B845 0000 8000
        move	*b5(1),*b2(32767)	; B8B2 0001 7FFF
        move	*a7(9),*sp(12)		; B8EF 0009 000C
        move	*sp(12),*a8(9)		; B9E8 000C 0009
        move	*b9(32767),*sp(1)	; B93F 7FFF 0001
        move	*sp(-32768),*b10(0)	; B9FA 8000 0000
        expect  1350
        move	*a1(12),*b10(12)
        endexpect

        move	*a2(0),*a5(-32768),1	; BA45 0000 8000
        move	*b5(1),*b2(32767),0	; B8B2 0001 7FFF
        move	*a7(9),*sp(12),1	; BAEF 0009 000C
        move	*sp(12),*a8(9),0	; B9E8 000C 0009
        move	*b9(32767),*sp(1),1	; BB3F 7FFF 0001
        move	*sp(-32768),*b10(0),0	; B9FA 8000 0000
        expect  1350
        move	*a1(12),*b10(12),1
        endexpect

        move	*a2(0),*a5		; B845 0000 0000 (dest encoded as '*a5(0)')
        move	*b5(1),*b2		; B8B2 0001 0000 (dest encoded as '*b2(0)')
        move	*a7(9),*sp		; B8EF 0009 0000 (dest encoded as '*sp(0)')
        move	*sp(12),*a8		; B9E8 000C 0000 (dest encoded as '*a8(0)')
        move	*b9(32767),*sp		; B93F 7FFF 0000 (dest encoded as '*sp(0)')
        move	*sp(-32768),*b10	; B9FA 8000 0000 (dest encoded as '*b10(0)')
        expect  1350
        move	*a1(12),*b10
        endexpect

        move	*a2(0),*a5,1		; BA45 0000 0000 (dest encoded as '*a5(0)')
        move	*b5(1),*b2,0		; B8B2 0001 0000 (dest encoded as '*b2(0)')
        move	*a7(9),*sp,1		; BAEF 0009 0000 (dest encoded as '*sp(0)')
        move	*sp(12),*a8,0		; B9E8 000C 0000 (dest encoded as '*a8(0)')
        move	*b9(32767),*sp,1	; BB3F 7FFF 0000 (dest encoded as '*sp(0)')
        move	*sp(-32768),*b10,0	; B9FA 8000 0000 (dest encoded as '*b10(0)')
        expect  1350
        move	*a1(12),*b10,1
        endexpect

        move	*a2,*a5(-32768)		; B845 0000 8000 (src encoded as '*a2(0)')
        move	*b5,*b2(32767)		; B8B2 0000 7FFF (src encoded as '*b5(0)')
        move	*a7,*sp(12)		; B8EF 0000 000C (src encoded as '*a7(0)')
        move	*sp,*a8(9)		; B9E8 0000 0009 (src encoded as '*sp(0)')
        move	*b9,*sp(1)		; B93F 0000 0001 (src encoded as '*b9(0)')
        move	*sp,*b10(0)		; B9FA 0000 0000 (src encoded as '*sp(0)')
        expect  1350
        move	*a1,*b10(12)
        endexpect

        move	*a2,*a5(-32768),1	; BA45 0000 8000 (src encoded as '*a2(0)')
        move	*b5,*b2(32767),0	; B8B2 0000 7FFF (src encoded as '*b5(0)')
        move	*a7,*sp(12),1		; BAEF 0000 000C (src encoded as '*a7(0)')
        move	*sp,*a8(9),0		; B9E8 0000 0009 (src encoded as '*sp(0)')
        move	*b9,*sp(1),1		; BB3F 0000 0001 (src encoded as '*b9(0)')
        move	*sp,*b10(0),0		; B9FA 0000 0000 (src encoded as '*sp(0)')
        expect  1350
        move	*a1,*b10(12),1
        endexpect

	move	@12345678h,a2	; 05A2 5678 1234
	move	@12345678h,b9	; 05B9 5678 1234

	move	@12345678h,a2,1	; 07A2 5678 1234
	move	@12345678h,b9,0	; 05B9 5678 1234

	move	@12345678h,*a2+	; D402 5678 1234
	move	@12345678h,*b9+	; D419 5678 1234

	move	@12345678h,*a2+,1	; D602 5678 1234
	move	@12345678h,*b9+,0	; D419 5678 1234

	move	@12345678h,@23456789h	; 05C0 5678 1234 6789 2345

	move	@12345678h,@23456789h,1	; 07C0 5678 1234 6789 2345

	movi	1234h,b5	; 09D5 1234
	movi	1234h,b5,w	; 09D5 1234
	movi	1234h,b5,l	; 09F5 1234 0000
	movi	12345678h,b5	; 09F5 5678 1234
	expect	260
	movi	12345678h,b5,w	; 09D5 5678
	endexpect
	movi	12345678h,b5,l	; 09F5 5678 1234

	movk	1,b3		; 1833
	movk	32,b3		; 1813
	expect	1315
	movk	0,b3
	endexpect
	expect	1320
	movk	33,b3
	endexpect

	movx	a2,a5		; EC45
	movx	b5,b2		; ECB2
	movx	a7,sp		; ECEF
	movx	sp,a8		; EDE8
	movx	b9,sp		; ED3F
	movx	sp,b10		; EDFA
	expect	1350
	movx	a1,b10
	endexpect

	movy	a2,a5		; EE45
	movy	b5,b2		; EEB2
	movy	a7,sp		; EEEF
	movy	sp,a8		; EFE8
	movy	b9,sp		; EF3F
	movy	sp,b10		; EFFA
	expect	1350
	movy	a1,b10
	endexpect

	mpys	a2,a5		; 5C45
	mpys	b5,b2		; 5CB2
	mpys	a7,sp		; 5CEF
	mpys	sp,a8		; 5DE8
	mpys	b9,sp		; 5D3F
	mpys	sp,b10		; 5DFA
	expect	1350
	mpys	a1,b10
	endexpect
	expect	510
	mpys	a3,a14		; 5C6E
	endexpect
	expect	510
	mpys	b6,b14		; 5CDE
	endexpect

	mpyu	a2,a5		; 5E45
	mpyu	b5,b2		; 5EB2
	mpyu	a7,sp		; 5EEF
	mpyu	sp,a8		; 5FE8
	mpyu	b9,sp		; 5F3F
	mpyu	sp,b10		; 5FFA
	expect	1350
	mpyu	a1,b10
	endexpect
	expect	510
	mpyu	a3,a14		; 5E6E
	endexpect
	expect	510
	mpyu	b6,b14		; 5EDE
	endexpect

	neg	a0		; 03A0
	neg	a1		; 03A1
	neg	a2		; 03A2
	neg	a3		; 03A3
	neg	a4		; 03A4
	neg	a5		; 03A5
	neg	a6		; 03A6
	neg	a7		; 03A7
	neg	a8		; 03A8
	neg	a9		; 03A9
	neg	a10		; 03AA
	neg	a11		; 03AB
	neg	a12		; 03AC
	neg	a13		; 03AD
	neg	a14		; 03AE
	expect	1147
	neg	a15
	endexpect
	neg	sp		; 03AF
	neg	b0		; 03B0
	neg	saddr
	neg	b1		; 03B1
	neg	sptch
	neg	b2		; 03B2
	neg	daddr
	neg	b3		; 03B3
	neg	dptch
	neg	b4		; 03B4
	neg	offset
	neg	b5		; 03B5
	neg	wstart
	neg	b6		; 03B6
	neg	wend
	neg	b7		; 03B7
	neg	dydx
	neg	b8		; 03B8
	neg	color0
	neg	b9		; 03B9
	neg	color1
	neg	b10		; 03BA
	neg	count
	neg	b11		; 03BB
	neg	inc1
	neg	b12		; 03BC
	neg	inc2
	neg	b13		; 03BD
	neg	pattrn
	neg	b14		; 03BE

	negb	a0		; 03C0
	negb	a1		; 03C1
	negb	a2		; 03C2
	negb	a3		; 03C3
	negb	a4		; 03C4
	negb	a5		; 03C5
	negb	a6		; 03C6
	negb	a7		; 03C7
	negb	a8		; 03C8
	negb	a9		; 03C9
	negb	a10		; 03CA
	negb	a11		; 03CB
	negb	a12		; 03CC
	negb	a13		; 03CD
	negb	a14		; 03CE
	expect	1147
	negb	a15
	endexpect
	negb	sp		; 03CF
	negb	b0		; 03D0
	negb	saddr
	negb	b1		; 03D1
	negb	sptch
	negb	b2		; 03D2
	negb	daddr
	negb	b3		; 03D3
	negb	dptch
	negb	b4		; 03D4
	negb	offset
	negb	b5		; 03D5
	negb	wstart
	negb	b6		; 03D6
	negb	wend
	negb	b7		; 03D7
	negb	dydx
	negb	b8		; 03D8
	negb	color0
	negb	b9		; 03D9
	negb	color1
	negb	b10		; 03DA
	negb	count
	negb	b11		; 03DB
	negb	inc1
	negb	b12		; 03DC
	negb	inc2
	negb	b13		; 03DD
	negb	pattrn
	negb	b14		; 03DE

	nop			; 0300

	not	a0		; 03E0
	not	a1		; 03E1
	not	a2		; 03E2
	not	a3		; 03E3
	not	a4		; 03E4
	not	a5		; 03E5
	not	a6		; 03E6
	not	a7		; 03E7
	not	a8		; 03E8
	not	a9		; 03E9
	not	a10		; 03EA
	not	a11		; 03EB
	not	a12		; 03EC
	not	a13		; 03ED
	not	a14		; 03EE
	expect	1147
	not	a15
	endexpect
	not	sp		; 03EF
	not	b0		; 03F0
	not	saddr
	not	b1		; 03F1
	not	sptch
	not	b2		; 03F2
	not	daddr
	not	b3		; 03F3
	not	dptch
	not	b4		; 03F4
	not	offset
	not	b5		; 03F5
	not	wstart
	not	b6		; 03F6
	not	wend
	not	b7		; 03F7
	not	dydx
	not	b8		; 03F8
	not	color0
	not	b9		; 03F9
	not	color1
	not	b10		; 03FA
	not	count
	not	b11		; 03FB
	not	inc1
	not	b12		; 03FC
	not	inc2
	not	b13		; 03FD
	not	pattrn
	not	b14		; 03FE

	or	a2,a5		; 5445
	or	b5,b2		; 54B2
	or	a7,sp		; 54EF
	or	sp,a8		; 55E8
	or	b9,sp		; 553F
	or	sp,b10		; 55FA
	expect	1350
	or	a1,b10
	endexpect

	ori	1234h,b5	; 0BB5 1234 0000
	expect	1130
	ori	1234h,b5,w
	endexpect
	ori	1234h,b5,l	; 0BB5 1234 0000
	ori	12345678h,b5	; 0BB5 5678 1234
	expect	1130
	ori	12345678h,b5,w
	endexpect
	ori	12345678h,b5,l	; 0BB5 5678 1234

	pixblt	b,l		; 0F80
	pixblt	b,xy		; 0FA0
	pixblt	l,l		; 0F00
	pixblt	l,xy		; 0F20
	pixblt	xy,l		; 0F40
	pixblt	xy,xy		; 0F60

	pixt	a2,*a5		; F845
	pixt	b5,*b2		; F8B2
	pixt	a7,*sp		; F8EF
	pixt	sp,*a8		; F9E8
	pixt	b9,*sp		; F93F
	pixt	sp,*b10		; F9FA
	expect	1350
	pixt	a1,*b10
	endexpect

	pixt	a2,*a5.xy	; F045
	pixt	b5,*b2.xy	; F0B2
	pixt	a7,*sp.xy	; F0EF
	pixt	sp,*a8.xy	; F1E8
	pixt	b9,*sp.xy	; F13F
	pixt	sp,*b10.xy	; F1FA
	expect	1350
	pixt	a1,*b10.xy
	endexpect

	pixt	*a2,a5		; FA45
	pixt	*b5,b2		; FAB2
	pixt	*a7,sp		; FAEF
	pixt	*sp,a8		; FBE8
	pixt	*b9,sp		; FB3F
	pixt	*sp,b10		; FBFA
	expect	1350
	pixt	*a1,b10
	endexpect

	pixt	*a2,*a5		; FC45
	pixt	*b5,*b2		; FCB2
	pixt	*a7,*sp		; FCEF
	pixt	*sp,*a8		; FDE8
	pixt	*b9,*sp		; FD3F
	pixt	*sp,*b10	; FDFA
	expect	1350
	pixt	*a1,*b10
	endexpect

	pixt	*a2.xy,a5	; F245
	pixt	*b5.xy,b2	; F2B2
	pixt	*a7.xy,sp	; F2EF
	pixt	*sp.xy,a8	; F3E8
	pixt	*b9.xy,sp	; F33F
	pixt	*sp.xy,b10	; F3FA
	expect	1350
	pixt	*a1.xy,b10
	endexpect

	pixt	*a2.xy,*a5.xy	; F445
	pixt	*b5.xy,*b2.xy	; F4B2
	pixt	*a7.xy,*sp.xy	; F4EF
	pixt	*sp.xy,*a8.xy	; F5E8
	pixt	*b9.xy,*sp.xy	; F53F
	pixt	*sp.xy,*b10.xy	; F5FA
	expect	1350
	pixt	*a1.xy,*b10.xy
	endexpect

	popst			; 01C0

	pushst			; 01E0

	reti			; 0940

	rets			; 0960
	rets	0		; 0960
	rets	31		; 097F
	expect	1320
	rets	32
	endexpect

	rev	a0		; 0020
	rev	a1		; 0021
	rev	a2		; 0022
	rev	a3		; 0023
	rev	a4		; 0024
	rev	a5		; 0025
	rev	a6		; 0026
	rev	a7		; 0027
	rev	a8		; 0028
	rev	a9		; 0029
	rev	a10		; 002A
	rev	a11		; 002B
	rev	a12		; 002C
	rev	a13		; 002D
	rev	a14		; 002E
	expect	1147
	rev	a15
	endexpect
	rev	sp		; 002F
	rev	b0		; 0030
	rev	saddr
	rev	b1		; 0031
	rev	sptch
	rev	b2		; 0032
	rev	daddr
	rev	b3		; 0033
	rev	dptch
	rev	b4		; 0034
	rev	offset
	rev	b5		; 0035
	rev	wstart
	rev	b6		; 0036
	rev	wend
	rev	b7		; 0037
	rev	dydx
	rev	b8		; 0038
	rev	color0
	rev	b9		; 0039
	rev	color1
	rev	b10		; 003A
	rev	count
	rev	b11		; 003B
	rev	inc1
	rev	b12		; 003C
	rev	inc2
	rev	b13		; 003D
	rev	pattrn
	rev	b14		; 003E

	rl	0,b14		; 301E
	rl	1,b14		; 303E
	rl	30,b14		; 33DE
	rl	31,b14		; 33FE
	expect	1320
	rl	32,b14
	endexpect
	rl	a2,a5		; 6845
	rl	b5,b2		; 68B2
	rl	a7,sp		; 68EF
	rl	sp,a8		; 69E8
	rl	b9,sp		; 693F
	rl	sp,b10		; 69FA
	expect	1350
	rl	a1,b10
	endexpect

	setc			; 0DE0

	setf	32,0		; 0540
	setf	32,1		; 0560
	setf	31,1		; 057F
	setf	16,0		; 0550
	setf	32,0,0		; 0540
	setf	32,1,0		; 0560
	setf	31,1,0		; 057F
	setf	16,0,0		; 0550
	setf	32,0,1		; 0740
	setf	32,1,1		; 0760
	setf	31,1,1		; 077F
	setf	16,0,1		; 0750

	sext	a2		; 0502
	sext	a2,0		; 0502
	sext	a2,1		; 0702
	sext	b9		; 0519
	sext	b9,0		; 0519
	sext	b9,1		; 0719

	sla	0,b14		; 201E
	sla	1,b14		; 203E
	sla	30,b14		; 23DE
	sla	31,b14		; 23FE
	expect	1320
	sla	32,b14
	endexpect
	sla	a2,a5		; 6045
	sla	b5,b2		; 60B2
	sla	a7,sp		; 60EF
	sla	sp,a8		; 61E8
	sla	b9,sp		; 613F
	sla	sp,b10		; 61FA
	expect	1350
	sla	a1,b10
	endexpect

	sll	0,b14		; 241E
	sll	1,b14		; 243E
	sll	30,b14		; 27DE
	sll	31,b14		; 27FE
	expect	1320
	sll	32,b14
	endexpect
	sll	a2,a5		; 6245
	sll	b5,b2		; 62B2
	sll	a7,sp		; 62EF
	sll	sp,a8		; 63E8
	sll	b9,sp		; 633F
	sll	sp,b10		; 63FA
	expect	1350
	sll	a1,b10
	endexpect

	sra	0,b14		; 281E
	sra	1,b14		; 2BFE
	sra	30,b14		; 285E
	sra	31,b14		; 283E
	expect	1320
	sra	32,b14
	endexpect
	sra	a2,a5		; 6445
	sra	b5,b2		; 64B2
	sra	a7,sp		; 64EF
	sra	sp,a8		; 65E8
	sra	b9,sp		; 653F
	sra	sp,b10		; 65FA
	expect	1350
	sra	a1,b10
	endexpect

	srl	0,b14		; 2C1E
	srl	1,b14		; 2FFE
	srl	30,b14		; 2C5E
	srl	31,b14		; 2C3E
	expect	1320
	srl	32,b14
	endexpect
	srl	a2,a5		; 6645
	srl	b5,b2		; 66B2
	srl	a7,sp		; 66EF
	srl	sp,a8		; 67E8
	srl	b9,sp		; 673F
	srl	sp,b10		; 67FA
	expect	1350
	srl	a1,b10
	endexpect

	sub	a2,a5		; 4445
	sub	b5,b2		; 44B2
	sub	a7,sp		; 44EF
	sub	sp,a8		; 45E8
	sub	b9,sp		; 453F
	sub	sp,b10		; 45FA
	expect	1350
	sub	a1,b10
	endexpect

	subb	a2,a5		; 4645
	subb	b5,b2		; 46B2
	subb	a7,sp		; 46EF
	subb	sp,a8		; 47E8
	subb	b9,sp		; 473F
	subb	sp,b10		; 47FA
	expect	1350
	subb	a1,b10
	endexpect

	subi	1234h,b5	; 0BF5 EDCB
	subi	1234h,b5,w	; 0BF5 EDCB
	subi	1234h,b5,l	; 0D15 EDCB FFFF
	subi	12345678h,b5	; 0D15 A987 EDCB
	expect	260
	subi	12345678h,b5,w	; 0BF5 A987
	endexpect
	subi	12345678h,b5,l	; 0D15 A987 EDCB

	subk	1,b3		; 1433
	subk	32,b3		; 1413
	expect	1315
	subk	0,b3
	endexpect
	expect	1320
	subk	33,b3
	endexpect

	subxy	a2,a5		; E245
	subxy	b5,b2		; E2B2
	subxy	a7,sp		; E2EF
	subxy	sp,a8		; E3E8
	subxy	b9,sp		; E33F
	subxy	sp,b10		; E3FA
	expect	1350
	subxy	a1,b10
	endexpect

	trap	0		; 0900
	trap	31		; 091F
	expect	1320
	trap	32
	endexpect

	xor	a2,a5		; 5645
	xor	b5,b2		; 56B2
	xor	a7,sp		; 56EF
	xor	sp,a8		; 57E8
	xor	b9,sp		; 573F
	xor	sp,b10		; 57FA
	expect	1350
	xor	a1,b10
	endexpect

	xori	1234h,b5	; 0BD5 1234 0000
	expect	1130
	xori	1234h,b5,w
	endexpect
	xori	1234h,b5,l	; 0BD5 1234 0000
	xori	12345678h,b5	; 0BD5 5678 1234
	expect	1130
	xori	12345678h,b5,w
	endexpect
	xori	12345678h,b5,l	; 0BD5 5678 1234

	zext	a2		; 0522
	zext	a2,0		; 0522
	zext	a2,1		; 0722
	zext	b9		; 0539
	zext	b9,0		; 0539
	zext	b9,1		; 0739

; -------------------------------------------

	.byte	10,20,30,40	; 0A 14 1E 28
	.string	"Hello, world"
	.word	10,20,30	; 000A 0014 001E
	.int	10,20,30	; 0000000A 00000014 0000001E
	.long	10,20,30	; 0000000A 00000014 0000001E

	;.float	-1.0e25		; E9C22CA9
	.float	.5		; 3FC00000
	.float	3		; 40E00000
	.float	-123		; C37B0000
	;.float	3.14159		; 40E487E8
	;.float	-0.014E-14	; A5D0B45A (A5734ACA?)
	;.float	36e10		; 5353D1AC

	;.double	-1.0e25		; 16140148 C5384595
	.double	.5		; 00000000 3FF80000
	.double 3		; 00000000 401COOOO
	.double	-123		; 00000000 C06F6000
	;.double	3.14159		; F80DC337 401C90FC
	;.double	-0.014E-14	; 4EBEFD02 BCBA168B (4BEC44DE BCAE6959?)
	.double	36e10		; 82000000 426A7A35

	.field	23,7		; 17
	expect	180,180
	nop
	nop
	endexpect
	even
	nop

f1	.space	20
f2	.bes	20

