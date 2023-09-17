        cpu     tms70c08

        page    0

        include reg7000

; additional syntax options marked with ***

	adc	b,a
        adc	r10,a
        adc	r20,b
        adc	r30,r40
        adc	%20,a
        adc	#20,a		; ***
        adc	%30,b
        adc	#30,b           ; ***
        adc	%40,r50
        adc	#40,r50         ; ***

	add	b,a
        add	r10,a
        add	r20,b
        add	r30,r40
        add	%20,a
        add	#20,a		; ***
        add	%30,b
        add	#30,b           ; ***
        add	%40,r50
        add	#40,r50         ; ***

	and	b,a
        and	r10,a
        and	r20,b
        and	r30,r40
        and	%20,a
        and	#20,a		; ***
        and	%30,b
        and	#30,b           ; ***
        and	%40,r50
        and	#40,r50         ; ***
	andp	a,p20
        and	a,p20		; ***
        andp	b,p30
        and	b,p30		; ***
        andp	#50,p40
        and	#50,p40		; ***

	btjo	b,a,$
        btjo	r10,a,$
        btjo	r20,b,$
        btjo	r30,r40,$
        btjo	%20,a,$
        btjo	#20,a,$		; ***
        btjo	%30,b,$
        btjo	#30,b,$         ; ***
        btjo	%40,r50,$
        btjo	#40,r50,$       ; ***
	btjop	a,p20,$
        btjo	a,p20,$		; ***
        btjop	b,p30,$
        btjo	b,p30,$		; ***
        btjop	#50,p40,$
        btjo	#50,p40,$	; ***

	btjz	b,a,$
        btjz	r10,a,$
        btjz	r20,b,$
        btjz	r30,r40,$
        btjz	%20,a,$
        btjz	#20,a,$		; ***
        btjz	%30,b,$
        btjz	#30,b,$         ; ***
        btjz	%40,r50,$
        btjz	#40,r50,$       ; ***
	btjzp	a,p20,$
        btjz	a,p20,$		; ***
        btjzp	b,p30,$
        btjz	b,p30,$		; ***
        btjzp	#50,p40,$
        btjz	#50,p40,$	; ***

	br	@1234h
        br	1234h		; ***
        br	@1234h(b)
        br	1234h(b)	; ***
        br	*r30

	call	@1234h
        call	1234h		; ***
        call	@1234h(b)
        call	1234h(b)	; ***
        call	*r30

        clr	a
        clr	b
        clr	r10

        clrc

	cmp	b,a
        cmp	r10,a
        cmp	r20,b
        cmp	r30,r40
        cmp	%20,a
        cmp	#20,a		; ***
        cmp	%30,b
        cmp	#30,b           ; ***
        cmp	%40,r50
        cmp	#40,r50         ; ***
	cmpa	@1234h
        cmpa	1234h		; ***
        cmp	@1234h,a	; ***
        cmp	1234h,a		; ***
	cmpa	@1234h(b)
        cmpa	1234h(b)	; ***
        cmp	@1234h(b),a	; ***
        cmp	1234h(b),a	; ***
	cmpa	*r60
        cmp	*r60,a		; ***

	dac	b,a
        dac	r10,a
        dac	r20,b
        dac	r30,r40
        dac	%20,a
        dac	#20,a		; ***
        dac	%30,b
        dac	#30,b           ; ***
        dac	%40,r50
        dac	#40,r50         ; ***

        dec	a
        dec	b
        dec	r10

        decd	a
        decd	b
        decd	r10

        dint

        djnz	a,$
        djnz	b,$
        djnz	r10,$

	dsb	b,a
        dsb	r10,a
        dsb	r20,b
        dsb	r30,r40
        dsb	%20,a
        dsb	#20,a		; ***
        dsb	%30,b
        dsb	#30,b           ; ***
        dsb	%40,r50
        dsb	#40,r50         ; ***

        eint

        idle

        inc	a
        inc	b
        inc	r10

        inv	a
        inv	b
        inv	r10

        jmp	$
        jc	$
        jeq	$
        jhs	$
        jl	$
        jn	$
	jlt	$
        jnc	$
        jne	$
        jnz	$
        jp	$
	jgt	$
        jpz	$
	jge	$
        jz	$

        lda	@1234h
        lda	1234h		; ***
        mov	@1234h,a	; ***
        mov	1234h,a		; ***
        lda	@1234h(b)
        lda	1234h(b)	; ***
        mov	@1234h(b),a	; ***
        mov	1234h(b),a	; ***
	lda	*r10
        mov	*r10,a		; ***

        ldsp

        mov	a,b
        mov	a,r10
        mov	b,a
        mov	b,r20
        mov	r30,a
        mov	r40,b
        mov	r50,r60
        mov	%10,a
        mov	#10,a		; ***
        mov	%20,b
        mov	#20,b		; ***
        mov	%30,r70
        mov	#30,r70		; ***

        movd	%1234h,r10
        movd	#1234h,r10	; ***
        movd	%1234h(b),r20
        movd	#1234h(b),r20   ; ***
        movd	r30,r40

        movw	%1234h,r10      ; ***
        movw	#1234h,r10	; ***
        movw	%1234h(b),r20   ; ***
        movw	#1234h(b),r20   ; ***
        movw	r30,r40         ; ***

        movp	a,p10
        mov	a,p10		; ***
        movp	b,p20
        mov	b,p20		; ***
        movp	%10,p30
        movp	#10,p30		; ***
        mov	%10,p30		; ***
        mov	#10,p30		; ***
        movp	p40,a
        mov	p40,a		; ***
        movp	p50,b
        mov	p50,b		; ***

	mpy	b,a
        mpy	r10,a
        mpy	r20,b
        mpy	r30,r40
        mpy	%20,a
        mpy	#20,a		; ***
        mpy	%30,b
        mpy	#30,b           ; ***
        mpy	%40,r50
        mpy	#40,r50         ; ***

	nop

	or	b,a
        or	r10,a
        or	r20,b
        or	r30,r40
        or	%20,a
        or	#20,a		; ***
        or	%30,b
        or	#30,b           ; ***
        or	%40,r50
        or	#40,r50         ; ***
	orp	a,p20
        or	a,p20		; ***
        orp	b,p30
        or	b,p30		; ***
        orp	#50,p40
        or	#50,p40		; ***

        pop	a
        pop	b
        pop	r10
        pop	st

        push	a
        push	b
        push	r10
        push	st

        reti
        rti			; ***

        rets
        rts			; ***

        rl	a
        rl	b
        rl	r10

        rlc	a
        rlc	b
        rlc	r10

        rr	a
        rr	b
        rr	r10

        rrc	a
        rrc	b
        rrc	r10

	sbb	b,a
        sbb	r10,a
        sbb	r20,b
        sbb	r30,r40
        sbb	%20,a
        sbb	#20,a		; ***
        sbb	%30,b
        sbb	#30,b           ; ***
        sbb	%40,r50
        sbb	#40,r50         ; ***

        setc

        sta	@1234h
        sta	1234h		; ***
        mov	a,@1234h	; ***
        mov	a,1234h		; ***
        sta	@1234h(b)
        sta	1234h(b)	; ***
        mov	a,@1234h(b)	; ***
        mov	a,1234h(b)	; ***
	sta	*r10
        mov	a,*r10		; ***

        stsp

	sub	b,a
        sub	r10,a
        sub	r20,b
        sub	r30,r40
        sub	%20,a
        sub	#20,a		; ***
        sub	%30,b
        sub	#30,b           ; ***
        sub	%40,r50
        sub	#40,r50         ; ***

        swap	a
        swap	b
        swap	r10

        trap	0
        trap	23

        tsta
        tst	a		; ***
        tstb
        tst	b		; ***

        xchb	a
        xchb	b		; ***
        xchb	r10

	xor	b,a
        xor	r10,a
        xor	r20,b
        xor	r30,r40
        xor	%20,a
        xor	#20,a		; ***
        xor	%30,b
        xor	#30,b           ; ***
        xor	%40,r50
        xor	#40,r50         ; ***
	xorp	a,p20
        xor	a,p20		; ***
        xorp	b,p30
        xor	b,p30		; ***
        xorp	#50,p40
        xor	#50,p40		; ***
