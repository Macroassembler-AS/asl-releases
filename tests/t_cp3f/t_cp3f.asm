	cpu	cp-3f
	page	0

port1	port	4
port2	port	5

	las	7	; f7
	ld	a,#7
	las	15	; ff
	ld	a,#15
	expect	1320,1320
	las	16	; !
	ld	a,#<16
	endexpect

	lss	6	; 2e
	ld	s,#6
	lss	7	; 2f
	ld	s,#7
	expect	1320,1320
	lss	8	; !
	ld	s,#8
	endexpect

	lts	3	; 3b
	ld	t,#3
	lts	7	; 3f
	ld	t,#7
	expect	1320,1320
	lts	8	; !
	ld	t,#8
	endexpect

	lal	10	; 04 0a
	ld	a,#>10
	lal	-10	; 04 f6
	ld	a,#-10
	lal	255	; 04 ff
	ld	a,#255
	lal	-128	; 04 80
	ld	a,#-128
	expect	1320,1320
	lal	256	; !
	ld	a,#256
	endexpect
	expect	1315,1315
	lal	-129	; !
	ld	a,#-129
	endexpect

	anl	24h	; 05 24
	and	#24h
	and	a,#24h
	anl	-24h	; 05 dc
	and	#-24h
	and	a,#-24h
	anl	255	; 05 ff
	and	#255
	and	a,#255
	anl	-128	; 05 80
	and	#-128
	and	a,#-128
	expect	1320,1320,1320
	anl	256	; !
	and	#256
	and	a,#256
	endexpect
	expect	1315,1315,1315
	anl	-129	; !
	and	#-129
	and	a,#-129
	endexpect

	eol	35h	; 0c 35
	xor	#35h
	xor	a,#35h
	eol	-35h	; 0c cb
	xor	#-35h
	xor	a,#-35h
	eol	255	; 0c ff
	xor	#255
	xor	a,#255
	eol	-128	; 0c 80
	xor	#-128
	xor	a,#-128
	expect	1320,1320,1320
	eol	256	; !
	xor	#256
	xor	a,#256
	endexpect
	expect	1315,1315,1315
	eol	-129	; !
	xor	#-129
	xor	a,#-129
	endexpect

	orl	4ah	; 0d 4a
	or	#4ah
	or	a,#4ah
	orl	-4ah	; 0d b6
	or	#-4ah
	or	a,#-4ah
	orl	255	; 0d ff
	or	#255
	or	a,#255
	orl	-128	; 0d 80
	or	#-128
	or	a,#-128
	expect	1320,1320,1320
	orl	256	; !
	or	#256
	or	a,#256
	endexpect
	expect	1315,1315,1315
	orl	-129	; !
	or	#-129
	or	a,#-129
	endexpect

	adl	161	; 0e a1
	add	#161
	add	a,#161
	adl	-61	; 0e c3
	add	#-61
	add	a,#-61
	adl	255	; 0e ff
	add	#255
	add	a,#255
	adl	-128	; 0e 80
	add	#-128
	add	a,#-128
	expect	1320,1320,1320
	adl	256	; !
	add	#256
	add	a,#256
	endexpect
	expect	1315,1315,1315
	adl	-129	; !
	add	#-129
	add	a,#-129
	endexpect

	cml	16	; 0f 10
	cp	#16
	cp	a,#16
	cml	-16	; 0f f0
	cp	#-16
	cp	a,#-16
	cml	255	; 0f ff
	cp	#255
	cp	a,#255
	cml	-128	; 0f 80
	cp	#-128
	cp	a,#-128
	expect	1320,1320,1320
	cml	256	; !
	cp	#256
	cp	a,#256
	endexpect
	expect	1315,1315,1315
	cml	-129	; !
	cp	#-129
	cp	a,#-129
	endexpect

	lav		; 08
	ld	a,v
	law		; 09
	ld	a,w
	lax		; 0a
	ld	a,x
	lay		; 0b
	ld	a,y
	sav		; 18
	ld	v,a
	saw		; 19
	ld	w,a
	sax		; 1a
	ld	x,a
	say		; 1b
	ld	y,a
	sat		; 01
	ld	t,a
	sst		; 03
	ld	st,a

	als		; 1c
	sla
	sla	a
	sla	a,1
	ars		; 1d
	srl
	srl	a
	srl	a,1
	alf		; 1e
	sla	4
	sla	a,4
	arf		; 1f
	srl	4
	srl	a,4

	lar	10	; 8a
	ld	a,10
	lar	12	; 8c
	ld	a,(st)
	lar	13	; 8d
	ld	a,(st)-
	lar	14	; 8e
	ld	a,(st)+
	sar	10	; 9a
	ld	10,a
	sar	11	; 9b
	ld	11,a
	sar	12	; 9c
	ld	(st),a
	sar	13	; 9d
	ld	(st)-,a
	sar	14	; 9e
	ld	(st)+,a
	adr	14	; ae
	add	(st)+
	add	a,(st)+
	anr	11	; bb
	and	11
	and	a,11
	eor	12	; cc
	xor	(st)
	xor	a,(st)
	der	9	; d9
	dec	9
	dar	14	; ee

	inp	port1	; 24
	out	port2	; 35

	jmp	123h	; 41 23
	jaz	123h	; 49 23
	jan	123h	; 51 23
	jap	123h	; 59 23
	jsd	123h	; 61 23
	jcn	123h	; 69 23
	jcz	123h	; 71 23
	jsb	123h	; 79 23
	gos	123h	; 79 23 (alias on LP8000)
	ret		; 00

	six		; 02
	ld	(z(x)),a
	lix		; 06
	ld	a,(z(x))
	liy		; 07
	ld	a,(z(y))
	sqx		; 16
	ld	q(x),a
	sqy		; 17
	ld	q(y),a
	szx		; 12
	ld	z(x),a
	szy		; 13
	ld	z(y),a
