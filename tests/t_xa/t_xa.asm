        cpu     xag3

        page	0

        include stddefxa

        segment io

port1   ds.b    1
port2   ds.b    1

        segment code

        supmode on

regbit1	bit	r3l.4
regbit2	bit	sp.5
regbit3	bit	r5h.5
regbit4	bit	sp.14
membit1	bit	22h.5
membit2	bit	50023h.5
iobit1	bit	port1.2
regbit5	bit	regbit4+1

        add.b   r4h,r1l
        add.w   r5,r3
        add.b   r5l,[r6]
        add.w   r4,[sp]
        add.b   [r6],r5l
        add.w   [sp],r4
        add.b   r3h,[r6+2]
        add.w   r4,[r3+100]
        add.b   [r6+2],r3h
        add.w   [r3+100],r4
        add.b   r3h,[r6+200]
        add.w   r4,[r3+1000]
        add.b   [r6+200],r3h
        add.w   [r3+1000],r4
        add.b   r4h,[r1+]
        add.w   r5,[r6+]
        add.b   [r1+],r4h
        add.w   [r6+],r5
        add.b   200h,r2l
        add.w   123h,r6
        add.b   r2l,200h
        add.w   r6,123h
        add.b   r5h,#34h
        add.w   r3,#1234h
        add.b   [r5],#34h
        add.w   [r3],#1234h
        add.b   [r5+],#34h
        add.w   [r3+],#1234h
        add.b   [r5+2],#34h
        add.w   [r3+100],#1234h
        add.b   [r5+200],#34h
        add.w   [r3+1000],#1234h
        add.b   200h,#34h
        add.w   123h,#1234h

        addc.b  r4h,r1l
        addc.w  r5,r3
        addc.b  r5l,[r6]
        addc.w  r4,[sp]
        addc.b  [r6],r5l
        addc.w  [sp],r4
        addc.b  r3h,[r6+2]
        addc.w  r4,[r3+100]
        addc.b  [r6+2],r3h
        addc.w  [r3+100],r4
        addc.b  r3h,[r6+200]
        addc.w  r4,[r3+1000]
        addc.b  [r6+200],r3h
        addc.w  [r3+1000],r4
        addc.b  r4h,[r1+]
        addc.w  r5,[r6+]
        addc.b  [r1+],r4h
        addc.w  [r6+],r5
        addc.b  200h,r2l
        addc.w  123h,r6
        addc.b  r2l,200h
        addc.w  r6,123h
        addc.b  r5h,#34h
        addc.w  r3,#1234h
        addc.b  [r5],#34h
        addc.w  [r3],#1234h
        addc.b  [r5+],#34h
        addc.w  [r3+],#1234h
        addc.b  [r5+2],#34h
        addc.w  [r3+100],#1234h
        addc.b  [r5+200],#34h
        addc.w  [r3+1000],#1234h
        addc.b  200h,#34h
        addc.w  123h,#1234h

        adds.b  r5h,#3
        adds.w  r6,#5
        adds.b  [r4],#3
        adds.w  [sp],#5
        adds.b  [r4+],#3
        adds.w  [sp+],#5
        adds.b  [r4+20],#3
        adds.w  [sp+20],#5
        adds.b  [r4-200],#3
        adds.w  [sp-200],#5
        adds.b  200h,#3
        adds.w  123h,#5

        and.b   r4h,r1l
        and.w   r5,r3
        and.b   r5l,[r6]
        and.w   r4,[sp]
        and.b   [r6],r5l
        and.w   [sp],r4
        and.b   r3h,[r6+2]
        and.w   r4,[r3+100]
        and.b   [r6+2],r3h
        and.w   [r3+100],r4
        and.b   r3h,[r6+200]
        and.w   r4,[r3+1000]
        and.b   [r6+200],r3h
        and.w   [r3+1000],r4
        and.b   r4h,[r1+]
        and.w   r5,[r6+]
        and.b   [r1+],r4h
        and.w   [r6+],r5
        and.b   200h,r2l
        and.w   123h,r6
        and.b   r2l,200h
        and.w   r6,123h
        and.b   r5h,#34h
        and.w   r3,#1234h
        and.b   [r5],#34h
        and.w   [r3],#1234h
        and.b   [r5+],#34h
        and.w   [r3+],#1234h
        and.b   [r5+2],#34h
        and.w   [r3+100],#1234h
        and.b   [r5+200],#34h
        and.w   [r3+1000],#1234h
        and.b   200h,#34h
        and.w   123h,#1234h

	anl	c,regbit1
        anl	c,/iobit1
        anl	c,r5.12
        anl	c,/r4h.1

        asl.b	r4h,r1l
        asl.w	r6,r3h
        asl.d	r2,r4l
        asl.b	r4h,#6
        asl.w	r6,#12
        asl.d	r2,#24

        asr.b	r4h,r1l
        asr.w	r6,r3h
        asr.d	r2,r4l
        asr.b	r4h,#6
        asr.w	r6,#12
        asr.d	r2,#24

	bcc	label1
        nop
        bcc	label2
        nop
        bcs	label1
        beq	label1
        bg	label1
        bge	label1
        bgt	label1
        ble	label1
        blt	label1
        bmi	label1
        bne	label1
        bnv	label1
        bov	label1
        bpl	label1
        br	label1

        call	label1
        call	label1
        call	[r4]

        cjne	r5l,123h,label1
        cjne	r6,456h,label1
        cjne	r5l,#34h,label1
        cjne	r6,#1234h,label1
        cjne.b	[r6],#34h,label1
	cjne.w	[r6],#1234h,label1

label1:	nop
label2:	nop

	clr     regbit1

        cmp.b   r4h,r1l
        cmp.w   r5,r3
        cmp.b   r5l,[r6]
        cmp.w   r4,[sp]
        cmp.b   [r6],r5l
        cmp.w   [sp],r4
        cmp.b   r3h,[r6+2]
        cmp.w   r4,[r3+100]
        cmp.b   [r6+2],r3h
        cmp.w   [r3+100],r4
        cmp.b   r3h,[r6+200]
        cmp.w   r4,[r3+1000]
        cmp.b   [r6+200],r3h
        cmp.w   [r3+1000],r4
        cmp.b   r4h,[r1+]
        cmp.w   r5,[r6+]
        cmp.b   [r1+],r4h
        cmp.w   [r6+],r5
        cmp.b   200h,r2l
        cmp.w   123h,r6
        cmp.b   r2l,200h
        cmp.w   r6,123h
        cmp.b   r5h,#34h
        cmp.w   r3,#1234h
        cmp.b   [r5],#34h
        cmp.w   [r3],#1234h
        cmp.b   [r5+],#34h
        cmp.w   [r3+],#1234h
        cmp.b   [r5+2],#34h
        cmp.w   [r3+100],#1234h
        cmp.b   [r5+200],#34h
        cmp.w   [r3+1000],#1234h
        cmp.b   200h,#34h
        cmp.w   123h,#1234h

	cpl	r4l
        cpl	sp

        da	r4l

        div.w	r4,r1h
        div.w	r5,#23
        div.d	r2,r5
        div.d	r6,#1234h

        divu.b	r4l,r5l
        divu.b	r4l,#23
        divu.w	r4,r1h
        divu.w	r5,#23
        divu.d	r2,r5
        divu.d	r6,#1234h

d1:     djnz	r5l,d1
d2:     djnz.b	123h,d2
d3:     djnz	r5,d3
d4:     djnz.w	123h,d4

	fcall	123456h

        fjmp	123456h

        jb	regbit1,d1
        jbc	regbit1,d2

        jmp	1234h
        jmp	[r3]
        jmp	[a+dptr]
        jmp	[[r5+]]

        jnb	regbit1,d3

        jnz	d3

        jz	d3

        lea	r5,r4+4
        lea	r6,r1+1000

        lsr.b	r4h,r1l
        lsr.w	r6,r3h
        lsr.d	r2,r4l
        lsr.b	r4h,#6
        lsr.w	r6,#12
        lsr.d	r2,#24

	mov	c,regbit1
        mov	regbit1,c
        mov	usp,r4
        mov	sp,usp
        mov.b   r4h,r1l
        mov.w   r5,r3
        mov.b   r5l,[r6]
        mov.w   r4,[sp]
        mov.b   [r6],r5l
        mov.w   [sp],r4
        mov.b   r3h,[r6+2]
        mov.w   r4,[r3+100]
        mov.b   [r6+2],r3h
        mov.w   [r3+100],r4
        mov.b   r3h,[r6+200]
        mov.w   r4,[r3+1000]
        mov.b   [r6+200],r3h
        mov.w   [r3+1000],r4
        mov.b   r4h,[r1+]
        mov.w   r5,[r6+]
        mov.b   [r1+],r4h
        mov.w   [r6+],r5
        mov.b	[r3+],[r4+]
        mov.w	[r3+],[r4+]
        mov.b   200h,r2l
        mov.w   123h,r6
        mov.b   r2l,200h
        mov.w   r6,123h
        mov.b	123h,[r5]
        mov.w	456h,[sp]
        mov.b	[r5],123h
        mov.w	[sp],456h
        mov.b   r5h,#34h
        mov.w   r3,#1234h
        mov.b   [r5],#34h
        mov.w   [r3],#1234h
        mov.b   [r5+],#34h
        mov.w   [r3+],#1234h
        mov.b   [r5+2],#34h
        mov.w   [r3+100],#1234h
        mov.b   [r5+200],#34h
        mov.w   [r3+1000],#1234h
        mov.b   200h,#34h
        mov.w   123h,#1234h
	mov.b	123h,200h
	mov.w	123h,200h

        movc	r4l,[r5+]
        movc	r4,[r5+]
        movc	a,[a+dptr]
        movc	a,[a+pc]

        movs.b  r5h,#3
        movs.w  r6,#5
        movs.b  [r4],#3
        movs.w  [sp],#5
        movs.b  [r4+],#3
        movs.w  [sp+],#5
        movs.b  [r4+20],#3
        movs.w  [sp+20],#5
        movs.b  [r4-200],#3
        movs.w  [sp-200],#5
        movs.b  200h,#3
        movs.w  123h,#5

        movx    r3l,[r6]
        movx    r3,[sp]
        movx    [r6],r3l
        movx    [sp],r3

        mul     r0,r5
        mul     r6,#1234h

        mulu    r3l,r4h
        mulu    r5l,#100
        mulu    r0,r5
        mulu    r6,#1234h

        neg     r4l
        neg     sp

        nop

        norm.b  r4h,r1l
        norm.w  r6,r3h
        norm.d  r2,r4l

        or.b    r4h,r1l
        or.w    r5,r3
        or.b    r5l,[r6]
        or.w    r4,[sp]
        or.b    [r6],r5l
        or.w    [sp],r4
        or.b    r3h,[r6+2]
        or.w    r4,[r3+100]
        or.b    [r6+2],r3h
        or.w    [r3+100],r4
        or.b    r3h,[r6+200]
        or.w    r4,[r3+1000]
        or.b    [r6+200],r3h
        or.w    [r3+1000],r4
        or.b    r4h,[r1+]
        or.w    r5,[r6+]
        or.b    [r1+],r4h
        or.w    [r6+],r5
        or.b    200h,r2l
        or.w    123h,r6
        or.b    r2l,200h
        or.w    r6,123h
        or.b    r5h,#34h
        or.w    r3,#1234h
        or.b    [r5],#34h
        or.w    [r3],#1234h
        or.b    [r5+],#34h
        or.w    [r3+],#1234h
        or.b    [r5+2],#34h
        or.w    [r3+100],#1234h
        or.b    [r5+200],#34h
        or.w    [r3+1000],#1234h
        or.b    200h,#34h
        or.w    123h,#1234h

        orl     c,regbit1
        orl     c,/iobit1
        orl     c,r5.12
        orl     c,/r4h.1

        pop.b   123h
        pop.w   200h
        pop     r2l
        pop     r2l,r3l
        pop     r4h
        pop     r4h,r5h
        pop     r2l,r3l,r4h,r5h
        pop     r1
        pop     r2,r5;,sp

        popu.b  123h
        popu.w  200h
        popu    r2l
        popu    r2l,r3l
        popu    r4h
        popu    r4h,r5h
        popu    r2l,r3l,r4h,r5h
        popu    r1
        popu    r2,r5,sp

        push.b  123h
        push.w  200h
        push    r2l
        push    r2l,r3l
        push    r4h
        push    r4h,r5h
        push    r2l,r3l,r4h,r5h
        push    r1
        push    r2,r5,sp

        pushu.b 123h
        pushu.w 200h
        pushu   r2l
        pushu   r2l,r3l
        pushu   r4h
        pushu   r4h,r5h
        pushu   r2l,r3l,r4h,r5h
        pushu   r1
        pushu   r2,r5,sp

        reset

        ret

        reti

        rl      r3h,#3
        rl      r5,#12

        rlc     r3h,#3
        rlc     r5,#12

        rr      r3h,#3
        rr      r5,#12

        rrc     r3h,#3
        rrc     r5,#12

        setb    regbit1

        sext    r1l
        sext    r2

        sub.b   r4h,r1l
        sub.w   r5,r3
        sub.b   r5l,[r6]
        sub.w   r4,[sp]
        sub.b   [r6],r5l
        sub.w   [sp],r4
        sub.b   r3h,[r6+2]
        sub.w   r4,[r3+100]
        sub.b   [r6+2],r3h
        sub.w   [r3+100],r4
        sub.b   r3h,[r6+200]
        sub.w   r4,[r3+1000]
        sub.b   [r6+200],r3h
        sub.w   [r3+1000],r4
        sub.b   r4h,[r1+]
        sub.w   r5,[r6+]
        sub.b   [r1+],r4h
        sub.w   [r6+],r5
        sub.b   200h,r2l
        sub.w   123h,r6
        sub.b   r2l,200h
        sub.w   r6,123h
        sub.b   r5h,#34h
        sub.w   r3,#1234h
        sub.b   [r5],#34h
        sub.w   [r3],#1234h
        sub.b   [r5+],#34h
        sub.w   [r3+],#1234h
        sub.b   [r5+2],#34h
        sub.w   [r3+100],#1234h
        sub.b   [r5+200],#34h
        sub.w   [r3+1000],#1234h
        sub.b   200h,#34h
        sub.w   123h,#1234h

        subb.b  r4h,r1l
        subb.w  r5,r3
        subb.b  r5l,[r6]
        subb.w  r4,[sp]
        subb.b  [r6],r5l
        subb.w  [sp],r4
        subb.b  r3h,[r6+2]
        subb.w  r4,[r3+100]
        subb.b  [r6+2],r3h
        subb.w  [r3+100],r4
        subb.b  r3h,[r6+200]
        subb.w  r4,[r3+1000]
        subb.b  [r6+200],r3h
        subb.w  [r3+1000],r4
        subb.b  r4h,[r1+]
        subb.w  r5,[r6+]
        subb.b  [r1+],r4h
        subb.w  [r6+],r5
        subb.b  200h,r2l
        subb.w  123h,r6
        subb.b  r2l,200h
        subb.w  r6,123h
        subb.b  r5h,#34h
        subb.w  r3,#1234h
        subb.b  [r5],#34h
        subb.w  [r3],#1234h
        subb.b  [r5+],#34h
        subb.w  [r3+],#1234h
        subb.b  [r5+2],#34h
        subb.w  [r3+100],#1234h
        subb.b  [r5+200],#34h
        subb.w  [r3+1000],#1234h
        subb.b  200h,#34h
        subb.w  123h,#1234h

        trap    #5

        xch     r3h,r5l
        xch     r5l,r3h
        xch     r3,r5
        xch     r5,r3
        xch     r3h,[r5]
        xch     [r5],r3h
        xch     r3,[r5]
        xch     [r5],r3
        xch     r3h,123h
        xch     123h,r3h
        xch     r3,200h
        xch     200h,r3

        xor.b   r4h,r1l
        xor.w   r5,r3
        xor.b   r5l,[r6]
        xor.w   r4,[sp]
        xor.b   [r6],r5l
        xor.w   [sp],r4
        xor.b   r3h,[r6+2]
        xor.w   r4,[r3+100]
        xor.b   [r6+2],r3h
        xor.w   [r3+100],r4
        xor.b   r3h,[r6+200]
        xor.w   r4,[r3+1000]
        xor.b   [r6+200],r3h
        xor.w   [r3+1000],r4
        xor.b   r4h,[r1+]
        xor.w   r5,[r6+]
        xor.b   [r1+],r4h
        xor.w   [r6+],r5
        xor.b   200h,r2l
        xor.w   123h,r6
        xor.b   r2l,200h
        xor.w   r6,123h
        xor.b   r5h,#34h
        xor.w   r3,#1234h
        xor.b   [r5],#34h
        xor.w   [r3],#1234h
        xor.b   [r5+],#34h
        xor.w   [r3+],#1234h
        xor.b   [r5+2],#34h
        xor.w   [r3+100],#1234h
        xor.b   [r5+200],#34h
        xor.w   [r3+1000],#1234h
        xor.b   200h,#34h
        xor.w   123h,#1234h

	expect	140
         mov.b   [r5+],[r5+]
	endexpect
	expect	140
         xch     r4l,r4l
	endexpect
	expect	140
         pop     r7
	endexpect
	expect	140
         norm.b  r4l,r4l
	endexpect
	expect	140
         norm.w  r4,r4h
	endexpect
	expect	140
         norm.d  r4,r5l
	endexpect
	expect	140
         mov     [r4+],r4l
	endexpect
	expect	140
         mov     r4h,[r4+]
	endexpect
	expect	140
         movc    r4h,[r4+]
	endexpect
	expect	140
         add     [r4+],r4l
	endexpect
	expect	140
         add     r4h,[r4+]
	endexpect
	expect	140
         mov     r5,[r5+]
	endexpect

	; register aliases

regr0l	equ	r0l
regr0h	equ	r0h
regr1l	equ	r1l
regr1h	equ	r1h
regr2l	equ	r2l
regr2h	equ	r2h
regr3l	equ	r3l
regr3h	equ	r3h
regr4l	equ	r4l
regr4h	equ	r4h
regr5l	equ	r5l
regr5h	equ	r5h
regr6l	equ	r6l
regr6h	equ	r6h
regr7l	equ	r7l
regr7h	equ	r7h

regr0	reg	r0
regr1	reg	r1
regr2	reg	r2
regr3	reg	r3
regr4	reg	r4
regr5	reg	r5
regr6	reg	r6
regr7	reg	r7

regr0d	equ.d	r0
regr2d	equ.d	r2
regr4d	equ.d	r4
regr6d	equ.d	r6

regsp	equ	sp

	add.b	r4h,r1l
	add.b	regr4h,regr1l
	add.w	r4,[r3+100]
	add.w	regr4,[regr3+100]
	asl.d	r2,r4l
	asl.d	regr2d,regr4l
	expect	1131
	asl.d	regr2,regr4l
	endexpect

;-----------------------------------------------------
; standard Intel/MASM-style pseudo instructions

	include "../t_dx/t_dn.inc"
	include "../t_dx/t_db.inc"
	include "../t_dx/t_dw.inc"
	include "../t_dx/t_dd.inc"
	include "../t_dx/t_dq.inc"
	include "../t_dx/t_dt.inc"
	include "../t_dx/t_do.inc"
