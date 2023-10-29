	page	0
	z80syntax on

	switch	momcpuname
	 case	"78C06"
	  ; EA & V do not exist and are treated as ordinary symbols:
	  ; locate on 'impossible' addresses so usage triggers errors:
ea	  equ	10000h
eal	  equ	ea
eah	  equ	eal+1
v	  equ	10002h
zcm	  equ	10003h
	 case 	"7801"
	  ; EA does not exist and is treated as ordinary symbols:
	  ; locate on 'impossible' address so usage triggers errors:
ea	  equ	10000h
eal	  equ	ea
eah	  equ	eal+1
	  assume v:0ffh
	 case	"7807"
	 case	"78C10"
	  assume v:0ffh
	 elsecase
	  error	"unknown CPU set"
	endcase

;--------------------------------------------
; MOV r1,A -> LD r1,A (all cores, EAL/EAH on 07/10)

	mov	eah,a		; 18
	ld	eah,a
	mov	eal,a		; 19
	ld	eal,a
	mov	b,a		; 1A
	ld	b,a
	mov	c,a		; 1B
	ld	c,a
	mov	d,a		; 1C
	ld	d,a
	mov	e,a		; 1D
	ld	e,a
	mov	h,a		; 1E
	ld	h,a
	mov	l,a		; 1F
	ld	l,a

;--------------------------------------------
; MOV A,r1 -> LD A,r1 (all cores, EAL/EAH on 07/10)

	mov	a,eah		; 08
	ld	a,eah
	mov	a,eal		; 09
	ld	a,eal
	mov	a,b		; 0A
	ld	a,b
	mov	a,c		; 0B
	ld	a,c
	mov	a,d		; 0C
	ld	a,d
	mov	a,e		; 0D
	ld	a,e
	mov	a,h		; 0E
	ld	a,h
	mov	a,l		; 0F
	ld	a,l

	expect	1350,1350,1350
	ld	a,a
	ld	a,v
	ld	v,a
	endexpect

;--------------------------------------------
; MOV sr,A -> LD sr,A (all cores)

	mov	pb,a		; 4D C1
	ld	pb,a

;--------------------------------------------
; MOV A,sr1 -> LD A,sr1 (all cores)

	mov	a,pb		; 4C C1
	ld	a,pb

;--------------------------------------------
; MOV r,word -> LD r,(word) (all cores, V on Hi/07/10)

	mov	v,1234h		; 70 68 34 12
	ld	v,(1234h)
	mov	a,1234h		; 70 69 34 12
	ld	a,(1234h)
	mov	b,1234h		; 70 6A 34 12
	ld	b,(1234h)
	mov	c,1234h		; 70 6B 34 12
	ld	c,(1234h)
	mov	d,1234h		; 70 6C 34 12
	ld	d,(1234h)
	mov	e,1234h		; 70 6D 34 12
	ld	e,(1234h)
	mov	h,1234h		; 70 6E 34 12
	ld	h,(1234h)
	mov	l,1234h		; 70 6F 34 12
	ld	l,(1234h)

;--------------------------------------------
; MOV word,r -> LD (word),r (all cores, V on Hi/07/10)

	mov	1234h,v		; 70 78 34 12
	ld	(1234h),v
	mov	1234h,a		; 70 79 34 12
	ld	(1234h),a
	mov	1234h,b		; 70 7A 34 12
	ld	(1234h),b
	mov	1234h,c		; 70 7B 34 12
	ld	(1234h),c
	mov	1234h,d		; 70 7C 34 12
	ld	(1234h),d
	mov	1234h,e		; 70 7D 34 12
	ld	(1234h),e
	mov	1234h,h		; 70 7E 34 12
	ld	(1234h),h
	mov	1234h,l		; 70 7F 34 12
	ld	(1234h),l

;--------------------------------------------
; MVI r,byte -> LD r,byte (all cores, V on Hi/07/10)

	mvi	v,12h		; 68 12
	ld	v,12h
	mvi	a,12h		; 69 12
	ld	a,12h
	mvi	b,12h		; 6A 12
	ld	b,12h
	mvi	c,12h		; 6B 12
	ld	c,12h
	mvi	d,12h		; 6C 12
	ld	d,12h
	mvi	e,12h		; 6D 12
	ld	e,12h
	mvi	h,12h		; 6E 12
	ld	h,12h
	mvi	l,12h		; 6F 12
	ld	l,12h

	expect	1320
	ld	a,1234h
	endexpect

;--------------------------------------------
; MVI sr2,byte -> LD sr2,byte (Hi/07/10)

	mvi	smh,12h		; 64 81 12
	ld	smh,12h

;--------------------------------------------
; MVIW wa,byte -> LD (wa),byte (Hi/07/10)

	mviw	0ff12h,34h	; 71 12 34
	ld	(0ff12h),34h

;--------------------------------------------
; MVIX rpa1,byte -> LD (rpa1),byte (Hi/07/10)

	mvix	b,12h		; 49 12
	mvix	bc,12h
	mvix	(b),12h
	mvix	(bc),12h
	ld	(b),12h
	ld	(bc),12h
	mvix	d,12h		; 4A 12
	mvix	de,12h
	mvix	(d),12h
	mvix	(de),12h
	ld	(d),12h
	ld	(de),12h
	mvix	h,12h		; 4B 12
	mvix	hl,12h
	mvix	(h),12h
	mvix	(hl),12h
	ld	(h),12h
	ld	(hl),12h

;--------------------------------------------
; STAW wa -> LD (wa),A (all cores)

	staw	0ff12h		; 63 12 or 38 12
	ld	(0ff12h),a
	ld	(0ff12h),b	; 70 7A 12 FF (changed to absolute)

;--------------------------------------------
; LDAW wa -> LD A,(wa) (all cores)

	ldaw	0ff12h		; 01 12 or 28 12
	ld	a,(0ff12h)
	ld	b,(0ff12h)	; 70 6A 12 FF (changed to absolute)

;--------------------------------------------
; STAX rpa2 -> LD (rpa2),A (all cores, not all modes on Lo/Hi)

	stax	b		; 39
	stax	bc
	stax	(b)
	stax	(bc)
	ld	(b),a
	ld	(bc),a
	stax	d		; 3A
	stax	de
	stax	(d)
	stax	(de)
	ld	(d),a
	ld	(de),a
	stax	h		; 3B
	stax	hl
	stax	(h)
	stax	(hl)
	ld	(h),a
	ld	(hl),a
	stax	d+		; 3C
	stax	de+
	stax	(d)+
	stax	(de)+
	ld	(d)+,a
	ld	(de)+,a
	ld	d+,a
	ld	de+,a
	stax	h+		; 3D
	stax	hl+
	stax	(h)+
	stax	(hl)+
	ld	h+,a
	ld	hl+,a
	ld	(h)+,a
	ld	(hl)+,a
	stax	d-		; 3E
	stax	de-
	stax	(d)-
	stax	(de)-
	ld	(d)-,a
	ld	(de)-,a
	ld	d-,a
	ld	de-,a
	stax	h-		; 3F
	stax	hl-
	stax	(h)-
	stax	(hl)-
	ld	h-,a
	ld	hl-,a
	ld	(h)-,a
	ld	(hl)-,a
	stax	d+12h		; BB 12
	stax	(d+12h)
	stax	de+12h
	stax	(de+12h)
	ld	(d+12h),a
	ld	(de+12h),a
	ld	( 12h + d ) , a
	ld	( 12h + de ) , a
	stax	h+a		; BC
	stax	(h+a)
	stax	hl+a
	stax	(hl+a)
	ld	(h+a),a
	ld	(hl+a),a
	ld	( a + h ) , a
	ld	( a + hl ) , a
	stax	h+b		; BD
	stax	(h+b)
	stax	hl+b
	stax	(hl+b)
	ld	(h+b),a
	ld	(hl+b),a
	ld	( b + h ) , a
	ld	( b + hl ) , a
	stax	h+ea		; BE
	stax	(h+ea)
	stax	hl+ea
	stax	(hl+ea)
	ld	(h+ea),a
	ld	(hl+ea),a
	ld	( ea + h ) , a
	ld	( ea + hl ) , a
	stax	h+12h		; BF 12
	stax	(h+12h)
	stax	hl+12h
	stax	(hl+12h)
	ld	(h+12h),a
	ld	(hl+12h),a
	ld	( 12h + h ) , a
	ld	( 12h + hl ) , a

;--------------------------------------------
; LDAX rpa2 -> LD A,(rpa2) (all cores, not all modes on Lo/Hi)

	ldax	b		; 29
	ldax	bc
	ldax	(b)
	ldax	(bc)
	ld	a,(b)
	ld	a,(bc)
	ldax	d		; 2A
	ldax	de
	ldax	(d)
	ldax	(de)
	ld	a,(d)
	ld	a,(de)
	ldax	h		; 2B
	ldax	hl
	ldax	(h)
	ldax	(hl)
	ld	a,(h)
	ld	a,(hl)
	ldax	d+		; 2C
	ldax	de+
	ldax	(d)+
	ldax	(de)+
	ld	a,(d)+
	ld	a,(de)+
	ld	a,d+
	ld	a,de+
	ldax	h+		; 2D
	ldax	hl+
	ldax	(h)+
	ldax	(hl)+
	ld	a,h+
	ld	a,hl+
	ld	a,(h)+
	ld	a,(hl)+
	ldax	d-		; 2E
	ldax	de-
	ldax	(d)-
	ldax	(de)-
	ld	a,(d)-
	ld	a,(de)-
	ld	a,d-
	ld	a,de-
	ldax	h-		; 2F
	ldax	hl-
	ldax	(h)-
	ldax	(hl)-
	ld	a,h-
	ld	a,hl-
	ld	a,(h)-
	ld	a,(hl)-
	ldax	d+12h		; AB 12
	ldax	(d+12h)
	ldax	de+12h
	ldax	(de+12h)
	ld	a,(d+12h)
	ld	a,(de+12h)
	ld	a, ( 12h + d )
	ld	a, ( 12h + de )
	ldax	h+a		; AC
	ldax	(h+a)
	ldax	hl+a
	ldax	(hl+a)
	ld	a,(h+a)
	ld	a,(hl+a)
	ld	a, ( a + h )
	ld	a, ( a + hl )
	ldax	h+b		; AD
	ldax	(h+b)
	ldax	hl+b
	ldax	(hl+b)
	ld	a,(h+b)
	ld	a,(hl+b)
	ld	a, ( b + h )
	ld	a, ( b + hl )
	ldax	h+ea		; AE
	ldax	(h+ea)
	ldax	hl+ea
	ldax	(hl+ea)
	ld	a,(h+ea)
	ld	a,(hl+ea)
	ld	a, ( ea + h )
	ld	a, ( ea + hl )
	ldax	h+12h		; AF 12
	ldax	(h+12h)
	ldax	hl+12h
	ldax	(hl+12h)
	ld	a,(h+12h)
	ld	a,(hl+12h)
	ld	a, ( 12h + h )
	ld	a, ( 12h + hl )

;--------------------------------------------
; DMOV rp3,EA -> LD rp3,EA (07/10)

	dmov	bc,ea		; B5
	ld	bc,ea
	dmov	de,ea		; B6
	ld	de,ea
	dmov	hl,ea		; B7
	ld	hl,ea

;--------------------------------------------
; DMOV sr3,EA -> LD EA,sr3 (07/10)

	dmov	etm0,ea		; 48 D2
	ld	etm0,ea
	dmov	etm1,ea		; 48 D3
	ld	etm1,ea

;--------------------------------------------
; DMOV EA,sr4 -> LD EA,sr4 (07/10)

	dmov	ea,ecnt		; 48 C0
	ld	ea,ecnt
	dmov	ea,ecpt		; 48 C1
	ld	ea,ecpt

;--------------------------------------------
; DMOV EA,rp3 -> LD EA,rp3 (07/10)

	dmov	ea,bc		; A5
	ld	ea,bc
	dmov	ea,de		; A6
	ld	ea,de
	dmov	ea,hl		; A7
	ld	ea,hl

;--------------------------------------------
; SBCD word -> LD (word),BC (all cores)

	sbcd	1234h		; 70 1E 34 12
	ld	(1234h),bc
	ld	(0ff12h),bc	; 70 1E 12 FF (WA changed to absolute)

;--------------------------------------------
; SDED word -> LD (word),DE (all cores)

	sded	1234h		; 70 2E 34 12
	ld	(1234h),de

;--------------------------------------------
; SHLD word -> LD (word),HL (all cores)

	shld	1234h		; 70 3E 34 12
	ld	(1234h),hl

;--------------------------------------------
; SSPD word -> LD (word),SP (all cores)

	sspd	1234h		; 70 0E 34 12
	ld	(1234h),sp

;--------------------------------------------
; STEAX rpa3 -> LD (rpa3),EA (07/10)

	steax	d		; 48 92
	steax	de
	steax	(d)
	steax	(de)
	ld	(d),ea
	ld	(de),ea
	steax	h		; 48 93
	steax	hl
	steax	(h)
	steax	(hl)
	ld	(h),ea
	ld	(hl),ea
	steax	d++		; 48 94
	steax	de++
	steax	(d)++
	steax	(de++)
	ld	(d)++,ea
	ld	(de)++,ea
	steax	h++		; 48 95
	steax	hl++
	steax	(h)++
	steax	(hl++)
	ld	(h)++,ea
	ld	(hl)++,ea
	steax	d+12h		; 48 9B 12
	steax	de+12h
	steax	(d+12h)
	steax	(de+12h)
	ld	(d+12h),ea
	ld	(de+12h),ea
	ld	(12h+d),ea
	ld	(12h+de),ea
	steax	h+a		; 48 9C
	steax	(h+a)
	steax	hl+a
	steax	(hl+a)
	steax	a+h
	steax	(a+h)
	steax	a+hl
	steax	(a+hl)
	ld	(h+a),ea
	ld	(hl+a),ea
	ld	(a+h),ea
	ld	(a+hl),ea
	steax	h+b		; 48 9D
	steax	(h+b)
	steax	hl+b
	steax	(hl+b)
	steax	b+h
	steax	(b+h)
	steax	b+hl
	steax	(b+hl)
	ld	(h+b),ea
	ld	(hl+b),ea
	ld	(b+h),ea
	ld	(b+hl),ea
	steax	h+ea		; 48 9E
	steax	(h+ea)
	steax	hl+ea
	steax	(hl+ea)
	steax	ea+h
	steax	(ea+h)
	steax	ea+hl
	steax	(ea+hl)
	ld	(h+ea),ea
	ld	(hl+ea),ea
	ld	(ea+h),ea
	ld	(ea+hl),ea
	steax	h+12h		; 48 9F 12
	steax	hl+12h
	steax	(h+12h)
	steax	(hl+12h)
	ld	(h+12h),ea
	ld	(hl+12h),ea
	ld	(12h+h),ea
	ld	(12h+hl),ea

;--------------------------------------------
; LBCD word -> LD BC,(word) (all cores)

	lbcd	1234h		; 70 1F 34 12
	ld	bc,(1234h)

;--------------------------------------------
; LDED word -> LD DE,(word) (all cores)

	lded	1234h		; 70 2F 34 12
	ld	de,(1234h)

;--------------------------------------------
; LHLD word -> LD HL,(word) (all cores)

	lhld	1234h		; 70 3F 34 12
	ld	hl,(1234h)

;--------------------------------------------
; LSPD word -> LD SP,(word) (all cores)

	lspd	1234h		; 70 0F 34 12
	ld	sp,(1234h)

;--------------------------------------------
; LDEAX rpa3 -> LD EA,(rpa3) (07/10)

	ldeax	d		; 48 82
	ldeax	de
	ldeax	(d)
	ldeax	(de)
	ld	ea,(d)
	ld	ea,(de)
	ldeax	h		; 48 83
	ldeax	hl
	ldeax	(h)
	ldeax	(hl)
	ld	ea,(h)
	ld	ea,(hl)
	ldeax	d++		; 48 84
	ldeax	de++
	ldeax	(d)++
	ldeax	(de++)
	ld	ea,(d)++
	ld	ea,(de)++
	ldeax	h++		; 48 85
	ldeax	hl++
	ldeax	(h)++
	ldeax	(hl++)
	ld	ea,(h)++
	ld	ea,(hl)++
	ldeax	d+12h		; 48 8B 12
	ldeax	de+12h
	ldeax	(d+12h)
	ldeax	(de+12h)
	ld	ea,(d+12h)
	ld	ea,(de+12h)
	ld	ea,(12h+d)
	ld	ea,(12h+de)
	ldeax	h+a		; 48 8C
	ldeax	(h+a)
	ldeax	hl+a
	ldeax	(hl+a)
	ldeax	a+h
	ldeax	(a+h)
	ldeax	a+hl
	ldeax	(a+hl)
	ld	ea,(h+a)
	ld	ea,(hl+a)
	ld	ea,(a+h)
	ld	ea,(a+hl)
	ldeax	h+b		; 48 8D
	ldeax	(h+b)
	ldeax	hl+b
	ldeax	(hl+b)
	ldeax	b+h
	ldeax	(b+h)
	ldeax	b+hl
	ldeax	(b+hl)
	ld	ea,(h+b)
	ld	ea,(hl+b)
	ld	ea,(b+h)
	ld	ea,(b+hl)
	ldeax	h+ea		; 48 8E
	ldeax	(h+ea)
	ldeax	hl+ea
	ldeax	(hl+ea)
	ldeax	ea+h
	ldeax	(ea+h)
	ldeax	ea+hl
	ldeax	(ea+hl)
	ld	ea,(h+ea)
	ld	ea,(hl+ea)
	ld	ea,(ea+h)
	ld	ea,(ea+hl)
	ldeax	h+12h		; 48 8F 12
	ldeax	hl+12h
	ldeax	(h+12h)
	ldeax	(hl+12h)
	ld	ea,(h+12h)
	ld	ea,(hl+12h)
	ld	ea,(12h+h)
	ld	ea,(12h+hl)

;--------------------------------------------
; LXI rp2,word -> LD rp2,word (all cores, EA on 07/10)

	lxi	sp,1234h	; 04 34 12
	ld	sp,1234h
	lxi	b,1234h		; 14 34 12
	ld	bc,1234h
	lxi	d,1234h		; 24 34 12
	ld	de,1234h
	lxi	h,1234h		; 34 34 12
	ld	hl,1234h
	lxi	ea,1234h	; 44 34 12
	ld	ea,1234h

;--------------------------------------------
; ADD A,r (remains unchanged) (all cores, V on Hi/07/10)

	add	a,v		; 60 C0
	add	a,a		; 60 C1
	add	a,b		; 60 C2
	add	a,c		; 60 C3
	add	a,d		; 60 C4
	add	a,e		; 60 C5
	add	a,h		; 60 C6
	add	a,l		; 60 C7

;--------------------------------------------
; ADD r,A (remains unchanged) (Hi/07/10)

	add	v,a		; 60 40
	add	>a,a		; 60 41
	add	b,a		; 60 42
	add	c,a		; 60 43
	add	d,a		; 60 44
	add	e,a		; 60 45
	add	h,a		; 60 46
	add	l,a		; 60 47

;--------------------------------------------
; ADC A,r (remains unchanged) (all cores, V on Hi/07/10)

	adc	a,v		; 60 D0
	adc	a,a		; 60 D1
	adc	a,b		; 60 D2
	adc	a,c		; 60 D3
	adc	a,d		; 60 D4
	adc	a,e		; 60 D5
	adc	a,h		; 60 D6
	adc	a,l		; 60 D7

;--------------------------------------------
; ADC r,A (remains unchanged) (Hi/07/10)

	adc	v,a		; 60 50
	adc	>a,a		; 60 51
	adc	b,a		; 60 52
	adc	c,a		; 60 53
	adc	d,a		; 60 54
	adc	e,a		; 60 55
	adc	h,a		; 60 56
	adc	l,a		; 60 57

;--------------------------------------------
; ADDNC A,r (remains unchanged) (all cores, V on Hi/07/10)

	addnc	a,v		; 60 A0
	addnc	a,a		; 60 A1
	addnc	a,b		; 60 A2
	addnc	a,c		; 60 A3
	addnc	a,d		; 60 A4
	addnc	a,e		; 60 A5
	addnc	a,h		; 60 A6
	addnc	a,l		; 60 A7

;--------------------------------------------
; ADDNC r,A (remains unchanged) (Hi/07/10)

	addnc	v,a		; 60 20
	addnc	>a,a		; 60 21
	addnc	b,a		; 60 22
	addnc	c,a		; 60 23
	addnc	d,a		; 60 24
	addnc	e,a		; 60 25
	addnc	h,a		; 60 26
	addnc	l,a		; 60 27

;--------------------------------------------
; SUB A,r (remains unchanged) (all cores, V on Hi/07/10)

	sub	a,v		; 60 E0
	sub	a,a		; 60 E1
	sub	a,b		; 60 E2
	sub	a,c		; 60 E3
	sub	a,d		; 60 E4
	sub	a,e		; 60 E5
	sub	a,h		; 60 E6
	sub	a,l		; 60 E7

;--------------------------------------------
; SUB r,A (remains unchanged) (Hi/07/10)

	sub	v,a		; 60 60
	sub	>a,a		; 60 61
	sub	b,a		; 60 62
	sub	c,a		; 60 63
	sub	d,a		; 60 64
	sub	e,a		; 60 65
	sub	h,a		; 60 66
	sub	l,a		; 60 67

;--------------------------------------------
; SBB A,r (remains unchanged) (all cores, V on Hi/07/10)

	sbb	a,v		; 60 F0
	sbb	a,a		; 60 F1
	sbb	a,b		; 60 F2
	sbb	a,c		; 60 F3
	sbb	a,d		; 60 F4
	sbb	a,e		; 60 F5
	sbb	a,h		; 60 F6
	sbb	a,l		; 60 F7

;--------------------------------------------
; SBB r,A (remains unchanged) (Hi/07/10)

	sbb	v,a		; 60 70
	sbb	>a,a		; 60 71
	sbb	b,a		; 60 72
	sbb	c,a		; 60 73
	sbb	d,a		; 60 74
	sbb	e,a		; 60 75
	sbb	h,a		; 60 76
	sbb	l,a		; 60 77

;--------------------------------------------
; SUBNB A,r (remains unchanged) (all cores, V on Hi/07/10)

	subnb	a,v		; 60 B0
	subnb	a,a		; 60 B1
	subnb	a,b		; 60 B2
	subnb	a,c		; 60 B3
	subnb	a,d		; 60 B4
	subnb	a,e		; 60 B5
	subnb	a,h		; 60 B6
	subnb	a,l		; 60 B7

;--------------------------------------------
; SUBNB r,A (remains unchanged) (Hi/07/10)

	subnb	v,a		; 60 30
	subnb	>a,a		; 60 31
	subnb	b,a		; 60 32
	subnb	c,a		; 60 33
	subnb	d,a		; 60 34
	subnb	e,a		; 60 35
	subnb	h,a		; 60 36
	subnb	l,a		; 60 37

;--------------------------------------------
; ANA A,r -> AND A,r (all cores, V on Hi/07/10)

	ana	a,v		; 60 88
	and	a,v
	ana	a,a		; 60 89
	and	a,a
	ana	a,b		; 60 8A
	and	a,b
	ana	a,c		; 60 8B
	and	a,c
	ana	a,d		; 60 8C
	and	a,d
	ana	a,e		; 60 8D
	and	a,e
	ana	a,h		; 60 8E
	and	a,h
	ana	a,l		; 60 8F
	and	a,l

;--------------------------------------------
; ANA r,A -> AND r,A (Hi/07/10)

	ana	v,a		; 60 08
	and	v,a
	ana	>a,a		; 60 09
	and	>a,a
	ana	b,a		; 60 0A
	and	b,a
	ana	c,a		; 60 0B
	and	c,a
	ana	d,a		; 60 0C
	and	d,a
	ana	e,a		; 60 0D
	and	e,a
	ana	h,a		; 60 0E
	and	h,a
	ana	l,a		; 60 0F
	and	l,a

;--------------------------------------------
; ORA A,r -> OR A,r (all cores, V on Hi/07/10)

	ora	a,v		; 60 98
	or	a,v
	ora	a,a		; 60 99
	or	a,a
	ora	a,b		; 60 9A
	or	a,b
	ora	a,c		; 60 9B
	or	a,c
	ora	a,d		; 60 9C
	or	a,d
	ora	a,e		; 60 9D
	or	a,e
	ora	a,h		; 60 9E
	or	a,h
	ora	a,l		; 60 9F
	or	a,l

;--------------------------------------------
; ORA r,A -> OR r,A (Hi/07/10)

	ora	v,a		; 60 18
	or	v,a
	ora	>a,a		; 60 19
	or	>a,a
	ora	b,a		; 60 1A
	or	b,a
	ora	c,a		; 60 1B
	or	c,a
	ora	d,a		; 60 1C
	or	d,a
	ora	e,a		; 60 1D
	or	e,a
	ora	h,a		; 60 1E
	or	h,a
	ora	l,a		; 60 1F
	or	l,a

;--------------------------------------------
; XRA A,r -> XOR A,r (all cores, V on Hi/07/10)

	xra	a,v		; 60 90
	xor	a,v
	xra	a,a		; 60 91
	xor	a,a
	xra	a,b		; 60 92
	xor	a,b
	xra	a,c		; 60 93
	xor	a,c
	xra	a,d		; 60 94
	xor	a,d
	xra	a,e		; 60 95
	xor	a,e
	xra	a,h		; 60 96
	xor	a,h
	xra	a,l		; 60 97
	xor	a,l

;--------------------------------------------
; XRA r,A -> XOR r,A (Hi/07/10)

	xra	v,a		; 60 10
	xor	v,a
	xra	>a,a		; 60 11
	xor	>a,a
	xra	b,a		; 60 12
	xor	b,a
	xra	c,a		; 60 13
	xor	c,a
	xra	d,a		; 60 14
	xor	d,a
	xra	e,a		; 60 15
	xor	e,a
	xra	h,a		; 60 16
	xor	h,a
	xra	l,a		; 60 17
	xor	l,a

;--------------------------------------------
; GTA A,r -> SKGT A,r (all cores, V on Hi/07/10)

	gta	a,v		; 60 A8
	skgt	a,v
	gta	a,a		; 60 A9
	skgt	a,a
	gta	a,b		; 60 AA
	skgt	a,b
	gta	a,c		; 60 AB
	skgt	a,c
	gta	a,d		; 60 AC
	skgt	a,d
	gta	a,e		; 60 AD
	skgt	a,e
	gta	a,h		; 60 AE
	skgt	a,h
	gta	a,l		; 60 AF
	skgt	a,l

;--------------------------------------------
; GTA r,A -> SKGT r,A (Hi/07/10)

	gta	v,a		; 60 28
	skgt	v,a
	gta	>a,a		; 60 29
	skgt	>a,a
	gta	b,a		; 60 2A
	skgt	b,a
	gta	c,a		; 60 2B
	skgt	c,a
	gta	d,a		; 60 2C
	skgt	d,a
	gta	e,a		; 60 2D
	skgt	e,a
	gta	h,a		; 60 2E
	skgt	h,a
	gta	l,a		; 60 2F
	skgt	l,a

;--------------------------------------------
; LTA A,r -> SKLT A,r (all cores, V on Hi/07/10)

	lta	a,v		; 60 B8
	sklt	a,v
	lta	a,a		; 60 B9
	sklt	a,a
	lta	a,b		; 60 BA
	sklt	a,b
	lta	a,c		; 60 BB
	sklt	a,c
	lta	a,d		; 60 BC
	sklt	a,d
	lta	a,e		; 60 BD
	sklt	a,e
	lta	a,h		; 60 BE
	sklt	a,h
	lta	a,l		; 60 BF
	sklt	a,l

;--------------------------------------------
; LTA r,A -> SKLT r,A (Hi/07/10)

	lta	v,a		; 60 38
	sklt	v,a
	lta	>a,a		; 60 39
	sklt	>a,a
	lta	b,a		; 60 3A
	sklt	b,a
	lta	c,a		; 60 3B
	sklt	c,a
	lta	d,a		; 60 3C
	sklt	d,a
	lta	e,a		; 60 3D
	sklt	e,a
	lta	h,a		; 60 3E
	sklt	h,a
	lta	l,a		; 60 3F
	sklt	l,a

;--------------------------------------------
; NEA A,r -> SKNE A,r (all cores, V on Hi/07/10)

	nea	a,v		; 60 E8
	skne	a,v
	nea	a,a		; 60 E9
	skne	a,a
	nea	a,b		; 60 EA
	skne	a,b
	nea	a,c		; 60 EB
	skne	a,c
	nea	a,d		; 60 EC
	skne	a,d
	nea	a,e		; 60 ED
	skne	a,e
	nea	a,h		; 60 EE
	skne	a,h
	nea	a,l		; 60 EF
	skne	a,l

;--------------------------------------------
; NEA r,A -> SKNE r,A (Hi/07/10)

	nea	v,a		; 60 68
	skne	v,a
	nea	>a,a		; 60 69
	skne	>a,a
	nea	b,a		; 60 AA
	skne	b,a
	nea	c,a		; 60 AB
	skne	c,a
	nea	d,a		; 60 AC
	skne	d,a
	nea	e,a		; 60 AD
	skne	e,a
	nea	h,a		; 60 AE
	skne	h,a
	nea	l,a		; 60 AF
	skne	l,a

;--------------------------------------------
; EQA A,r -> SKEQ A,r (all cores, V on Hi/07/10)

	eqa	a,v		; 60 F8
	skeq	a,v
	eqa	a,a		; 60 F9
	skeq	a,a
	eqa	a,b		; 60 FA
	skeq	a,b
	eqa	a,c		; 60 FB
	skeq	a,c
	eqa	a,d		; 60 FC
	skeq	a,d
	eqa	a,e		; 60 FD
	skeq	a,e
	eqa	a,h		; 60 FE
	skeq	a,h
	eqa	a,l		; 60 FF
	skeq	a,l

;--------------------------------------------
; EQA r,A -> SKEQ r,A (Hi/07/10)

	eqa	v,a		; 60 78
	skeq	v,a
	eqa	>a,a		; 60 79
	skeq	>a,a
	eqa	b,a		; 60 7A
	skeq	b,a
	eqa	c,a		; 60 7B
	skeq	c,a
	eqa	d,a		; 60 7C
	skeq	d,a
	eqa	e,a		; 60 7D
	skeq	e,a
	eqa	h,a		; 60 7E
	skeq	h,a
	eqa	l,a		; 60 7F
	skeq	l,a

;--------------------------------------------
; ONA A,r -> SKON A,r (Hi/07/10)

	ona	a,v		; 60 C8
	skon	a,v
	ona	a,a		; 60 C9
	skon	a,a
	ona	a,b		; 60 CA
	skon	a,b
	ona	a,c		; 60 CB
	skon	a,c
	ona	a,d		; 60 CC
	skon	a,d
	ona	a,e		; 60 CD
	skon	a,e
	ona	a,h		; 60 CE
	skon	a,h
	ona	a,l		; 60 CF
	skon	a,l

;--------------------------------------------
; ONA r,A -> SKON r,A (Hi/07/10)

	ona	v,a		; 60 C8
	skon	v,a
	ona	>a,a		; 60 C9
	skon	>a,a
	ona	b,a		; 60 CA
	skon	b,a
	ona	c,a		; 60 CB
	skon	c,a
	ona	d,a		; 60 CC
	skon	d,a
	ona	e,a		; 60 CD
	skon	e,a
	ona	h,a		; 60 CE
	skon	h,a
	ona	l,a		; 60 CF
	skon	l,a

;--------------------------------------------
; OFFA A,r -> SKOFF A,r (Hi/07/10)

	offa	a,v		; 60 D8
	skoff	a,v
	offa	a,a		; 60 D9
	skoff	a,a
	offa	a,b		; 60 DA
	skoff	a,b
	offa	a,c		; 60 DB
	skoff	a,c
	offa	a,d		; 60 DC
	skoff	a,d
	offa	a,e		; 60 DD
	skoff	a,e
	offa	a,h		; 60 DE
	skoff	a,h
	offa	a,l		; 60 DF
	skoff	a,l

;--------------------------------------------
; OFFA r,A -> SKOFF r,A (Hi/07/10)

	offa	v,a		; 60 D8
	skoff	v,a
	offa	>a,a		; 60 D9
	skoff	>a,a
	offa	b,a		; 60 DA
	skoff	b,a
	offa	c,a		; 60 DB
	skoff	c,a
	offa	d,a		; 60 DC
	skoff	d,a
	offa	e,a		; 60 DD
	skoff	e,a
	offa	h,a		; 60 DE
	skoff	h,a
	offa	l,a		; 60 DF
	skoff	l,a

;--------------------------------------------
; ADDX rpa -> ADD a,(rpa) (all cores)

	addx	b		; 70 C1
	addx	bc
	addx	(b)
	addx	(bc)
	add	a,(b)
	add	a,(bc)
	addx	d		; 70 C2
	addx	de
	addx	(d)
	addx	(de)
	add	a,(d)
	add	a,(de)
	addx	h		; 70 C3
	addx	hl
	addx	(h)
	addx	(hl)
	add	a,(h)
	add	a,(hl)
	addx	d+		; 70 C4
	addx	de+
	addx	(d)+
	addx	(de)+
	add	a,(d)+
	add	a,(de)+
	addx	h+		; 70 C5
	addx	hl+
	addx	(h)+
	addx	(hl)+
	add	a,(h)+
	add	a,(hl)+
	addx	d-		; 70 C6
	addx	de-
	addx	(d)-
	addx	(de)-
	add	a,(d)-
	add	a,(de)-
	addx	h-		; 70 C7
	addx	hl-
	addx	(h)-
	addx	(hl)-
	add	a,(h)-
	add	a,(hl)-

;--------------------------------------------
; ADCX rpa -> ADC a,(rpa) (all cores)

	adcx	b		; 70 D1
	adcx	bc
	adcx	(b)
	adcx	(bc)
	adc	a,(b)
	adc	a,(bc)
	adcx	d		; 70 D2
	adcx	de
	adcx	(d)
	adcx	(de)
	adc	a,(d)
	adc	a,(de)
	adcx	h		; 70 D3
	adcx	hl
	adcx	(h)
	adcx	(hl)
	adc	a,(h)
	adc	a,(hl)
	adcx	d+		; 70 D4
	adcx	de+
	adcx	(d)+
	adcx	(de)+
	adc	a,(d)+
	adc	a,(de)+
	adcx	h+		; 70 D5
	adcx	hl+
	adcx	(h)+
	adcx	(hl)+
	adc	a,(h)+
	adc	a,(hl)+
	adcx	d-		; 70 D6
	adcx	de-
	adcx	(d)-
	adcx	(de)-
	adc	a,(d)-
	adc	a,(de)-
	adcx	h-		; 70 D7
	adcx	hl-
	adcx	(h)-
	adcx	(hl)-
	adc	a,(h)-
	adc	a,(hl)-

;--------------------------------------------
; ADDNCX rpa -> ADDNC a,(rpa) (all cores)

	addncx	b		; 70 A1
	addncx	bc
	addncx	(b)
	addncx	(bc)
	addnc	a,(b)
	addnc	a,(bc)
	addncx	d		; 70 A2
	addncx	de
	addncx	(d)
	addncx	(de)
	addnc	a,(d)
	addnc	a,(de)
	addncx	h		; 70 A3
	addncx	hl
	addncx	(h)
	addncx	(hl)
	addnc	a,(h)
	addnc	a,(hl)
	addncx	d+		; 70 A4
	addncx	de+
	addncx	(d)+
	addncx	(de)+
	addnc	a,(d)+
	addnc	a,(de)+
	addncx	h+		; 70 A5
	addncx	hl+
	addncx	(h)+
	addncx	(hl)+
	addnc	a,(h)+
	addnc	a,(hl)+
	addncx	d-		; 70 A6
	addncx	de-
	addncx	(d)-
	addncx	(de)-
	addnc	a,(d)-
	addnc	a,(de)-
	addncx	h-		; 70 A7
	addncx	hl-
	addncx	(h)-
	addncx	(hl)-
	addnc	a,(h)-
	addnc	a,(hl)-

;--------------------------------------------
; SUBX rpa -> SUB a,(rpa) (all cores)

	subx	b		; 70 E1
	subx	bc
	subx	(b)
	subx	(bc)
	sub	a,(b)
	sub	a,(bc)
	subx	d		; 70 E2
	subx	de
	subx	(d)
	subx	(de)
	sub	a,(d)
	sub	a,(de)
	subx	h		; 70 E3
	subx	hl
	subx	(h)
	subx	(hl)
	sub	a,(h)
	sub	a,(hl)
	subx	d+		; 70 E4
	subx	de+
	subx	(d)+
	subx	(de)+
	sub	a,(d)+
	sub	a,(de)+
	subx	h+		; 70 E5
	subx	hl+
	subx	(h)+
	subx	(hl)+
	sub	a,(h)+
	sub	a,(hl)+
	subx	d-		; 70 E6
	subx	de-
	subx	(d)-
	subx	(de)-
	sub	a,(d)-
	sub	a,(de)-
	subx	h-		; 70 E7
	subx	hl-
	subx	(h)-
	subx	(hl)-
	sub	a,(h)-
	sub	a,(hl)-

;--------------------------------------------
; SBBX rpa -> SBB a,(rpa) (all cores)

	sbbx	b		; 70 F1
	sbbx	bc
	sbbx	(b)
	sbbx	(bc)
	sbb	a,(b)
	sbb	a,(bc)
	sbbx	d		; 70 F2
	sbbx	de
	sbbx	(d)
	sbbx	(de)
	sbb	a,(d)
	sbb	a,(de)
	sbbx	h		; 70 F3
	sbbx	hl
	sbbx	(h)
	sbbx	(hl)
	sbb	a,(h)
	sbb	a,(hl)
	sbbx	d+		; 70 F4
	sbbx	de+
	sbbx	(d)+
	sbbx	(de)+
	sbb	a,(d)+
	sbb	a,(de)+
	sbbx	h+		; 70 F5
	sbbx	hl+
	sbbx	(h)+
	sbbx	(hl)+
	sbb	a,(h)+
	sbb	a,(hl)+
	sbbx	d-		; 70 F6
	sbbx	de-
	sbbx	(d)-
	sbbx	(de)-
	sbb	a,(d)-
	sbb	a,(de)-
	sbbx	h-		; 70 F7
	sbbx	hl-
	sbbx	(h)-
	sbbx	(hl)-
	sbb	a,(h)-
	sbb	a,(hl)-

;--------------------------------------------
; SUBNBX rpa -> SUBNB a,(rpa) (all cores)

	subnbx	b		; 70 B1
	subnbx	bc
	subnbx	(b)
	subnbx	(bc)
	subnb	a,(b)
	subnb	a,(bc)
	subnbx	d		; 70 B2
	subnbx	de
	subnbx	(d)
	subnbx	(de)
	subnb	a,(d)
	subnb	a,(de)
	subnbx	h		; 70 B3
	subnbx	hl
	subnbx	(h)
	subnbx	(hl)
	subnb	a,(h)
	subnb	a,(hl)
	subnbx	d+		; 70 B4
	subnbx	de+
	subnbx	(d)+
	subnbx	(de)+
	subnb	a,(d)+
	subnb	a,(de)+
	subnbx	h+		; 70 B5
	subnbx	hl+
	subnbx	(h)+
	subnbx	(hl)+
	subnb	a,(h)+
	subnb	a,(hl)+
	subnbx	d-		; 70 B6
	subnbx	de-
	subnbx	(d)-
	subnbx	(de)-
	subnb	a,(d)-
	subnb	a,(de)-
	subnbx	h-		; 70 B7
	subnbx	hl-
	subnbx	(h)-
	subnbx	(hl)-
	subnb	a,(h)-
	subnb	a,(hl)-

;--------------------------------------------
; ANAX rpa -> AND a,(rpa) (all cores)

	anax	b		; 70 89
	anax	bc
	anax	(b)
	anax	(bc)
	and	a,(b)
	and	a,(bc)
	anax	d		; 70 8A
	anax	de
	anax	(d)
	anax	(de)
	and	a,(d)
	and	a,(de)
	anax	h		; 70 8B
	anax	hl
	anax	(h)
	anax	(hl)
	and	a,(h)
	and	a,(hl)
	anax	d+		; 70 8C
	anax	de+
	anax	(d)+
	anax	(de)+
	and	a,(d)+
	and	a,(de)+
	anax	h+		; 70 8D
	anax	hl+
	anax	(h)+
	anax	(hl)+
	and	a,(h)+
	and	a,(hl)+
	anax	d-		; 70 8E
	anax	de-
	anax	(d)-
	anax	(de)-
	and	a,(d)-
	and	a,(de)-
	anax	h-		; 70 8F
	anax	hl-
	anax	(h)-
	anax	(hl)-
	and	a,(h)-
	and	a,(hl)-

;--------------------------------------------
; ORAX rpa -> OR a,(rpa) (all cores)

	orax	b		; 70 99
	orax	bc
	orax	(b)
	orax	(bc)
	or	a,(b)
	or	a,(bc)
	orax	d		; 70 9A
	orax	de
	orax	(d)
	orax	(de)
	or	a,(d)
	or	a,(de)
	orax	h		; 70 9B
	orax	hl
	orax	(h)
	orax	(hl)
	or	a,(h)
	or	a,(hl)
	orax	d+		; 70 9C
	orax	de+
	orax	(d)+
	orax	(de)+
	or	a,(d)+
	or	a,(de)+
	orax	h+		; 70 9D
	orax	hl+
	orax	(h)+
	orax	(hl)+
	or	a,(h)+
	or	a,(hl)+
	orax	d-		; 70 9E
	orax	de-
	orax	(d)-
	orax	(de)-
	or	a,(d)-
	or	a,(de)-
	orax	h-		; 70 9F
	orax	hl-
	orax	(h)-
	orax	(hl)-
	or	a,(h)-
	or	a,(hl)-

;--------------------------------------------
; XRAX rpa -> XOR a,(rpa) (all cores)

	xrax	b		; 70 91
	xrax	bc
	xrax	(b)
	xrax	(bc)
	xor	a,(b)
	xor	a,(bc)
	xrax	d		; 70 92
	xrax	de
	xrax	(d)
	xrax	(de)
	xor	a,(d)
	xor	a,(de)
	xrax	h		; 70 93
	xrax	hl
	xrax	(h)
	xrax	(hl)
	xor	a,(h)
	xor	a,(hl)
	xrax	d+		; 70 94
	xrax	de+
	xrax	(d)+
	xrax	(de)+
	xor	a,(d)+
	xor	a,(de)+
	xrax	h+		; 70 95
	xrax	hl+
	xrax	(h)+
	xrax	(hl)+
	xor	a,(h)+
	xor	a,(hl)+
	xrax	d-		; 70 96
	xrax	de-
	xrax	(d)-
	xrax	(de)-
	xor	a,(d)-
	xor	a,(de)-
	xrax	h-		; 70 97
	xrax	hl-
	xrax	(h)-
	xrax	(hl)-
	xor	a,(h)-
	xor	a,(hl)-

;--------------------------------------------
; GTAX rpa -> SKGT a,(rpa) (all cores)

	gtax	b		; 70 A9
	gtax	bc
	gtax	(b)
	gtax	(bc)
	skgt	a,(b)
	skgt	a,(bc)
	gtax	d		; 70 AA
	gtax	de
	gtax	(d)
	gtax	(de)
	skgt	a,(d)
	skgt	a,(de)
	gtax	h		; 70 AB
	gtax	hl
	gtax	(h)
	gtax	(hl)
	skgt	a,(h)
	skgt	a,(hl)
	gtax	d+		; 70 AC
	gtax	de+
	gtax	(d)+
	gtax	(de)+
	skgt	a,(d)+
	skgt	a,(de)+
	gtax	h+		; 70 AD
	gtax	hl+
	gtax	(h)+
	gtax	(hl)+
	skgt	a,(h)+
	skgt	a,(hl)+
	gtax	d-		; 70 AE
	gtax	de-
	gtax	(d)-
	gtax	(de)-
	skgt	a,(d)-
	skgt	a,(de)-
	gtax	h-		; 70 AF
	gtax	hl-
	gtax	(h)-
	gtax	(hl)-
	skgt	a,(h)-
	skgt	a,(hl)-

;--------------------------------------------
; LTAX rpa -> SKLT a,(rpa) (all cores)

	ltax	b		; 70 B9
	ltax	bc
	ltax	(b)
	ltax	(bc)
	sklt	a,(b)
	sklt	a,(bc)
	ltax	d		; 70 BA
	ltax	de
	ltax	(d)
	ltax	(de)
	sklt	a,(d)
	sklt	a,(de)
	ltax	h		; 70 BB
	ltax	hl
	ltax	(h)
	ltax	(hl)
	sklt	a,(h)
	sklt	a,(hl)
	ltax	d+		; 70 BC
	ltax	de+
	ltax	(d)+
	ltax	(de)+
	sklt	a,(d)+
	sklt	a,(de)+
	ltax	h+		; 70 BD
	ltax	hl+
	ltax	(h)+
	ltax	(hl)+
	sklt	a,(h)+
	sklt	a,(hl)+
	ltax	d-		; 70 BE
	ltax	de-
	ltax	(d)-
	ltax	(de)-
	sklt	a,(d)-
	sklt	a,(de)-
	ltax	h-		; 70 BF
	ltax	hl-
	ltax	(h)-
	ltax	(hl)-
	sklt	a,(h)-
	sklt	a,(hl)-

;--------------------------------------------
; NEAX rpa -> SKNE a,(rpa) (all cores)

	neax	b		; 70 E9
	neax	bc
	neax	(b)
	neax	(bc)
	skne	a,(b)
	skne	a,(bc)
	neax	d		; 70 EA
	neax	de
	neax	(d)
	neax	(de)
	skne	a,(d)
	skne	a,(de)
	neax	h		; 70 EB
	neax	hl
	neax	(h)
	neax	(hl)
	skne	a,(h)
	skne	a,(hl)
	neax	d+		; 70 EC
	neax	de+
	neax	(d)+
	neax	(de)+
	skne	a,(d)+
	skne	a,(de)+
	neax	h+		; 70 ED
	neax	hl+
	neax	(h)+
	neax	(hl)+
	skne	a,(h)+
	skne	a,(hl)+
	neax	d-		; 70 EE
	neax	de-
	neax	(d)-
	neax	(de)-
	skne	a,(d)-
	skne	a,(de)-
	neax	h-		; 70 EF
	neax	hl-
	neax	(h)-
	neax	(hl)-
	skne	a,(h)-
	skne	a,(hl)-

;--------------------------------------------
; EQAX rpa -> SKEQ a,(rpa) (all cores)

	eqax	b		; 70 F9
	eqax	bc
	eqax	(b)
	eqax	(bc)
	skeq	a,(b)
	skeq	a,(bc)
	eqax	d		; 70 FA
	eqax	de
	eqax	(d)
	eqax	(de)
	skeq	a,(d)
	skeq	a,(de)
	eqax	h		; 70 FB
	eqax	hl
	eqax	(h)
	eqax	(hl)
	skeq	a,(h)
	skeq	a,(hl)
	eqax	d+		; 70 FC
	eqax	de+
	eqax	(d)+
	eqax	(de)+
	skeq	a,(d)+
	skeq	a,(de)+
	eqax	h+		; 70 FD
	eqax	hl+
	eqax	(h)+
	eqax	(hl)+
	skeq	a,(h)+
	skeq	a,(hl)+
	eqax	d-		; 70 FE
	eqax	de-
	eqax	(d)-
	eqax	(de)-
	skeq	a,(d)-
	skeq	a,(de)-
	eqax	h-		; 70 FF
	eqax	hl-
	eqax	(h)-
	eqax	(hl)-
	skeq	a,(h)-
	skeq	a,(hl)-

;--------------------------------------------
; ONAX rpa -> SKON a,(rpa) (all cores)

	onax	b		; 70 C9
	onax	bc
	onax	(b)
	onax	(bc)
	skon	a,(b)
	skon	a,(bc)
	onax	d		; 70 CA
	onax	de
	onax	(d)
	onax	(de)
	skon	a,(d)
	skon	a,(de)
	onax	h		; 70 CB
	onax	hl
	onax	(h)
	onax	(hl)
	skon	a,(h)
	skon	a,(hl)
	onax	d+		; 70 CC
	onax	de+
	onax	(d)+
	onax	(de)+
	skon	a,(d)+
	skon	a,(de)+
	onax	h+		; 70 CD
	onax	hl+
	onax	(h)+
	onax	(hl)+
	skon	a,(h)+
	skon	a,(hl)+
	onax	d-		; 70 CE
	onax	de-
	onax	(d)-
	onax	(de)-
	skon	a,(d)-
	skon	a,(de)-
	onax	h-		; 70 CF
	onax	hl-
	onax	(h)-
	onax	(hl)-
	skon	a,(h)-
	skon	a,(hl)-

;--------------------------------------------
; OFFAX rpa -> SKOFF a,(rpa) (all cores)

	offax	b		; 70 D9
	offax	bc
	offax	(b)
	offax	(bc)
	skoff	a,(b)
	skoff	a,(bc)
	offax	d		; 70 DA
	offax	de
	offax	(d)
	offax	(de)
	skoff	a,(d)
	skoff	a,(de)
	offax	h		; 70 DB
	offax	hl
	offax	(h)
	offax	(hl)
	skoff	a,(h)
	skoff	a,(hl)
	offax	d+		; 70 DC
	offax	de+
	offax	(d)+
	offax	(de)+
	skoff	a,(d)+
	skoff	a,(de)+
	offax	h+		; 70 DD
	offax	hl+
	offax	(h)+
	offax	(hl)+
	skoff	a,(h)+
	skoff	a,(hl)+
	offax	d-		; 70 DE
	offax	de-
	offax	(d)-
	offax	(de)-
	skoff	a,(d)-
	skoff	a,(de)-
	offax	h-		; 70 DF
	offax	hl-
	offax	(h)-
	offax	(hl)-
	skoff	a,(h)-
	skoff	a,(hl)-

;--------------------------------------------
; ADI A,byte -> ADD a,byte (all cores)

	adi	a,12h		; 46 12
	add	a,12h

;--------------------------------------------
; ADI r,byte -> ADD r,byte (Hi/07/10)

	adi	v,12h		; 74 40 12
	add	v,12h
	adi	>a,12h		; 74 41 12
	add	>a,12h
	adi	b,12h		; 74 42 12
	add	b,12h
	adi	c,12h		; 74 43 12
	add	c,12h
	adi	d,12h		; 74 44 12
	add	d,12h
	adi	e,12h		; 74 45 12
	add	e,12h
	adi	h,12h		; 74 46 12
	add	h,12h
	adi	l,12h		; 74 47 12
	add	l,12h

;--------------------------------------------
; ADI sr2,byte -> ADD sr2,byte (Hi/07/10)

	adi	pa,12h		; 64 40 12
	add	pa,12h
	adi	pb,12h		; 64 41 12
	add	pb,12h
	adi	pc,12h		; 64 42 12
	add	pc,12h
	adi	pd,12h		; 64 43 12
	add	pd,12h
	adi	pf,12h		; 64 45 12
	add	pf,12h
	adi	mkh,12h		; 64 46 12
	add	mkh,12h
	adi	mkl,12h		; 64 47 12
	add	mkl,12h
	adi	anm,12h		; 64 C0 12
	add	anm,12h
	adi	smh,12h		; 64 C1 12
	add	smh,12h
	adi	eom,12h		; 64 C3 12
	add	eom,12h
	adi	tmm,12h		; 64 C5 12
	add	tmm,12h

;--------------------------------------------
; ACI A,byte -> ADC a,byte (all cores)

	aci	a,12h		; 56 12
	adc	a,12h

;--------------------------------------------
; ACI r,byte -> ADC r,byte (Hi/07/10)

	aci	v,12h		; 74 50 12
	adc	v,12h
	aci	>a,12h		; 74 51 12
	adc	>a,12h
	aci	b,12h		; 74 52 12
	adc	b,12h
	aci	c,12h		; 74 53 12
	adc	c,12h
	aci	d,12h		; 74 54 12
	adc	d,12h
	aci	e,12h		; 74 55 12
	adc	e,12h
	aci	h,12h		; 74 56 12
	adc	h,12h
	aci	l,12h		; 74 57 12
	adc	l,12h

;--------------------------------------------
; ACI sr2,byte -> ADC sr2,byte (Hi/07/10)

	aci	pa,12h		; 64 50 12
	adc	pa,12h
	aci	pb,12h		; 64 51 12
	adc	pb,12h
	aci	pc,12h		; 64 52 12
	adc	pc,12h
	aci	pd,12h		; 64 53 12
	adc	pd,12h
	aci	pf,12h		; 64 55 12
	adc	pf,12h
	aci	mkh,12h		; 64 56 12
	adc	mkh,12h
	aci	mkl,12h		; 64 57 12
	adc	mkl,12h
	aci	anm,12h		; 64 D0 12
	adc	anm,12h
	aci	smh,12h		; 64 D1 12
	adc	smh,12h
	aci	eom,12h		; 64 D3 12
	adc	eom,12h
	aci	tmm,12h		; 64 D5 12
	adc	tmm,12h

;--------------------------------------------
; ADINC A,byte -> ADDNC a,byte (all cores)

	adinc	a,12h		; 26 12
	addnc	a,12h

;--------------------------------------------
; ADINC r,byte -> ADDNC r,byte (Hi/07/10)

	adinc	v,12h		; 74 20 12
	addnc	v,12h
	adinc	>a,12h		; 74 21 12
	addnc	>a,12h
	adinc	b,12h		; 74 22 12
	addnc	b,12h
	adinc	c,12h		; 74 23 12
	addnc	c,12h
	adinc	d,12h		; 74 24 12
	addnc	d,12h
	adinc	e,12h		; 74 25 12
	addnc	e,12h
	adinc	h,12h		; 74 26 12
	addnc	h,12h
	adinc	l,12h		; 74 27 12
	addnc	l,12h

;--------------------------------------------
; ADINC sr2,byte -> ADDNC sr2,byte (Hi/07/10)

	adinc	pa,12h		; 64 20 12
	addnc	pa,12h
	adinc	pb,12h		; 64 21 12
	addnc	pb,12h
	adinc	pc,12h		; 64 22 12
	addnc	pc,12h
	adinc	pd,12h		; 64 23 12
	addnc	pd,12h
	adinc	pf,12h		; 64 25 12
	addnc	pf,12h
	adinc	mkh,12h		; 64 26 12
	addnc	mkh,12h
	adinc	mkl,12h		; 64 27 12
	addnc	mkl,12h
	adinc	anm,12h		; 64 A0 12
	addnc	anm,12h
	adinc	smh,12h		; 64 A1 12
	addnc	smh,12h
	adinc	eom,12h		; 64 A3 12
	addnc	eom,12h
	adinc	tmm,12h		; 64 A5 12
	addnc	tmm,12h

;--------------------------------------------
; SUI A,byte -> SUB a,byte (all cores)

	sui	a,12h		; 66 12
	sub	a,12h

;--------------------------------------------
; SUI r,byte -> SUB r,byte (Hi/07/10)

	sui	v,12h		; 74 60 12
	sub	v,12h
	sui	>a,12h		; 74 61 12
	sub	>a,12h
	sui	b,12h		; 74 62 12
	sub	b,12h
	sui	c,12h		; 74 63 12
	sub	c,12h
	sui	d,12h		; 74 64 12
	sub	d,12h
	sui	e,12h		; 74 65 12
	sub	e,12h
	sui	h,12h		; 74 66 12
	sub	h,12h
	sui	l,12h		; 74 67 12
	sub	l,12h

;--------------------------------------------
; SUI sr2,byte -> SUB sr2,byte (Hi/07/10)

	sui	pa,12h		; 64 60 12
	sub	pa,12h
	sui	pb,12h		; 64 61 12
	sub	pb,12h
	sui	pc,12h		; 64 62 12
	sub	pc,12h
	sui	pd,12h		; 64 63 12
	sub	pd,12h
	sui	pf,12h		; 64 65 12
	sub	pf,12h
	sui	mkh,12h		; 64 66 12
	sub	mkh,12h
	sui	mkl,12h		; 64 67 12
	sub	mkl,12h
	sui	anm,12h		; 64 E0 12
	sub	anm,12h
	sui	smh,12h		; 64 E1 12
	sub	smh,12h
	sui	eom,12h		; 64 E3 12
	sub	eom,12h
	sui	tmm,12h		; 64 E5 12
	sub	tmm,12h

;--------------------------------------------
; SBI A,byte -> SBB a,byte (all cores)

	sbi	a,12h		; 76 12
	sbb	a,12h

;--------------------------------------------
; SBI r,byte -> SBB r,byte (Hi/07/10)

	sbi	v,12h		; 74 70 12
	sbb	v,12h
	sbi	>a,12h		; 74 71 12
	sbb	>a,12h
	sbi	b,12h		; 74 72 12
	sbb	b,12h
	sbi	c,12h		; 74 73 12
	sbb	c,12h
	sbi	d,12h		; 74 74 12
	sbb	d,12h
	sbi	e,12h		; 74 75 12
	sbb	e,12h
	sbi	h,12h		; 74 76 12
	sbb	h,12h
	sbi	l,12h		; 74 77 12
	sbb	l,12h

;--------------------------------------------
; SBI sr2,byte -> SBB sr2,byte (Hi/07/10)

	sbi	pa,12h		; 64 70 12
	sbb	pa,12h
	sbi	pb,12h		; 64 71 12
	sbb	pb,12h
	sbi	pc,12h		; 64 72 12
	sbb	pc,12h
	sbi	pd,12h		; 64 73 12
	sbb	pd,12h
	sbi	pf,12h		; 64 75 12
	sbb	pf,12h
	sbi	mkh,12h		; 64 76 12
	sbb	mkh,12h
	sbi	mkl,12h		; 64 77 12
	sbb	mkl,12h
	sbi	anm,12h		; 64 F0 12
	sbb	anm,12h
	sbi	smh,12h		; 64 F1 12
	sbb	smh,12h
	sbi	eom,12h		; 64 F3 12
	sbb	eom,12h
	sbi	tmm,12h		; 64 F5 12
	sbb	tmm,12h

;--------------------------------------------
; SUINB A,byte -> SUBNB a,byte (all cores)

	suinb	a,12h		; 36 12
	subnb	a,12h

;--------------------------------------------
; SUINB r,byte -> SUBNB r,byte (Hi/07/10)

	suinb	v,12h		; 74 30 12
	subnb	v,12h
	suinb	>a,12h		; 74 31 12
	subnb	>a,12h
	suinb	b,12h		; 74 32 12
	subnb	b,12h
	suinb	c,12h		; 74 33 12
	subnb	c,12h
	suinb	d,12h		; 74 34 12
	subnb	d,12h
	suinb	e,12h		; 74 35 12
	subnb	e,12h
	suinb	h,12h		; 74 36 12
	subnb	h,12h
	suinb	l,12h		; 74 37 12
	subnb	l,12h

;--------------------------------------------
; SUINB sr2,byte -> SUBNB sr2,byte (Hi/07/10)

	suinb	pa,12h		; 64 30 12
	subnb	pa,12h
	suinb	pb,12h		; 64 31 12
	subnb	pb,12h
	suinb	pc,12h		; 64 32 12
	subnb	pc,12h
	suinb	pd,12h		; 64 33 12
	subnb	pd,12h
	suinb	pf,12h		; 64 35 12
	subnb	pf,12h
	suinb	mkh,12h		; 64 36 12
	subnb	mkh,12h
	suinb	mkl,12h		; 64 37 12
	subnb	mkl,12h
	suinb	anm,12h		; 64 B0 12
	subnb	anm,12h
	suinb	smh,12h		; 64 B1 12
	subnb	smh,12h
	suinb	eom,12h		; 64 B3 12
	subnb	eom,12h
	suinb	tmm,12h		; 64 B5 12
	subnb	tmm,12h

;--------------------------------------------
; ANI A,byte -> AND a,byte (all cores)

	ani	a,12h		; 07 12
	and	a,12h

;--------------------------------------------
; ANI r,byte -> AND r,byte (Hi/07/10)

	ani	v,12h		; 74 08 12
	and	v,12h
	ani	>a,12h		; 74 09 12
	and	>a,12h
	ani	b,12h		; 74 0A 12
	and	b,12h
	ani	c,12h		; 74 0B 12
	and	c,12h
	ani	d,12h		; 74 0C 12
	and	d,12h
	ani	e,12h		; 74 0D 12
	and	e,12h
	ani	h,12h		; 74 0E 12
	and	h,12h
	ani	l,12h		; 74 0F 12
	and	l,12h

;--------------------------------------------
; ANI sr2,byte -> AND sr2,byte (all cores)

	ani	pa,12h		; 64 08 12 or 64 88 12
	and	pa,12h
	ani	pb,12h		; 64 09 12 or 64 89 12
	and	pb,12h
	ani	pc,12h		; 64 0A 12 or 64 8A 12
	and	pc,12h
	ani	pd,12h		; 64 0B 12
	and	pd,12h
	ani	pf,12h		; 64 0D 12
	and	pf,12h
	ani	mkh,12h		; 64 0E 12
	and	mkh,12h
	ani	mkl,12h		; 64 0F 12
	and	mkl,12h
	ani	anm,12h		; 64 88 12
	and	anm,12h
	ani	smh,12h		; 64 89 12
	and	smh,12h
	ani	eom,12h		; 64 8B 12
	and	eom,12h
	ani	tmm,12h		; 64 8D 12
	and	tmm,12h

;--------------------------------------------
; ORI A,byte -> OR a,byte (all cores)

	ori	a,12h		; 17 12
	or	a,12h

;--------------------------------------------
; ORI r,byte -> OR r,byte (Hi/07/10)

	ori	v,12h		; 74 18 12
	or	v,12h
	ori	>a,12h		; 74 19 12
	or	>a,12h
	ori	b,12h		; 74 1A 12
	or	b,12h
	ori	c,12h		; 74 1B 12
	or	c,12h
	ori	d,12h		; 74 1C 12
	or	d,12h
	ori	e,12h		; 74 1D 12
	or	e,12h
	ori	h,12h		; 74 1E 12
	or	h,12h
	ori	l,12h		; 74 1F 12
	or	l,12h

;--------------------------------------------
; ORI sr2,byte -> OR sr2,byte (all cores)

	ori	pa,12h		; 64 18 12 or 64 98 12
	or	pa,12h
	ori	pb,12h		; 64 19 12 or 64 99 12
	or	pb,12h
	ori	pc,12h		; 64 1A 12 or 64 9A 12
	or	pc,12h
	ori	pd,12h		; 64 1B 12
	or	pd,12h
	ori	pf,12h		; 64 1D 12
	or	pf,12h
	ori	mkh,12h		; 64 1E 12
	or	mkh,12h
	ori	mkl,12h		; 64 1F 12
	or	mkl,12h
	ori	anm,12h		; 64 98 12
	or	anm,12h
	ori	smh,12h		; 64 99 12
	or	smh,12h
	ori	eom,12h		; 64 9B 12
	or	eom,12h
	ori	tmm,12h		; 64 9D 12
	or	tmm,12h

;--------------------------------------------
; XRI A,byte -> XOR a,byte (Hi/07/10)

	xri	a,12h		; 17 12
	xor	a,12h

;--------------------------------------------
; XRI r,byte -> XOR r,byte (Hi/07/10)

	xri	v,12h		; 74 10 12
	xor	v,12h
	xri	>a,12h		; 74 11 12
	xor	>a,12h
	xri	b,12h		; 74 12 12
	xor	b,12h
	xri	c,12h		; 74 13 12
	xor	c,12h
	xri	d,12h		; 74 14 12
	xor	d,12h
	xri	e,12h		; 74 15 12
	xor	e,12h
	xri	h,12h		; 74 16 12
	xor	h,12h
	xri	l,12h		; 74 17 12
	xor	l,12h

;--------------------------------------------
; XRI sr2,byte -> XOR sr2,byte (Hi/07/10)

	xri	pa,12h		; 64 10 12
	xor	pa,12h
	xri	pb,12h		; 64 11 12
	xor	pb,12h
	xri	pc,12h		; 64 12 12
	xor	pc,12h
	xri	pd,12h		; 64 13 12
	xor	pd,12h
	xri	pf,12h		; 64 15 12
	xor	pf,12h
	xri	mkh,12h		; 64 16 12
	xor	mkh,12h
	xri	mkl,12h		; 64 17 12
	xor	mkl,12h
	xri	anm,12h		; 64 90 12
	xor	anm,12h
	xri	smh,12h		; 64 91 12
	xor	smh,12h
	xri	eom,12h		; 64 93 12
	xor	eom,12h
	xri	tmm,12h		; 64 95 12
	xor	tmm,12h

;--------------------------------------------
; GTI A,byte -> SKGT a,byte (all cores)

	gti	a,12h		; 27 12
	skgt	a,12h

;--------------------------------------------
; GTI r,byte -> SKGT r,byte (Hi/07/10)

	gti	v,12h		; 74 28 12
	skgt	v,12h
	gti	>a,12h		; 74 29 12
	skgt	>a,12h
	gti	b,12h		; 74 2A 12
	skgt	b,12h
	gti	c,12h		; 74 2B 12
	skgt	c,12h
	gti	d,12h		; 74 2C 12
	skgt	d,12h
	gti	e,12h		; 74 2D 12
	skgt	e,12h
	gti	h,12h		; 74 2E 12
	skgt	h,12h
	gti	l,12h		; 74 2F 12
	skgt	l,12h

;--------------------------------------------
; GTI sr2,byte -> SKGT sr2,byte (Hi/07/10)

	gti	pa,12h		; 64 28 12
	skgt	pa,12h
	gti	pb,12h		; 64 29 12
	skgt	pb,12h
	gti	pc,12h		; 64 2A 12
	skgt	pc,12h
	gti	pd,12h		; 64 2B 12
	skgt	pd,12h
	gti	pf,12h		; 64 2D 12
	skgt	pf,12h
	gti	mkh,12h		; 64 2E 12
	skgt	mkh,12h
	gti	mkl,12h		; 64 2F 12
	skgt	mkl,12h
	gti	anm,12h		; 64 A8 12
	skgt	anm,12h
	gti	smh,12h		; 64 A9 12
	skgt	smh,12h
	gti	eom,12h		; 64 AB 12
	skgt	eom,12h
	gti	tmm,12h		; 64 AD 12
	skgt	tmm,12h

;--------------------------------------------
; LTI A,byte -> SKLT a,byte (all cores)

	lti	a,12h		; 37 12
	sklt	a,12h

;--------------------------------------------
; LTI r,byte -> SKLT r,byte (Hi/07/10)

	lti	v,12h		; 74 38 12
	sklt	v,12h
	lti	>a,12h		; 74 39 12
	sklt	>a,12h
	lti	b,12h		; 74 3A 12
	sklt	b,12h
	lti	c,12h		; 74 3B 12
	sklt	c,12h
	lti	d,12h		; 74 3C 12
	sklt	d,12h
	lti	e,12h		; 74 3D 12
	sklt	e,12h
	lti	h,12h		; 74 3E 12
	sklt	h,12h
	lti	l,12h		; 74 3F 12
	sklt	l,12h

;--------------------------------------------
; LTI sr2,byte -> SKLT sr2,byte (Hi/07/10)

	lti	pa,12h		; 64 38 12
	sklt	pa,12h
	lti	pb,12h		; 64 39 12
	sklt	pb,12h
	lti	pc,12h		; 64 3A 12
	sklt	pc,12h
	lti	pd,12h		; 64 3B 12
	sklt	pd,12h
	lti	pf,12h		; 64 3D 12
	sklt	pf,12h
	lti	mkh,12h		; 64 3E 12
	sklt	mkh,12h
	lti	mkl,12h		; 64 3F 12
	sklt	mkl,12h
	lti	anm,12h		; 64 B8 12
	sklt	anm,12h
	lti	smh,12h		; 64 B9 12
	sklt	smh,12h
	lti	eom,12h		; 64 BB 12
	sklt	eom,12h
	lti	tmm,12h		; 64 BD 12
	sklt	tmm,12h

;--------------------------------------------
; NEI A,byte -> SKNE a,byte (all cores)

	nei	a,12h		; 67 12
	skne	a,12h

;--------------------------------------------
; NEI r,byte -> SKNE r,byte (Hi/07/10)

	nei	v,12h		; 74 68 12
	skne	v,12h
	nei	>a,12h		; 74 69 12
	skne	>a,12h
	nei	b,12h		; 74 6A 12
	skne	b,12h
	nei	c,12h		; 74 6B 12
	skne	c,12h
	nei	d,12h		; 74 6C 12
	skne	d,12h
	nei	e,12h		; 74 6D 12
	skne	e,12h
	nei	h,12h		; 74 6E 12
	skne	h,12h
	nei	l,12h		; 74 6F 12
	skne	l,12h

;--------------------------------------------
; NEI sr2,byte -> SKNE sr2,byte (Hi/07/10)

	nei	pa,12h		; 64 68 12
	skne	pa,12h
	nei	pb,12h		; 64 69 12
	skne	pb,12h
	nei	pc,12h		; 64 6A 12
	skne	pc,12h
	nei	pd,12h		; 64 6B 12
	skne	pd,12h
	nei	pf,12h		; 64 6D 12
	skne	pf,12h
	nei	mkh,12h		; 64 6E 12
	skne	mkh,12h
	nei	mkl,12h		; 64 6F 12
	skne	mkl,12h
	nei	anm,12h		; 64 E8 12
	skne	anm,12h
	nei	smh,12h		; 64 E9 12
	skne	smh,12h
	nei	eom,12h		; 64 EB 12
	skne	eom,12h
	nei	tmm,12h		; 64 ED 12
	skne	tmm,12h

;--------------------------------------------
; EQI A,byte -> SKEQ a,byte (all cores)

	eqi	a,12h		; 77 12
	skeq	a,12h

;--------------------------------------------
; EQI r,byte -> SKEQ r,byte (Hi/07/10)

	eqi	v,12h		; 74 78 12
	skeq	v,12h
	eqi	>a,12h		; 74 79 12
	skeq	>a,12h
	eqi	b,12h		; 74 7A 12
	skeq	b,12h
	eqi	c,12h		; 74 7B 12
	skeq	c,12h
	eqi	d,12h		; 74 7C 12
	skeq	d,12h
	eqi	e,12h		; 74 7D 12
	skeq	e,12h
	eqi	h,12h		; 74 7E 12
	skeq	h,12h
	eqi	l,12h		; 74 7F 12
	skeq	l,12h

;--------------------------------------------
; EQI sr2,byte -> SKEQ sr2,byte (Hi/07/10)

	eqi	pa,12h		; 64 78 12
	skeq	pa,12h
	eqi	pb,12h		; 64 79 12
	skeq	pb,12h
	eqi	pc,12h		; 64 7A 12
	skeq	pc,12h
	eqi	pd,12h		; 64 7B 12
	skeq	pd,12h
	eqi	pf,12h		; 64 7D 12
	skeq	pf,12h
	eqi	mkh,12h		; 64 7E 12
	skeq	mkh,12h
	eqi	mkl,12h		; 64 7F 12
	skeq	mkl,12h
	eqi	anm,12h		; 64 F8 12
	skeq	anm,12h
	eqi	smh,12h		; 64 F9 12
	skeq	smh,12h
	eqi	eom,12h		; 64 FB 12
	skeq	eom,12h
	eqi	tmm,12h		; 64 FD 12
	skeq	tmm,12h

;--------------------------------------------
; ONI A,byte -> SKON a,byte (all cores)

	oni	a,12h		; 47 12
	skon	a,12h

;--------------------------------------------
; ONI r,byte -> SKON r,byte (Hi/07/10)

	oni	v,12h		; 74 48 12 or 64 C8 12
	skon	v,12h
	oni	>a,12h		; 74 49 12 or 64 C9 12
	skon	>a,12h
	oni	b,12h		; 74 4A 12 or 64 CA 12
	skon	b,12h
	oni	c,12h		; 74 4B 12
	skon	c,12h
	oni	d,12h		; 74 4C 12
	skon	d,12h
	oni	e,12h		; 74 4D 12
	skon	e,12h
	oni	h,12h		; 74 4E 12
	skon	h,12h
	oni	l,12h		; 74 4F 12
	skon	l,12h

;--------------------------------------------
; ONI sr2,byte -> SKON sr2,byte (Hi/07/10)

	oni	pa,12h		; 64 48 12
	skon	pa,12h
	oni	pb,12h		; 64 49 12
	skon	pb,12h
	oni	pc,12h		; 64 4A 12
	skon	pc,12h
	oni	pd,12h		; 64 4B 12
	skon	pd,12h
	oni	pf,12h		; 64 4D 12
	skon	pf,12h
	oni	mkh,12h		; 64 4E 12
	skon	mkh,12h
	oni	mkl,12h		; 64 4F 12
	skon	mkl,12h
	oni	anm,12h		; 64 C8 12
	skon	anm,12h
	oni	smh,12h		; 64 C9 12
	skon	smh,12h
	oni	eom,12h		; 64 CB 12
	skon	eom,12h
	oni	tmm,12h		; 64 CD 12
	skon	tmm,12h

;--------------------------------------------
; OFFI A,byte -> SKOFF a,byte (all cores)

	offi	a,12h		; 57 12
	skoff	a,12h

;--------------------------------------------
; OFFI r,byte -> SKOFF r,byte (Hi/07/10)

	offi	v,12h		; 74 58 12
	skoff	v,12h
	offi	>a,12h		; 74 59 12
	skoff	>a,12h
	offi	b,12h		; 74 5A 12
	skoff	b,12h
	offi	c,12h		; 74 5B 12
	skoff	c,12h
	offi	d,12h		; 74 5C 12
	skoff	d,12h
	offi	e,12h		; 74 5D 12
	skoff	e,12h
	offi	h,12h		; 74 5E 12
	skoff	h,12h
	offi	l,12h		; 74 5F 12
	skoff	l,12h

;--------------------------------------------
; OFFI sr2,byte -> SKOFF sr2,byte (Hi/07/10)

	offi	pa,12h		; 64 58 12 or 64 D8 12
	skoff	pa,12h
	offi	pb,12h		; 64 59 12 or 64 D9 12
	skoff	pb,12h
	offi	pc,12h		; 64 5A 12 or 64 DA 12
	skoff	pc,12h
	offi	pd,12h		; 64 5B 12
	skoff	pd,12h
	offi	pf,12h		; 64 5D 12
	skoff	pf,12h
	offi	mkh,12h		; 64 5E 12
	skoff	mkh,12h
	offi	mkl,12h		; 64 5F 12
	skoff	mkl,12h
	offi	anm,12h		; 64 D8 12
	skoff	anm,12h
	offi	smh,12h		; 64 D9 12
	skoff	smh,12h
	offi	eom,12h		; 64 DB 12
	skoff	eom,12h
	offi	tmm,12h		; 64 DD 12
	skoff	tmm,12h

;--------------------------------------------
; ADDW wa -> ADD A,(wa) (Hi/07/10)

	addw	0ff12h		; 74 C0 12
	add	a,(0ff12h)

;--------------------------------------------
; ADCW wa -> ADC A,(wa) (Hi/07/10)

	adcw	0ff12h		; 74 D0 12
	adc	a,(0ff12h)

;--------------------------------------------
; ADDNCW wa -> ADDNC A,(wa) (Hi/07/10)

	addncw	0ff12h		; 74 A0 12
	addnc	a,(0ff12h)

;--------------------------------------------
; SUBW wa -> SUB A,(wa) (Hi/07/10)

	subw	0ff12h		; 74 E0 12
	sub	a,(0ff12h)

;--------------------------------------------
; SBBW wa -> SBB A,(wa) (Hi/07/10)

	sbbw	0ff12h		; 74 F0 12
	sbb	a,(0ff12h)

;--------------------------------------------
; SUBNBW wa -> SUBNB A,(wa) (Hi/07/10)

	subnbw	0ff12h		; 74 B0 12
	subnb	a,(0ff12h)

;--------------------------------------------
; ANAW wa -> AND A,(wa) (Hi/07/10)

	anaw	0ff12h		; 74 88 12
	and	a,(0ff12h)

;--------------------------------------------
; ORAW wa -> OR A,(wa) (Hi/07/10)

	oraw	0ff12h		; 74 98 12
	or	a,(0ff12h)

;--------------------------------------------
; XRAW wa -> XOR A,(wa) (Hi/07/10)

	xraw	0ff12h		; 74 90 12
	xor	a,(0ff12h)

;--------------------------------------------
; GTAW wa -> SKGT A,(wa) (Hi/07/10)

	gtaw	0ff12h		; 74 A8 12
	skgt	a,(0ff12h)

;--------------------------------------------
; LTAW wa -> SKLT A,(wa) (Hi/07/10)

	ltaw	0ff12h		; 74 B8 12
	sklt	a,(0ff12h)

;--------------------------------------------
; NEAW wa -> SKNE A,(wa) (Hi/07/10)

	neaw	0ff12h		; 74 E8 12
	skne	a,(0ff12h)

;--------------------------------------------
; EQAW wa -> SKEQ A,(wa) (Hi/07/10)

	eqaw	0ff12h		; 74 F8 12
	skeq	a,(0ff12h)

;--------------------------------------------
; ONAW wa -> SKON A,(wa) (Hi/07/10)

	onaw	0ff12h		; 74 C8 12
	skon	a,(0ff12h)

;--------------------------------------------
; OFFAW wa -> SKOFF A,(wa) (Hi/07/10)

	offaw	0ff12h		; 74 D8 12
	skoff	a,(0ff12h)

;--------------------------------------------
; ADIW wa,byte -> ADD (wa),byte (does not exist)

	if	0
	adiw	0ff12h,34h	; XX XX XX
	add	(0ff12h),34h
	endif

;--------------------------------------------
; ACIW wa,byte -> ADC (wa),byte (does not exist)

	if	0
	aciw	0ff12h,34h	; XX XX XX
	adc	(0ff12h),34h
	endif

;--------------------------------------------
; ADINCW wa,byte -> ADDNC (wa),byte (does not exist)

	if	0
	adincw	0ff12h,34h	; XX XX XX
	addnc	(0ff12h),34h
	endif

;--------------------------------------------
; SUIW wa,byte -> SUB (wa),byte (does not exist)

	if	0
	suiw	0ff12h,34h	; XX XX XX
	sub	(0ff12h),34h
	endif

;--------------------------------------------
; SBIW wa,byte -> SBB (wa),byte (does not exist)

	if	0
	sbiw	0ff12h,34h	; XX XX XX
	sbb	(0ff12h),34h
	endif

;--------------------------------------------
; SUINBW wa,byte -> SUBNB (wa),byte (does not exist)

	if	0
	suinbw	0ff12h,34h	; XX XX XX
	subnb	(0ff12h),34h
	endif

;--------------------------------------------
; ANIW wa,byte -> AND (wa),byte (all cores)

	aniw	0ff12h,34h	; 05 12 34
	and	(0ff12h),34h

;--------------------------------------------
; ORIW wa,byte -> OR (wa),byte (all cores)

	oriw	0ff12h,34h	; 15 12 34
	or	(0ff12h),34h

;--------------------------------------------
; XRIW wa,byte -> XOR (wa),byte (does not exist)

	if	0
	xriw	0ff12h,34h	; XX XX XX
	xor	(0ff12h),34h
	endif

;--------------------------------------------
; GTIW wa,byte -> SKGT (wa),byte (all cores)

	gtiw	0ff12h,34h	; 25 12 34
	skgt	(0ff12h),34h

;--------------------------------------------
; LTIW wa,byte -> SKLT (wa),byte (all cores)

	ltiw	0ff12h,34h	; 35 12 34
	sklt	(0ff12h),34h

;--------------------------------------------
; NEIW wa,byte -> SKNE (wa),byte (all cores)

	neiw	0ff12h,34h	; 65 12 34
	skne	(0ff12h),34h

;--------------------------------------------
; EQIW wa,byte -> SKEQ (wa),byte (all cores)

	eqiw	0ff12h,34h	; 75 12 34
	skeq	(0ff12h),34h

;--------------------------------------------
; ONIW wa,byte -> SKON (wa),byte (all cores)

	oniw	0ff12h,34h	; 45 12 34
	skon	(0ff12h),34h

;--------------------------------------------
; OFFIW wa,byte -> SKOFF (wa),byte (all cores)

	offiw	0ff12h,34h	; 55 12 34
	skoff	(0ff12h),34h

;--------------------------------------------
; EADD EA,r2 -> ADD EA,r2 (07/10)

	eadd	ea,a		; 70 41
	add	ea,a
	eadd	ea,b		; 70 42
	add	ea,b
	eadd	ea,c		; 70 43
	add	ea,c

;--------------------------------------------
; DADD EA,rp3 -> ADD EA,rp3 (07/10)

	dadd	ea,b		; 74 C5
	dadd	ea,bc
	add	ea,bc
	dadd	ea,d		; 74 C6
	dadd	ea,de
	add	ea,de
	dadd	ea,h		; 74 C7
	dadd	ea,hl
	add	ea,hl

;--------------------------------------------
; EADC EA,r2 -> ADC EA,r2 (does not exist)

	if	0
	eadc	ea,a		; XX XX
	adc	ea,a
	eadc	ea,b		; XX XX
	adc	ea,b
	eadc	ea,c		; XX XX
	adc	ea,c
	endif

;--------------------------------------------
; DADC EA,rp3 -> ADC EA,rp3 (07/10)

	dadc	ea,b		; 74 D5
	dadc	ea,bc
	adc	ea,bc
	dadc	ea,d		; 74 D6
	dadc	ea,de
	adc	ea,de
	dadc	ea,h		; 74 D7
	dadc	ea,hl
	adc	ea,hl

;--------------------------------------------
; EADDNC EA,r2 -> ADDNC EA,r2 (does not exist)

	if	0
	eaddnc	ea,a		; XX XX
	addnc	ea,a
	eaddnc	ea,b		; XX XX
	addnc	ea,b
	eaddnc	ea,c		; XX XX
	addnc	ea,c
	endif

;--------------------------------------------
; DADDNC EA,rp3 -> ADDNC EA,rp3 (07/10)

	daddnc	ea,b		; 74 A5
	daddnc	ea,bc
	addnc	ea,bc
	daddnc	ea,d		; 74 A6
	daddnc	ea,de
	addnc	ea,de
	daddnc	ea,h		; 74 A7
	daddnc	ea,hl
	addnc	ea,hl

;--------------------------------------------
; ESUB EA,r2 -> SUB EA,r2 (07/10)

	esub	ea,a		; 70 61
	sub	ea,a
	esub	ea,b		; 70 62
	sub	ea,b
	esub	ea,c		; 70 63
	sub	ea,c

;--------------------------------------------
; DSUB EA,rp3 -> SUB EA,rp3 (07/10)

	dsub	ea,b		; 74 E5
	dsub	ea,bc
	sub	ea,bc
	dsub	ea,d		; 74 E6
	dsub	ea,de
	sub	ea,de
	dsub	ea,h		; 74 E7
	dsub	ea,hl
	sub	ea,hl

;--------------------------------------------
; ESBB EA,r2 -> SBB EA,r2 (does not exist)

	if 0
	esbb	ea,a		; XX XX
	sbb	ea,a
	esbb	ea,b		; XX XX
	sbb	ea,b
	esbb	ea,c		; XX XX
	sbb	ea,c
	endif

;--------------------------------------------
; DSBB EA,rp3 -> SBB EA,rp3 (07/10)

	dsbb	ea,b		; 74 F5
	dsbb	ea,bc
	sbb	ea,bc
	dsbb	ea,d		; 74 F6
	dsbb	ea,de
	sbb	ea,de
	dsbb	ea,h		; 74 F7
	dsbb	ea,hl
	sbb	ea,hl

;--------------------------------------------
; ESUBNB EA,r2 -> SUBNB EA,r2 (does not exist)

	if 0
	esubnb	ea,a		; XX XX
	subnb	ea,a
	esubnb	ea,b		; XX XX
	subnb	ea,b
	esubnb	ea,c		; XX XX
	subnb	ea,c
	endif

;--------------------------------------------
; DSUBNB EA,rp3 -> SUBNB EA,rp3 (07/10)

	dsubnb	ea,b		; 74 B5
	dsubnb	ea,bc
	subnb	ea,bc
	dsubnb	ea,d		; 74 B6
	dsubnb	ea,de
	subnb	ea,de
	dsubnb	ea,h		; 74 B7
	dsubnb	ea,hl
	subnb	ea,hl

;--------------------------------------------
; EAN EA,r2 -> AND EA,r2 (does not exist)

	if 0
	ean	ea,a		; XX XX
	and	ea,a
	ean	ea,b		; XX XX
	and	ea,b
	ean	ea,c		; XX XX
	and	ea,c
	endif

;--------------------------------------------
; DAN EA,rp3 -> AND EA,rp3 (07/10)

	dan	ea,b		; 74 8D
	dan	ea,bc
	and	ea,bc
	dan	ea,d		; 74 8E
	dan	ea,de
	and	ea,de
	dan	ea,h		; 74 8F
	dan	ea,hl
	and	ea,hl

;--------------------------------------------
; EOR EA,r2 -> OR EA,r2 (does not exist)

	if 0
	eor	ea,a		; XX XX
	or	ea,a
	eor	ea,b		; XX XX
	or	ea,b
	eor	ea,c		; XX XX
	or	ea,c
	endif

;--------------------------------------------
; DOR EA,rp3 -> OR EA,rp3 (07/10)

	dor	ea,b		; 74 9D
	dor	ea,bc
	or	ea,bc
	dor	ea,d		; 74 9E
	dor	ea,de
	or	ea,de
	dor	ea,h		; 74 9F
	dor	ea,hl
	or	ea,hl

;--------------------------------------------
; EXR EA,r2 -> XOR EA,r2 (does not exist)

	if 0
	exr	ea,a		; XX XX
	xor	ea,a
	exr	ea,b		; XX XX
	xor	ea,b
	exr	ea,c		; XX XX
	xor	ea,c
	endif

;--------------------------------------------
; DXR EA,rp3 -> XOR EA,rp3 (07/10)

	dxr	ea,b		; 74 95
	dxr	ea,bc
	xor	ea,bc
	dxr	ea,d		; 74 96
	dxr	ea,de
	xor	ea,de
	dxr	ea,h		; 74 97
	dxr	ea,hl
	xor	ea,hl

;--------------------------------------------
; EGT EA,r2 -> SKGT EA,r2 (does not exist)

	if 0
	egt	ea,a		; XX XX
	skgt	ea,a
	egt	ea,b		; XX XX
	skgt	ea,b
	egt	ea,c		; XX XX
	skgt	ea,c
	endif

;--------------------------------------------
; DGT EA,rp3 -> SKGT EA,rp3 (07/10)

	dgt	ea,b		; 74 AD
	dgt	ea,bc
	skgt	ea,bc
	dgt	ea,d		; 74 AE
	dgt	ea,de
	skgt	ea,de
	dgt	ea,h		; 74 AF
	dgt	ea,hl
	skgt	ea,hl

;--------------------------------------------
; ELT EA,r2 -> SKLT EA,r2 (does not exist)

	if 0
	elt	ea,a		; XX XX
	sklt	ea,a
	elt	ea,b		; XX XX
	sklt	ea,b
	elt	ea,c		; XX XX
	sklt	ea,c
	endif

;--------------------------------------------
; DLT EA,rp3 -> SKLT EA,rp3 (07/10)

	dlt	ea,b		; 74 BD
	dlt	ea,bc
	sklt	ea,bc
	dlt	ea,d		; 74 BE
	dlt	ea,de
	sklt	ea,de
	dlt	ea,h		; 74 BF
	dlt	ea,hl
	sklt	ea,hl

;--------------------------------------------
; ENE EA,r2 -> SKNE EA,r2 (does not exist)

	if 0
	ene	ea,a		; XX XX
	skne	ea,a
	ene	ea,b		; XX XX
	skne	ea,b
	ene	ea,c		; XX XX
	skne	ea,c
	endif

;--------------------------------------------
; DNE EA,rp3 -> SKNE EA,rp3 (07/10)

	dne	ea,b		; 74 ED
	dne	ea,bc
	skne	ea,bc
	dne	ea,d		; 74 EE
	dne	ea,de
	skne	ea,de
	dne	ea,h		; 74 EF
	dne	ea,hl
	skne	ea,hl

;--------------------------------------------
; EEQ EA,r2 -> SKEQ EA,r2 (does not exist)

	if 0
	eeq	ea,a		; XX XX
	skeq	ea,a
	eeq	ea,b		; XX XX
	skeq	ea,b
	eeq	ea,c		; XX XX
	skeq	ea,c
	endif

;--------------------------------------------
; DEQ EA,rp3 -> SKEQ EA,rp3 (07/10)

	deq	ea,b		; 74 FD
	deq	ea,bc
	skeq	ea,bc
	deq	ea,d		; 74 FE
	deq	ea,de
	skeq	ea,de
	deq	ea,h		; 74 FF
	deq	ea,hl
	skeq	ea,hl

;--------------------------------------------
; EON EA,r2 -> SKON EA,r2 (does not exist)

	if 0
	eon	ea,a		; XX XX
	skon	ea,a
	eon	ea,b		; XX XX
	skon	ea,b
	eon	ea,c		; XX XX
	skon	ea,c
	endif

;--------------------------------------------
; DON EA,rp3 -> SKON EA,rp3 (07/10)

	don	ea,b		; 74 CD
	don	ea,bc
	skon	ea,bc
	don	ea,d		; 74 CE
	don	ea,de
	skon	ea,de
	don	ea,h		; 74 CF
	don	ea,hl
	skon	ea,hl

;--------------------------------------------
; EOFF EA,r2 -> SKOFF EA,r2 (does not exist)

	if 0
	eoff	ea,a		; XX XX
	skoff	ea,a
	eoff	ea,b		; XX XX
	skoff	ea,b
	eoff	ea,c		; XX XX
	skoff	ea,c
	endif

;--------------------------------------------
; DOFF EA,rp3 -> SKOFF EA,rp3 (07/10)

	doff	ea,b		; 74 DD
	doff	ea,bc
	skoff	ea,bc
	doff	ea,d		; 74 DE
	doff	ea,de
	skoff	ea,de
	doff	ea,h		; 74 DF
	doff	ea,hl
	skoff	ea,hl

;--------------------------------------------
; INR r2 -> INC r2 (all cores)

	inr	a		; 41
	inc	a
	inr	b		; 42
	inc	b
	inr	c		; 43
	inc	c

;--------------------------------------------
; INRW wa -> INC (wa) (all cores)

	inrw	0ff12h		; 20 12
	inc	(0ff12h)	

;--------------------------------------------
; INX rp -> INC rp (all cores)

	inx	sp		; 02
	inc	sp
	inx	bc		; 12
	inc	bc
	inx	de		; 22
	inc	de
	inx	hl		; 32
	inc	hl

;--------------------------------------------
; INX EA -> INC EA (07/10)

	inx	ea		; A8
	inc	ea

;--------------------------------------------
; DCR r2 -> DEC r2 (all cores)

	dcr	a		; 51
	dec	a
	dcr	b		; 52
	dec	b
	dcr	c		; 53
	dec	c

;--------------------------------------------
; DCRW wa -> DEC (wa) (all cores)

	dcrw	0ff12h		; 30 12
	dec	(0ff12h)

;--------------------------------------------
; DCX rp -> DEC rp (all cores)

	dcx	sp		; 03
	dec	sp
	dcx	bc		; 13
	dec	bc
	dcx	de		; 23
	dec	de
	dcx	hl		; 33
	dec	hl

;--------------------------------------------
; DCX EA -> DEC EA (07/10)

	dcx	ea		; A9
	dec	ea
