        cpu     8085
        page    0

        mov     d,h
        mov     m,a
        mov     c,m
        mvi     d,12h
        mvi     m,12h
        lxi     b,1234h
        lxi     d,1234h
        lxi     h,1234h
        stax    b
        stax    d
        stax    h
        ldax    b
        ldax    d
        ldax    d
        ldax    h
        sta     1234h
        lda     1234h
        shld    1234h
        lhld    1234h
        xchg

        push    b
        push    d
        push    h
        push    psw
        pop     b
        pop     d
        pop     h
        pop     psw
        xthl
        sphl
        lxi     sp,1234h
        inx     sp
        dcx     sp

        jmp     1234h
        jc      1234h
        jnc     1234h
        jz      1234h
        jnz     1234h
        jp      1234h
        jm      1234h
        jpe     1234h
        jpo     1234h
        pchl

        call    1234h
        cc      1234h
        cnc     1234h
        cz      1234h
        cnz     1234h
        cp      1234h
        cm      1234h
        cpe     1234h
        cpo     1234h

        ret
        rc
        rnc
        rz
        rnz
        rp
        rm
        rpe
        rpo

        rst     2

        in      12h
        out     12h

        inr     d
        dcr     h
        inr     m
        dcr     m
        inx     b
        inx     d
        inx     h
        dcx     b
        dcx     d
        dcx     h

        add     c
        adc     d
        add     m
        adc     m
        adi     12h
        aci     12h
        dad     b
        dad     d
        dad     h
        dad     sp

        sub     c
        sbb     d
        sub     m
        sbb     m
        sui     12h
        sbi     12h

        ana     c
        xra     c
        ora     c
        cmp     c
        ana     m
        xra     m
        ora     m
        cmp     m
        ani     12h
        xri     12h
        ori     12h
        cpi     12h

        rlc
        rrc
        ral
        rar

        cma
        stc
        cmc
        daa

        ei
        di
        nop
        hlt

        rim
        sim

;---------------------------------
; undocumented 8085 instructions

	cpu	8085undoc

	dsub
	dsub	b

	arhl

	rdel

	ldhi	12h
	ldsi	12h

	rstv

	shlx

	jnx5	1234h
	jx5	1234h

	shlx
	shlx	d
	lhlx
	lhlx	d

;-----------------------------------------------------
; instructions supported by V30 in 8080 emulation mode

	cpu	v30emu

	calln	5
	retem
